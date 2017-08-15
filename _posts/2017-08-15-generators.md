---
layout: post
title: Generators
tags: [python]
comments: true
---

A generator is like a box with a button. Each time you press the button,
the box pops something new.

It is generally used to construct a finite/infinite list lazily.

We can write a generator to make an infinite list of natural numbers
starting from a given integer.

<!--more-->

```python
def natural_list(n):
    while True:
        yield n
        n = n + 1
```

Here is a simple program to use the generator:

```python
nl = natural_list(10)
print(next(nl))
print(next(nl))
```

The first line `nl = natural_list(10)` creates a generator object with
the parameter `n` initialized to `10`. The second line runs the generator
from the beginning until the yield statement, where `n` is `10`.
The function temporarily stops at the yield statement and returns 10 to
the caller. The third line resumes the generator from where it stops and
continues until the yield statement is executed again with `n` being `11`.

A generator can also be used in a **for** loop. However, in the example above,
the loop will never terminate.

```python
for i in natural_list(10):
    print(i)
```

For a terminating generator, when the generator function exits, a StopIteration
exception is raised instead.

This is explained in the following example.

```python
def plus_one(n):    # Define a generator that yields n and n + 1.
    yield n
    yield n + 1

po = plus_one(10)   # Create a generator object.
next(po)            # This expression evaluates to 10.
next(po)            # This expression evaluates to 11.
next(po)            # A StopIteration exception is raised.

list(plus_one(1))   # We'll get a list [1, 2]
```

What makes a generator special is that you can not only get values from it,
but pass values back, even exceptions. This is very much like what
a coroutine is for.

Let's write a generator that receives an integer and multiplies it by a given
integer.

```python
def times(multiplier):
    n = None
    m = None
    while m != 0:
        m = (yield n)
        n = m * multiplier

times_two = times(2)      # Create a generator with multiplier 2
next(times_two)           # Run the generator till the yield statement.
times_two.send(6)         # Evaluate to 12.
times_two.send(3)         # Evaluate to 3.
times_two.send(0)         # Raise a StopIteration exception.
```

When the generator object is created by `times(2)`, the function body has
not been started yet. Hence the second line `next(times_two)` starts the
execution of the function and yields `None`, which is the initial value of
`n`. The third line `times_two.send(6)` resumes the function body and passes
`6` back as the value of the yield expression. The generator continues its
execution until it yields `12`.

To raise an exception at the yield expression instead of passing a value,
you should call `throw` in the place of `send`.

In the following example, a RuntimeError exception is raised at the yield
statement to stop the generator eagerly.

```python
times_two = times(2)
next(times_two)
times_two.throw(RuntimeError('quit'))
```

You may try implementing a coroutine with generators just like what the **co**
library does for Node.js. It's okay for experimental purposes, but not
encouraged in practice, because
Python supports coroutines natively with its asynchronous I/O library.

When you want to construct a list lazily, use generators. When you want a
coroutine, try `asyncio` instead.
