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
called **upcasting**. However, in the unique\_ptr above, we cannot easily upcast
a unique\_ptr to one that points to the base class. For example, the following
code won't work.

```c++
struct A {};
struct B : A {};
unique_ptr<B> pb = unique_ptr<B>(new B);
unique_ptr<A> pa = std::move(pb);           // Compile error.
```

[1]: {% post_url 2016-10-16-copy-and-swap %}
[2]: {% post_url 2016-10-24-rvalue-reference %}
