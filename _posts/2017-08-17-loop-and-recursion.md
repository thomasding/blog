---
layout: post
title: 循环与递归
tags: [程序理论]
comments: true
---

在[最小的编程语言][1]中，我们已经成功构造了一种可以做简单的整数和布尔运算的语言。
但和一个成熟的语言相比，我们所设计的语言缺少循环结构。
这就意味着，我们甚至无法定义一个计算阶乘的函数。

如果是这样，我们的语言就不会有太大的实际用途。

然而现实情况是，这个最小语言的计算能力与 C++ 丝毫不差。
我们只有洞察循环的本质，才能认识到这个语言的强大能力。

<!--more-->

## 循环等价于尾递归 ##

递归指的是**在函数的内部调用了这个函数自己**。
尾递归指的是**对自己的调用是函数返回前做的最后一件事情**。

需要注意的是，尽管以下两个阶乘算法对函数自己的调用都在返回语句中，但只有第二个函数是尾递归。
第一个函数只是普通的递归。因为，第一个函数在递归地调用自己后，还需要计算一个加法才能返回。

```c++
// 计算 n 的阶乘
int factorial_1(int n) {
    if (n == 0) {
        return 1;
    } else {
        return factorial_1(n-1) * n;
    }
}

// 真正的尾递归实现
int factorial_2_iter(int n, int product) {
    if (n == 0) {
        return product;
    } else {
        return factorial_2_iter(n-1, product * n);
    }
}

int factorial_2(int n) {
    return factorial_2_iter(n, 1);
}
```


我们从接触递归开始，就被警告，不要随便写递归程序，否则，过深的递归调用会把栈撑爆。

以阶乘的第一种实现为例，它所需的栈空间就是 O(n) 的。

但尾递归并非如此，因为：**任何尾递归都是循环**。

以阶乘的第二种实现为例，我们完全可以将 `factorial_2_iter` 的尾递归理解为：
**将 n 赋值为 n-1 ，将 product 赋值为 product * n，然后从头开始执行**。

按照这样的理解，它完全可以写成循环的形式：

```c++
int factorial_3(int n) {
    int product = 1;
    while (n != 0) {
        product = product * n;
        n = n - 1;
    }
    return product;
}
```

对于我们的最小语言，只要我们能够写出递归函数，就能说明它的计算能力和支持循环的语言是一样的。

## 最简单的死循环 ##

巧的是，恰好有这么一个 term，按照运算规则，可以无限运算下去：

```text
(λx. x x) (λx. x x)
```

我们发现，在使用 beta reduction 将第一个函数的 `x` 替换为 `(λx. x x)` 后，我们重新得到了`(λx. x x) (λx. x x)` 。

实际上，这个 term 做的事情，就是让 `(λx. x x)` 以自己为参数，不断递归地调用自身。

这证明了我们的语言是支持递归的，只是这个递归除了让程序卡住外没有任何实际用途。
但这启发我们，可以将函数自身当作参数传给自己来实现递归。

## 实现阶乘函数 ##

基于这样的思想，我们可以让阶乘函数接受两个参数，第一个参数是这个函数自己，第二个参数是整数 `n`
（虽然我们的语言只允许函数接受一个参数，但我们可以通过嵌套函数的方式来接受多参数。这被称为 **currying** ）。

```text
fct = λf.λn. if n = 0 then 1 else (f f (n-1)) * n
```

我们可以使用 `fct fct 5` 来计算 5 的阶乘。

然而，这样仍然很不方便，因为我们不得不额外传递一个参数。

## 不动点函数 ##

对于上面的阶乘函数，如果我们定义 `factorial = fct fct` ，那么在使用时就可以省去一个参数，但函数内部仍然需要显式地将函数自身当作参数。

我们能否省去递归调用时的麻烦呢？

我们希望找到一个函数 `fix`， 它满足条件：`(fix g) n ⇒ g (fix g) n`。

如果找到了这样的函数，我们就可以定义：

```text
fct = λf.λn. if n = 0 then 1 else (f (n-1)) * n
factorial = fix fct
```

根据 `fix g n ⇒ g (fix g) n` ，我们可以推断：

```text
  factorial n
⇒ fix fct n
⇒ fct (fix fct) n
⇒ if n = 0 then 1 else ((fix fct) n-1) * n
⇒ if n = 0 then 1 else (factorial (n-1)) * n
```

我们直接给出满足条件的 `fix` 函数：

```text
fix = λf. (λx. f (λy. x x y)) (λx. f (λy. x x y))
```

下面我们验证该函数确实满足条件：

```text
  fix g n
= (λx. g (λy. x x y)) (λx. g (λy. x x y)) n
= g (λy. (λx. g (λy1. x x y1)) (λx. g (λy1. x x y1)) y) n
= g (λy. (fix g) y) n
= g (fix g) n
```

因此，我们找到了写递归函数的通用方法：

1. 定义 `g = λf.<定义递归函数的实现>`
2. 定义 `func = fix g`

[1]: {% post_url 2017-08-16-the-most-simple-language %}
