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
    unique_ptr() noexcept:ptr_(nullptr){}
    unique_ptr(std::nullptr_t) noexcept:ptr_(nullptr){}

    // Constructor that makes a unique_ptr that holds the given object.
    explicit unique_ptr(T* p) noexcept: ptr_(p){}

    // Swap with another unique_ptr object.
    void swap(unique_ptr& other) noexcept {
        using std::swap;
        swap(ptr_, other.ptr_);
    }

    // Move constructor. Transfer the object from u to this object.
    unique_ptr(unique_ptr&& u) noexcept: ptr_(u.ptr_) {
        u.ptr_ = nullptr;
    }

    // Move assignment operator can be implemented by swapping.
    unique_ptr& operator=(unique_ptr&& u) noexcept {
        swap(u);
        return *this;
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
```
