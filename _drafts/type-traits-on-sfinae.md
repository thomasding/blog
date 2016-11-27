---
layout: post
title: Determine whether a class has the specific member function
tags: [c++, metaprogramming]
comments: true
---

Supposing that we are writing a function template called **write_log** that
supports two different destination file handlers, one providing member function
**write** and the other **put**. Then, we hope that the function template always
calls the right one regardless of the given type, which is possible with the
help of the **SFINAE** rule.

Providing a unified interface for two categories of types requires two function
templates of the same name **write_log** and the same signature, namely,
function argument types. However, to avoid redefinition error, if the type of
the file handler is determined, one of the two have to result in a substitution
failure.

<!--more-->

A possible implementation can be like the following code.

```c++
template <class T, class M>
auto write_log(T&& out, M&& msg) -> decltype(out.write(""), void()) {
  out.write("Log:");
  out.write(std::forward<M>(msg));
}

template <class T, class M>
auto write_log(T&& out, M&& msg) -> decltype(out.put(""), void()){
  out.put("Log:");
  out.put(std::forward<M>(msg));
}
```

In the example above we use the **Expression SFINAE** on function result type.
The type indicated by `decltype(out.write(""), void())` is always **void**, for
we used the comma expression here. However, if the class of **out** has no
member function named **write** that accepts a **const char\*** argument, the
expression will be illegal, and hence regarded as a substitution failure instead
of compile error.

Now let's take a further step to define a type trait to determine whether or not
a class has a member function named **write** and accepting an argument of type
**const char\***.

A typical type trait takes a template parameter and provides a member boolean
constant. Hence, our type trait **is_writable** looks like:

```c++
template <class T>
struct is_writable {
// Some definitions.
static constexpr bool value = false /* or true */;
};
```

Now let's add two functions to distinguish a type satisfying the condition from
others.

```c++
#include <utility>

template <class T>
struct is_writable {
  template <class U>
  static auto test(const char *v)
      -> decltype(std::declval<U>().write(v), std::true_type());

  template <class U> static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(nullptr))::value;
};
```

Firstly, the helper function `std::declval<T>()` is used to make an object in
the **decltype** expression, for it can be faulty to use `T()` to make an object of type
**T** if **T** does not have a constructor that accepts no arguments.
`std::declval<T>()`, however, can not be used outside of the **decltype**
expression, so it can never be really called. Only in type deduction is it legal.

Secondly, the catch-all function **test** uses an ancient syntax `...` derived from
the C age, namely, ellipsis. When type **T** has a member function **write** and
hence both of the **test** functions are satisfied, the ellipsis one is less
preferred than the specific one in order to avoid the ambiguous error.
