---
layout: post
title: Church Booleans 和 Church Numerals
tags: [程序理论]
comments: true
---

在[最小的编程语言][1]中，除了**变量**、**函数**和**调用**这三种 term，我们还引入了**布尔值**、**整数**和**数学运算符**。

实际上，仅仅依靠前三种基本 term，我们就足以表达布尔值、整数和数学运算。

<!--more-->

## Church Booleans ##

我们可以将 true 和 false 定义为接受两个参数的函数。为了和布尔值区分，我们使用 `tru` 和 `fls`：

```text
tru = λt.λf. t
fls = λt.λf. f
```

`tru` 接受两个参数并返回第一个参数，`fls` 接受两个参数并返回第二个参数。
它们称为 **Church booleans** 。

接下来，我们定义判断函数 `test`，它接受一个 Church boolean 和另外两个参数，并根据 Church boolean 决定返回哪一个参数：

```text
test = λb.λt.λf. b t f
```

我们还可以继续定义 `and` 和 `or` 函数，他们接受两个 Church boolean 并返回新的 Church boolean ：

```text
and = λa.λb. a b fls
or = λa.λb. a tru b
```

接受一个 Church boolean 并返回相反值的 `not` 也很容易定义：

```text
not = λb. b fls tru
```

有了这些逻辑运算，我们就可以计算复杂的逻辑表达式，比如 `or (and tru tru) fls`。

我们使用 call-by-value 的运算顺序来计算这个 term：

```text
    or (and tru tru) fls
    将所有函数都按定义展开
⇒   (λa.λb. a (λt.λf.t) b) ((λa.λb. a b (λt.λf.f)) (λt.λf.t) (λt.λf.t)) (λt.λf.f)
⇒   ((λa.λb. a b (λt.λf.f)) (λt.λf.t) (λt.λf.t)) (λt.λf.t) (λt.λf.f)
⇒   ((λt.λf.t) (λt.λf.t) (λt.λf.f)) (λt.λf.t) (λt.λf.f)
⇒   (λt.λf.t) (λt.λf.t) (λt.λf.f)
⇒   (λt.λf.t)
⇒   tru
```

## Church Numerals ##

我们同样可以使用函数来定义自然数。这称为 Church numerals。

一个 Church numeral 接受两个参数 `s` 和 `z`，其定义如下：

```text
c0 = λs.λz. z
c1 = λs.λz. s z
c2 = λs.λz. s (s z)
```

`c0` 直接返回 `z`，`c1` 对 `z` 调用一次 `s`，`c2` 对 `z` 调用两次 `s` ……

非常巧合的是，`c0` 的定义和 `fls` 是完全一样的。
这也许冥冥之中暗示了 `false` 和 `0` 在计算机世界中不可分离的关系。

下面，我们定义基本的加一函数 `scc` （successor 的缩写），
它接受一个 Church numeral 并返回下一个 Church numeral。

```text
scc = λc.λs.λz. s (c s z)
```

加法函数 `pls` 和乘法函数 `tms` 的定义同样直观：

```text
pls = λa.λb. b scc a
```
```text
tms = λa.λb. b (pls a) c0
```

我们可以理解为：
加法操作就是对 `a` 做 `b` 次加一；乘法操作就是对 `c0` 执行 `b` 次加 `a`。

我们可以定义出乘方函数 `pow`：

```text
pow = λa.λb. b (tms a) c1
```

乘方可以理解为：对 `c1` 执行 `b` 次乘 `a`。

这些定义都很符合我们对算数的认识。

## 数学函数的行为等价性 ##

除了 `pls` 函数，我们可以写出另一种更直观的加法函数 `pls2` ：

```text
pls2 = λa.λb.λs.λz b s (a s z)
```

将 `pls` 和 `pls2` 展开，我们会发现它们并不一致，甚至可以说相差甚远：

```text
pls  = λa.λb. b (λc.λs.λz. s (c s z)) a
pls2 = λa.λb.λs.λz. b s (a s z)
```

为了验证这两种加法的定义是否等效，我们计算 `c1` 和 `c2` 的和
（在执行 beta reduction 时替换了冲突的变量名）：

