---
layout: post
title: Decorators
tags: [python]
comments: true
---

Decorator（修饰器）提供了一种修改函数行为的巧妙方式。

理解 decorator 最容易的方式，就是将 decorator 的功能用基础的 Python 语法表示出来。

<!--more-->

我们定义一个使用了 decorator `mydec` 的函数 `foo`：

```python
@mydec
def foo():
  # the body of the function
```

不使用 decorator 的等价形式为：

```python
def _foo():
  # the body of the function

foo = mydec(_foo)
```

在这个例子中，decorator `mydec` 实际上是一个函数。
它接受一个函数作为参数并返回一个新的函数。

满足这个条件的最简单 decorator 是返回该参数本身的函数，它没有任何额外的效果：

```python
def identity(f):
  return f
```

## 高阶函数 ##

如果一个函数的**参数或者返回值是函数**，那么这个函数就被称为**高阶函数**。
Decorator 就是一种高阶函数。

作为返回值的函数也被称为闭包（closure），因为它不但包括函数体本身，还包括该函数体所在的所有作用域。

换句话说，闭包可以访问外层作用域的变量，即使在外层函数已经返回之后。

假设我们写一个 decorator ，它会检查函数的输入是否为 int 类型，如果不是则抛出异常。
一种可能的实现是：

```python
from functools import wraps

def int_required(f):    # 接受被修饰的函数作为参数
  # 需要返回的修饰后的函数
  @wraps(f)             # 稍后讲解 wraps 的用途
  def wrapped(arg):     # arg 为被修饰函数接受的参数
    if isinstance(arg, int):
      return f(arg)     # 调用被修饰的函数
    else:
      raise ValueError('int required')
  return wrapped        # 返回修饰后的函数

# 一个使用 int_required 的函数
@int_required
def plus_one(value):
  return value + 1
```

使用了 `int_required` 修饰之后，`plus_one` 的实际值是以原函数为参数调用 `int_required` 后的返回值，也就是内层的 `wrapped` 闭包。

## 接受参数的 Decorator ##

接受参数的 decorator 与简单 decorator 相比，多了一层函数调用，即：

```python
@decorator(arg1, arg2)
def foo(k):
  # 函数体 ...
```

等价于：

```python
def _foo(k):
  # foo 的函数体 ...

foo = decorator(arg1, arg2)(foo)
```

下面我们定义一个接受类型作为参数的 decorator，它为被修饰函数做类型检查。

```python
from functools import wraps

def typecheck(t):    # t 是期望的类型
  def wrapper(f):    # wrapper 是以被修饰函数 f 为参数的闭包
    @wraps(f)
    def wrapped(v):  # 修饰过的函数
      if isinstance(v, t):
        return f(v)
      else:
        ValueError('unexpected type')
    return wrapped
  return wrapper


# 一个使用该类型检查 decorator 的函数
@typecheck(int)
def plus_one(value):
  return value + 1
```

## Decorator wraps ##

我们已经多次使用了 `wraps`，但还没有说明它的用途。

一个 Python 函数有许多相关联的属性，比如函数名和 docstring：

```python
def foo(x):
  '''The docstring of foo.'''
  return x

foo.__name__ == 'foo'                  # 函数名称
foo.__doc__ == 'The docstring of foo.' # 函数 docstring
```

然而，经过一个 decorator 修饰后，函数的这些属性会发生变化：

```python
def int_required_no_wraps(f):
  def wrapped(v):
    if isinstance(v, int):
      return f(v)
    else:
      raise ValueError('int required')
  return wrapped

@int_required_no_wraps
def plus_one(x):
  '''Add one to `x`.'''
  return x + 1

# 变成了闭包 wrapped 的属性
plus_one.__name__ == 'wrapped'
plus_one.__doc__ == ''
```

如果某些功能依赖于函数属性，那么它对于这些被修饰的函数就会失效。

为了避免这类事情，我们希望被修饰的函数的相关属性能够保留，这正是 `wraps` 做的事情：
`wraps` 修饰后的函数的属性和作为 `wraps` 参数的函数相同。

## 按原样传递参数 ##

有一些 decorator 对被修饰函数的参数并不关心，比如一个用于捕获所有异常并记录日志的 decorator。
在这里，我们可以使用 `*args, **kwargs` 来匹配所有参数，而不关心参数具体是什么。

```python
from functools import wraps
import logging

def catch_all_and_log(f):
  @wraps(f)
  def wrapped(*args, **kwargs):
    try:
      return f(*args, **kwargs)
    except Exception as e:
      logging.error('Exception {}'.format(e))
      raise       # 重新抛出异常
  return wrapped
```

在参数声明中，`*args` 用于匹配所有非关键字参数并生成一个 tuple，`**kwargs` 用于匹配所有关键字参数并生成一个 dict。
其中 `args` 和 `kwargs` 可以换成其他变量名。

在函数调用时，`*args` 用于将一个 iterable （比如 tuple 和 list）按顺序展开成多个非关键字参数，
`**kwargs` 用于将一个 dict 展开成多个关键字参数。

## Currying ##

这和 decorator 并不直接相关，但也是很有趣的函数式编程知识。

一个接受多个参数的函数，比如 `add(a, b)` ，可以写成一串接受单一参数的函数，就像接受参数的 decorator 一样：

```python
def add(a):
  def add_b(b):
    return a + b
  return add_b

add(1)(2) == 3
```

这个将多参数函数改写为多层单参数函数的过程，被称作 **Currying** 。

**Currying** 最显而易见的用途就是可以将计算了一半的函数当作数据传递给其他函数，或留待日后使用：

```python
# 将一个数组中的所有数字做指定的处理
def mymap(transform, array):
  return [transform(item) for item in array]

mymap(add(3), [1, 3, 4]) == [4, 6, 7]
```
