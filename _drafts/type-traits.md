---
layout: post
title: Type Traits
tags: [c++, metaprogramming]
comments: true
---

Only given a type **T** without any knowledge of how it is implemented, just as
we encounter in template programming, what question can we ask about it? Here
are some questions that you may ask:

1. Is it an integral type/array/class/function/enum/union?
2. Is it a lvalue/rvalue reference or non-reference?
3. Is it const/volatile qualified?
4. If it is a reference, what type is the non-reference part?
5. If it is a pointer, what type is the object it points to?
6. Is it the same type as type **U**?
7. Is it a copy/move constructible/assignable?

STL provides a collection of utility templates, called **type traits**, to
answer these questions about what feature a specific type **T** has. They are
defined in the header file **\<type_traits\>**.

<!--more-->

After reading the
post [SFINAE: Conditionally Instantiating Function Templates][1], you may have
already understood the skills about providing different implementations based on
the types of template parameters. However, the prerequisites of using
SFINAE are to determine the features that the given types provide. Before you
write two function templates that serve for pointers and non-pointers,
respectively, for example, it is required to distinguish a pointer type from a
non-pointer one.

Therefore, in order to master SFINAE skills, it is a necessity to be familiar
with type traits, from what they are used for to how they work. Finally, you
will be proficient at defining your own type traits.

## An overview of type traits ##

Type traits can be grouped in two forms, one to determine whether a given type
**T** has a feature or not, which are usually called **is_xxx**, for example:

1. types: **is\_void**, **is\_null\_pointer**, **is\_integral**, **is_array**,
   ...
2. type properties: **is\_const**, **is\_volatie**, **is\_reference**,
   **is\_abstract**, ...
3. type relations: **is\_same**, **is\_base\_of**, **is\_convertible**, ...
4. supported operations: **is\_default\_constructible**,
   **is\_move\_assignable**, ...
5. and etc.

These type traits have a static constant data member **value**, which equals to
**true** if the type has the specified feature. For example:

```c++
std::is_array<int[10]>::value == true;
std::is_const<const int>::value == true;
std::is_rvalue_reference<float&>::value == false;
std::is_same<int, unsigned int>::value == false;

struct TrivialType { int a; };
std::is_default_constructible<TrivialType>::value == true;

struct ComplicatedType {
  ComplicatedType(int a) : val(a*2) {}
  ComplicatedType(const ComplicatedType&) = delete;
  ComplicatedType(ComplicatedType&&) = default;
  ComplicatedType& operator=(const ComplicatedType&) = delete;
  ComplicatedType& operator=(ComplicatedType&&) = default;
  ~ComplicatedType() = default;
  
  int val;
};
std::is_default_constructible<ComplicatedType>::value == false;
std::is_copy_constructible<ComplicatedType>::value == false;
std::is_move_constructible<ComplicatedType>::value == true;
std::is_destructible<ComplicatedType>::value == true;
```

The other form of type traits modifies the given type and returns another
derived type, for example:

1. const/volatile modifiers: **add\_const**, **remove\_const**, **remove\_volatile**, ...
2. reference modifiers: **remove\_reference**, **add\_lvalue\_reference**, ...
3. pointer modifiers: **add\_pointer**, **remove\_pointer**
4. sign modifiers: **make\_signed**, **make\_unsigned**,
5. array modifiers: **remove\_extent**, **remove\_all\_extents**
6. and etc.

These type traits have a type member **type**, which is the modified type from
the given types. Like:

```c++
typename std::add_const<int>::type a;                 // const int
typename std::remove_reference<const int&>::type b;   // const int
typename std::remove_extent<int[]>::type c;           // int
typename std::remove_extent<int[10][20]>::type d;     // int[20]
typename std::remove_all_extents<int[10][20]>::type e;// int
```

Apart from the mentioned ones, there are some other utilities for type
manipulations, for example:

1. the equivalent if-statement for types: **conditional**
2. SFINAE tool: **enable_if**
3. type transformations happening in function arguments: **decay**
4. return type of a function: **result_of**
5. and etc.

Just like those for type modifications, these utilities define a type member
**type**, which is the result of the specified operations.

