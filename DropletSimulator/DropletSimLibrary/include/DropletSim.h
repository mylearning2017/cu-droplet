#pragma once

#ifndef _DROPLET_SIM
#define _DROPLET_SIM

#include <btBulletDynamicsCommon.h>

#include "DropletDataStructs.h"
#include "DropletSimGlobals.h"
#include "DropletTimeControl.h"
#include "IDropletProjector.h"
#include "DropletUtil.h"
#include "IDroplet.h"
#include "DSimPhysicalObject.h"

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <string>
#include <math.h>

class SimSetupData;	// Defined after DropletSim

/**
 * Droplet simulator.
 */
class DropletSim
{
	friend class DropletSimInfo;

private :

	SimPhysicsData *simPhysics;
	SimSetupData *simSetupData;

	float halfArenaLength, halfArenaWidth;

	/**
	 * Initialises the physics.
	 *
	 * \return	.
	 */

	DS_RESULT initPhysics(void);

	/**
	 * Ends the physics.
	 *
	 * \return	.
	 */

	DS_RESULT endPhysics(void);

	/**
	 * \deprecated
	 * Initialises a physics object.
	 *
	 * \param [in,out]	objPhysics	If non-null, the object physics.
	 * \param [in,out]	origin	  	The origin.
	 *
	 * \return	.
	 */

	DS_RESULT initPhysicsObject(
		ObjectPhysicsData *objPhysics, btVector3 &origin);


	/**
	 * Gather positions and correct.
	 */

	void gatherPositionsAndCorrect(void);

	/**
	 * Motion controller.
	 */

	void motionController(void);

	/**
	 * Set Power to droplet legs
	 */

	void setLegPower(IDroplet *pDroplet, GPSInfo *gpsInfo);

	/**
	 * Sensor controller.
	 */

	void sensorController(void);

	/**
	 * Communication controller.
	 */

	void commController(void);
	
	void calcRelativePos(unsigned int dID);

	/**
	 * Timer controller.
	 */

	void timerController(void);

protected :

	bool firstRun, projSet;
	Ran *goodRand;

	/**
	 * The projector.
	 */

	IDropletProjector *projector;

	/**
	 * Vector containing droplet information.
	 */

	std::vector<IDroplet *> droplets;

	/**
	 * Vector containing physical object information (other than droplets).
	 */

	std::vector<DSimPhysicalObject *> physicalObjects;

	/**
	 * Vector containing droplet positions.
	 */

//	std::vector<GPSInfo *> dropletPositions;
//	TrigArray *dropletRelPos;

	/**
	 * Vector containing physical object positions (other than droplets).
	 */
	std::vector<GPSInfo *> objectPositions;
	
	// time info class
	DropletTimeControl timer;

public :

	
	/**
	 * Default constructor.
	 */

	DropletSim(void);

	/**
	 * Destructor.
	 */

	~DropletSim();

	/**
	 * Adds a collision shape to 'colShapeIndex'.
	 *
	 * \param [in,out]	colShape	 	If non-null, the col shape.
	 * \param [in,out]	colShapeIndex	If non-null, zero-based index of the col shape.
	 *
	 * \return	.
	 */

	DS_RESULT AddCollisionShape(btCollisionShape *colShape, int *colShapeIndex);

	/**
	 * \deprecated
	 * Creates a floor.
	 *
	 * \param	floorShapeIndex	Zero-based index of the floor shape.
	 * \param	wallXShapeIndex	(optional) zero-based index of the wall x coordinate shape.
	 * \param	wallYShapeIndex	(optional) zero-based index of the wall y coordinate shape.
	 *
	 * \return	The new floor.
	 */

	DS_RESULT CreateFloor(
		int floorShapeIndex, 
		int wallXShapeIndex = -1, 
		int wallYShapeIndex = -1);

	/**
	 * Adds a droplet.
	 *
	 * \param [in,out]	pDroplet	If non-null, the droplet.
	 * \param	startPos			The starting (x, y) pos of the droplet on the arena.
	 * \param	startAngle			The start angle.
	 *
	 * \return	.
	 */

	DS_RESULT AddDroplet(
		IDroplet *pDroplet, 
		std::pair<float, float> startPos,
		float startAngle);

	/**
	 * Adds a physical oject.
	 *
	 * \param [in,out]	pObject		If non-null, the object.
	 * \param	startPos			The starting (x, y) pos of the object on the arena.
	 * \param	startAngle			The start angle.
	 *
	 * \return	.
	 */

	DS_RESULT AddPhysicalObject(
		DSimPhysicalObject *pObject,
		std::pair<float, float> startPos,
		float startAngle);
	/**
	 * Adds a physical oject.
	 *
	 * \param [in,out]	pObject		If non-null, the object.
	 * \param	startPos			The starting (x, y) pos of the object on the arena.
	 * \param   startHeight			The starting z pos of the object on the arena.
	 * \param	startAngle			The start angle.
	 *
	 * \return	.
	 */

	DS_RESULT AddPhysicalObject(
		DSimPhysicalObject *pObject,
		std::pair<float, float> startPos,
		float startHeight,
		float startAngle);

	/**
	 * Sets up the projector.
	 *
	 * \param	imgDir 	The image dir.
	 * \param	imgName	Name of the image.
	 *
	 * \return	.
	 */

	DS_RESULT SetUpProjector(std::string imgDir, std::string imgName);

	/**
	 * Sets up the projector.
	 *
	 * \param	imgDir	  	The image dir.
	 * \param	imgName   	Name of the image.
	 * \param	projWidth 	Width of the project.
	 * \param	projLength	Length of the project.
	 *
	 * \return	.
	 */

	DS_RESULT SetUpProjector(
		std::string imgDir, 
		std::string imgName,
		int projWidth, 
		int projLength);

	/**
	 * Initialises this object.
	 *
	 * \param	setupData	Information describing the setup.
	 *
	 * \return	.
	 */

	DS_RESULT Init(const SimSetupData &setupData);

	/**
	 * Executes a step in the simulation.
	 *
	 * \return	.
	 */

	DS_RESULT Step(void);

	/**
	 * Cleans up and frees up memory used by the physics engine.
	 *
	 * \return	.
	 */

	DS_RESULT Cleanup(void);
};

/**
 * Class containing simulation setup data.
 */

class SimSetupData
{
private :

	int numRowTiles, numColTiles;
	float tileLength, dropletRadius;
	float fps, timestep;
	bool autoBuildBoundaryWalls, staggeredStart;

	friend class DropletSim;

public :

	/**
	 * Constructor.
	 *
	 * \param	numRowTiles			  	Number of row tiles.
	 * \param	numColTiles			  	Number of col tiles.
	 * \param	tileLength			  	Length of the tile.
	 * \param	dropletRadius		  	The droplet radius.
	 * \param	fps					  	The FPS.
	 * \param	autoBuildBoundaryWalls	true to automatically build boundary walls.
	 */

	SimSetupData(
		int numRowTiles,
		int numColTiles,
		float tileLength,
		float dropletRadius,
		float fps,
		bool autoBuildBoundaryWalls
	);

	/**
	 * Copy Constructor.
	 *
	 * \param	setupData	Information describing the setup.
	 */

	SimSetupData(const SimSetupData& setupData);

};

#endif