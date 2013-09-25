/*
 * DropletSimTimeControl.h - 
 * 
 */
/**
 * \file	cu-droplet\DropletSimulator\DropletSimLibrary\include\DropletTimeControl.h
 *
 * \brief	Declares the droplet time control class that gathers timing info. Times are returned
 *			as doubles in seconds. The UNIX compilation calculates time with microsecond accuracy
 *			and the windows compilation calculates time with millisecond accuracy but both are
 *			only displayed to millisecond accuracy.
 */

#include <cstdio>
#include <cmath>
#pragma comment(lib, "winmm.lib")

#pragma once
#ifdef _WIN32 // Used if compiled on Windows
#include <Windows.h>
#else
#include <time.h> // Used if compiled on unix systems
#endif

#ifndef _DROPLET_TIME_CONTROL

/**
 * \def	_DROPLET_TIME_CONTROL
 *
 * \brief	A macro that defines droplet time control.
 *
 */

#define _DROPLET_TIME_CONTROL


/**
 * Class that controls droplet time properties.
 */

/**
 * \class	DropletTimeControl
 *
 * \brief	class used for calculating timing information for the Droplet Simulator. In order
 *			to access timing information from outside the simulator, use DropletSimInfo.
 *
 */
class DropletTimeControl
{
private :

#ifndef _WIN32

	/**
	 * \struct	timeval time
	 *
	 * \brief	Temporary variable that stores time.
	 */

	struct timeval time;
#endif

	/**
	 * \property	double init_time, current_time, last_step
	 *
	 * \brief	stores initialization time, current time, and time of the last step
	 *
	 */

	double init_time, current_time, last_step;

	/**
	 * \property	double sim_step_size
	 *
	 * \brief	time simulated per step.
	 */

	double sim_step_size;

	/**
	 * \property	double elapsed_ST
	 *
	 * \brief	total time simulated.
	 */

	double elapsed_ST;

public :

	//RT indicates real time, ST indicates simulator time

	/**
	 * \fn	void DropletTimeControl::initTimer(double sss);
	 *
	 * \brief	initializes the timer with a specified sim step size.
	 *
	 * \param	sss	sim step size.
	 */

	void initTimer(double sss);

	/**
	 * \fn	void DropletTimeControl::updateTimer(double sss);
	 *
	 * \brief	updates relevant variables and should be called each step. can be used to update the
	 * 			sim step size.
	 *
	 * \param	sss	sim step size.
	 */

	void updateTimer(double sss);
	void updateTimer();

	/**
	 * \fn	void DropletTimeControl::resetTimer();
	 *
	 * \brief	resets the timer.
	 */

	void resetTimer();

	/**
	 * \fn	double DropletTimeControl::getTotalRT();
	 *
	 * \brief	get total real time elapsed since the simulator started.
	 *
	 * \return	The total real time.
	 */

	double getTotalRT();

	/**
	 * \fn	double DropletTimeControl::getTotalST();
	 *
	 * \brief	get total time simulated so far.
	 *
	 * \return	The total sim time.
	 */

	double getTotalST();

	/**
	 * \fn	double DropletTimeControl::getStepRT();
	 *
	 * \brief	gets the real time elapsed since the last step.
	 *
	 * \return	The real time elapsed since the last step.
	 */

	double getStepRT();

	/**
	 * \fn	double DropletTimeControl::getTotalDiff();
	 *
	 * \brief	gets the difference between total real time and sim time.
	 *
	 * \return	The difference between total real time and sim time.
	 */

	double getTotalDiff();

	/**
	 * \fn	double DropletTimeControl::getTimeRatio();
	 *
	 * \brief	gets the ratio between the time simulated in one step and real time elapsed
	 *			since the last step.
	 *
	 * \return	The last step's time ratio.
	 */

	double getTimeRatio();

	/**
	 * \fn	void DropletTimeControl::printTotalRT();
	 *
	 * \brief	prints total real time to the console.
	 */

	void printTotalRT();
	
	/**
	 * \fn	void DropletTimeControl::printTotalST();
	 *
	 * \brief	prints total simulator time to the console.
	 */

	void printTotalST();
	
	/**
	 * \fn	void DropletTimeControl::printStepRT();
	 *
	 * \brief	prints real time elapsed since the last step to the console.
	 */

	void printStepRT();
	
	/**
	 * \fn	void DropletTimeControl::printTotalDiff();
	 *
	 * \brief	prints the difference between total real time and simulated time to the console.
	 */

	void printTotalDiff();
	
	/**
	 * \fn	void DropletTimeControl::printTimeRatio();
	 *
	 * \brief	prints the ratio between the sim time and real tim of the last step to the console.
	 */

	void printTimeRatio();

	
	/**
	 * \fn	void DropletTimeControl::printAll();
	 *
	 * \brief	prints all calculated time values on a single line to the console.
	 */

	void printAll();

	/**
	 * \fn	void DropletTimeControl::printVars();
	 *
	 * \brief	prints the stored timestamps: init time, current time, and time of the last step.
	 */

	void printVars();
};

#endif