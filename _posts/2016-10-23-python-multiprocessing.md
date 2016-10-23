---
layout: post
title: Python Multiprocessing
tags: [python, concurrency]
comments: true
---

Multiprocessing is an approach to improve performance by utilizing many CPUs in
Python. A C++ programmer is usually more familiar with multithreading, which
creates many worker threads to do the tasks in parallel. However, due to the
existence of GIL in Python, only one thread can run Python code at a time, when
other threads must wait for it to release the GIL, such as waiting for I/O
operations or mutexes. To achieve real parallelism, it is required to create
many processes rather than many threads. Thankfully, python package
**multiprocessing** provides easy-to-use API to create many processes and
communicate between them. Furthermore, the API of **multiprocessing** is similar
to **multithreading**, so that the same technique can be applied to
multithreading scripts as well.

<!--more-->

## Creating worker processes ##

Creating a worker process is as simple as creating a **Process** object and
calling the **start** method.

```python
import multiprocessing as mp
def worker(a, b):
    print(a + b)
if __name__ == '__main__':
    workerproc = mp.Process(target=worker, args=(1, 2))
    workerproc.start()
    workerproc.join()     # Wait for it to complete.
```

## Doing tasks in parallel gracefully  ##

Though you can manually create processes, it is rare that you need such
low-level API. A more common scenario is doing a list of mutual independent
tasks in parallel and collecting the results. For example, provided with a list
of file names, you want to load each file and count the characters in it. In
that case, **multiprocessing.Pool** is what you need, which manages a fixed
number of workers, automatically assigns tasks to them and collects the results.

```python
from multiprocessing import Pool
def count_chars(filename):
    with open(filename) as fin:
        return len(fin.read())
filelist = ['file1', 'file2', 'file3', 'file4']
if __name__ == '__main__':
    with Pool(processes=2) as pool:
        result = pool.map(count_chars, filelist)
        print(result)
```

In constrast to **Pool.map**, **Pool** also provides an async version named
**map\_async**. In addition to the function and iterable object, **map\_async**
accepts an optional argument **callback**, a function receiving the result
object that will be called when the operation is done successfully, and another
optional argument **error_callback**, similar to **callback** but will be called
on failure with the exception object instead.

The method **map\_async** returns an **AsyncResult** object, which provides
methods to get the result in a blocking manner, that is, blocking the current
thread until the result is available.

Here is an example using **Pool.map\_async**.

```python
from multiprocessing import Pool

def count_chars(filename):
    with open(filename) as fin:
        return len(fin.read())

filelist = ['file1', 'file2', 'file3', 'file4']

if __name__ == '__main__':
    with Pool(processes=2) as pool:
        async_result = pool.map_async(count_chars, filelist)
        # Do something else while the workers are counting the characters...
        # Get the result (may block the current thread).
        print(async_result.get())
```

Or you may prefer the callback version:

```python
from multiprocessing import Pool

def count_chars(filename):
    with open(filename) as fin:
        return len(fin.read())

def success_callback(result):
    print('Get result', result)

def error_callback(e):
    print('Throw exception', e)

filelist = ['file1', 'file2', 'file3', 'file4']

if __name__ == '__main__':
    with Pool(processes=2) as pool:
        async_result = pool.map_sync(count_chars, filelist, 
                                     callback=success_callback,
                                     error_callback=error_callback)
        # Wait for the task to complete.
        async_result.wait()
```

For curious readers who wants to known where the callback is called, here is the
answer: the **Pool** object internally manages many background threads, one of
which is a result fetching thread that waits for the results from the worker
processes. The callback is called in that result fetching thread in the main
process.

In all the examples above, we uses the with-statement to manage a **Pool**
object. It is equivalent to the following implementation:

```python
pool = Pool(processes=2)
try:
    # The with-statement body...
finally:
    # Execute the finally-clause whether an exception is raised or not.
    # Terminate all the worker processes violently by sending signal SIGTERM.
    pool.terminate()
```

Hence, all the async tasks must be guaranteed to be done before exiting the
with-statement. Otherwise, the worker processes will be terminated, leaving the
tasks undone. One way to ensure that all the tasks are done is to close the pool
before joining the worker processes, like the following code.

```python
with Pool(processes=2) as pool:
    # Assign tasks to pool...
    # Close the pool, which means we won't assign new tasks.
    # Must be called before joining.
    pool.close()
    # Wait for the worker processes to complete the tasks that have been
    # assigned.
    pool.join()
```

Nevertheless, the with-statement style is encouraged, because it ensures that no
processes will be left running whether or not an exception is raised in the
body.

Another common scenario is that you want some tasks to be done in parallel,
while the order of the result is insignificant. Here, you should use
**Pool.apply_async**, which assigns the task to a worker process and returns an
**AsyncResult** object.

