---
layout: post
title: 用模版写出通用代码
tags: [c++]
comments: false
---

在实际编程中，我们常常要写出只有类型不同，而具体实现相同的代码。
比如我们要实现一个简单的矩阵乘法函数，但既需要支持 int 类型，也需要支持 float 类型。
如果在 C 中，我们只能将相似的代码重复一遍，而在 C++ 中，我们可以使用**模版**来减少重复代码。

<!--more-->

## 简单的函数模版 ##

**函数模版 (function templates)** 可以用于定义一组类型不同但实现相同的函数。以返回较小值的函数 **min** 为例，用函数模版可以实现为：

```cpp
template <class T>
T min(T a, T b) {
  return a < b ? a : b;
}
```

在这个栗子中，**T** 是模版参数，**class** 关键字表示 **T** 可以被替换为任意类型。
在 `min<int>(2, 3)` 中，**T** 会被替换为 **int**，因此该表达式的值为2。
在 `min<double>(2.0, 3.0)` 中，**T** 会被替换为 **double**，因此该表达式的值为2.0。

在调用函数模版时，C++会尝试根据函数参数的类型推导出模版参数，因此我们可以省略模版参数，即可以将刚才的例子分别简化为 `min(2, 3)` 和 `min(2.0, 3.0)` 。

## 简单的类模版 ##

**类模版 (class templates)**可以用于定义一组类型不同但实现相同的类（或结构体）。以 **Pair** 类为例，用类模版可以实现为：

```cpp
template <class A, class B>
struct Pair {
  A first;
  B second;
};
```

从这个类模版，我们可以使用由两个 **int** 组成的类 `Pair<int, int>`，
由 **int** 和 **double** 组成的类 `Pair<int, double>` 等等。
我们也可以创建这些类的实例，如下所示：

```cpp
Pair<int, int> p1{1, 2};
Pair<int, double> p2{1, 3.0};
```

与函数模版不同，类模版的模版参数必须显式指定，不可以省略。

## 模版进阶知识 ##

### 1. 模版必须被定义在头文件中 ###

在定义函数时，我们在头文件中声明函数的类型，在 cpp 文件中定义函数体。在定义类时，我们在头文件中定义类，但在 cpp 文件中定义类的成员函数体。

然而，在使用模版时，因为模版代码是根据模版参数而动态生成的，所以，整个模版代码必须在使用它之前已知，所以，函数模版和类模版必须被完整地定义在头文件中。
C++ 编译器会保证在链接时不会出现同名函数冲突。


### 2. 不能推导函数模版的返回类型 ###

虽然在使用函数模版时，可以根据函数参数的类型自动推导模版参数，但无法根据返回值类型去推导，因此，下面的函数模版在使用时是不可以省略模版参数的：

```cpp
template <class T>
T convert(int v) {
  return static_cast<T>(v);
}

float v1 = convert<float>(3);
double v2 = convert<double>(3);
// char v3 = convert(3);   // 错误！
```

### 3. 模版参数还可以是整数 ###

当我们使用 `template <class T>` 时，我们声明了一个模版参数 **T**，并且表示它可以是 **任意类型**。
我们也可以声明一个整型模版参数（int/long/size_t 等，但不可以是浮点数或者对象），例如 `template <int N>`，在使用模版时会被替换为指定的整数。

整型模版参数必须是在编译期已知的，否则，无法实例化出模版代码。

我们可以定义用整型模版参数定义**数组的类模版**：

```cpp
template <class T, size_t N>
struct Array {
  T data[N];
};

Array<int, 3> a1;             // 正确。3 是编译期常量
Array<int, 2 * 4> a2;         // 正确。2 * 4 也是编译期常量
// Array<int, getint()> a3;   // 错误！getint() 的值在编译期未知
```

### 4. 模版特化 ###

**模版特化 (template specialization)** 指的是对特定的模版参数提供特别的实现。

以求最小值的函数模版 `min` 为例。
默认的实现不支持比较两个字符串指针 (const char *)，而这个功能可以通过模版特化提供：

```cpp
template <class T>    // 通用的实现
T min(T a, T b) {
  return a < b ? a : b;
}

template <>           // const char * 的实现
const char* min<const char *>(const char* a, const char* b) {
  return strcmp(a, b) < 0 ? a : b;
}
```
