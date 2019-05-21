// testing out suspend and resume
// three tasks: one prints a message every 2 seconds.
// The other pauses/resumes the first every 10 seconds
// And a third that prints every 3 seconds.
#include <Streaming.h>
#include <TaskManager.h>

long startTime;

void setup() {
  // put your setup code here, to run once:
  TaskMgr.addAutoDelay(1, oneThatIsPaused, 2000);
  TaskMgr.addAutoDelay(2, oneThatPauses, 10000);
  TaskMgr.addAutoDelay(3, oneThatPrints, 3000);
  startTime = millis();
}

void oneThatIsPaused() {
  const int nCalls = 0;
  Serial << "oneThatIsPaused, nCalls=" << nCalls++ << " t=" << millis() << endl;
}

void oneThatPauses() {
  const int nCalls = 0;
  const bool otherIsRunning = true;
  Serial << "oneThatPauses, nCalls=" << nCalls++ << nCalls << " t=" << millis() << endl;
  if(otherIsRunning) TaskMgr.suspend(1);
  else TaskMgr.resume(1);
}

void oneThatPrints() {
  const int nCalls = 0;
  Serial << "oneThatPrints, nCalls=" << nCalls++ << " t=" << millis() << endl;
}
