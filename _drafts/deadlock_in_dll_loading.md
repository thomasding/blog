---
layout: post
title: Deadlock in DLL Loading
tags: [windows, dialog]
comments: true
---

*John:* Hi Tommy, I met a problem when I was writing a DLL recently. The program was stuck before entering the main function, but I don't know why. However, when I linked the library statically instead of dynamically, the program ran normally without blocking.

*Tommy:* While... it seems that the global variables that you defined in the DLL caused this deadlock. May I ask what the global variables did in their constructors?

*John:* Let me think... I created a worker thread in one of the constructors and waited for the thread to finish its initialization and...

*Tommy:* OK, that's enough to cause a severe deadlock.

*John:* But it worked well in Linux, where I compiled it as a shared object. Only after I migrated it to Windows did it refuse to work.

*Tommy:* That's because of the different initialization mechanisms between Windows DLLs and ELF shared objects. The same code just won't work in Windows even though it ran successfully in Linux.

<!--more-->

*John:* Please elaborate it, Tommy.

*Tommy:* There is a concept called **DLL entry point**, which you may consider as a special function. People sometimes use **DllMain** as a synonym for it, which will be called in the following four situations:

- when the DLL is to be first loaded in a process;
- when the DLL is to be finally unloaded in a process;
- when a new thread is created after the DLL is loaded;
- when a new thread exits cleanly after the DLL is loaded.

There is a parameter to distinguish the four situations, whose value can be:**DLL\_PROCESS\_ATTACH **, **DLL\_PROCESS\_DETACH **, **DLL\_THREAD\_ATTACH **, **DLL\_THREAD\_DETACH **.

*John:* I see... When the DLL is being loaded into a process, the system calls the DllMain in the DLL with DLL\_PROCESS\_ATTACH, which initializes the global variables by calling their constructors. When the DLL is being unloaded, the system calls it with DLL\_PROCESS\_DETACH, which in turn cleans up the global variables by calling their destructors. Isn't it?

*Tommy:* That's mostly true. However, the tricky problem is that there is a process-wide loader lock. The system acquires the lock before calling DllMain and releases it afterwards. Therefore, as you can see, it will cause a deadlock if you dare to call any function that tries to acquire the loader lock in DllMain.

*John:* Well, I hope the functions that requires the loader lock are rare.

*Tommy:* Sadly, I must say, quite a lot of functions requires the loader lock directly or indirectly. Only a few that calls either no external functions or part of functions in kernel32.dll are really safe to be called.

In your problem, let's assume that there is a main thread A who is going to create another thread B. The process of execution can be described like this:

1. Acquire the loader lock.
2. Call DllMain with DLL\_PROCESS\_ATTACH.
3. In DLLMain, create a new thread B.
4. Thread B tries to acquire the loader lock before calling DllMain with DLL\_THREAD\_ATTACH.
5. Thread A waits for thread B to complete its initialization. However, thread B is being blocked by thread A, thus causing a deadlock.

*John:* Oh I see... It's really dangerous to do elaborate tasks in the initialization code. Do you have any suggestions on writing a safe DllMain?

*Tommy:* I agree. The general suggestion is to keep the initialization code as simple as possible and postpones the complicated ones as much as possible by doing lazily. Global pointer setting to NULL? OK. Allocating memory? Fine. Setting global variables to the initialized values? Of course. Allocating thread-local slots? That's what it does for. Calling a function in user32.dll or gdi32.dll? I'd rather not. Creating a new process? Never. Synchronizing with another thread or creating a new one? No. Calling LoadLibrary? Are you kidding me?

To know more, you can read [DLL Best Practice][1] and [DllMain entry point][2].
