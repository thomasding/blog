---
layout: post
title: Implementing unique_ptr
tags: [c++]
comments: true
---

**unique_ptr** is one of the best features of C++11. It is not only a necessity
at hand, but an exemplar of C++11 style programming. Unlike its cousin
**shared\_ptr** and **weak\_ptr**, **unique\_ptr** is simple and free from
details about reference counting. Therefore, we are going to implement our own
**unique\_ptr** in this post and show how to program with C++11.

<!--more-->

## Basic step ##

A **unique\_ptr** holds the pointer to an object, which can be transferred
between **unique\_ptrs**. When a **unique\_ptr** is destroyed as it holds an
object, the object is freed by the default **delete** operator or a custom
deleter function. In the basic step, we assume that an object can only be
deleted by the **delete** operator. We will support custom deleter functions in
the the following sections.

```c++
#include <utility>

template <typename T>
class unique_ptr {
 public:
  // Constructors that makes an empty unique_ptr.
  constexpr unique_ptr() noexcept : ptr_(nullptr) {}
  constexpr unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  // Constructor that makes a unique_ptr that holds the given object.
  explicit unique_ptr(T* p) noexcept : ptr_(p) {}

  // Move constructor. Transfer the object from u to this object.
  unique_ptr(unique_ptr&& u) noexcept : ptr_(u.ptr_) { u.ptr_ = nullptr; }

  // Move assignment operator can be implemented by swapping.
  unique_ptr& operator=(unique_ptr&& u) noexcept {
    swap(u);
    return *this;
  }

  // Swap with another unique_ptr object.
  void swap(unique_ptr& other) noexcept {
    using std::swap;
    swap(ptr_, other.ptr_);
  }

  // Explicitly delete the copy constructor and copy assignment operator.
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  // Destructor.
  ~unique_ptr() {
    // Delete the object if the unique_ptr is holding one.
    if (ptr_) {
      delete ptr_;
    }
  }

  // Access the object.
  T* get() const noexcept { return ptr_; }

  // Overloads dereferencing operators so that a unique_ptr resembles a raw
  // pointer, like *uptr and uptr->member.
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  
  // Check whether it holds an object.
  explicit operator bool() const noexcept { return ptr_ == nullptr; }

 private:
  T* ptr_;
};

template <typename T>
void swap(unique_ptr<T>& a, unique_ptr<T>& b) noexcept {
  a.swap(b);
}
```

Let's dive into the details of the implementation above.

### Constructors with constexpr ###

**constexpr** is a new keyword introduced by C++11. One may be confused about
the difference of **constexpr** and the old **const** qualifier at first sight.

1. **const** qualifier is used to indicate that an object cannot be modified,
   though its value may not be determined until at runtime.

2. **constexpr** is used to indicate the value of an object can be determined at
   compile time, as well as a constant.
   
Hence, **constexpr** is stricter than **const** in that it requires the
qualified object to be a **compile-time constant**. Hence, a **constexpr**
object can be used in template instantiation and array length, where the value
is demanded to be known at compile time.

```c++
constexpr int a = 12;         // a is known to be 12 at compile time.
const int b = get_count();    // b cannot be modified, while its value won't be
                              // known until get_count() returns.
template <typename T, int N>
struct array {
  T data[N];
};
array<float, b> foo;          // ERROR! b is not known.
array<float, a> bar;          // OK. a is known.
```

Sometimes, the value of an object can be determined at compile time, too. By
specifying **constexpr** before the constructor, the object is regarded as
**constexpr** if all the arguments passed to the constructor are all
**constexpr**. If any argument is otherwise not **constexpr**, the object is not
considered as **constexpr**. Nevertheless, a **constexpr** constructor does not
restrict all its arguments to be **constexpr**.

```c++
struct add2 {
  int value;
  constexpr add2(int a, int b) : value(a + b) {}
};
// We can use the struct as a template argument.
constexpr int k = 10;
array<float, add2(12, k).value> baz;   // The length of baz is 22 floats.
// It can also be used like an ordinary constructor.
int add(int a, int b) {
  return add2(a, b).value;
}
```

