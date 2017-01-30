/*
*	README:
*	For consistencies sake, any time you loop through the brightness matrix, it should look like this:
*		for(emitter){
*			for(sensor){
*				brightness_matrix[emitter][sensor] ...
*			}
*		}
*	There were previous inconsistencies in this code.
*/
#include "range_algs.h"

//This is based on the time that elapses between when a RXing Droplet gets the end
// of a message sent from dir N, and when the TXing droplet finishes on its last channel.
static const uint8_t txDirOffset[6] = {7, 6, 3, 5, 4, 2};

typedef union fd_step_union{
	float f;
	int32_t d;
} fdStep;

static const float bearingBasis[6][2]=	{
	{SQRT3_OVER2 , -0.5},
	{0           , -1  },
	{-SQRT3_OVER2, -0.5},
	{-SQRT3_OVER2,  0.5},
	{0           ,  1  },
	{SQRT3_OVER2 ,  0.5}
};

static const float scaledBearingBasis[6][2]= {
	{1.8038f , -1.0414f},
	{0.0f  , -2.0828f},
	{-1.8038f, -1.0414f},
	{-1.8038f,  1.0414f},
	{0.0f  ,  2.0828f},
	{1.8038f ,  1.0414f}
};

static const float headingBasis[6][2]={
	{-1          ,  0  },
	{-0.5,  SQRT3_OVER2},
	{ 0.5,  SQRT3_OVER2},
	{ 1          ,  0  },
	{ 0.5, -SQRT3_OVER2},
	{-0.5, -SQRT3_OVER2}
};

static const float basis_angle[6] = {-(M_PI/6.0), -M_PI_2, -((5.0*M_PI)/6.0), ((5.0*M_PI)/6.0), M_PI_2, (M_PI/6.0)};
static uint32_t sensorHealthHistory;
static int16_t brightMeas[6][6];

static inline float getCosBearingBasis(uint8_t i __attribute__ ((unused)), uint8_t j){
	return bearingBasis[j][0];
}

static inline float getSinBearingBasis(uint8_t i __attribute__ ((unused)), uint8_t j){
	return bearingBasis[j][1];
}

static inline float getCosHeadingBasis(uint8_t i, uint8_t j){
	return headingBasis[(j+(6-i))%6][0];
}

static inline float getSinHeadingBasis(uint8_t i, uint8_t j){
	return headingBasis[(j+(6-i))%6][1];
}

static inline float getBearingAngle(uint8_t i, uint8_t j){
	//return pretty_angle((-M_PI/6.0)-j*(M_PI/3.0));
	return atan2f(getSinBearingBasis(i,j),getCosBearingBasis(i,j));
}

static inline float getHeadingAngle(uint8_t i, uint8_t j){
	//return pretty_angle((-M_PI/6.0)-((j+(6-i))%6)*(M_PI/3.0));
	return atan2f(getSinHeadingBasis(i,j), getCosHeadingBasis(i,j));
}

static inline float rnb_constrain(float x){ //constrains the value to be within or equal to the bounds.
	return (x < FD_MIN_STEP ? FD_MIN_STEP : (x > FD_MAX_STEP ? FD_MAX_STEP : x));
}


static void calculate_bearing_and_heading(float* bearing, float* heading);
static float get_initial_range_guess(float bearing, float heading, uint8_t power);
static float range_estimate(float init_range, float bearing, float heading, uint8_t power);

static int16_t processBrightMeas();

static float sensor_model(float alpha);
static float emitter_model(float beta);
static float amplitude_model(float r);
static float inverse_amplitude_model(float ADC_val, uint8_t power);

static void debug_print_timer(uint32_t timer[19]);
static void print_brightMeas();

static float calculate_innovation(float r, float b, float h);

static void full_expected_bright_mat(float bM[6][6], float r, float b, float h);
												
void range_algs_init(){
	sensorHealthHistory = 0;
	for(uint8_t i=0 ; i<6 ;i++){
		for(uint8_t j=0 ; j<6 ; j++){
			brightMeas[i][j] = 0;
		}
	}
	rnbCmdID=0;
	rnbProcessingFlag=0;
}


