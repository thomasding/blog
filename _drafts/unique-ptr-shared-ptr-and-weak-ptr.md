---
layout: post
title: unique_ptr, shared_ptr and weak_ptr
tags: [c++]
comments: true
---

Smart pointers are one of the most enchanting features that C++11 provides.
Using smart pointers, it's possible that you will never need to remember freeing
a dynamically allocated object any more. However, C++11 provides three kinds of
smart pointers together. It's important that you use the right one in practice.

<!--more-->

## unique_ptr ##

A unique\_ptr should be the first that comes to your mind when you need to
dynamically allocate an object.

In comparison to a raw pointer, a unique\_ptr has no space overhead if you don't
specify a custom deleter, which means it occupies the same space as a raw
pointer. Furthermore, with the help of function inlining, a unique\_ptr has no
time overhead compared to a raw pointer. Therefore, a unique\_ptr is a perfect
replacement to a raw pointer, which should be the first to consider.

As is its name suggested, a unique\_ptr owns a object uniquely, that is, the
unique\_ptr can be moved, but not copied.

Here are some examples of using unique\_ptrs.

```c++
class User;

// Return a unique_ptr to extend the life time of the object out of the 
// function scope.
std::unique_ptr<User> create_user(const std::string& name) {
    auto user = std::unique_ptr<User>(new User(name));
    // Do something to user.
    // If you return a local variable, it's moved rather than copied.
    return user;
}

// Hold a local object by its base type.
class Admin: public User { /* ... */ };
class Visitor: public User { /* ... */ };
void add_user(const std::string& name, const std::string& type) {
    std::unique_ptr<User> user;
    if (type == "admin") {
        user = std::unique_ptr<Admin>(new Admin(name));
    } else {
        user = std::unique_ptr<Visitor>(new Visitor(name));
    }
    // Do something to user.
    // When the function returns, the user object will be
    // automatically freed.
}

// Hold a data member in a class by its base type.
class Post {
    // ...
private:
    // When the Post object is freed, the user_ object will be
    // automatically freed.
    std::unique_ptr<User> user_;   // May be Admin or Visitor.
};
```

To pass a unique\_ptr-held object to a function, you should avoid passing value,
for unique\_ptr cannot be copied (in C++ terms, a unique\_ptr is not copy
constructible or copy assignable). Instead, you should pass
lvalue-reference-to-const of the unique\_ptr or that of the object being held.
The latter is preferred, because the function should not know if the object is
being held by a unique\_ptr or a shared\_ptr. If the object may be a nullptr,
you should consider passing a raw pointer. A raw pointer is safe to use as long
as it does not own the object that it points to ("A owns B" means A is
responsible to free B).

```c++
// Compile error most of the time, unless you moves the unique_ptr to the
// function.
// void foo(std::unique_ptr<Object> p);

// Preferred method. But obj cannot be bound to a nullptr.
void foo(const Object& obj);
// Preferred, too, if the object may be nullptr.
void foo(const Object* pobj);
// Not preferred, but okay.
void foo(const std::unique_ptr<Object>& p);
// Okay if the function modifies obj.
void foo(Object& obj);
// Explicitly indicates that the ownership of the object
// should be transferred to the function.
void foo(std::unique_ptr<Object>&& p);
```

A unique\_ptr overloads **operator\*** and **operator->** so that it can be
dereferenced like a pointer. It also provides a public method **get** to return
the raw pointer.

```c++
struct Box {
    int width;
    int height;
}
void foo(const std::unique_ptr<Box>& pbox) {
    // Like pointers.
    pbox->width = 10;
    Box& box = *pbox;
    // Get the raw pointer.
    Box* box = pbox.get();
}
```

A unique\_ptr, in addition, provides a public method **release** that returns
the raw pointer and renounces the ownership of the object, that is, the receiver
of the raw pointer is responsible to release the object. It is useful if you are
implementing a C interface, where a function cannot return a smart pointer.

```c++
extern "C" Box* new_box() {
    auto box = std::unique\_ptr<Box>(new Box);
    // The following to functions that calculates the height and width of the
    // box, respectively, may throw exceptions.
    box->width = calculate_width();
    box->height = calculate_height();
    return box->release();
}
```

It is exception-safe inside the function by using unique\_ptr inside a function
and releasing it in the end. If an exception is thrown before it is released,
the object will be properly deallocated.

In addition to be a smart pointer, a unique\_ptr can use a custom deleter in
place of `delete` to free the object, the type of which is the second template
parameter. 

```c++
// The object of a simple class/struct that overloads operator() is usually
// called a functional object.
struct MyCustomDeleter {
    void operator()(Box* p) {
        // Deallocate the object pointed by p, like:
        // delete p;
    }
};
void my_custom_deleter(Box* p);
void foo() {
    auto pbox = std::unique_ptr<Box, MyCustomDeleter>(new Box);
}
```