By specifying the two empty constructors of **unique\_ptr** to be **constexpr**,
we can create compile-time constants like `unique_ptr<SomeType>()` and
`unique_ptr<SomeType>(nullptr)`.

### Keyword noexcept ###

If a function is declared with **noexcept**, it guarantees never throwing
exceptions, so that the compiler can optimize the code based on this knowledge.
Only if all the statements and function calls are certain not to throw
exceptions should a function be declared as **noexcept**.

**swap** function is usually implemented as a **noexcept** function in order
that the copy assignment can be exception-safe by applying [Copy and Swap][1]
idiom.

### Explicitly deleting functions ###

Prior to C++11, it is common to prohibit the copy constructor and copy
assignment operator by declaring them to be **private** members. While, in
C++11, one should prefer deleting them explicitly, because it is not only more
self-explanatory but error-friendly. Furthermore, it can be used with any
overloaded functions to delete some versions. If the deleted function is by
accident called, the compiler will report in a straightforward way instead of
complaining the violation to access permissions.

```c++
// Any overloaded functions can be deleted.
void foo(int a) { /* ... */ }
void foo(const char*) = delete;   // Delete the const char* version.

void bar() {
  foo(10);         // OK.
  foo("string");   // Compile error.
}
```

### Explicit constructor ###

If an constructor accepts only one argument, it is always a good practice to
declare it with **explicit** in order to prevent unexpected implicit type
conversion.

```c++
struct Person {
  int age;
  Person(int a) : age(a) {}
};
void foo(const Person& p);
// Unexpectedly construct a temporary Person object.
foo(1);

struct Person {
  int age;
  // Prohibit implicit type conversion.
  explicit Person(int a) : age(a) {}
};
// Cause compile error.
foo(1);
```
### Move semantics ###

C++11 introduces rvalue references as supplementary to the lvalue references.
Read [Rvalue Reference][2] for details.

## Upcasting unique\_ptr ##

We can assign an object to a pointer that points to its base type, which is
called **upcasting**. However, in the **unique\_ptr** above, we cannot upcast a
**unique\_ptr** to one that points to the base class, because they are different
types from the compiler's view. For example, the following code won't work.

```c++
struct A {};
struct B : A {};
unique_ptr<B> pb = unique_ptr<B>(new B);
unique_ptr<A> pa = std::move(pb);           // Compile error.
```

In order to upcast a unique\_ptr, we add an constructor template that can be used
in implicit type conversions to it.

```c++
// In the definition of class template unique_ptr
// Implicit type conversions between unique_ptrs.
template <typename U>
unique_ptr(unique_ptr<U>&& u) noexcept : ptr_(u.ptr_) {
  u.ptr_ = nullptr;
}
```

Thus, when we are implicitly converting an object of **unique\_ptr\<A\>** to one
of **unique\_ptr\<B\>**, the compiler will instantiate a constructor from the
template that accepts **unique\_ptr\<A\>** as its argument. Because the compiler
checks types after template instantiation, if A is found not convertible to B,
the compiler will report errors. Otherwise, the conversion will be done.

One may consider the constructor template above as a generalized version for
a move constructor. However, **the move/copy constructors must be explicitly
defined even if a matchable template is given**. That's because the compiler
will add a default move/copy constructor to the class if one is not found.
However, when checking existing constructors, the compiler won't go for
templates. Therefore, the move/copy constructors must be explicitly given.
Otherwise, the compiler will provide a default one instead of instantiating a
constructor template.

## Ownership releasing ##

In our **unique\_ptr**, the ownership of an object can only be transferred
between **unique\_ptr** objects. Sometimes, we need the **unique\_ptr** to
release the ownership of the object. For example, when we return the dynamically
allocated object in a C style, we cannot return a smart pointer. Hence, we add a
public method **release** so as to explicitly renounce the ownership.

