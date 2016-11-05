---
layout: post
title: Move and Forward
tags: [c++]
comments: true
---

One of the most confusing thing that one new to C++11 experiences is that
**std::move** actually never moves anything. All that it does is **to cast a
reference type, whether a lvalue or rvalue reference, to a rvalue reference.**

By casting a reference to a rvalue (a call to a function that returns a rvalue
reference is a rvalue), the compiler will choose the **A&&** version if both the
**const A&** and **A&&** version are provided. Usually, the version that
receives **A&&** moves (or steals) the underlying resource from the argument
instead of making a copy, so the function **std::move** semantically reveals its
usage rather than its behavior.

<!--more-->

# std::move #

A possible implementation of **std::move** is nothing but a **static_cast** to
rvalue references **A&&**, which can be implemented as:

```c++
template< class T >
typename std::remove_reference<T>::type&& move( T&& x ) noexcept {
    return static_cast<std::remove_reference<T>::type>(x);
}
```

Three key points are worth noticing in the implementation above:

1. **T&&** is called a universal reference, because during template
   instantiation it can be deduced to **const A&** as well as **A&&** according
   to the actual type passed to the function template;

2. **std::remove_reference<T>::type&&** is used to get a rvalue reference from
   type **T**, no matter which reference type **T** is, explained later;

3. **noexcept** is added to the declaration, for it obviously throws no
   exceptions.

The rule how **T&&** becomes **const A&** or **A&&** is called **reference
collapsing**. Reference collapsing defines what type **T&&** is if **T** is a
reference. The C++ standard forbids a reference to another reference, so the
type like **A& &&** is illegal. Therefore, when **T** is replaced with a
reference type, the compiler deduces the type of **T** by the reference
collapsing rule. It can be summarized as:

1. A& & => A&
2. A&& & => A&
3. A& && => A&
4. A&& && => A&&

And it can be memorized as **only rvalue reference to rvalue reference produces
rvalue reference**.

When instantiating template **std::move**, if the argument passed to move is a
lvalue reference **A&**, **T&&** will be deduced to be **A&**. If the argument
is a rvalue reference, **T&&** will be **A&&**. Both conditions conform to the
reference collapsing rule.

However, it is required that **std::move** should return a rvalue reference in
spite of the reference type passed to it. So we have to remove the reference
part of the type before appending **&&** to it in order to bypass reference
collapsing. Thankfully, STL provides a type trait called
**std::remove\_reference** to remove the reference part of a given type. Hence,
the return type of move can be conveniently defined as **typename
std::remove_reference<T>::type&&**. (Recall the basic knowledge of template that
if we are referring to a type defined in a struct or class, we have to append
**typename** before it so that the compiler knows we are naming a type rather
than a data or method member.)

Hence, we can easily implement **remove_reference** as:

```c++
template< class T >
struct remove_reference       { typedef T type; };    // Version 1
// Specialized on lvalue reference.
template< class T >
struct remove_reference<T&>   { typedef T type; };    // Version 2
// Specialized on rvalue reference.
template< class T >
struct remove_reference<T&&>  { typedef T type; };    // Version 3
```

The definition of **remove_reference** uses a method called template
specialization, more precisely, partial template specialization, for we are not
specializing it for a complete type but a smaller group of types, such as all
the lvalue reference types.

We can compare template specialization to function overloading. **Given many
versions of a template, the compiler will choose the one that is closest to the
given argument.**

If we instantiate **remove\_reference<SomeType&>**, the compiler will choose the
second version and deduce **T** to be **SomeType**; if provided with
**remove\_reference<SomeType&&>**, the compiler will choose the third version
and deduce **T** to be **SomeType**, too. If, apart from the above two, the
argument is not a reference at all, the compiler will choose the first (and
basic) version.

Therefore, **typename remove_reference<T>::type** is always a type without the
reference part. The same technique is used to define many other type traits, for
example, the following ones to remove const or volatile qualifiers of a given
type, the name of which is self-explained enough.

```c++
template< class T >
struct remove_cv;
template< class T >
struct remove_const;
template< class T >
struct remove_volatile;
```

# std::forward #

Chances are we are defining a function template that receives both reference
types by using a universal reference.

In the example below, we define a function **bar** to receive an object and
forward it to another function **foo**.

```c++
template< class T >
void foo( T&& x ) { /* Do something on x */ }
template< class T >
void bar( T&& x ) {
    // Only lvalue reference version of foo is called.
    foo(x);
}
```

The problem is, as is explained in [Rvalue Reference][1], the variable **x** in
function **bar** is always regarded as a lvalue, though it is *bound* to a
rvalue, so only the lvalue reference version of function foo is called whatever
reference is passed to function **bar**. The behavior we are expecting is that
the rvalue reference version of **bar** calls the rvalue reference version of
**foo**, and vice versa. To forward the argument **x** to another function as
the same type it is passed in, we can use **std::forward**:

```c++
template< class T >
void bar( T&& x ) {
    // Now both versions of foo can be called based on the type of x.
    foo(std::forward<T>(x));
}
```

Considering we are already familiar with the basic knowledge of reference
collapsing rule and template partial specialization, it is straightforward to
implement our own version of **std::forward**.

Let's recall the type deduction rule on universal reference first:

1. if passed with **A&**, the type **T** of the universal reference **T&&** is
   deduced to be **A&**;

2. if passed with **A&&**, **T** is deduced to be the type without the reference
   part **A**.
   
Hence, the magic of **forward** lies in the exact type of **T**. Now let's think
about how to return the correct reference type based on T. According to the
reference collapsing rule, if **T** is **A&**, **T&&** will be **A&**, but if
**T** is **A**, **T&&** will be **A&&**. That's exactly what we want.

We can demonstrate the route from the argument type through **T** finally to
**T&&** in the following table.

| Argument Type | T | T&& |
| --- | --- | --- |
| A& | A& | A& |
| A&& | A | A&& |

Hence, we can define our forward in the following code.

```c++
// Deals with both lvalue and rvalue references.
template< class T >
T&& forward( typename std::remove_reference<T>::type& x ) noexcept {
    return static_cast<T&&>(x);
}
```

But there is an edge case that we should consider. The caller may pass a rvalue
like a temporary object to forward, in which case it should return a rvalue
reference. Considering this situation, we add another function overloaded on
rvalue references.

```c++
// Deals with both lvalue and rvalue references.
template< class T >
T&& forward( typename std::remove_reference<T>::type& x ) noexcept {
    return static_cast<T&&>(x);
}
// Deals with rvalues, such as a temporary object.
template< class T >
T&& forward( typename std::remove_reference<T>::type&& x ) noexcept {
    return static_cast<T&&>(x);
}
```

That's how **std::forword** works in STL.
