---
layout: post
title: Continuation Passing Style
tags: [程序理论]
comments: true
---

我们的最小语言有条件表达式和递归，因此，我们继续考虑能否向它引入更多语法元素，比如异常处理。

异常处理要求当执行到 `throw` 的时候，程序可以直接跳到运行时最内层的 `try...catch...` 块的 `catch` 部分。
而这是我们目前的语言很难做到的。

但 continuation passing style （简称 CPS）可以很容易地解决这个问题，甚至有更多的用途。

<!--more-->

## Redex 和 Continuation ##

在 lambda calculus 中，形如 `(λx.t1) t2` 的 term 被称为 **redex**， 即 reducible expression。
其中，如果采用 call-by-value 的运算规则，那么要求 `t2` 必须为不含 redex 的 term 。

除了 redex 以外，这个 term 剩下的部分被称为 **continuation**，即化简完这个 redex 后需要做的事情。

以 `((λx.λy. x y) (λx.x)) (λx.x)` 为例，`(λx.λy. x y) (λx.x)` 是它的 redex，而 `[] (λx.x)` 则是它的 continuation。

我们可以用函数来表示该 continuation，即 `λr. r (λx.x)`。

<!--more-->

对于加入了布尔值、整数等元素的扩展 lambda calculus，
我们可以将 redex 的概念扩大为所有不需要递归就可以化简的 term。

以下 term 都可以认为是 redex ：

```text
1 + 2
not true
if true then 3 + 2 else 2
```

最后一条的 if term 也是 redex，虽然它的的 then 部分有另一个 redex 。
这是因为在我们的语言中，`if` term 是延迟计算的。
也就是说，解释器会根据 `if` 的条件 term 来选择继续计算另外两个 term 中的哪一个。
如此一来，`if` 的两个分支就不应该看作含有 redex ，
因为解释器不可能直接进入 `if` term 的分支内部去计算它们。

在我们的语言中，以下 term 则不是 redex ：

```text
3 + (2 * 3)        含有 redex 2 * 3
(λx.x) (not true)  含有 redex not true
if (not false) then 3 else 2   含有 redex not false
```

## 将 continuation 作为函数参数 ##

当
