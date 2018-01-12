---
layout: post
title: 写 Linux 动态库的最佳实践
tags: [操作系统, c++]
comments: true
---

{% include image.html name="cover.jpg" %}

在定义全局变量和函数是，如果我们使用 **static** 关键字修饰他们，就只能够在同一个文件内引用他们；如果我们不使用 **static** 关键字，就可以在其他文件中引用他们。

然而，当实现动态库时，问题就变得有些复杂。

动态库的接口函数可以被动态库内的其他文件引用，也可以被其他动态库引用。而动态库的内部函数只能被同一个动态库内的其他文件引用，不能被其他动态库引用。

对于“如何让函数可以被动态库内的其他文件引用，而不能被其他动态库引用”的需求，**static** 关键字是无能为力的。

这时，我们就需要修改符号的**可见性（visibility）**。

<!--more-->

## 符号 ##

对于 ELF 文件来说，程序中出现的所有变量和函数都是**符号（symbol）**。

变量所在的内存单元和函数的函数体被称作符号的**定义（definition）**。

当我们使用 **static** 关键字修饰变量或者函数时，我们是在修改符号的 **binding（绑定关系）**。在 C 语言中，我们通常称之为作用域。

## 符号的 Binding ##

符号一共有三种 binding，分别是：

| binding | 含义  |
| --- | --- |
| LOCAL | 本地符号，只能在文件内被引用 |
| GLOBAL | 强全局符号，可以被其他文件引用，而且只能在一个文件中被定义 |
| WEAK | 弱全局符号，可以被其他文件引用，但是可以在多个文件中被定义 |

### Local Symbol ###

使用 **static** 关键字修饰的全局变量和函数是 local symbol。

这类符号只能在同一个文件中被引用，而不能被其他文件引用。多个文件可以定义同名的 local 符号，但是这些符号不会互相影响。

一个动态库中的 local symbol 和另一个动态库的同名 local symbol 之间不会互相影响。

### Global Symbol ###

不使用 **static** 关键字修饰的全局变量和函数是 global symbol 。

这类符号能在其他文件中被引用，也可以其他动态库引用。也就是说，这样的符号在整个进程空间内有唯一的定义。

在链接时，如果多个文件中定义了重名的 global 符号，就会引发链接错误。

在动态加载时，如果多个动态库定义了重名的 global 符号，那么就只会保留其中的一个定义。这就意味着，在访问同一个动态库内定义的 global 符号时，有可能访问到的是其他动态库中的定义。

在 ELF 文件层面，在动态库中访问 global symbol 都需要借助 PLT 和 GOT，而不能直接访问，因此速度也比访问 local symbol 慢。

### Weak 符号 ###

在 C 和 C++ 程序中，有以下方法可以定义 weak symbol：

1. 使用 `__attribute__((weak))` 修饰的全局变量和函数是 weak symbol；
2. C++ 库中的 `operator new` 和 `operator delete` 是 weak symbol；
3.如果定义了内联函数，但是该内联函数生成了一个独立的函数体，那么该符号为 weak symbol；
4. 在 C++ 中，在类定义里直接定义的成员函数都自带 **inline** 效果，因此也是 weak symbol；
5. 函数模版实例化后的代码是 weak symbol。

Weak symbol 可以在多个文件中被定义，但是链接时只有一个定义会被保留。保留的规则是：

1. 如果有多个同名的 weak symbol，那么符号长度最长的会被保留。
   1. 对于变量，就是大小最大的定义会被保留。
   2. 对于函数，就是函数体最长的定义会被保留。
2. 如果有多个同名的 weak symbol 和一个 global symbol，那么那个 global symbol 的定义会被保留。

因此，如果用户定义了 `operator new` 函数，那么链接器就会使用用户定义的实现，而不是标准库中的实现。

## 符号的 Visiblity ##

为了解决全局符号可能在动态库之间互相干扰的问题，ELF 引入了符号的可见性（visibility）。

在链接成动态库或者可执行文件时，链接器根据符号的 visibility 修改它的 binding。

Visibility 一共有 7 种，但是常用的只有 default 和 hidden 两种。它们的修饰符分别是：

- `__attribute__((visibility ("default")))`
- `__attribute__((visibility ("hidden")))`

默认的 visibility 是 default，但是可以在编译时传入命令行参数 `-fvisibility=hidden` 将默认 visibility 设置为 hidden。

### Default Visibility ###

在链接时，符号的 binding 保持不变。

Visibility 为 default 的 global 符号可能被其他动态库的同名符号覆盖，导致在运行时访问的是其他动态库中的定义，而非该动态库内的定义。

**通常，需要导出的符号的 visibility 为 default。**

### Hidden Visibility ###

这类符号在链接成动态库或者可执行文件后，binding 会从 global 变成 local，同时 visibility 变成 default。

因此，这类符号只能在动态库内部被访问，而不能被其他动态库访问。

**对于动态库或者可执行程序来说，所有不需要导出的符号的 visibility 都应该是 hidden。**

## 最佳实践 ##

**在实现 C 和 C++ 的动态库时，使用 `-fvisibility=hidden` 来编译动态库**。

在定义 API 时，建议使用 `DLL_PUBLIC` 和 `DLL_LOCAL` 宏来控制符号的可见性，它在 Windows、Cygwin、Linux 和 macOS 上都可以正常工作：

```c
#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      // Note: actually gcc seems to also supports this syntax.
      #define DLL_PUBLIC __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      // Note: actually gcc seems to also supports this syntax.
      #define DLL_PUBLIC __declspec(dllimport)
    #endif
    #define DLL_LOCAL
  #endif
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif
```

在 C 中，可以使用这个宏导出函数和变量：

```c
// 使用 DLL_PUBLIC 修饰需要导出的符号
DLL_PUBLIC int my_exported_api_func();
DLL_PUBLIC int my_exported_api_val;

// 不使用 DLL_PUBLIC 修饰动态库内部的符号，
// 因为默认可见性被修改为 hidden
int my_internal_global_func();
```

在 C++ 中，可以使用这个宏来导出一个类：

```cpp
// 使用 DLL_PUBLIC 修饰需要导出的类
class DLL_PUBLIC MyExportedClass {
 public:
  // 类里面的所有方法默认都是 DLL_PUBLIC 的
  MyExportedClass();
  ~MyExportedClass();
  int my_exported_method();
  
 private:
  int c;

  // 使用 DLL_LOCAL 修饰动态库的内部符号
  DLL_LOCAL int my_internal_method();
};
```

## 参考阅读 ##

[Symbol Table Section](https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-79797.html#chapter7-27) ELF 文件中符号表的定义，详细描述了 binding 与 visibility。

[Visibility](https://gcc.gnu.org/wiki/Visibility) GCC wiki 中关于 visibility 的最佳实践。
