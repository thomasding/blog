---
layout: post
title: "SFINAE: Conditionally Instantiating Function Templates"
tags: [c++, metaprogramming]
comments: true
---

When we want to provide different implementations of a function according to the
type of the arguments, function overloading is the most usual way. When more
than one implementations match the types of the given arguments, the compiler
will choose the one that matches the most specifically. For example, the
function that accepts a pointer to the derived class is more specific than one
that accepts a pointer to the base class. One that accept the exact type is more
specific than one that needs implicit type conversion. Furthermore, in C++11, a
function can even overload on whether the argument is a lvalue reference or a
rvalue reference.

However, function overloading cannot handle all the cases. For example, when
defining a function template, it is impossible to enumerate all the types of the
arguments, so function overloading cannot be used. On the other hand, though
partial specialization can provide different implementations according to the
category of the given type, it can only be used in very limited situations where
the difference lies in whether or not the given type is a pointer, reference,
array or const/volatile-qualified.

Hence, we need more powerful mechanism to provide diverse implementations based
on the types of the arguments. That mechanism is **SFINAE**.

<!--more-->

## What is SFINAE ##

Like many C++ terms, the full name of SFINAE is very confusing: **Substitution
Failure Is Not An Error**. Briefly speaking, when substituting the template
parameters occurring in template parameter declarations and the function type
(which includes the return type and all the parameter types) during
instantiation, if the substitution is not legal, the instantiated function is
removed from the possible candidates instead of causing compile error.

The first concept that we need to understand is **substitution**. When
instantiating a function template, the compiler replaces all the occurrences of
template parameters, such as the commonly-used **T**, with the actual types or
values (if the parameter is a integer, for example). This procedure is called
**substitution**. As we all know, not all substitutions generate legal codes.
For example, the function template calls a member function of template parameter
**T**. If the actual type of **T** doesn't have such member function, the
generated code will cause a compile error. Apart from illegal member function
calls, there are still many ways to generate bad functions, such as declaring an
array of negative size (the size is a template parameter), using a non-existing
member type (a type declared in a class or struct with **typedef** or **using**
statements), creating a pointer to member of non-class T (related to pointer to
member), creating a reference to void and any other situations that cause
syntax errors.

However, if such syntax error introduced by substitution happens in **function
argument declaration, return type and template parameter declaration**, instead
of resulting in a compile error, the substitution is simply discarded by the
compiler. On the other hand, the syntax error occurring in the function body
still causes compile error.

Assume that we are writing a function template whose template parameter is
**T**:

```c++
template <class T> void foo(class T::B& x) {}
template <class T> void foo(T& x) {}
```

If we call function foo with **T** being a type with member type **B**, the
first is chosen, for it is more specific than the second. Otherwise, the second
is chosen, like the following examples.

```c++
struct A { using B = float; };
foo<A>(1.2f);       // call the first
foo<float>(1.2f);   // call the second
```

Assume that we want to provide two implementations based on whether the template
parameter **N** is even or odd. There is a possible solution by declaring a
not-used pointer to array, whose size can be illegal (array size 0 is illegal):

```c++
// Chosen if N is even.
template <int N> void bar(char(*)[N % 2 == 0] = nullptr) {}
// Chosen if N is odd.
template <int N> void bar(char(*)[N % 2 == 1] = nullptr) {}
```

The argument type `char(*)[N % 2 == 0] = nullptr` is very confusing at first
sight. Let's see how it is composed:

```c++
int x[10];    // an array of size 10 of type int
int* x[10];   // an array of size 10 of type int* (* is closer to the type
              // before it)
int *x[10];   // still an array of type int* (the spaces around * don't matter)
int (*x)[10]; // a pointer to an array of size 10 of type int
```

The last example is a little hard to understand. Because **\*** is explicitly
combined into **x**, let's see the rest of the type at first, which is `int
[10]`, an array of size 10 of type int. Then the **\*** says that x is a pointer
to the rest of the type, so a pointer to `int [10]`.

Because the compiler allows default values to function arguments, we can define
a function like this:

```c++
void bar(char (*x)[10] = nullptr) {}  // x is a pointer to char[10]
```

In the last step, we omit the argument name, for we don't use it in the function
body, and replace the array size with a compile-time constant expression (which
can be computed in compile time):

```c++
template <int N> void bar(char(*)[N % 2 == 0] = nullptr) {}
```

However, it must be careful to use **array size SFINAE**. For example, the
following two function templates look plausible, while causing syntax error in
practice:

```c++
// Chosen if N is even. 
// The type of argument is an array of size (N % 2 == 0).
template <int N> void bar(char[N % 2 == 0] = nullptr) {}
// Chosen if N is odd. 
// The type of the argument is an array of size (N % 2== 1).
template <int N> void bar(char[N % 2 == 1] = nullptr) {}
```

In the compiler's eyes, the two function templates above look identical, hence
it treats the second as a duplicate of the first, reporting a redefinition error.
The reason why the compiler treats the two declarations as the same is a
mechanism called **array decaying**. Practically, all the following declarations
are the same:

```c++
// A group of identical functions (=> means "be decayed into").
void foo(int *a);      // pointer to int
void foo(int a[10]);   // int[10] => pointer to int
void foo(int a[20]);   // int[20] => pointer to int

// Another group of identical functions.
void foo(int (*a)[10]);  // pointer to int[20]
void foo(int a[20][10]); // int[20][20] => pointer to int[20]
void foo(int a[30][10]); // int[30][20] => pointer to int[20]
```

However, the following declarations are different:

```c++
void foo(int (*a)[10]);  // pointer to int[10]
void foo(int a[10][20]); // int[10][10] => pointer to int[20]
void foo(int a[10][30]); // int[10][20] => pointer to int[30]

void foo(int *a[10]);     // It's actually an array of int* of size 10.
```

It can be summarized as **the first dimension of an array is decayed into a
pointer, while the other dimensions preserve**.

Therefore, to avoid array decaying, we have to carefully use SFINAE on the
second dimension of the array type instead of the first, which will be
eventually decayed into a pointer in spite of its size.

## Type SFINAE and expression SFINAE ##

## Examples ##
