---
layout: post
title: 用 Mixin 扩展类的功能
tags: [c++]
comments: false
---

当多个类有一些共有的功能时，我们可以将共同的功能设计成 **mixin** ，以便复用代码。
在和类相关的各种设计方法中， **interface** 侧重于定义外界与类交互的方式；
**inheritance** 侧重于表达类之间的一般与特殊的关系；
而 **mixin** 则侧重于同时提供新的接口和实现，即扩展基础类的功能。

当某种接口和实现可以在多个类之间复用时，我们就可以考虑使用 **mixin** 的方法来设计。

<!--more-->

## 一个基本的例子：撤销功能 ##

首先，我们看一个基本的例子。许多具有状态的类都有撤销的功能需求，即返回上一次设置的状态。
一种直观的实现方式是：对每个需要撤销功能的类，既增加一个叫做 **previous_** 的成员变量以记录上一次的状态，
又增加一个叫做 **undo** 的成员函数，提供撤销功能。

如果使用 mixin 的方式，我们可以设计一个叫做 **Undo** 的 mixin ，让它支持撤销功能。

在实现 **Undo** 之间，我们假设所有可以撤销状态的类都具有一个 **set** 函数用于修改状态，一个 **get** 
函数用于获得当前状态。

```cpp
template <class Base, class State>
class Undo : public Base {
 public:
  void set(State state) override { 
    previous_ = Base::get();
    Base::set(state);
  }
  void undo() {
    State new_previous = Base::get();
    Base::set(previous_);
    previous_ = new_previous;
  }
 private:
  State previous_;
};
```

现在我们使用 **Undo** mixin 来为一个存储整数的类增加撤销功能：

```cpp
class IntegerBase {
 public:
  Integer() : value_(0) {}
  virtual void set(int value) { value_ = value; }
  int get() const { return value_; }
 private:
  int value_;
};

struct Integer : Undo<IntegerBase, int> {};
```

## 使用 CRTP 实现 Mixin ##

上面是第一种实现 Mixin 的方法，即将 mixin 的基类做为模版参数。
这种实现方式存在局限性，即 mixin 无法知道最终生成的类型，也就是上面例子中的 Integer 类。
比如，无法用这种方法实现 **clone** mixin。

Clone mixin 的功能是提供一个 clone 函数，它会将自己复制一份，并返回新生成的对象的指针。
这在某些不知道具体类型而又需要复制自身的情形下是必要的，比如如下情况：

```cpp
class Shape {
 public:
  virtual ~Shape() {}
  virtual int area() const = 0;
};

class Rectangle : public Shape {
 public:
  Rectangle(int height, int width) : height_(height), width_(width) {}
  int area() const override { return height_ * width_; }
 private:
  int height_;
  int width_;
};

class Square : public Shape {
 public:
  Square(int len) : len_(len) {}
  int area() const override { return len_ * len_; }
 private:
  int len_;
};

void make_a_copy(const Shape* shape) {
  // 如何在只知道基类指针的情况下复制一个相同的对象？
  // Shape* copy = ?
}
```

简单的方式是要求 Shape 提供一个 clone 接口，如下所示（只列出新增部分）：

```cpp
class Shape {
 public:
  virtual Shape* clone() const = 0;
};

class Rectangle : public Shape {
 public:
  Shape* clone() const override { 
    return new Rectangle(*this); 
  }
};

class Square : public Shape {
 public:
  Shape* clone() const override { 
    return new Square(*this); 
  }
};
```

然而每个子类的 clone 方法的实现都几乎相同，除了实际类型有差别，因此我们可以考虑将 clone
功能实现为一个 mixin 。然而因为第一种 mixin 无法知道最终类型是什么，所以不能实现 clone 函数。

这里，我们需要使用 C++ 模版的特性，叫做 **Curiously Recurring Template Prattern**，简称 CRTP。
简而言之，就是 **父类模版可以使用子类的类型**。

```cpp
class Shape {
 public:
  virtual Shape* clone() const = 0;
};

template <class B, class T>
struct Clone : B {
  virtual B* clone() const {
    // 注意 this 的类型是 const Clone<B, T>*，需要首先做类型转换
    return new T(static_cast<const T&>(*this));
  }
};

class Rectangle: public Clone<Shape, Rectangle> {};
class Square: public Clone<Shape, Square> {};
```

## 两种方法的比较 ##

第一种不使用 CRTP 的 mixin 实现简单，而且不容易出问题，但是无法处理某些需要知道最终类型的情况。
而使用 CRTP 的 mixin 虽然可以引入最终类型，但会降低代码可读性，尤其让没有接触 CRTP 的人感到束手无策。

因此建议只有在万不得已的情况下才使用 CRTP 。

## 然而 clone 还没有讨论完 ##

我们知道，虚函数在覆盖（override）的时候，编译器只要求子类虚函数的返回类型可以转换为父类虚函数的返回类型，
而不要求完全相同，因此，clone 最理想的实现是这样的：

```cpp
class Shape {
 public:
  virtual Shape* clone() const = 0;
};

class Rectangle : public Shape {
 public:
  Rectangle* clone() const override { 
    return new Rectangle(*this); 
  }
};

class Square : public Shape {
 public:
  Square* clone() const override { 
    return new Square(*this); 
  }
};
```

这样，如果我们使用 Rectangle 的指针，就可以得到一个新的 Rectangle 类型的对象，而不是 Shape 类型。

那么，如何使用 CRTP 实现一个支持这样的 clone 功能的 mixin 呢？

提示：下面的直观实现是错误的。

```cpp
class Shape {
 public:
  virtual Shape* clone() const = 0;
};

template <class B, class T>
struct Clone : B {
  // 直接返回 T* 而不是 B*
  virtual T* clone() const {
    return new T(static_cast<const T&>(*this));
  }
};

class Rectangle: public Clone<Shape, Rectangle> {};
class Square: public Clone<Shape, Square> {};
```

这个实现错误的原因是，编译器在梳理 Rectangle 的继承关系之前，先要梳理 Clone<Shape, Rectangle> 的继承关系。
而在实例化 Clone<Shape, Rectangle> 时，遇到 Rectangle* 转换为 Shape* 的需要，而这个时候，
我们还不知道 Shape* 是 Rectangle* 的父类。因此，这段代码是无法编译通过的。
