---
layout: post
title: 用访问者模式遍历树状结构
tags: [c++]
comments: false
---

在写编译器和解释器的过程中，我们需要遍历抽象语法树并完成某些操作，比如生成目标代码。所有的语法类都继承自同一个基类，但对每个语法类的操作都不同。比如生成 if 语句目标代码的方法与生成 while 语句目标代码的方法就不一样。而且，这些操作还需要可扩展性。比如，我们除了生成目标代码，还希望写一个代码优化器。同样是遍历抽象语法树，代码优化器对语法类的操作显然与生成目标代码完全不同。

<!--more-->

我们可以将这种**访问模式**抽象成一种通用的模型，它满足以下两点：

1. 需要访问的一组类型有公共的基类；
2. 对类型的访问操作必须是可以扩展的。

如果我们的访问模式符合这种类型，就可以使用**访问者模式(Visitor Pattern)**来实现。

## 访问者模式解决了什么问题 ##

在 C++ 中，如果不使用 RTTI，我们无法在运行时知道某个对象的具体类型。即使可以使用 RTTI ，但因为编译器实现的原因，RTTI 的效率通常也难以接受。为了提供某种操作，通常的做法是在基类中提供一个纯虚函数接口，并且在子类中实现这个纯虚函数。

以抽象语法树为例。我们假设在某种语言中，只有整数节点 **IntegerNode** 和加法节点 **SumNode** 两种语法，它们都继承自公共基类 **Node**。为了支持**求值(evaluation)**操作，我们可以在公共基类 **Node** 中提供纯虚函数接口 **Evaluate**，并在整数节点和加法节点中分别实现求值方法。代码如下：

```c++
class Node {
 public:
  virtual ~Node() {}
  virtual int Evaluate() = 0;
};

class IntegerNode : public Node {
 public:
  Integer(int value) : value_(value) {}
  int Value() { return value_; }
  int Evaluate() override { 
    return Value();
  }
 private:
  int value_;
};

class SumNode : public Node {
 public:
  SumNode(Node* a, Node* b) : a_(a), b_(b) {}
  Node* A() { return a_; }
  Node* B() { return b_; }
  int Evaluate() override {
    return A().Evaluate() + B().Evaluate(); 
  }
 private:
  Node* a_;
  Node* b_;
};
```

在目前看来，虚函数机制已经很好地满足了我们的需求。然而，很快我们又有了新的需求：将抽象语法树打印出来。为此，我们可以提供一个新的接口 **Print** 来实现这个操作。增加的代码如下：

```c++
class Node {
 public:
  virtual void Print() = 0;
};

class IntegerNode : public Node {
 public:
  void Print() override {
    std::cout << Value();
  }
};

class SumNode : public Node {
 public:
  void Print() override {
    std::cout << "(";
    A().Print();
    std::cout << "+";
    B().Print();
    std::cout << ")";
  }
};
```

经过简单的修改，新的需求被满足了。然而好景不长，我们又有了新的需求：将抽象语法树存储到文件中。

在满足新的需求之前，请我们稍作停留，反思一下不断加虚函数的方式是否存在不足：

1. 每次新增功能，都需要修改 Node 的接口，如果这样下去，最后 Node 和相关的具体类会变得臃肿不堪；
2. 求值和打印操作实际上并不必须是 Node 的一部分，因为我们可以发现这两个函数仅仅使用抽象语法树提供的公共接口就完成了相应的功能。

这正是**访问者模式**所要解决的问题：

1. 增加新的访问者（也就是新的功能）时不需要修改 Node 的接口；
2. 访问者的实现和 Node 的实现分离。

## 访问者模式想要达到的效果是什么 ##

在使用访问者模式重构我们的抽象语法树之前，请思考以下我们希望达到的效果是什么。我们可以脱离 C++ 的限制，假设有一种语言有我们想要的所有语法功能，在此条件下我们该如何写求值函数呢？

假设我们所用的 C++ 提供了判断某个对象是否是某个类型的操作符 **instanceof** ，一种实现求值函数的方法可能是：

```c++
int Evaluate(Node* node) {
  if (node instanceof IntegerNode) {
    return static_cast<IntegerNode*>(node)->Value();
  } else {
    SumNode* snode = static_cast<SumNode*>(node);
    return Evaluate(snode->A()) + Evaluate(snode->B());
  }
}
```

如果我们想要实现打印函数，也完全不需要修改 Node 的定义，而只需要新增一个 Print 函数，实现方法与 Evaluate 函数类似。

然而，现实的情况是我们并没有类似于 instanceof 的操作符来判断对象的类型。（RTTI 确实可以用于在运行时判断对象类型，但是 RTTI 的效率十分低下，而且有些工程会禁用 RTTI，因此我们只能另辟蹊径）

如何绕过 C++ 的这个限制呢？

如果继续从**如何让 Evaluate 知道 node 的具体类型**这个角度思考，我们总会走上实现一套粗糙而且错误百出的动态类型系统的道路。因此，不妨换个角度去想，那就是** node 所指向的对象在运行时永远知道自己是什么类型**，这是显而易见的。如果能够让 node 所指向的对象去完成对应的操作，问题就解决了。那这样是不是又绕回文章刚开始处的实现方案了呢？

