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
mechanism called **array decaying**, which is applied to function arguments.
Practically, all the following declarations are the same:

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

## Type SFINAE ##

All the SFINAE cases mentioned in the previous section occur on the validity of
types, such as accessing non-existing member types and constructing invalid
array types. These SFINAE cases based on types are called **type SFINAE**, which
is supported by C++ before C++11.

In type SFINAE, we construct a type dependent on template parameters on purpose,
whose validity is determined by the actual value of the template parameters. It
looks like a switch that can be turned on or off in order to determine whether
or not to take the function as a possible candidate.

Here is another of type SFINAE, which attempts to assign a value to a
incompatible type as a function argument or template parameter.

```c++
template <class T> void foo(T a, T* b) {}
template <class T, class B> void foo(T a, B b) {}

void foo_test() {
  int k = 10;
  foo(10, &k);    // Call the first function => void foo<int>(int, int*)
  foo(10, 20);    // Call the second function => void foo<int, int>(int, int)
}
```

Interestingly, the common case of such SFINAE is usually made as a mistake by
the programmer. In practice, when calling a function template with the arguments
of incompatible types, instead of being reminded of the message like "cannot
assign a (type X) to b (type Y)", which is commonly seen in non-template
function calls, we are actually warned that no suitable functions are found.
That's because the function template is removed by the compiler due to
substitution failure, hence, the compiler cannot even find a function that we
are intended to call. Eventually, the compiler can only report that no such
functions exist.

## Expression SFINAE ##

### Introduction to delctype specifier ###

Starting from C++11, a new keyword **decltype** is introduced. It represents the
type of the expression or entity in the parentheses. An entity is a variable or
member of an object. If the x part of **decltype(x)** is an entity, its
represented type is the type of x itself. In any other cases, its represented
type is the type of the expression, including **decltype(expr)** and
**decltype((x))**.

Here are some examples of **decltype** uses:

```c++
int x = 10;
decltype(x) y;      // x is an entity, the type of y is int
decltype(&x) px;    // &x is an expression, the type of px is int*
decltype((x)) lrx;  // (x) is an expression, the type of lrx is int&
decltype(std::move(x)) rrx;  // the type of rrx is int&&

struct A { int m; };
const A a;
decltype(a) b;      // a is an entity, the type of b is const A
decltype(a.m) n;    // a.m is an entity, the type of n is int
decltype((a.m)) rm; // (a.m) is an expression, the type of rm is const int&

decltype(1 + 2) z;  // 1 + 2 is an expression, the type of z is int

int foo();
decltype(foo()) fr;    // type of fr is int
decltype(a.m, 10) c;   // the type of a comma operator is that of the last
                       // element, hence the type of c is int
```

It is worth noticing that the expression in a **decltype** operator don't
generate real code. It is only used to deduce types in compile time and discarded
afterwards.

To comprehend **decltype** thoroughly, it is advised to refer to the standard
[value category][1] and [decltype][2]. But here, a little understanding of
**decltype** is enough for us to take a look at **expression SFINAE**.

### Expression SFINAE based on ill-formed decltype expression ###

Similar to type SFINAE, expression SFINAE occurs in ill-formed decltype
expression in the function argument declaration, return type and template
parameter declaration. One of the most common uses is to combine the delctype
operator with comma operators in order to construct SFINAE on arbitrary
expressions without affecting the prototype of the function.

As we know from C, the expression `a, b` evaluates a and b sequentially and
return the value of b as the value of the expression. Thus, the type of a comma
expression `a, b` is always the type of b in spite of a. Hence, we can replace a
with any expressions so as to make use of SFINAE.

```c++
// Two animal classes that provide different methods to speak.
struct Dog { void bark() {} };
struct Cat { void mew() {} };

// A pair of generalized speak templates.
template <class T>
auto speak(T&& animal) -> decltype(animal.bark(), void) {
  animal.bark();
}

template <class T>
auto speak(T&& animal) -> decltype(animal.mew(), void) {
  animal.mew();
}
```

You may have noticed that we put the return type after the function argument
list because it is too long to put beforehand. Because we have moved the return
type after the argument list, we need to put **auto** specifier at the old place
as a placeholder. As an example, the following to definitions are identical:

```c++
int foo() { return 10; }
auto foo() -> int { return 10; }
```

In the previous example, **decltype(animal.bark(), void)** is always **void**,
because of the comma operator. However, if **animal** does not have a method
called **bark** that accepts no arguments, the expression will be ill-formed,
hence triggering expression substitution failure.

Therefore, if the class provides a **bark()** method, the first is called. If
the class provides a **mew()** method, the second is called. Otherwise, neither
is called, the function call being illegal.

## std::void_t ##

In C++17, a new type alias **void_t<T1, T2, ...>** is introduced, which, as its
name suggests, is always **void**. It is used as a utility of SFINAE. Hence we
can rewrite the example like:

```c++
template <class T>
auto speak(T&& animal) -> std::void_t<decltype(animal.bark())> {
  animal.bark();
}

template <class T>
auto speak(T&& animal) -> std::void_t<decltype(animal.mew())> {
  animal.mew();
}
```

The code above is more self-explanatory than `decltype(A, void)`. Furthermore,
we can even use it as a template parameter.

```c++
// We omit the parameter name and use void_t as a default type.
// Because the only role it plays here is the context of SFINAE.
template <class T, 
          typename = std::void_t<decltype(animal.bark())> >
void speak(T&& animal) {
  animal.bark();
}
```

**std::void_t** can be possibly implemented by a helper struct in order to avoid
the template parameters being ignored by the compiler (the compile can ignore
unused template parameters in the using statement):

```c++
template <typename... Ts> struct void_s { using type = void; };
template <typename... Ts> using void_t = typename void_s<Ts...>::type;
```

[1]: http://en.cppreference.com/w/cpp/language/value_category
[2]: http://en.cppreference.com/w/cpp/language/decltype
