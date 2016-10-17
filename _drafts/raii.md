---
layout: post
title: RAII
tags: [c++]
comments: true
---

RAII is a very important technique in C++, although its full name is a little
obscure: Resource Acquisition Is Initialization. While RAII can be briefly
described as: **Bind a resource to an object, acquiring it in the constructor,
releasing it in the destructor, so that its life cycle is bound to that of the
object.**

To write a good RAII class, the following implementations should be **avoided**:
1. to manually acquire or release the resource like open()/close()
   method;
2. to transfer the ownership of the resource by manual methods like move() or
   transfer(), rather than move constructor or move assignment.
   
By conforming to RAII, many situations that introduces resource leaks can be
easily reduced, and at the same time, with clearer code (usually shorter).

<!--more-->

In C and C++ prior to C++11, the programmer must remember when to release a
resource, considering many incidents like exceptions and errors. The programmer
must ensure all the resources be properly released whatever route the program
chooses at runtime. It can be tough sometimes, especially if there are more than
one to manage in a function or class. In the next section, we can closely review
an implementation that violates RAII, growing convolving in order to properly
release the resources.

# Bad designs #

