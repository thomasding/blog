---
layout: post
title: Fork Safety With Multithreading
tags: [concurrency]
comments: true
---

In Linux, forking in multithreading programs can be dangerous. Though the forked
child process copies the address space of the parent, only the caller thread is
cloned into the child process, while all the other threads get disappeared. As
for synchronization, if another thread is holding a lock when the process forks,
the lock will never be freed in the child process, in addition possibly leaving
the shared data being protected by the lock in a corrupted state.

Hence, thread-safe is not enough for the forked child process. We call for
stricter constraints so that the functions can be safely called in the child
process. The constraint, as is stated in the POSIX standard, is
async-signal-safe.

<!--more-->

## Async-signal-safe functions ##

Before understanding async-signal-safe functions, we should take a look at
signal handlers in Linux. Signal is an asynchronous communication method between
processes. If a process receives a signal, one thread of it must stop what it is
doing and execute the signal handler. After the signal handler returns, the
thread will resume from where it is interrupted. Since a signal may be delivered
to a thread at any time, when the thread is executing the signal handler, the
context can be in any state, even with some mutexes locked. The functions that
are safe to be called in a signal handler are said to be async-signal-safe.

Hence, async-signal-safe is a stricter constraint than thread-safe. Although a
function can be thread-safe by protecting the shared data by mutexes, **an
async-signal-safe function must not use any mutexes or locks**, for the thread
can be interrupted when it is holding the lock, resulting in deadlock if the
signal handler. Even if the lock is recursive, which can be reacquired by the
same thread more than once as long as it is freed for the same times, the shared
data protected by the lock can be in any intermediate state.

Unfortunately, many library functions by nature use internal locks so that they
are thread-safe, such as:

+ malloc/free
+ I/O functions like printf/getc

All the functions above are not safe to be called in a signal handler.

Forking is very similar to handling a signal. In the child process after
forking, all the other threads disappear, leaving the global data in the state
when the process is forked, just like in a signal handler, where the caller
thread can be interrupted anywhere, only stricter, as if all the threads are
interrupted.

Due to this similarity, the functions that are called in the child process are
required to be async-signal-safe if multithreading is present.

## How to lessen the async-signal-safe restriction ##

As is compared to thread-safe, async-signal-safe is much too strict. The library
functions like malloc can not avoid locks anyway. To lessen the restriction,
pthread library provides a function **pthread_atfork** so that a program that
uses lock-protected functions can be safely forked.

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
the **prepare** callbacks are called in the reverse order.

For the library author, it is common to **acquire all the locks used in the
library in prepare callback, release them in parent callback, and reset them to
the unlocked state in child callback**. Furthermore, **the locks must be
recursive so that it is safe to fork in a signal handler**.

By acquiring all the locks, it is guaranteed that all the shared data are in a
valid state, for no other threads can modify them then. After forking, it is
safe to release all the locks that are acquired before in the parent and child
process, respectively. Curious readers may have noticed that we release the
locks in the parent process but reset them to the unlocked state in the child
process. In addition, it is required that all the locks are recursive. These
restrictions are added so as to fork in a signal handler.

Without consideration of forking in signal handlers, it is equivalent to simply
release locks in parent and child processes. Besides, the locks are not required
to be recursive, either.

As an example, the malloc-related locks in glibc are recursive, and glibc
registers the three callbacks so that it can be safely forked in a
multithreading program without causing memory corruption.

## Forking in a signal handler ##

The library call **fork** itself is said to be async-signal-safe in the POSIX
standard, which means it can be called in a signal handler. Paradoxically, the
callbacks registered by **pthread_atfork** may be not async-signal-safe. In that
case, the consequence is *undefined*.

## Fork and execv ##
