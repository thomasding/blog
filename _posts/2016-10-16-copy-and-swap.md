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
        name_(name), address_(address) {}
    User(const User& other): name_(other.name_), address_(other.address_) {}
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
        name_(name), address_(address) {}
    User(const User& other): name_(other.name_), address_(other.address_) {}
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

1. calling std::swap to interchange the data members;
2. defining swap as a a public method as well as a non-member function;
3. `using std::swap` at the beginning of our own swap.

Most STL containers and objects has its own public member swap as well as a
non-member function one. When we are defining our own swap function, we'd
better conform to this convention. The advantage of defining the extraneous
non-member swap is taken when implementing a template. 

```c++
template< typename T >
void rotate(T& a, T& b, T& c) {
    using std::swap;
    swap(a, b);
    swap(b, c);
}
```

In the function above, if T is deduced to be a STL container, the non-member swap
of the container is called. If T is deduced to be our own type, for example,
User, our non-member swap function is called instead. If T is deduced to be a
primitive type, such as int, the std::swap template is instantiated.

It may seem weird to place `using std::swap` at the beginning of the function.
Calling std::swap is the first impression that comes to our mind, like the
following version:

```c++
template< typename T >
void rotate(T& a, T& b, T& c) {
    std::swap(a, b);
    std::swap(b, c);
}
```

Unluckily, the latter version is wrong. Long story short, **always put `using
std::swap` before calling swap and call it without any namespace qualifiers**.
The reason why we should not use the namespace-qualified swap (one with `std::`
prefix) is a C++ name lookup rule named **argument-dependent lookup**,
abbreviated as **ADL**.

# The right way to call a swap function #

Curious readers may have found that in the implementation of User, we define
the non-member swap function in our own namespace, outside of namespace std.
So how does the compiler determine which namespace to lookup when a call to swap
is found?

**ADL** makes the compiler to lookup the namespaces of the arguments first for
the function in addition to the current scope and namespaces.

To understand ADL, let's look at a very common scenario making use of this rule:

```c++
#include <iostream>
namespace myns {
  void print() {
      std::cout << 1;
  }
}
```

We are calling operator\<< in the function, but `operator<<(ostream&, int)` was
not defined in namespace myns and we didn't use namespace std either.
However, the code is compiled successfully, because the compiler tries to find
operator\<< in the namespaces of the arguments, namely, std, and there it
finds the definition.

Back to our call to swap, using the unqualified swap, the compiler will try to
find the definition in the namespaces of the arguments, where the
object-specific swap is usually defined. But there are cases that the swap
template in namespace std is used, for example, for primitive types and those
who don't have their own swap function, we have to put `using std::swap` so that
the compiler will find it in namespace std.

If we use the std::swap version, only the swap template defined in namespace
std is called. However, it will result in our swap function to be called
twice. What? I've heard you say. Here, let's have a look at the default version
of std::swap to figure it out. 

# How is std::swap implemented #

Two versions of swap are provided by STL, one for generalized types,
and the other specifically for arrays.

```c++
// Defined in <utility>
template< class T >
void swap( T& a, T& b );
template< class T2, size_t N >
void swap( T2 (&a)[N], T2 (&b)[N]);
```

The former version, generalized for arbitrary type T, is implemented by calling
the move constructor and move assignment of type T. A possible implementation
may be like:

```c++
template< class T >
void swap( T& a, T& b ) {
    T tmp(std::move(a));   // Move constructor of a
    a = std::move(b);      // Move assignment of b
    b = std::move(tmp);      // Move assignment of tmp
}
```

In the example above, due to the lack of user-defined move constructor and move
assignment, the copy constructor and copy assignment is called instead. Considering
our copy assignment calls user-defined swap, the user-defined swap will be
called twice if we apply the std::swap function to swap two User objects,
causing unnecessary overhead. Hence, whenever swapping two objects, the best
practice is to write `using std::swap` at first and then call the unqualified swap.

The latter version, specialized for array type T[N], is implemented by calling
the unqualified swap of T (which may be the user-defined swap or std::swap by
default) to swap each elements of the two arrays, rather than to swap the two
pointers directly. The operation to swap the elements of two arrays is done by
calling a more generalized version of swap for sequence called swap_ranges that
is used to swap two iterators, like `swap_ranges(a, a+N, b)`.

```c++
// Defined in <algorithm>
template< class ForwardIt1, class ForwardIt2 >
ForwardIt2 swap_ranges( ForwardIt1 first1, ForwardIt1 last1, ForwardIt2 first2 );
// Omit the execution policy version for brevity.
```

Hence, **the time complexity of swapping two arrays is O(N)**. If swapping two
pointers is enough in practice, you have to write your own swap operation
instead of calling std::swap on them.

Furthermore, let's look at the last generalized swap in the swap family that is
used on two iterators (or pointers, which the iterators resembles).

```c++
// Defined in <algorithm>
template< class ForwardIt1, class ForwardIt2 >
void iter_swap( ForwardIt1 a, ForwardIt2 b );
// Omit the execution policy version for previty.
```

It simply swaps the objects that the iterators point to, like:

```c++
template< class ForwardIt1, class ForwardIt2 >
void iter_swap( ForwardIt a, ForwardIt b) {
    using std::swap;
    swap(*a, *b);
}
```

For those readers who are not familiar to iterators, you can simply regard them
as pointers. Don't be afraid of them, for they are designed to work like pointers.

# Do swap throws? #

All the guarantee brought by copy-and-swap idiom is merely based on a plausible
assumption that swap never throws. Considering the implementation of swap
usually requires no memory allocation and complicated operations, it is
generally regarded as no-fail (even stronger than no-throw, for a function that
throws no exception may fail and report it by logging, for example). However,
the reality is subtler than this simple assumption.

Let's take video memory as an example. Nowadays, most computers for scientific
computation are equipped with powerful video cards. To deliver the computation
task to GPU, the program have to copy the data from the mainboard memory to the
video memory at first. There is a situation that we are going to swap two
objects, one on the mainboard memory and the other on the video memory.
Meanwhile, only the contents of the objects should be changed, not the
positions, that is, after the swap, the content of the object on the mainboard
memory becomes the other while the the position of it still remains on the
mainboard memory, not being moved to the video memory, though the exact address
of the object may be modified. Such swap, inevitably, is related to copying
between devices, and hence, is more likely to fail in comparison to simply
interchanging two pointers on the same device.

Therefore, swap may fail, although the scenario is more complicated. And hence,
copy-and-swap is not versatile.

Curious readers who know the noexcept qualifier may have asked how we decide
whether or not a swap function is noexcept. The answer is, we don't, but we
leave it to the compiler to decide by describing the condition on which the swap
function is noexcept, which is called noexcept specification. Further discussion
about noexcept specification of swap will be posted in another article, for it's
not only related to the other swaps we call, and the way how allocators are
abstracted and implemented in STL. It's easy to understand, but requires more
efforts.