```c++
typename std::conditional<std::is_const<const int>,
                          float,
                          double> a;                // float
typename std::decay<int[10][20]>::type b;           // int(*)[20]

// std::enable_if has type member `type` only if the boolean value passed to 
// it is true. In this example, if T is not a reference, std::enable_if will
// have no `type` member, causing type substitution failure.
template <class T, 
          typename = typename std::enable_if<
                       std::is_reference<T>::value
                     >::type>
void foo(T v) {}
```

For those who are not familiar with template programming, type traits look like
mysterious magic. After all, how can you judge whether or not a type is a class
or a primitive type? How can you remove the constness of a type? Consequently,
it turns out that the best way to learn type traits is to find out how they are
implemented, in which way not only are we accustomed to the thinking of template
programming, but skilled to extend the power of type traits.

## Defining type traits 1: the direct way ##

Some type traits like **add_const**, **add_lvalue_reference** and
**add_volatile** are very straightforward. Hence, we can take them as a warm-up
to the real fantastic part.

Although STL uses **typedef** to define type members, we apply the **using**
statement in all our examples, since they are more readable than the convoluted
**typedef** and more powerful as well.

```c++
template <class T> struct add_const { using type = const T; };
template <class T> struct add_lvalue_reference { using type = T&; };
template <class T> struct add_rvalue_reference { using type = T&&; };
template <class T> struct add_pointer { using type = T*; };
```

## Defining type traits 2: template specialization ##

More non-trivial type traits like **remove_reference** cannot be directly
defined. Hence, we need to pursue more powerful tools, like template
specialization.

### Background knowledge of template specialization ###

Given the most basic implementation of a template, you can define a different
implementation for a specific type, which is called **full specialization**. The
code above defines a basic template and two implementations that are specialized
for int and float, respectively.

```c++
template <class T> struct A { T v; };
template <> struct A<int> { int v; bool is_zero; };
template <> struct A<float> { double v; };
```

To specialize a template for a concrete type **A**, you need to preserve the
template declaration with `template <>` and put the actual type in the brackets
after the template name. The same rule applies for the cases of more than one
template parameters. For example:

```c++
templ
ate <class T, class U> struct B { T a; U b; };
template <> struct B<int, float> { int a; float b; char c; };
template <> struct B<int, int> {};
```

The specialized version of a template can be completed different from the basic
one. Hence, it is the author's responsibility to guarantee that the
specialization is reasonable. In the following examples, we will omit the actual
body of a template definition if we don't care about it.

Besides a concrete type, you can also specialize a template for a smaller case,
for example, providing another implementation for all pointer types. This is
called **partial specialization**.

```c++
template <class T> struct B {};
template <class T> struct B<T&> {};  // Specialized for lvalue references.
template <class T> struct B<T*> {};  // Specialized for pointers.

template <class T, class U> struct C {};
template <class T> struct C<T, T> {}; // Specialized for the same types.
template <class T> struct C<T, T&> {}; // Specialized for C<A, A&> cases.
```

By applying partial specialization, we can define more sophisticated type
traits. But before that, we need to know how to represent true and false in
template programming, especially in the world of type traits.

### Background knowledge of std::true_type and std::false_type ###

In type traits, instead of directly using boolean literals like **true** and
**false**, we apply two special types: **std::true_type** and
**std::false_type**. They both contain a static constant member named
**value**, which is **true** for **std::true_type** and **false** for
**std::false_type**. The type of **value** is **bool**. Furthermore, a
**std::true_type** object is able to be implicitly converted to a boolean value
whose value is **true**, and so is **std::false_type**.

```c++
std::true_type::value == true;
std::false_type::value == false;
```

There are many advantages that two different types have over boolean
literals. The first one is that deriving from types is shorter than defining
static constants:

```c++
// Use true_type.
template <class T> struct my_trait : true_type {};
// Use boolean literal.
template <class T> struct my_trait { static const bool value = true; };
```

However, only taking the length of code into account does not answer the
question completely, because it seems where **std::true_type** and
**std::false_type** are applied, **true** and **false** can do as well, only
more redundant.

