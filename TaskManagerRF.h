#ifndef TASKMANAGER_H_INCLUDED
#define TASKMANAGER_H_INCLUDED

/*!	\file TaskManagerRF.h
	Wrapper header for Arduino TaskManager with Radio library
*/

/*	Valid values for TASKMGR_RADIO_RF
 *	Select which radio to use.
 *	"NONE" must have the value 0
 *	0	NONE
 *	1	RF24, standard cheap 2.4MHz transciever
*/
#define	TASKMGR_RADIO_NONE		0
#define TASKMGR_RADIO_RF24		1

//  Set the radio we are using
#define TASKMGR_RADIO	TASKMGR_RADIO_RF24

#endif