```text
   pls c1 c2
=  (λa.λb. b (λc.λs.λz. s (c s z)) a) (λs.λz. s z) (λs.λz. s (s z))
⇒  (λs.λz. s (s z)) (λc.λs.λz. s (c s z)) (λs.λz. s z)
⇒  (λc.λs.λz. s (c s z)) ((λc.λs.λz. s (c s z)) (λs.λz. s z))
⇒  (λc.λs.λz. s (c s z)) (λs.λz. s ((λs1.λz1. s1 z1) s z))
⇒  λs.λz. s ((λs2.λz2. s2 ((λs1.λz1. s1 z1) s2 z2)) s z)

   pls2 c1 c2
=  (λa.λb.λs.λz. b s (a s z)) (λs.λz. s z) (λs.λz. s (s z))
⇒  λs.λz. (λs1.λz1. s1 (s1 z1)) s ((λs2.λz2. s2 z2) s z)
```

在 call-by-value 的运算限制下，我们无法在函数内部做 beta reduction ，仍然只能得到两个看起来完全不同的 Church numeral。

为此，我们引入整数 `0` 和整数上的加一函数 `succ`，以此验证这两个 Church numeral 是否相同。

```text
   pls c1 c2 succ 0
⇒  (λs.λz. s ((λs2.λz2. s ((λs1.λz1. s1 z1) s2 z2)) s z)) succ 0
⇒  succ ((λs2.λz2. s2 ((λs1.λz1. s1 z1) s2 z2)) succ 0)
⇒  succ (succ ((λs1.λz1. s1 z1) succ 0))
⇒  succ (succ (succ 0))

   pls2 c1 c2 succ 0
⇒  (λs.λz. (λs1.λz1. s1 (s1 z1)) s ((λs2.λz2. s2 z2) s z)) succ 0
⇒  (λs1.λz1. s1 (s1 z1)) succ ((λs2.λz2. s2 z2) succ 0)
⇒  (λs1.λz1. s1 (s1 z1)) succ (succ 0)
⇒  succ (succ (succ 0))
```

因此，`pls` 和 `pls2` 的定义虽然不同，但是最终的效果是完全一样的。

## Church Numeral 上的减法  ##

首先，我们考虑减法的最简单形式：减一函数 `prd` （predecessor 的缩写）。

我们发现，如果已经有函数将 `s` 对 `z` 调用了 n 次，反过来让它少调用一次是不可能的事情。
直观上看，一个人不可能通过做某些事情来让他做过的事情反而变少。所以，我们必须寻求精妙的方法。

我们考虑由两个 Church numeral 组成的数对 `<ci, cj>` 和递增函数 `next`：
递增函数 `next` 的效果为 `next <ci, cj> ⇒  <cj, plus (cj)> ⇒ <cj, cj+1>` .

直观来讲，`next` 接受一个数对并返回一个新的数对，它的第一个元素为原数对的第二个元素，
它的第二个元素为原数对第二个元素加1。

我们将 `<c0, c0>` 作为起始数对。
那么，对 `<c0, c0>` 做 n 次 `next` 操作后会得到 `<cn-1, cn>`。
此时数对的第一个元素恰好为 `cn-1` 。

我们暂且引入这些函数，但不定义它们：

```text
pair = λci.λcj. 返回数对 <ci, cj>
fst = λp. 返回数对的第一个元素
snd = λp. 返回数对的第二个元素
next = λp. 生成下一个数对
```

根据这四个函数，我们可以定义减一函数 `prd` 。

```text
prd = λc. fst (c next (pair c0 c0))
```

根据减一函数，可以定义减法函数 `mns`。

```text
mns = λa.λb. b prd a
```

反过来，我们着手处理之前引入但没有定义的四个数对操作函数。

```text
pair = λci.λcj.λb. b ci cj
```

`pair` 函数除了接受两个 Church numeral `ci` 和 `cj` 外，
还接受一个 Church boolean `b`。
如果 `b` 为 `tru` ，则返回 `ci`，否则返回 `cj`。

据此，我们可以直观地定义出 `fst` 和 `snd`。

```text
fst = λp. p tru
snd = λp. p fls
```

根据以上三个函数定义 `next` 也很容易：

```text
next = λp. pair (snd p) (scc (snd p))
```

## 总结 ##

Church boolean 和 Church numeral 从数学上证明了：
我们可以根据三条简单的语法规则和一条运算规则完成复杂的数学运算。

这反映出计算机语言的一个特性：日渐复杂的语法特性很有可能并不是全新的事物，而只是古老语言核心的某种语法糖。
两种表面上完全不同的事物，其本质有可能是相同的。

正如这句一直没有找到出处的名言所说：

<blockquote>
<p>Objects are poor man's closures.</p>
<p>Closures are poor man's objects.</p>
</blockquote>

[1]: {% post_url 2017-08-16-the-most-simple-language %}

{% include plt.html %}
