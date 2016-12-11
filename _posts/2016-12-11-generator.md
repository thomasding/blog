---
layout: post
title: Generator
tags: [python]
comments: true
---

Generator are powerful tools that make it easy and convenient to create
iterators with shorter and cleaner code. Perhaps seeming unfamiliar though, the
whole looping concept of Python is constructed on iterators.

An iterator can be regarded as a tool to traverse a group of objects in a
specific order. Any object can be an iterator as long as it has a special member
function named **__next__(self)**. Each time **__next__(self)** is called, the
function returns the next accessible object to the caller, until there are no
more objects remaining to be iterated, at which time an exception
**StopIteration** is raised. Python provides a built-in function called
**next(obj)** that calls the **__next__** method of the given **obj**.

<!--more-->

## Iterators and iterables ##

A Python object that can be iterated is regarded as an **iterable**, such as a
list, set and range, which has a special member function named
**__iter__(self)** that returns an iterator. Do not get confused with an
iterable object and an iterator: an iterator always moves forward, returns the
next object until reaching the end, whereas an iterable object is the container
over which an iterator traverses. Nevertheless, an iterator can have
**__iter__(self)**, too, which usually returns itself. Python provides a
built-in function **iter(obj)** that calls the **__iter__(self)** method of the
given **obj**.

Let's take a practical look at some examples of iterables and iterators.

```python
evens = [2, 4, 6, 8]   # A list of even numbers that is iterable
ten = range(10)        # A range from 0 to 9 that is iterable

people = {'Alan': 0, 'Bob': 1}   # A dict, not an iterable
people_items = people.items()    # An object of dict_items which is iterable

it = iter(evens)       # Create an iterator of list evens
next(it)               # Returns 2
next(it)               # Returns 4
next(it)               # Returns 6
next(it)               # Returns 8
next(it)               # Raises StopIteration
```

In fact, we can rewrite the **for** syntax completely in the form of how it
actually works:

```python
#The classical for - loop syntax
for item in collection:
    do_something(item)

#The equivalent iterator - ish syntax
it = iter(collection)
try:
  while True:
        item = next(it)
        do_something(item)
except StopIteration:
    pass
```

## How generators make easy the life of writing iterators ##

Writing iterators is not as happy as using it, for it requires a special member
function named **__next__(self)**, hence, restricting itself to be a class
instead of a function, introducing the headache of data members and
indentations. Suppose an iterator that walks over two given iterables side by
side.

```python
# An iterator class that walks over two iterables side by side.
class MyZip:

    def __init__(self, a, b):
        self._it_a = iter(a)
        self._it_b = iter(b)

    def __iter__(self):
        return self

    def __next__(self):
        na = next(self._it_a)
        nb = next(self._it_b)
        return na, nb

# How to use it in practice
for a, b in MyZip([1, 3, 5], [2, 4, 6]):
    print(a, b)
```

Thankfully, generators make it quite clear to write an iterator. Briefly
speaking, a generator is a special function that has at least one **yield**
statement in the body of it (we will revisit this definition later on, which is,
strictly speaking, not adequate). In another way, as long as having at least one
**yield** in it, a function will no longer be regarded as an ordinary function,
but a generator instead. Each time the **yield** statement is reached, the
function temporarily exits and returns the object that is yielded. When **next**
is called on it, the function resumes its execution from the yield statement
until it finally exits, at which time an exception **StopIteration** is thrown.

Hence, the **MyZip** can be written in a generator style.

```python
def my_zip(a, b):
    it_a = iter(a)
    it_b = iter(b)
    try:
        while True:
            yield next(it_a), next(it_b)
    except StopIteration:
        pass
```

We can write a generator that multiplies each number by a given number.

```python
def my_mul(lst, factor):
    for num in lst:
        yield num * factor
```

We can also write a generator that removes all the **None** object in an
iterable.

```python
def my_filter(lst):
    for obj in lst:
        if obj is not None:
            yield obj
```

We can write our own **map** function:

```python
def my_map(func, iterable):
    for item in iterable:
        yield func(item)
```

## Generator functions and generator iterators ##

After a brief look at what a generator is capable of, we need to figure out some
important things of it. First of all, the function that contains **yield**
statement is not a generator, it is **what the object that the function
returns** that is the generator, which is also an iterator. To distinguish the
two concepts, the function is usually called a **generator function** and the
returned object a **generator iterator**.

Take the **my_filter** function above as an example.

**my_filter** itself is a function, yet a special one. An ordinary function returns
what it **return**s in the function body, while a generator function returns a
generator iterator.

Now we execute `gen = my_filter([1, 2, None, 3])`. After it, **gen** is referred
to a generator iterator returned by the function call `my_filter([1, 2, None,
3])`. However, the function body of **my_filter** has not started executing at this
time yet.

When **next** is first called on the iterator **gen**, the function body starts
executing until the yield statement is reached. Then the function suspends its
execution and takes the object being yielded as the return value of the **next**
function call.

Not only an ordinary iterator, a generator iterator is capable of sending
information back to the function itself by member method **send(self, obj)** and
**throw(self, type, [value, [, traceback]])**. 

The member method **send** resumes the generator function, takes the object
passed to it as the value of the suspended yield expression and keeps its
execution until another yield or the end of the function, in which case an
exception StopIteration is raised. Instead of taking the parameter as the value
of the yield expression, the member method **throw** raises the specified
exception at the yield statement.

```python
# A generator function that multiplies a number by two
def my_times2():
    num = yield None
    while True:
        num = yield num * 2   # First yield out num * 2 and then receive a number
        
times2_iter = my_times2()
next(times2_iter)     # Start the execution of the function
print(times2_iter.send(2))  # Print 4
print(times2_iter.send(4))  # Print 8
```