static void full_expected_bright_mat(float bM[6][6], float r, float b, float h){
	float rX = r*cosf(b);
	float rY = r*sinf(b);
	float riX, riY;
	float jX, jY;
	float rijX, rijY;
	float alpha, beta;
	float rijMag, rijMagSq;
	for(uint8_t i=0;i<6;i++){
		jX = DROPLET_RADIUS*cosf(basis_angle[i]+h);
		jY = DROPLET_RADIUS*sinf(basis_angle[i]+h);
		riX = rX + jX;
		riY = rY + jY;
		for(uint8_t j=0;j<6;j++){
			rijX = riX-scaledBearingBasis[j][0];
			rijY = riY-scaledBearingBasis[j][1];
			alpha = rijX*bearingBasis[j][0]+rijY*bearingBasis[j][1];
			beta =  -rijX*jX - rijY*jY;
			rijMagSq = rijX*rijX+rijY*rijY;
			rijMag = sqrtf(rijMagSq);
			bM[i][j] = (alpha>0 && beta>0) ? ((alpha*beta*amplitude_model(rijMag))/rijMagSq) : 0;
		}
	}
}

static float calculate_innovation(float r, float b, float h){
	float expBM[6][6];
	full_expected_bright_mat(expBM, r, b, h);
	float expNorm  = 0.0;
	float realNorm = 0.0;

	float* fastExpBM = (float*)expBM;
	int16_t* fastBM = (int16_t*)brightMeas;

	for(uint8_t i=0;i<36;i++){
		expNorm += fastExpBM[i]*fastExpBM[i];
		realNorm += ((int32_t)fastBM[i])*((int32_t)fastBM[i]);
	}
	float expNormInv  = powf(expNorm,-0.5);
	float realNormInv = powf(realNorm,-0.5);
	//printf("%04X | %7.3f %7.3f %7.1f %7.1f\r\n",cmdID, realFrob, expFrob, realTot, expTot);
	
	
	float error=0.0;
	for(uint8_t i=0;i<36;i++){
		error+=fabsf(fastBM[i]*realNormInv-fastExpBM[i]*expNormInv);
	}
	return error;
}

//TODO: handle variable power.
void broadcast_rnb_data(){
	uint8_t power = 255;
	uint8_t goAhead =0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		if(!rnbProcessingFlag){
			rnbProcessingFlag = 1;
			goAhead = 1;
		}
	}
	if(goAhead){
		rnbCmdSentTime = get_time();
		char c = 'r';
		uint8_t result = hp_ir_targeted_cmd(ALL_DIRS, &c, 65, (uint16_t)(rnbCmdSentTime&0xFFFF));
		if(result){
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
				hp_ir_block_bm = 0xFF;
			}		
			ir_range_blast(power);
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
				hp_ir_block_bm = 0;
			}
			//printf("rnb_b\r\n");
		}
	}
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		rnbProcessingFlag = 0;
	}
}

void use_rnb_data(){
	//uint32_t start = get_time();
	uint8_t power = 255;
	int16_t matrixSum = processBrightMeas();
	//if(rand_byte()%2) broadcastBrightMeas();
	float bearing, heading;
	float error;
	calculate_bearing_and_heading(&bearing, &heading);
	print_brightMeas();
	float initial_range = get_initial_range_guess(bearing, heading, power);
	if(initial_range!=0&&!isnanf(initial_range)){	
		float range = range_estimate(initial_range, bearing, heading, power);
		if(!isnanf(range)){
			if(range<2*DROPLET_RADIUS) range=5.0;

			float conf = sqrtf(matrixSum);
			error = calculate_innovation(range, bearing, heading);
			if(error>2.5){
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
					rnbProcessingFlag=0;
				}
				return;
			}
			conf = conf/(error*error);
			if(error>3.0){
				conf = conf/10.0; //Nerf the confidence hard if the calculated error was too high.
			}
			if(isnan(conf)){
				conf = 0.01;
			}
	
			last_good_rnb.id = rnbCmdID;
			last_good_rnb.range		= (uint16_t)(10.0*range);
			last_good_rnb.bearing	= (int16_t)rad_to_deg(bearing);
			last_good_rnb.heading	= (int16_t)rad_to_deg(heading);
			last_good_rnb.conf		= (uint8_t)conf;
			rnb_updated=1;
		}
	}
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		rnbProcessingFlag=0;
	}
}