## shared_ptr ##

A shared\_ptr should be used in situations where an object is held by many
owners, and the last owner that releases it is responsible to deallocate it.
Typically, a shared\_ptr occupies the size of two pointers, one pointing to the
object and the other to a shared block that contains the reference count and
custom deleter bound to the object, called a control block.

{% include svg.html name="shared_ptr" caption="The diagram of shared_ptrs" %}

Compared to a unique\_ptr, a shared\_ptr not only brings about additional space
overhead, but time cost. Each time a shared\_ptr is copied or destroyed, the
reference count must be increased or decreased, respectively.

Except for the mentioned difference between unique\_ptrs and shared\_ptrs, the
usage are mostly the same.

## weak_ptr ##

A weak\_ptr is used to hold an object without affecting the reference count.
It must be converted to a shared\_ptr before accessing the object being held.
The object will be freed as soon as the last shared\_ptr is destroyed, in spite
of existing weak\_ptrs.

A weak\_ptr is usually used to:

1. keep a cache of object;
2. break circular dependencies.

To distinguish from reference count, the number of weak\_ptrs is recorded in the
control block as the weak count. When the reference count reaches zero, the
object being held is deallocated, while the control block remains existing till
the weak count also decreases to zero.

A weak\_ptr has to be converted into a shared\_ptr before accessing the object.
If the object has been freed, the conversion throws an exception or returns a
null shared\_ptr.

Here is a simple example of using weak\_ptr to cache results.

```c++
class Account { /* .. */ };
// A time consuming operation to lookup an account from a database.
std::shared_ptr<Account> get_account_from_db(const std::string& name);

std::map<std::string, std::weak_ptr<Account>> cache;
// Cache the result without affecting reference counts.
std::shared_ptr<Account> get_account(const std::string& name) {
    auto cached_pair = cache.find(name);
    if (cached_pair != cache.end()) {
        std::shared_ptr<Account> ptr = cached_pair->second.lock();
        if (ptr) {
            return ptr;
        }
    }
    std::shared_ptr<Account> ptr = get_account_from_db(name);
    cache[name] = std::weak_ptr<Account>(ptr);
    return ptr;
}
```

## Don't abuse smart pointers ##

Smart pointers may be abused, shared\_ptr particularly. As a good habit, you
should keep the use of smart pointers as few as possible, especially
shared\_ptrs and weak\_ptrs.

### Scenario 1: as function arguments ###

If the function is not responsible to free the object, that is, it only accesses
or modifies it, it is better to pass a lvalue reference (or lvalue reference to
const) to the function. If the object is optional, passing a raw pointer (or
pointer to const) is also okay. If the smart pointer is necessary, prefer
passing by lvalue reference to by value, because unique\_ptr cannot be passed by
value, and copying shared\_ptrs brings about time overhead.

This scenario is covered in the unique\_ptr section.

### Scenario 2: as data members ###

Storing as non-pointer is the first choice. When it comes to polymorphic types,
consider unique\_ptr at first. 

If many objects keep a pointer to the object, it seems plausible to use
shared\_ptrs at first sight, but actually not. Like the following example, each
UI components hold a shared\_ptr of the Drawer object.

```c++
// Bad design.
class Drawer;
class NaiveDrawer: Drawer { /* ... */ };
class FastDrawer: Drawer { /* ... */ };

class ToolBar {
    std::shared_ptr<Drawer> drawer;
    // ...
};
class Panel {
    std::shared_ptr<Drawer> drawer;
    // ...
};
class StatusBar {
    std::shared_ptr<Drawer> drawer;
    // ...
};
class Window {
    ToolBar toolbar;
    Panel panel;
    StatusBar statusbar;
    std::shared_ptr<Drawer> drawer;
    // ...
};
```

Semantically, ToolBar, Panel and StatusBar are all components of Window, so the
life time of a Window object covers that of ToolBar, Panel and StatusBar
objects. Theoretically, the drawer is managed by the window, while the
components only use it, not own it. Therefore, the drawer object should be held
by the Window class by a unique\_ptr, while the components hold a reference or
raw pointer.

```c++
// Good design.
class Drawer;
class NaiveDrawer: Drawer { /* ... */ };
class FastDrawer: Drawer { /* ... */ };

class ToolBar {
    Drawer* drawer;
    // ...
};
class Panel {
    Drawer* drawer;
    // ...
};
class StatusBar {
    Drawer* drawer;
    // ...
};
class Window {
    ToolBar toolbar;
    Panel panel;
    StatusBar statusbar;
    // Prefer unique_ptr to shared_ptr.
    std::unique_ptr<Drawer> drawer;
    // ...
};
```
