---
layout: post
title: Forking in Multi-threaded Programs and Signal Handlers
tags: [concurrency]
comments: true
---

In Linux, forking in multi-threaded programs can be dangerous, even more in
signal handlers. Though the forked child process copies the address space of the
parent, only the caller thread is cloned into the child process, while all the
other threads get disappeared. When it comes to synchronization, if another
thread is holding a lock when the process forks, the lock will never be freed in
the child process, possibly also leaving the protected shared data in a
corrupted state.

Hence, thread-safe is not safe enough for a forked child process.

To be safely called in a child process, the function should be not only
thread-safe but async-signal-safe.

<!--more-->

## Async-signal-safe functions ##

Before understanding async-signal-safe functions, we should take a look at
signal handlers in Linux. 

Signal is a method of asynchronous communication between
processes. When a process receives a signal, a thread of it must stop what it is
doing and execute the signal handler. After the signal handler returns, the
thread will resume from where it is interrupted and continue. Since a signal can be delivered
to a thread at any time, the data can be in any state when the thread is executing the signal handler, even with some locks occupied.
Therefore, a function that is safe to be called inside a signal handler should
not touch the unreliable global variables, or acquire the locks that can lead to
deadlock. Those are called async-signal-safe functions.

Hence, async-signal-safe is stricter than thread-safe. Although a
function can be thread-safe by protecting the shared data by locks, **an
async-signal-safe function must not use any locks**. Even if the lock is recursive, which can be reacquired by the
same thread more than once without causing deadlocks, the shared
data protected by the lock can be in a corrupted state.

Unfortunately, many library functions by nature cannot avoid internal locks in
order to be thread-safe, such as:

+ malloc/free
+ I/O functions like printf/getc

Hence, all the functions above are not safe to be called in a signal handler.

Similar to signal handlers, when forking in a multi-threaded program, all the
other threads in the child process disappear, leaving the data they are
modifying in a corrupted state.

Due to this similarity, the functions that are called in the child process are
required to be async-signal-safe in a multi-threaded program.

## How to lessen the async-signal-safe restriction ##

As is compared to thread-safe, async-signal-safe is much too strict. The library
functions like malloc can not avoid locks in any case. To lessen the
restriction, pthread library provides a function **pthread_atfork** to register
callbacks around the forking procedure so that a multi-threaded program can be
safely forked.

```c
int pthread_atfork(
    void (*prepare)(void), 
    void (*parent)(void),
    void (*child)(void));
```

**pthread_atfork** accepts three callbacks, the first of which is called before
forking in the parent process, the second after forking in the parent process,
and the last after forking in the child process. **pthread_atfork** can be
called many times so as to install more than one callbacks. The **parent** and
**child** callbacks are called in the exact order that they are installed, while
the **prepare** callbacks are called reversely.

For library authors, it is common to **acquire all the locks used in the library
in the prepare callback, release them in the parent callback, and reset them to
the unlocked state in the child callback**. Furthermore, **the locks must be
recursive so that it is safe to fork in a signal handler**.

By acquiring all the locks, it is guaranteed that all the shared data are in a
valid state, since no other threads can be modifying them then. After forking,
it is safe to release all the acquired locks in the parent and child process,
respectively. It is noticeable that we release the locks in the parent process
but reset them to the unlocked state in the child process, and we require all
the locks to be recursive in order to safely fork in signal handlers.

Without consideration of forking in signal handlers, it is equivalent to simply
release locks in parent and child processes. Besides, the locks are not required
to be recursive, either.

## Forking in a signal handler ##

The library call **fork** itself is said to be async-signal-safe in the POSIX
standard, which means it can be safely called in a signal handler. However, the
callbacks registered by **pthread_atfork** are not required to be
async-signal-safe, in which case, the consequence is *undefined*.

In case of forking in a signal handler, glibc uses recursive locks instead, so
the prepare callback is able to reacquire the locks even if they are being held
by the interrupted thread. However, it leads to the problem that the child
callback does not know how many times the locks have been acquired recursively.
Hence, it simply resets the locks to the initial unlocked state.

Nevertheless, forking in multi-threaded programs and signal handlers is
dangerous and tricky. It is hard to write correctly. Therefore, better avoid
forking in those cases, or restrict the use of forking in the fork-and-exec
idiom.