```c++
// In the definition of class template unique_ptr
T* release() noexcept {
  T* p = ptr_;
  ptr_ = nullptr;
  return p;
}
```

## Custom deleter ##

In this section, we are going to add custom deleter support to our
implementation by introducing a new template parameter to match the type of the
deleter, which requires passing functions like an object.

The first way that a function can be interpreted as an object in C++ is a
function pointer, which has been a feature since C is born. To mimic the
**deleter** operator, the type of the pointer to a function that accepts a
pointer and returns nothing can be `void (*)(T*)`, if **T** is the type of the
object to be released. The asterisk must be enclosed in parentheses, because
otherwise the type `void * (T*)` looks like a function that returns `void*`.

However, in C++, an object that overloads **operator()** can also be called like
a function, for which reason they are named function objects. Hence, the type of
the deleter function cannot only be a plain function but any class that
overloads **operator()** that accepts a pointer as its argument. Therefore, we
have to introduce a new template parameter so as to accept anything callable in
C++ as a deleter function. Furthermore, we need to provide a default deleter as
well, which releases objects with operator **delete**.

A possible implementation can be:

```c++
#include <utility>

// Default deleter.
template <typename T>
struct default_delete {
  void operator()(T* ptr) noexcept { delete ptr; }
};

template <typename T, typename Deleter = default_delete<T>>
class unique_ptr {
 public:
  // Constructors that makes an empty unique_ptr.
  constexpr unique_ptr() noexcept : ptr_(nullptr), deleter_() {}
  constexpr unique_ptr(std::nullptr_t) noexcept 
      : ptr_(nullptr), deleter_() {}

  // Constructor that makes a unique_ptr that holds the given object.
  explicit unique_ptr(T* p) noexcept : ptr_(p), deleter_() {}

  // Constructor that accepts a raw pointer and a deleter.
  unique_ptr(T* p, const Deleter& d) noexcept : ptr_(p), deleter_(d) {}
  unique_ptr(T* p, Deleter&& d) noexcept : ptr_(p), deleter_(std::move(d)) {}

  // Implicit type conversions between unique_ptrs.
  template <typename U, typename E>
  unique_ptr(unique_ptr<U, E>&& u) noexcept
      : ptr_(u.ptr_), deleter_(std::move(u.deleter_)) {
    u.ptr_ = nullptr;
  }

  // Move constructor. Transfer the object from u to this object.
  unique_ptr(unique_ptr&& u) noexcept
      : ptr_(u.ptr_), deleter_(std::move(u.deleter_)) {
    u.ptr_ = nullptr;
  }

  // Move assignment operator can be implemented by swapping.
  unique_ptr& operator=(unique_ptr&& u) noexcept {
    swap(u);
    return *this;
  }

  // Swap with another unique_ptr object.
  void swap(unique_ptr& other) noexcept {
    using std::swap;
    swap(ptr_, other.ptr_);
    swap(deleter_, other.deleter_);
  }

  // Explicitly delete the copy constructor and copy assignment operator.
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  // Destructor.
  ~unique_ptr() {
    // Delete the object if the unique_ptr is holding one.
    if (ptr_) {
      deleter_(ptr_);
    }
  }

  // Access the object.
  T* get() const noexcept { return ptr_; }
  
  // Access the deleter.
  Deleter& get_deleter() noexcept { return deleter_; }
  const Deleter& get_deleter() const noexcept { return deleter_; }

  // Overloads dereferencing operators so that a unique_ptr resembles a raw
  // pointer, like *uptr and uptr->member.
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }

  // Renounce the ownership.
  T* release() noexcept {
    T* p = ptr_;
    ptr_ = nullptr;
    return p;
  }

 private:
  T* ptr_;
  Deleter deleter_;
};

template <typename T, typename D>
void swap(unique_ptr<T, D>& a, unique_ptr<T, D>& b) noexcept {
  a.swap(b);
}
```

However, there is a special case that the type of the deleter is a lvalue
reference. Anyway, the definition above does not prohibit a user from
instantiating a **unique\_ptr** with Deleter being `A&`.