static void calculate_bearing_and_heading(float* bearing, float* heading){
	int16_t* fast_bm = (int16_t*)brightMeas;
	
	float bearingX = 0;
	float bearingY = 0;
	float headingX = 0;
	float headingY = 0;

	for(uint8_t i=0;i<36;i++){
		
		bearingX+=fast_bm[i]*getCosBearingBasis(i/6,i%6);
		bearingY+=fast_bm[i]*getSinBearingBasis(i/6,i%6);
		headingX+=fast_bm[i]*getCosHeadingBasis(i/6,i%6);
		headingY+=fast_bm[i]*getSinHeadingBasis(i/6,i%6);
	}
	
	*bearing = atan2f(bearingY, bearingX);	
	*heading = atan2f(headingY, headingX);
}

static float get_initial_range_guess(float bearing, float heading, uint8_t power){
	int8_t bestS = (6-((int8_t)ceilf((3.0*bearing)/M_PI)))%6;
	float alpha = pretty_angle(bearing - basis_angle[bestS]);				  //alpha using infinite approximation
	int8_t bestE = (6-((int8_t)ceilf((3.0*(bearing-heading-M_PI))/M_PI)))%6;					
	float  beta = pretty_angle(bearing - heading - basis_angle[bestE] - M_PI); //beta using infinite approximation	
	
	//printf("(alpha: %f, sensor %hd)\r\n", alpha, bestS); 	
	if((alpha > M_PI_2) || (alpha < -M_PI_2)){
		printf("ERROR: alpha out of range (alpha: %f, sensor %hd)\r\n", rad_to_deg(alpha), bestS); 
		return 0;
	}
	//printf("(beta: %f, emitter %hd)\r\n",  beta, bestE); 	
	if((beta > M_PI_2)  || (beta < -M_PI_2)){
		printf("ERROR: beta out of range (beta: %f, emitter %hd)\r\n",  beta, bestE); 
		return 0;
	}

	// expected contribution (using infinite distance approximation)
	float amplitude;
	float exp_con = sensor_model(alpha)*emitter_model(beta);
	
	if(exp_con > 0)	amplitude = brightMeas[bestE][bestS]/exp_con;	
	else{
		printf("ERROR: exp_con (%f) is negative (or zero)!\r\n", exp_con); 
		return 0;
	}
	//printf("amp_for_inv: %f\t",amplitude);
	float rMagEst = inverse_amplitude_model(amplitude, power);
	
	float RX = rMagEst*cos(bearing)+DROPLET_SENSOR_RADIUS*(bearingBasis[bestS][0]-headingBasis[bestE][0]);
	float RY = rMagEst*sin(bearing)+DROPLET_SENSOR_RADIUS*(bearingBasis[bestS][1]-headingBasis[bestE][1]);
	
	float rangeEst = hypotf(RX,RY);
	
	return rangeEst;
}

