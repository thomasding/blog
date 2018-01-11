---
layout: post
title: ELF 文件中符号的 Binding 与 Visibility
tags: [操作系统, c++]
comments: true
---

当我们使用 **static** 关键字修改变量和函数的作用域时，我们修改的是他们的 binding。

使用 **static** 关键字修饰的函数的作用域局限在文件内，而不使用 **static** 关键字修饰的函数可以被其他文件引用。

然而，当考虑共享对象时，问题就变得有些复杂。有些函数可以被共享对象内的其他文件引用，也可以被其他共享对象引用。而另一些函数只能被同一个共享对象内的其他文件引用，不能被其他共享对象引用。这时，就需要指定符号的 **visibility(可见性)**。

<!--more-->

## Binding ##

符号一共有三种 binding，分别是：

| binding | 含义  |
| --- | --- |
| LOCAL | 本地符号，只能在文件内被引用 |
| GLOBAL | 强全局符号，可以被其他文件引用，而且只能在一个文件中被定义 |
| WEAK | 弱全局符号，可以被其他文件引用，但是可以在多个文件中被定义 |

### Local 符号 ###

使用 **static** 关键字修饰的全局变量和函数的 binding 为 local 。

这类符号只能在同一个文件中被引用，而不能被其他文件引用。多个文件可以定义同名的 local 符号，但是这些符号不会互相影响。

### Global 符号 ###

不使用 **static** 关键字修饰的全局变量和函数的 binding 为 global 。

这类符号能在其他文件中被引用，也可以其他共享对象引用。也就是说，这样的符号在整个进程空间内是唯一的。

在链接时，如果多个文件中定义了重名的 global 符号，就会引发链接错误。

在动态加载时，如果多个共享对象定义了重名的 global 符号，那么就只会保留一个定义。

在 ELF 文件层面，global 的函数和变量都有对应的 PLT 和 GOT 表项。即使是在同一个共享对象内部访问 global 符号，也需要通过 PLT 和 GOT 来访问。**如果发生了符号重名，很有可能访问到的是其他共享对象中定义的符号，而不是同一个共享对象内定义的符号**。

为了避免这类问题，如果一个全局符号不需要导出到共享对象外，就需要将它的 visibility 设为 hidden。Visibility 的功能会在后面介绍。

### Weak 符号 ###

使用 `__attribute__((weak))` 修饰的全局变量和函数的 binding 为 weak 。

C++ 库中的 `operator new` 和 `operator delete` 等全局操作符函数的 binding 为 weak 。

如果定义了内联函数，但是该函数生成了一个函数体，那么该函数对应符号的 binding 为 weak 。

在 C++ 中，在类定义里直接定义的成员函数都是自带 **inline** 效果的，因此也是 weak symbol。

函数模版实例化后的代码是 weak symbol。

Weak symbol 可以在多个文件中被定义，但是链接时只有一个定义会被保留。

如果有多个同名的 weak symbol，那么符号长度最长的会被保留。对于变量，就是大小最大的定义会被保留。对于函数，就是函数体最长的定义会被保留。

如果有多个同名的 weak symbol 和一个 global symbol，那么那个 global symbol 的定义会被保留。

因此，如果用户定义了 `operator new` 函数，那么链接器就会使用用户定义的实现，而不是标准库中的实现。

## Visiblity ##

Visibility 控制符号在链接成共享对象或者可执行文件时的行为。

| visibility |
| --- | --- |
| DEFAULT |
| PROTECTED |
| HIDDEN |  |
| INTERNAL |
| EXPORTED |
| SINGLETON |
| ELIMINATE |

使用 `__attribute__((visibility ("xxx")))` 修饰符可以将符号的 visibility 设置为指定值。

最常用的两种 visibility 是 default 和 hidden，它们的修饰符分别是：

- `__attribute__((visibility ("default")))`
- `__attribute__((visibility ("hidden")))`

默认的 visibility 是 default，但是可以在编译时传入命令行参数 `-fvisibility=xxx` 将默认 visibility 设置为其他值。

在编译共享对象时，建议使用 `-fvisibility=hidden` 将默认 visibility 设置为 hidden。

### Default ###

在链接时，符号的 binding 保持不变。

Visibility 为 default 的 global 符号可能被其他共享对象的同名符号覆盖，导致在运行时访问的是其他共享对象中的定义，而非该共享对象内的定义。

### Protected ###

这类符号可以在链接后的 binding 保持不变，因此如果它的 binding 为 global，就可以被其他共享对象引用。但是，该符号的定义不会被其他共享对象覆盖。也就是说，在该共享对象内部，可以确保访问到的始终是该共享对象内的定义，而不是其他共享对象的定义。

只有 global 符号才可以被设置为 protected。

### Hidden ###

这类符号在链接成共享对象或者可执行文件后，binding 会从 global 变成 local 。

因此，这类符号只能在共享对象内部被访问，而不能被其他共享对象访问。

在写共享对象程序时，**建议将所有非导出符号的 visibility 都设置成 hidden**，方法是在编译时传入参数 `-fvisibility=hidden`。只将导出的符号的 visibility 设置为 default，方法是使用 `__attribute__((visibility ("default")))` 修饰需要导出的函数和变量。

### Internal ###

被保留使用。

### Exported ###

这个 visibility 确保 global 符号的 binding 在链接后仍然为 global ，而不被修改为 local。

只有 global 符号才可以将 visibility 设置为 exported 。

###  Singleton ###

这个 visibility 不但确保 global 符号的 binding 在链接后仍然为 global，而且在动态加载时，强制所有其他共享对象的同名符号都使用这个定义。

只有 global 符号才可以将 visibility 设置为 singleton 。

### Eliminate ###

这类符号是 hidden 符号的扩展。它们不但在链接后将 binding 改为 local，而且不会被添加到共享对象的符号表中。

## 小结 ##
