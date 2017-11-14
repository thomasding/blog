---
layout: post
title: 用右值引用减少对象拷贝
tags: [c++]
comments: false
---

在 C++11 之前，如果我们要将某个函数返回的 vector 传给另一个函数，要么需要复制这个 vector ，要么需要传递指针。
第一个方案需要拷贝数组，速度慢，第二个方案则需要记得手动释放内存，没有两全其美的方法。

然而在 C++11 之后，通过使用 **右值引用 (rvaule reference)**，我们可以同时克服两者的困哪，既不需要拷贝数组，也不需要手动释放内存。
更令人欣喜的是，使用右值引用实现的 **智能指针** 可以使 **C++ 程序员完全不需要手动释放内存** ，从而在源头避免内存泄露。

因此，可以说右值引用是 C++11 最值得学习的新特性。

<!--more-->

## 什么是右值引用 ##

与普通的引用不同，**右值引用** 通常引用的是 **临时对象** 。
在表达式 `(myint + 3) * 4` 中，`myint + 3` 就生成了一个临时的 int 对象。
在更复杂的表达式 `MyClass(make_range(10))` 中，`make_range(10)` 也生成了一个临时对象，这个临时对象在表达式完成后会被析构掉。
这里我们假设 make_range 返回一个 vector 。

对于 MyClass 的构造函数，在 C++11 之前，我们有两种定义方法：

```cpp
MyClass(const vector<int>& data);    // 第一种方法
MyClass(vector<int> data);           // 第二种方法
```

第一种方法的好处是避免内存拷贝，坏处是无法修改 **data**。第二种方法的好处是可以修改 **data**，但是需要额外的内存拷贝。

借助右值引用，我们可以给出第三种定义：

```cpp
MyClass(vector<int>&& data);         // 第三种方法
```

与普通的 **左值引用 (lvalue reference)** 相比，右值引用需要在类型后使用两个 **&** 符号。

第三种方法声明 **data** 引用一个临时对象，而且可以修改它，因为没有 **const** 修饰符。
如果我们将 **左值引用** 的 **const** 修饰符去掉，编译器就会报错。因为编译器不允许将临时对象绑定到非 **const** 的左值引用上。

因为左值引用和右值引用是两个不同的类型，我们可以使用 **函数重载 (overloading)** 让编译器自动选择正确的实现：

```cpp
void func(MyClass&& obj) {}         // 如果 obj 为临时对象，则使用右值引用版本
void func(const MyClass& obj) {}    // 否则，使用左值引用版本
```

## 使用右值引用的构造函数和赋值函数 ##

基于引用类型的重载最重要的用途是 **重载构造函数和赋值函数**。
相比于 C++11 之前的拷贝构造函数 (copy constructor) 和拷贝赋值函数 (copy assignment)，使用右值引用可以定义 **移动构造函数 (move constructor)** 和
**移动赋值函数 (move assignment)**。

当用临时对象构造新对象时，编译器会调用这个类的移动构造函数和移动赋值函数。因为我们知道传入的是临时对象，因此，我们可以选择将资源（比如内存块）从临时对象移动到新构造的对象中，而不是拷贝内存。
通过这样的方法，我们就节省了拷贝内存的时间。

以一个简单的数组类为例，我们可以这样实现（这里我们只列出移动构造函数和移动函数，而省略其他函数）：

```cpp
class Array {
 public:
  // 将数组从 other 移动到新对象中
  Array(Array&& other) : data_(other.data_) {
    other.data_ = nullptr;   // 必要，否则数组会被 other 的析构函数释放
  }
  
  // 先释放自己的数组，再将数组从 other 移动到自己中
  Array& operator=(Array&& other) {
    if (data_) delete [] data_; // 先释放自己的内存
    data_ = other.data_;
    other.data_ = nullptr;   // 必要，否则数组会被 other 的析构函数释放
    return *this;
  }

 private:
  int* data_;
};
```

回到本文开头的例子，我们可以为 **MyClass** 实现两个版本:

```cpp
MyClass(const vector<int> array);// 将 array 拷贝到 MyClass 对象中
MyClass(vector<int>&& array);    // 如果 array 是临时变量，则将它移动到 MyClass 对象中
```

## 右值引用是左值 ##

初学者对右值引用最大的迷惑莫过于右值引用本身是一个左值。用通俗的话讲，如果函数参数是右值引用，那么它的生存期会一直持续到函数退出，因此，对于这个函数来说，它不是临时对象，而是有名字的函数参数，因此，应该将它当作左值对待。如果我们直接讲右值引用传递给其他函数，我们会调用它的左值引用版本，而不是右值引用版本。下面的例子展示了这个问题：

```cpp
class Array {
 public:
  // 错误，data 是左值，会调用 vector 的拷贝构造函数
  Array(vector<int>&& data) : data_(data) {}
 private:
  vector<int> data_;
};
```

如果我们希望调用其他函数的右值引用版本，需要 **使用 std::move 函数将左值引用转换成右值引用。**如下所示：

```cpp
#include <utility>
class Array {
 public:
  // 正确
  Array(vector<int>&& data) : data_(std::move(data)) {}
 private:
  vector<int> data_;
};
```

需要强调的是，std::move 函数本身的功能仅仅是类型转换。它可以将任意类型左值引用转换为对应的右值引用。
