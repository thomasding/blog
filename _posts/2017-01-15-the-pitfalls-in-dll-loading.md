---
layout: post
title: The Pitfalls in DLL Loading
tags: [windows]
comments: true
---

I've recently met a problem in migrating a Linux project to Windows, which worked well when being statically linked but resulted in a deadlock when being dynamically linked. Furthermore, the process was stuck before entering the main function, which led me to guess that it was the initialization code of the dynamic library that caused the deadlock.

<!--more-->

In Windows, the entry point of a DLL, for which DllMain is the synonym, will be called in four situations, specifically:

1. DLL\_PROCESS\_ATTACH: when the library is first loaded into an process;
2. DLL\_PROCEES\_DETACH: when the library is finally unloaded from an process;
3. DLL\_THREAD\_ATTACH: when a new thread is created;
4. DLL\_THREAD\_DETACH: when a thread exits normally.

The life time of a dynamic linked library can be briefed in the following parts:

1. When a process first loads the DLL, it is loaded to the process address space, DllMain being called with flag DLL\_PROCESS\_ATTACH. However, DllMain won't be called again with DLL\_THREAD\_ATTACH. It is usually used for the initialization of global and static variables and the allocation of thread-local slots.
2. When the process creates a new thread, DllMain is called with DLL\_THREAD\_ATTACH.
3. When an thread exits normally, DllMain is called with DLL\_THREAD\_DETACH. In contrast, if the thread is terminated by TerminateThread, DllMain is not called. Hence, it is dangerous to terminate a thread violently.
4. When the process exists, any other threads other than the main thread should have existed. Those who is still running will be directly terminated, possibly leaving global objects in inconsistent states, for example, locks still held. DllMain will be called with DLL\_PROCESS\_DETACH.

In order to protect the process address space from being corrupted, DllMain is called with the loader lock being held. Hence trying to call a function that acquires the loader lock, like LoadLibrary and GetModuleNameA will cause a deadlock. Synchronizing with another thread, regardless of the lock order, may also cause a deadlock.

In the problem described in the beginning, the constructor creates a thread, whose execution is blocked by waiting for the loader lock to call DllMain. When it tries to synchronize with the new thread, it becomes trapped in the deadlock because the other thread never gets the chance to start running.

Hence, the initialization code of a dynamically linked library should be as simple as possible. The complicated ones should be implemented in the lazy mechanism.

On the other hand, the destruction code should also be careful, because the remaining threads will be terminated before DLL unloading, which may leave global objects in inconsistent states, causing memory corruption or deadlocks.