static float range_estimate(float init_range, float bearing, float heading, uint8_t power){
	float range_matrix[6][6];
	
	float sensorRXx, sensorRXy, sensorTXx, sensorTXy;
	float alpha, beta, sense_emit_contr;
	float calcRIJmag, calcRx, calcRy;

	int16_t maxBright = -32768;
	uint8_t maxE=0;
	uint8_t maxS=0;
	for(uint8_t e = 0; e < 6; e++){
		for(uint8_t s = 0; s < 6; s++){
			if(brightMeas[e][s]>maxBright){
				maxBright = brightMeas[e][s];
				maxE = e;
				maxS = s;
			}
			if(brightMeas[e][s] > 0){												
				sensorRXx = DROPLET_SENSOR_RADIUS*getCosBearingBasis(0,s);
				sensorRXy = DROPLET_SENSOR_RADIUS*getSinBearingBasis(0,s);
				sensorTXx = DROPLET_SENSOR_RADIUS*cosf(basis_angle[e]+heading) + init_range*cosf(bearing);
				sensorTXy = DROPLET_SENSOR_RADIUS*sinf(basis_angle[e]+heading) + init_range*sinf(bearing);

				alpha = atan2f(sensorTXy-sensorRXy,sensorTXx-sensorRXx) - basis_angle[s];
				beta = atan2f(sensorRXy-sensorTXy,sensorRXx-sensorTXx) - basis_angle[e] - heading;

				alpha = pretty_angle(alpha);
				beta = pretty_angle(beta);
				
				sense_emit_contr = sensor_model(alpha)*emitter_model(beta);
				if(sense_emit_contr>0){
					calcRIJmag = inverse_amplitude_model(brightMeas[e][s]/sense_emit_contr, power);
				}else{
					calcRIJmag = 0;
				}
				calcRx = calcRIJmag*cosf(alpha) + sensorRXx - DROPLET_SENSOR_RADIUS*cosf(basis_angle[e]+heading);
				calcRy = calcRIJmag*sinf(alpha) + sensorRXy - DROPLET_SENSOR_RADIUS*sinf(basis_angle[e]+heading);
				range_matrix[e][s] = hypotf(calcRx, calcRy);
				continue;
			}
			range_matrix[e][s]=0;
		}
	}
	
	float rangeMatSubset[3][3];
	float brightMatSubset[3][3];
	float froebNormSquared=0;
	for(uint8_t e = 0; e < 3; e++){
		for(uint8_t s = 0; s < 3; s++){
			uint8_t otherE = ((maxE+(e+5))%6);
			uint8_t otherS = ((maxS+(s+5))%6);
			rangeMatSubset[e][s] = range_matrix[otherE][otherS];
			brightMatSubset[e][s] = (float)brightMeas[otherE][otherS];
			froebNormSquared+=powf(brightMatSubset[e][s],2);
		}
	}
	float froebNorm = sqrtf(froebNormSquared);
	float range = 0;
	float froebWeight;
	for(uint8_t e = 0; e < 3; e++){
		for(uint8_t s = 0; s < 3; s++){
			froebWeight = powf(brightMatSubset[e][s]/froebNorm,2);
			range += rangeMatSubset[e][s]*froebWeight;
		}
	}
	return range;
}

static int16_t processBrightMeas(){
	int16_t val;
	int16_t valSum=0;
	uint8_t allColZeroCheck = 0b00111111;

	for(uint8_t e = 0; e < 6; e++){
		for(uint8_t s = 0; s < 6; s++){
			val = brightMeas[e][s];
			allColZeroCheck &= ~((!!val)<<s);	
			//val=val*(val>0);
			brightMeas[e][s] = val;
			valSum+=val;	
		}
	}

	uint8_t problem = 0;
	for(uint8_t i = 0; i<6; i++){
		if(allColZeroCheck&(1<<i)){
			sensorHealthHistory+=(1<<(4*i));
		}else{
			sensorHealthHistory&=~(0xF<<(4*i));
		}
		if(((sensorHealthHistory>>(4*i))&0xF)==0xF){
			printf_P(PSTR("!!!\tGot 15 consecutive nothings from sensor %hu.\t!!!\r\n"), i);
			sensorHealthHistory&=~(0xF<<(4*i));
			problem = 1;
		}		
	}
	if(problem){
		startup_light_sequence();
	}	
	return valSum;
}

