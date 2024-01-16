# Description
This library contains classes for working in multithreaded environment.
- *[CrashHandler](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/CrashHandler.h)*. May be used to wrap any task, generates core dump, error code and stacktrace (at the moment only for Windows). You may react on a crash event according to your design and software requirements.
- *[ThreadPool](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/ThreadPool.h)*. Contains bunch of working threads. All assigned tasks may be put into queue by any thread. Each task is processed by first available thread. In fact, every task is processed asyncronously relatively to invoking thread. In result we have auto-balanced system. 
- *[ThreadPoolQueued](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/ThreadPoolQueued.h)*. Similar to ThreadPool but there are multiple queues, one per each thread. Pool assigns tasks in a strict order to every thread.
- *[PostponeLoop](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/PostponeLoop.h)*. Is used for delayed processing task. For instance, you need to do task in N seconds. Internally it also has queue sorted by future execution time. 
- *[TimerLoop](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/TimerLoop.h)*. Similar to PostponeLoop but operates Timer objects. Timer object may put itself to loop and remove itself before execution.
- *[Timer](https://github.com/darkessence87/psi-thread/blob/master/psi/include/psi/thread/Timer.h)*. Task which can be scheduled, cancelled, re-started etc. 

# Usage examples
* [1.0 Simple ThreadPool](https://github.com/darkessence87/psi-thread/tree/master/psi/examples/1.0_Simple_ThreadPool)
* [1.1 Simple ThreadPoolQueued](https://github.com/darkessence87/psi-thread/tree/master/psi/examples/1.1_Simple_ThreadPoolQueued)