Here is a simplified example demonstrating a spider program that fetches pages
from the Internet.

```python
import multiprocessing as mp

class Spider:
    def __init__(self):
        '''Create a pool with 8 workers.'''
        self._pool = mp.Pool(processes=8)

    def download(self, url, filename):
        '''Add a new task that downloads the URL and save as the given 
        filename.'''
        self._pool.apply_async(self._do_download, args=(url, filename))
        
    def close(self):
        '''Close the Spider object and wait for all the tasks to be 
        done.'''
        self._pool.close()
        self._pool.join()
        
    def _do_download(self, url, filename):
        '''The method that downloads the URL.'''
        # Implementation omitted...
```

## Communication is expensive ##

In contrast to communication between threads, exchanging data between processes
is much more expensive. In Python, the data is pickled in to binary format
before transferring on pipes. Hence, the overhead of communication can be very
significant when the task is small. To reduce the extraneous cost, better assign
tasks in chunk.

```python
import multiprocessing as mp

def add2(i):
    return i + 2

def add2_for_all(number_list):
    with mp.Pool(processes) as pool:
        return pool.map(add2, number_list, chunksize=10)
```

If **chunksize** is given, the tasks are grouped into chunks before being
assigned to worker processes. Thankfully, the operation of packing into chunks
and unpacking into separate tasks are done internally in the `Pool` object, the
worker function doesn't need to receive chunks, that is, the chunk is
transparent to the worker function. The default value for **chunksize** is 1.

## Be careful about global variables ##

There are three ways to create subprocesses: **spawn**, **fork** and
**fork-server**. Two of them are mostly used:

1. **spawn**: starting a brand new python interpreter, loading the current
   module, passing the arguments in pickled format to the subprocess and calling
   the target function. It is the default and only way in Windows.
2. **fork**: cloning a subprocess from the current one, and the calling the
   target function in the subprocess. It is the default way in Unix-like
   systems, like Linux.
   
Because of the difference that spawn starts a new process from scratch while the
forking clones from the parent, the value of variables can be different, even
unpredictable, if the script is run both in Windows and UNIX. Take the following
code as an example:

```python
import multiprocessing as mp

msg = 'Init'

def worker():
    print(msg)
    
if __name__ == '__main__':
    msg = 'Main'
    proc = mp.Process(target=worker)
    proc.start()
    proc.join()
```

If the code is run in UNIX, the output will be "Main", for the memory space of
the subprocess is the same as the parent after forking. However, in Windows, the
output will be "Init" instead, for the subprocess loads the module from scratch.

In order that the behavior of subprocesses are the same in Windows and UNIX, be
careful not to use these global variables. To be more exact, **every variable
that may by modified after module initialization should be explicitly passed to
the function as an argument**. The arguments are passed in pickled format in
spawn mode, so it is required that **all the arguments be picklable**.

Besides, since the module is loaded again in spawn mode, remember to protect the
startup code in `if __name__ == '__main__'`:

```python
import multiprocessing as mp

msg = 'Init'

def worker():
    print(msg)
    
# DO NOT WRITE LIKE THIS. IT WON'T DO IN SPAWN MODE.
msg = 'Main'
proc = mp.Process(target=worker)
proc.start()
proc.join()
```

## Don't mix multiprocessing with multithreading ##

Although forking almost clones everything from the parent processing, when it
comes to threads, only the caller thread is cloned in the child process. It
may bring about deadlock if forking in multithreading programs.

In multithreading programs, when the resources are accessed by many
threads, they should be protected by concurrency mechanism, like a mutex (mutual
exclusive lock, which ensures only one thread holds the lock at a time). If
another thread holds the mutex while the process forks, the mutex will remain
locked forever in the child process, and any attempt to acquire the mutex will
cause deadlock. Therefore, the most simple rule is **never mixing
multiprocessing with multithreading**.

Nevertheless, it is allowed to fork in a multithreading program, but it is
tricky to write a correct one.

## More on GIL ##

GIL only exists in CPython, which is the default python interpreter in Linux and
Windows. Although GIL hinders the performance of multithreading, it is the
result of compromise. Back to the age of CPython 1.5, the attempt was once made
to replace GIL with more smaller locks, which, unfortunately, causes about 40%
performance penalty to single-threaded programs. Hence, it is acceptable to
maintain high performance for single-threaded programs by simplifying the
interpreter with GIL. As a result, multithreading is mostly used for
I/O-intensive applications rather than computation-intensive.

Other implementations like Jython (based on JVM) and IronPython (based on .NET)
have no GIL, and in consequence, don't suffer from GIL penalty on multithreading.