void ir_range_meas(){
	//int32_t times[16] = {0};
	cmd_arrival_dir;
	cmd_sender_dir;
	//times[0] = get_time();
	while((get_time()-rnbCmdSentTime+8)<POST_BROADCAST_DELAY);
	//times[1] = get_time();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){		
		uint32_t pre_sync_op = get_time();
		while((get_time() - pre_sync_op) < TIME_FOR_SET_IR_POWERS) delay_us(500);
		//times[2] = get_time();
		for(uint8_t emitter_dir = 0; emitter_dir < 6; emitter_dir++){
			pre_sync_op = get_time();
			//times[2*emitter_dir+3] = pre_sync_op;
			while((get_time() - pre_sync_op) < (TIME_FOR_GET_IR_VALS-TIME_FOR_IR_MEAS)/2) delay_us(500);
			get_ir_sensors(brightMeas[emitter_dir] , 9); //11
			//times[2*emitter_dir+4] = get_time();			
			while((get_time() - pre_sync_op) < TIME_FOR_GET_IR_VALS) delay_us(500);		
			delay_ms(DELAY_BETWEEN_RB_TRANSMISSIONS);
		}
	}
}

void ir_range_blast(uint8_t power __attribute__ ((unused))){
	//int32_t times[16] = {0};
	//times[0] = get_time();
	while((get_time() - rnbCmdSentTime) < POST_BROADCAST_DELAY) delay_us(500);
	//times[1] = get_time();
	uint32_t pre_sync_op = get_time();
	set_all_ir_powers(256);	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){		
		while((get_time() - pre_sync_op) < TIME_FOR_SET_IR_POWERS) delay_us(500);
		//times[2] = get_time();
		for(uint8_t dir = 0; dir < 6; dir++){
			pre_sync_op = get_time();
			//set_red_led(255);
			//times[2*dir+3] = pre_sync_op;			
			ir_led_on(dir);
			while((get_time() - pre_sync_op) < TIME_FOR_GET_IR_VALS) delay_us(500);
			ir_led_off(dir);
			//times[2*dir+4] = get_time();				
			//set_red_led(0);					
			delay_ms(DELAY_BETWEEN_RB_TRANSMISSIONS);
		}
	}
}


static float sensor_model(float alpha){
	if(fabsf(alpha)>=1.5){
		return 0.0;
	}else if(fabsf(alpha)<=0.62){
		return (1-powf(alpha,4));
	}else{
		return 0.125/powf(alpha,4);
	}
}

static float emitter_model(float beta){
	if(fabsf(beta)>=1.5){
		return 0.0;
	}else if(fabsf(beta)<=0.72){
		return (0.94+powf(beta,2)*0.5-powf(beta,4));
	}else{
		return 0.25/powf(beta,4);
	}
}

static float amplitude_model(float r){
	return (r<=0.5) ? 2597.1 : (3.90804+(13427.5/(5.17716+powf(r-0.528561,2.0))));
}

static float inverse_amplitude_model(float lambda, uint8_t power){
	if(power==255){
		return (lambda>=2597.1) ? 0.5 : (sqrtf(13427.5/(lambda-3.90804)-5.17716)+0.528561);
	}else{
		printf_P(PSTR("ERROR: Unexpected power: %hu\r\n"),power);
	}
	return 0;
}

static void debug_print_timer(uint32_t timer[14]){
	printf_P(PSTR("Duration: %lu\r\n"),(timer[13] - timer[0]));
	printf("|  ");
	for(uint8_t i=1 ; i<13 ; i++)
	{
		printf_P(PSTR("%3lu  |  "),timer[i] - timer[i-1]);
	}
	printf("\r\n");
}

static void print_brightMeas(){
	printf("{\"%04X\", \"%04X\", {", rnbCmdID, get_droplet_id());
	for(uint8_t emitter_num=0 ; emitter_num<6 ; emitter_num++){
		printf("{");
		for(uint8_t sensor_num=0 ; sensor_num<6 ; sensor_num++){
			printf("%d",brightMeas[emitter_num][sensor_num]);
			if(sensor_num<5) printf(",");
		}
		printf("}");		
		if(emitter_num<5) printf(",");
	}
	printf("}},\r\n");
}