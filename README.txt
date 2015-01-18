TaskManager -- a task manager for Arduino systems

This is a pre-release version.

Right now, it is (C) Copyright 2014 Stephen Platt.
You are free to use it for noncommercial purposes provided you do
not modify it or remove any of the files.  You may not redistribute
it.

At some point I'll research common licensing models and loosen things
up a bit.

Planned enhancements before 1.0:
* AutoWaitDelay calculation to be based on regular scheduling instead of
whenever "now" is.  In other words, if a task is 
    - scheduled to start at 2015
    - set with a 50ms auto-restart-delay
    - starts at 2020
    - takes 7ms before it exits ('now' is 2027 as it exits)
Right now, it will reschedule itself to start at 2027+50 -> 2077.
This can lead to irregular and cumulatively delayed start times depending
on other tasks.  The "regular scheduling" approach will reschedule based
upon the original scheduled time, so it will be scheduled to start at 2015,
2065, 2115, 2165, etc.
* Clean up the documentation and contents of the release directory
* Better installation instructions.  *Any* installation instructions.

Planned enhancements for 2.0:
* Signalling and message-passing between tasks running on different 
processors.