并不是这样。因为这一次，我们并不关注于**某一种访问方式**，而是尝试实现一套**通用的访问方式**。当我们提到**某一种**到**通用**的抽象的时候，继承就呼之欲出了。

## 访问者类型 ##

我们首先引入访问者类型 **NodeVisitor**：

```cpp
class NodeVisitor {
 public:
  virtual ~NodeVisitor();
  void Visit(Node* node) {
    node->Accept(this);
  }
  virtual void VisitImpl(IntegerNode* node) = 0;
  virtual void VisitImpl(SumNode* node) = 0;
};
```

访问者 NodeVisitor 提供了一个接口函数 Visit，用于访问一个 Node 对象。它还提供了两个纯虚函数，分别对应于访问 IntegerNode 和 SumNode 的具体实现，需要由子类提供具体的操作。

函数 Visit 将自己传递给了 Node 类型的 Accept 方法，这正是访问者模式的精妙之处：在 Accept 方法内部，Node 对象会明确地知道自己是什么类型，因此它能够反过来调用正确的访问方法。具体的实现如下：

```cpp
class Node {
 public:
  virtual void Accept(NodeVisitor* visitor) = 0;
};

class IntegerNode : public Node {
 public:
  void Accept(NodeVisitor* visitor) override {
    // 调用的是 NodeVisitor::VisitImpl(IntegerNode*)
    visitor->VisitImpl(this);
  }
};

class SumNode : public Node {
 public:
  void Accept(NodeVisitor* visitor) override {
    // 调用的是 NodeVisitor::VisitImpl(SumNode*)
    visitor->VisitImpl(this);
  }
};
```

我们可以在访问者模式的基础上，实现 Evaluate 功能：

```cpp
class EvaluateVisitor : public NodeVisitor {
 public:
  EvaluateVisitor() : result_(0) {}
  int Result() { return result_; }
  void VisitImpl(IntegerNode* node) {
    result_ = node->Value();
  }
  void VisitImpl(SumNode* node) {
    Visit(node->A());
    int a_result = result_;
    Visit(node->B());
    result_ += a_result;
  }
 private:
  int result_;
};

int Evaluate(Node* node) {
  EvaluateVisitor visitor;
  visitor.Visit(node);
  return visitor.Result();
}
```

## 其他的语言是如何处理这类问题的 ##

我们之所以绕了一大圈，根本的原因是无法在运行时知道某个对象的具体类型。

在绝大多数动态类型语言中，因为可以在运行时判断对象类型，所以不会遇到此类问题。在 JavaScript 中，我们可以使用 `obj instanceof Class` 来判断 obj 是否是 Class 类型；在 Python 中，我们可以使用全局函数 `isinstance(obj, Class)` 来完成同样的功能。因为 Java 也支持 `instanceof` 操作符，所以也不会遇到问题。但考虑到 instanceof 常常不被建议使用，很多时候我们仍然会实现访问者模式。这类方法不被建议的原因是，instanceof 类的处理方式不是类型安全的。如果我们在写这个函数时忘记了某个子类，只有在程序执行的时候才会发现。而访问者模式会在编译时就因为某个纯虚函数没有在子类中被实现而报错。

在另一方面，现代的静态类型语言都对数据变体（Data Variant) 有各种各样的支持。数据变体与**模式匹配(pattern matching)**特性相结合后，可以非常漂亮地解决此类问题。所谓**数据变体**，指的正是抽象语法树这类数据类型。它们在本质上是同一种类型，即**抽象语法树**，但各自是不同的变体，即**整数节点**和**加法节点**。类似的例子还包括二叉树类型，它可以有两种变体，**空节点**和**非空节点**。模式匹配可以根据某个对象在运行时的实际类型来执行正确的操作。通过将数据变体与模式匹配结合，我们就可以写出类型安全的 instanceof 代码。

在经典的函数式编程语言 OCaml 中，我们可以将整个 Evaluate 简洁地写成这样（用 eval 代替长名字 evaluate）：

```ocaml
type node = 
  | Integer of int
  | Sum of node * node
;;

let rec eval n = match n with 
  | Integer i -> i
  | Sum (a, b) -> (eval a) + (eval b)
;;
```

对于完全不了解 OCaml 语法的人，也可以直观地猜测出这段代码的功能，这正是函数式编程直观性的体现。

## 总结 ##

如果我们需要解决的设计难题满足以下两点，就可以使用访问者模式：

1. 多个类型有公共基类；
2. 每个类型的访问操作都不同，而且访问操作需要可扩展性。

访问者模式的实现分成三步：

1. 定义访问者抽象基类，它提供一个对外的访问接口和一组接受具体类型的访问方法（与具体类型一一对应）；
2. 公共基类提供接受访问者类型的接口，并且在子类中实现这一接口，每个子类反过来调用访问者的对应访问方法。
3. 实现具体的访问者类型。