A deeper reason that **std::true_type** and **std::false_type** are required
lies in the excessive use of SFINAE in template programming, especially type
traits. It is common to provide two complementary function templates with the
same signature (argument types), one applied for some types and the other for
the rest. Because we never actually call the function, the only way to
distinguish one from the other is by the return type. We don't call the
function, so we won't know the return value. Since C++11, **constexpr** has been
a way to define functions that return compile-time constants. However, type
traits exist prior to C++11 before **constexpr** is added to the language.

In the example below, we can determine the type passed to function **foo** by
examining the return type of **foo**.

```c++
template <class T> false_type is_int(T) {}
template <> true_type is_int<int>(int) {}
```

The functions above look quite like a type trait already.

One more step, **std::true_type** and **std::false_type** are actually template
specializations of **std::integral_constant**, whose first parameter is an
integral type and second parameter the value:

```c++
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;
```

### type traits using template specialization ###

Using partial specialization, we can define many type modifiers to remove
const/volatile qualifiers, references and pointers:

```c++
// Remove the const/volatile qualifiers of a type.
template <class T> struct remove_cv { using type = T; };
template <class T> struct remove_cv<const T> { using type = T; };
template <class T> struct remove_cv<volatile T> { using type = T; };
template <class T> struct remove_cv<const volatile T> { using type = T; };

// Remove the reference part of a type.
template <class T> struct remove_reference { using type = T; };
template <class T> struct remove_reference<T&> { using type = T; };
template <class T> struct remove_reference<T&&> { using type = T; };

// And so on.
```

Another basic type trait that is intensively used by other type traits is
**is_same**, which compares whether the two given types are identical:

```c++
template <class T, class U> struct is_same : false_type {};
tmeplate <class T> struct is_same<T, T> : true_type {};
```

Using the type traits above, we can define a type trait to check whether the
given type is a floating-point number:

```c++
// The C++11 style
template <class T> is_floating_point 
    : integral_type<bool, is_same<float, typename remove_cv<T>::type>::value ||
                          is_same<double, typename remove_cv<T>::type>::value ||
                          is_same<long double, typename remove_cv<T>::type>::value> {};
```

## Defining type traits 3: SFINAE ##

## Episode: simplifying the type traits returning types ##

The type declaration when using a type trait can be very long, like the
following code that adds the const qualifier to a given type:

```c++
template <class T> void foo(typename add_const<T>::type myvalue) {}
```

By type aliasing, we can obviously shrink the length of the type description:

```c++
template <class T> using add_const_t = typename add_const<T>::type;
template <class T> void foo(add_const_t<T> myvalue) {}
```

Sadly, these helpful type aliasing definitions were not introduced to C++ until
C++14. Hence, you would have to use the old long redundant type descriptions
prior to C++14 should you not define them manually. In the post, we assume these
type aliasing, which are uniformly in the form of `template <class T> using
foo_t = typename foo<T>::type` .

```c++
// Since C++14
template <class T> add_lvalue_reference_t 
    = typename add_lvalue_reference<T>::type;
template <class T> add_rvalue_reference_t
    = typename add_rvalue_reference<T>::type;
```

In the following sections, sometimes we omit these type aliasing definitions for
brevity. But we'll use them as if they have been defined as long as not causing
confusion.

Then, we add type aliasing definitions to the type traits above:

```c++
template <class T> using remove_cv_t = typename remove_cv<T>::type;
template <class T> using remove_reference_t 
    = typename remove_reference<T>::type; 
```

Similar to type aliasing, STL provides helpers to simplify the use of these
constant expressions by variable templates. Unfortunately, the syntax of
variable templates was added to C++14, and the helpers based on variable
templates for type traits weren't available until C++17. Nevertheless, since
they greatly improve the convenience of template meta-programming, we will be
using these variable templates through the post.

```c++
// Since C++17
template <class T, class U> constexpr bool is_same_v = is_same<T, U>::value;
```

// The C++14 & C++17 style
template <class T> is_floating_point
    : integral_type<bool, is_same_v<float, remove_cv_t<T>> ||
                          is_same_v<double, remove_cv_t<T>> ||
                          is_same_v<long double, remove_cv_t<T>>> {};

[1]: #
