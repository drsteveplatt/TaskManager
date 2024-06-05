// Main program include for TaskManagerRF
// Note this file should only be #include'd from the main program.
// Other C++ files in the program should #include TaskManagerSub.h
#ifndef TASKMANAGER_H_INCLUDED
#define TASKMANAGER_H_INCLUDED

//#define TASKMANAGER_DEBUG
#include <TaskManagerMacros.h>
#include <TaskManagerCore.h>
// These need to be here to ensure these are declared.
// Note that each main-file include in the TaskManager family has one of these.
// This ensures that the correct TaskMgr is used.
//#if false
TaskManager TaskMgr;
void loop() {
    TaskMgr.loop();
}
#endif // TASKMANAGER_H_INCLUDED




