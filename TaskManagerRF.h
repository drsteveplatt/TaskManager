#ifndef TASKMANAGER_H_INCLUDED
#define TASKMANAGER_H_INCLUDED
#define TASKMANAGERRF_H_INCLUDED

#include <TaskManagerMacros.h>
#include <TaskManagerRFCore.h>
TaskManagerRF TaskMgr;
void loop() {
    TaskMgr.loop();
}
#endif // TASKMANAGER_H_INCLUDED


