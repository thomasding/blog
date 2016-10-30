---
layout: post
title: "SFINAE: Conditionally Instantiating Function Templates"
tags: [c++, metaprogramming]
comments: true
---

When we want to provide different implementations of a function according to the
type of the arguments, function overloading is the most common method. When more
than one implementations match the types of the given arguments, the compiler
will choose the one that matches most specifically. For example, the function that
accepts a pointer to the derived class is more specific than one that accepts a
pointer to the base class. One that accept the exact type is more specific than
one that needs implicit type conversion. In C++11, a function can even overload
on whether the argument is a lvalue reference or a rvalue reference.

However, function overloading cannot handle all the cases. When defining a
function template, it is impossible to enumerate all the types of the arguments,
so function overloading cannot be used there. On the other hand, partial
specialization can only be used in very limited situations where the difference
lies in whether or not the given type is a pointer, reference, array or
const/volatile-qualified.

Hence, we need more powerful mechanism to provide diverse implementations based
on the types of the arguments. That mechanism is **SFINAE**.

<!--more-->

## What is SFINAE ##

Like many C++ terms, the full name of SFINAE is very confusing: **Substitution
Failure Is Not An Error**.
