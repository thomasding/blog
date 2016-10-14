---
layout: post
title: Copy and Swap
tags: [c++, exception-safe]
comments: true
---

Copy-and-swap idiom is frequently used to implement a copy assignment operator.

If an exception is thrown during copy assignment, it may cause some data members
to remain unchanged while some others already assigned to new values. In
order to ensure that the object to be copied to be in the original value if an
exception is thrown, we first **create a temporary object by calling its copy
constructor with the object to be copied from, and then swap the contents of the
temporary object with the destination object**. Swap is required to be a
no-failure operation, that is, it always succeeds and never throws exceptions. 
Considering swap is always implemented by interchanging pointers and primitive
variables, it is easy to satisfy the requirement. If an exception is thrown in
the copy step, the temporary object is properly freed, and the destination
object keeps its original value.

<!--more-->

# Copy-and-swap by example #

The following copy assignment operator suffers from exception damage.

```c++
class User {
public:
    User(const std::string& name, const std::string& address):
        name_(name), address_(address) {
    }

    User(const User& other): name_(other.name_), address_(other.address_) {
    }

    User& operator=(const User& other) {
        name_ = other.name_;
        address_ = other.address_;   // Consider an exception is thrown here.
        return *this;
    }

    // Omit getters and setters ...
private:
    std::string name_;
    std::string address_;
};
```

By applying copy-and-swap idiom, the object will keep its original value if an
exception is thrown in the copying assignment operator.

```c++
class User {
public:
    User(const std::string& name, const std::string& address):
        name_(name), address_(address) {
    }

    User(const User& other): name_(other.name_), address_(other.address_) {
    }

    User& operator=(const User& other) {
        // If an exception is thrown, *this will not be modified.
        User tmp(other);
        swap(tmp); 
        return *this;
    }
    
    void swap(User& other) {
        using std::swap;
        swap(name_, other.name_);
        swap(address_, other.address_);
    }

    // Omit getters and setters ...
private:
    std::string name_;
    std::string address_;
};

void swap(User& a, User& b) {
    a.swap(b);
}
```

# The right way to define a swap function #

In the code above, you may find some interesting places in the implementation, like:

1. calling `std::swap` to interchange the data members;
2. defining `swap` as a a public method as well as a non-member function;
3. `using std::swap` at the beginning of our own swap.

Most STL containers and objects has its own public member `swap` as well as a
non-member function `swap`. When we are defining our own `swap` function, we'd
better conform to this convention. The advantage of defining the extraneous
non-member `swap` is taken when implementing a template. 

```c++
template<typename T>
void rotate(T& a, T& b, T& c) {
    using std::swap;
    swap(a, b);
    swap(b, c);
}
```

In the function above, if T is deduced to be a STL container, the non-member swap
of the container is called. If T is deduced to be our own type, for example,
`User`, our non-member swap function is called instead. If T is deduced to be a
primitive type, such as int, the std::swap template is instantiated.

It may seem weird to place `using std::swap` at the beginning of the function.
Calling `std::swap` is the first impression that comes to our mind, like the
following version:

```c++
template<typename T>
void rotate(T& a, T& b, T& c) {
    std::swap(a, b);
    std::swap(b, c);
}
```

Unluckily, the latter version is wrong. Long story short, **always put `using
std::swap` before calling swap and call it without any namespace delimiters**.
The reason why we should not use the namespace-delimited swap is a C++ name lookup
rule named **argument-dependent lookup**, abbreviated as **ADL**.

# The right way to call a swap function #

Curious readers may have found that in the implementation of `User`, we define
the non-member swap function in our own namespace, outside of namespace `std`.
So how does the compiler determine which namespace to lookup when a call to swap
occurs?

**ADL** makes the compiler to lookup the namespaces of the arguments first for
the function in addition to the current scope and namespaces.

The following code is a very common scenario making use of this rule:

```c++
#include <iostream>

namespace myns {
  void print() {
      std::cout << 1;
  }
}
```

We are calling `operator<<` in the function, but `operator<<(ostream&, int)` was
not defined in namespace `myns` and we didn't use namespace `std` either.
However, the code is compiled successfully, because the compiler tries to find
`operator<<` in the namespaces of the arguments, namely, `std`, and there it
finds the definition.

Back to our call to swap, using the non-delimited swap, the compiler will try to
find the definition in the namespaces of the arguments, where the
object-specific swap is usually defined. But there are cases that the swap
template in namespace `std` is used, for example, for primitive types and those
who don't have their own swap function, we have to put `using std::swap` so that
the compiler will find it in namespace `std`.

If we use the `std::swap` version, only the swap template defined in namespace
`std` is called. And it will result in our swap function to be called twice.
What? I've heard you say. Here, let's have a look at the default version of
`std::swap` to figure it out. 

# How is std::swap implemented #