In the definition above, we assume the type of the deleter is a non-reference,
hence we provide two constructors that accept lvalue-references and
rvalue-references, respectively. However, if **Deleter** itself is a lvalue
reference `A&`, the two constructors will become the one that accepts `const A&`
and the other `A&` because of reference collapsing. Even worse, the constructor
that accepts `A&`, which is collapsed from rvalue reference, will mistakenly
move the deleter from the given object, even if the reference passed to it is
actually a lvalue reference. Furthermore, if **Deleter** itself is `const A&`,
the two constructors will have exactly the same signature. The following code
demonstrates the problem:

```c++
// The prototypes of the two constructors if T = A and Deleter = SomeDeleter&.
// This constructor will violate the constness of the given argument.
unique_ptr(A* ptr, const SomeDeleter& d); 
// This constructor will cast d to a rvalue reference by std::move.
unique_ptr(A* ptr, SomeDeleter& d); 

// The prototypes of the two constructors if T = A and
// Deleter = const SomeDeleter&.
unique_ptr(A* ptr, const SomeDeleter& d);
unique_ptr(A* ptr, const SomeDeleter& d);
```

Hence, the situation that Deleter is a lvalue reference must be carefully
treated. We can look up in the C++ reference to find out how STL deals with it:

If Deleter is lvalue reference `A&`:

1. the first constructor accepts `A&`; 
2. the second constructor accepts `A&&`.

Still further, if Deleter is lvalue reference `const A&`:

1. the first constructor accepts `const A&`;
2. the second constructor accepts `const A&&`.

Since we have known the direction, now we need to implement the rule in
practice. The second constructor is easy to define, for it accepts a rvalue
reference in spite of whether Deleter is a reference and the constness is the
same as Deleter. All that we need to do is to strip the reference part and
append the rvalue reference symbol **&&** to it.

```c++
// Include type_traits in order to use std::remove_reference.
#include <type_traits>
unique_ptr(T*, typename std::remove_reference<Deleter>::type&& d) noexcept
    : ptr_(u.ptr_), deleter_(std::move(d)) {}
```

The tricky one is the first constructor, because it adds the **const** qualifier
only if Deleter is a non-reference. If Deleter is otherwise a lvalue reference,
the type of the argument has the same constness as Deleter. Therefore, we need a
conditional expression on types, which is exactly what **std::conditional<B, T,
F>** is used for.

**std::conditional<B, T, F>** accepts a compile-time boolean constant **B** and
two types **T** and **F**. If **B** is true, the type member **type** will be
**T**. Otherwise, the type member will be **F**.

To check if a given type is a lvalue reference, we can use
**std::is_lvalue_reference\<T\>**, whose constant data member **value** is true
if **T** is a lvalue reference.

Hence, we can define the first constructor as:

```c++
unique_ptr(
  T*, 
  typename std::conditional<
    std::is_lvalue_reference<Deleter>::value,
    Deleter, 
    const Deleter>::type& d) noexcept : ptr_(u.ptr_), deleter_(d) {}
```

Now our **unique_ptr** is finally friendly to lvalue references. So what about a
rvalue reference **D&&**? Following the reference collapsing rule, the
prototypes of the two constructors are:

```c++
// This constructor will cause compile error if it is used in practice, because
// you cannot bind a lvalue to a rvalue reference.
unique_ptr(T* p, const D& d);
// This is the version that can be actually used in practice.
unique_ptr(T* p, D&& d);
```

Therefore, the implementation above works fine with rvalue references.

## Some more details ##

Although we have covered the most important parts of a **unique_ptr**, there are
still some features that we ignore, for example, the **make_unique** utility
function, comparison operators between two **unique_ptr**s and the hash
function. Since they are quite straightforward compared to the other features,
they can be easily understood by reaching up to the reference.

[1]: {% post_url 2016-10-16-copy-and-swap %}
[2]: {% post_url 2016-10-24-rvalue-reference %}
