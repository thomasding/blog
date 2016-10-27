---
layout: post
title: Rvalue Reference
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

Equally, there is a move assignment operator corresponding with a copy
assignment operator.

```c++
class A {
public:
    // Allocating a new memory and copying the contents of data_.
    A(const A& other) { /* ... */ }
    // Stealing data_ directly from the other.
    A(A&& other) {
        data_ = other.data_;
        len_ = other.len_;
        // Because we have stolen data_ from the other object, we have to reset
        // the pointer so that it knows data_ is gone.
        // Otherwise, other will free this memory during destruction.
        other.data_ = nullptr;
        other.len_ = 0;
    }
    // Copying from other, usually with copy-and-swap idiom.
    A& operator=(const A& other) { /* ... */ }
    // Usually swapping with other.
    A& operator=(A&& other) {
        using std::swap;
        swap(data_, other.data_);
        swap(len_, other.len_)
        return *this;
    }
    // Deallocate memory.
    ~A() { /* ... */ }
private:
    char* data_;
    size_t len_;
};
```

Implementing a correct copy or move assignment operator is a little tricker than
copy or move constructor, for you have to release the resource the objects owns
first, bringing about same resource release code written in copy/move
assignment and destructor for as much as three times, which is inelegant and
fallible. The usual way to write a good copy/move assignment is:

1. to implement copy assignment in copy-and-swap idiom;
2. to implement move assignment by swapping with the other object.

In the way above, the responsibility of properly releasing resources is solely
left to the destructor, reducing the chances to make mistakes. For more
information about copy-and-swap and the right way to swap objects, see
[Copy and Swap][1].

# Overloading functions on lvalue reference and rvalue reference #

The same approach is applied to overload a function by distinguishing whether
the argument is a lvalue or rvalue reference, like the following declarations.

```c++
class Car {
public:
    // The lvalue reference version, chosen in most cases, including when the
    // rvalue reference version does not exist.
    void set_wheel(const Wheel& wheel);
    // The rvalue reference version, chosen if the argument is a temporary
    // object. You may implement it by moving the resource from wheel instead of
    // copying.
    void set_wheel(Wheel&& wheel);
private:
    Wheel wheel_;
};
```

Here is a possible implementation.

```c++
class Car {
public:
    void set_wheel(const Wheel& wheel) {
        // Call the copy assignment of Wheel.
        wheel_ = wheel;
    }
    void set_wheel(Wheel&& wheel) {
        // WRONG! Still call the copy assignment!
        // wheel_ = wheel;
        // Correct. Call the move assignment.
        wheel_ = std::move(wheel);
    }
private:
    Wheel wheel_;
};
```

The issue is, though a rvalue reference is bound to a rvalue (here, the
temporary object is a rvalue), **the expression to use a rvalue reference
variable is a lvalue**, causing the compiler to select the copy assignment
operator. The most straightforward way to understand it is to realize that once
you bind a temporary object to a rvalue reference variable, you can access it as
many times as you want, modifying it, passing it to another function,
constructing new objects from it, just like accessing an ordinary lvalue
reference. Therefore, the expression `wheel` itself is a lvalue expression.

To fix the problem, you need to call std::move on the reference, and cast it
back into a rvalue. The rule is simple here: **the expression calling a function
that returns a rvalue reference is a rvalue.**

The same mistake can be made when defining a move constructor:

```c+++
class Car {
public:
    Car(Wheel&& wheel): 
        // WRONG! Call the copy constructor here.
        // wheel_(wheel)
        // Correct. Call the move constructor.
        wheel_(std::move(wheel)) {}
    // ...
};
```

We can summarize the reference bounding rule as:

1. a lvalue reference is bound to a lvalue;
2. a rvalue reference is bound to a rvalue;
3. a lvalue reference-to-const (like const int&) can be bound to a rvalue.

# Whether an expression is a lvalue or rvalue? #

To figure out whether an expression is a lvalue or rvalue, reading the very
detailed [Value categories][2] is helpful. To quickly make sense of it, you
can simply see the following examples at first.

```c++
/* --------------- lvalues ------------------------- */
// the name of a variable or function
std::cin
wheel     // wheel is a rvalue reference variable, such as Wheel&& wheel
// string literals
"don't panic"
// a call to a function that returns lvalue references.
std::cout << "String"  // equals to operator<<(std::cout, "String")
// assignment, compound assignment, pre-increment/decrement
a = 2
b *= 3
++a
// dereference, data member accessing, and array element accessing
*pint
user.name
puser->name
numbers[i]
// static cast to lvalue
static_cast<int&>(x)

/* --------------- rvalues ------------------------- */
// literals, except string
42
1.8
// a call to a function that returns non-references
get_name()  // std::string get_name()
// result of arithmetic, comparison and logic operators
a + b
age > 17
x && y
// address-of
&c
// static cast to non-reference
static_cast<int>(y)
// this pointer
this
// a call to a function that returns rvalue references
std::move(obj)
// array element or member accessing when the array or object
// is a rvalue
make_array()[3]
User().name
// static cast to rvalue reference
static_cast<int&&>(z)
```

Note that this pointer itself is rvalue, which means you cannot assign it to a
new value (and it is meaningless to do so), while dereferencing it is a lvalue,
according to the rule, like `*this`.

[1]: {% post_url 2016-10-16-copy-and-swap %}
[2]: http://en.cppreference.com/w/cpp/language/value_category
