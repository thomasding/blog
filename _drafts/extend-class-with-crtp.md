---
layout: post
title: 用 Mixin 扩展类的功能
tags: [c++]
comments: false
---

**Mixin** 是一种扩展类的功能的方法。
我们可以将可复用的功能设计成 Mixin，并通过组装这些 Mixin 的方式来生成最终的类。

<!--more-->

比如我们想要让 Value 类支持撤销的功能。Value 类的定义如下：

```cpp
class Value {
 public:
  Value() : value_(0) {}
  void set(int value) { value_ = value; }
  int get() const { return value_; }
 private:
  int value_;
};
```

与其直接在 Value 类中增加撤销函数，我们可以将撤销功能设计成更通用的 Mixin：

```cpp
template <class Base, class T>
class UndoMixin {
 public:
  void set
};
```
