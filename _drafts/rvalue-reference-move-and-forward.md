---
layout: post
title: Rvalue Reference, Move and Forward
tags: [c++]
comments: true
---

Rvalue reference is a new feature introduced by C++11 that seems confusing at
first sight. After all, why do we need another reference type in the existence
of one already?

Briefly speaking, a rvalue reference is bound to a temporary object. By
contrast, a lvalue reference is bound to other objects. Besides, a lvalue
reference to const, such as const MyClass&, can be bound to a temporary object,
too, which is the only way to bind to a temporary one prior to C++11.

But the explanation above says nothing about why we need a new reference type
specially for temporary objects. The simple way to explain it is: **Lvalue and
rvalue references are used to overload a function so that the rvalue reference
version can be implemented based on the knowledge that the argument passed to it
is temporary.**

<!--more-->

# The importance of rvalue references #

A temporary object is one that is created and destroyed inside an expression.
The most obvious difference of a temporary object from others is that it has no
names and can only be referred to once.

The following examples clearly demonstrate when a temporary object is created.

```c++
int a = 1;
int b = 2;
int c = 3;
// A temporary int is created to store the value of a + b.
int d = a + b + c;

struct Point {
    int x;
    int y;
    Point(int x_, int y_): x(x_), y(y_) {}
};
std::vector<Point> keypoints;
// A temporary Point object is created and immediately passed to push_back.
keypoints.push_back(Point(3, 10));
```

In the second example, there is an obvious waste to construct a temporary object,
copy it into the vector and then destroy it, especially if the object is so
big that copying is a time-consuming operation, such as a vector containing lots
of elements. If we know that the argument passed to the function is a temporary
object, which will be soon released anyway, we may steal the resource from it
to save a needless copy. It is natural to think further that we should implement
two versions of the function, one receiving an ordinary reference and copying
from it, the other receiving a temporary object and stealing from it. That's the
reason why rvalue references are added to C++11.

# Move constructor and move assignment #

The syntax of declaring a lvalue reference is appending an ampersand (&) to the
type, such as int& and string&. Similarly, to declare a rvalue reference, you
need to append two ampersands (&&) like int&& and string&&.

Provided rvalue references, there will be two ways to construct a new object
from another existing one, copying or stealing the resources from it depending
on if the existing one is a temporary object. The former is the well-known copy
constructor, and the latter is called move constructor.

```c++
class A {
public:
    // Implemented by allocating a new memory and copying the contents of data_.
    A(const A& other);
    // Implemented by stealing data_ directly from the other.
    A(A&& other) {
        data_ = other.data_;
        // Because we have stolen data_ from the other object, we have to reset
        // the pointer so that it knows data_ is gone.
        other.data_ = nullptr;
    }
private:
    char* data_;
};
```
