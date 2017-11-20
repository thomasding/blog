---
layout: post
title: 用 RAII 防止内存泄露
tags: [c++]
comments: false
---

在 C++ 中，最令人头痛的问题莫过于内存泄露。然而幸运的是，通过写出具有 **RAII** 风格的代码，我们可以在源头上避免忘记释放内存的错误。

RAII 风格可以概括为：**在构造函数中获得资源，在析构函数中释放资源。**因为 C++ 编译器会确保在函数退出时，所有栈上分配的局部对象都会被析构，所以，就不会出现忘记释放资源的情况。

在实际的编程中，以下两条准则虽然并不与 RAII 直接相关，但也非常建议遵守：

1. 资源类只负责管理资源的分配和释放，而不负责使用资源的复杂逻辑；
2. 一个资源类只管理一个资源。

<!--more-->

第一条准则可以确保资源类的简洁，从而减少出错的可能（因为它出错就可能导致资源泄露）。
第二条准则和异常相关，我们在文章的末尾会详细说明。

## 使用 RAII 的实例 ##

我们可以用一个 RAII 风格的资源类管理一个 UNIX socket。

```cpp
#include <sys/socket.h>
class Socket {
 public:
  Socket() : socket_(-1) {}
  Socket(int domain, int type, int protocol) : socket_(-1) {
    socket_ = socket(domain, type, protocol);
    if (socket_ == -1) throw std::runtime_error("socket failed");
  }
  ~Socket() {
    if (has()) close(socket_);
  }
  int get() const { 
    if (!has()) throw std::runtime_error("empty socket");
    return socket_; 
  }
  bool has() const { return socket_ != -1; }
 private:
  int socket_;
};
```

在它的实现中有如下的注意事项：

1. 在构造函数中分配 socket，如果失败就抛出异常。这样我们就可以假定，当构造函数成功返回后，我们一定成功获得了这个资源，而不需要额外的判断。
2. 在析构函数中判断是否分配了 socket，只有在持有资源时才释放。
3. 在 get 函数中检查资源是否存在，防止用户不检查就使用 get 的返回值（从而留下难以追踪的 bug）。
4. 提供 has 函数让用户可以判断资源是否存在。

然而目前的问题是，这个 Socket 资源类只允许我们在一个函数或类内部使用它，而不允许我们将它转移走。
比如，我们无法写一个返回 Socket 的函数。
因此，我们需要提供移动构造函数来允许资源的转移。

```cpp
class Socket {
 public:
  Socket(const Socket&) = delete;  // 禁止拷贝
  Socket(Socket&& other) : socket_(other.socket_) {
    other.socket_ = -1;
  }
  Socket& operator=(const Socket&) = delete; // 禁止拷贝
  Socket& operator=(Socket&& other) {
    if (has()) close(socket_);
    socket_ = other.socket_;
    other.socket_ = -1;
    return *this;
  }
};
```

在上面的代码中，我们使用 `= delete` 将某个函数设置为删除状态（C++11 的新语法），当用户不小心调用该函数时，编译器就会报错。

## 一个资源类只管理一个资源的原因 ##

假设我们允许一个资源类管理两个资源，我们可以尝试查找可能出错的地方。比如下面的类同时管理一个 socket 和一个 int （将其当作一个资源）。

```cpp
class IntSocket {
 public:
  Socket() : val_(nullptr), socket_(-1) {}
  Socket(int val, int domain, int type, int protocol) : 
    val_(nullptr), socket_(-1) {
    val_ = new int;
    *val_ = val;
    socket_ = socket(domain, type, protocol);
    if (socket_ == -1) throw std::runtime_error("socket failed");
  }
  ~Socket() {
    if (has_val()) delete val_;
    if (has_socket()) close(socket_);
  }
  int* get_val() const { 
    if (!has_val()) throw std::runtime_error("empty val");
    return val_; 
  }
  int get_socket() const { 
    if (!has_socket()) throw std::runtime_error("empty socket");
    return socket_; 
  }
  bool has_val() const { return val_ != nullptr; }
  bool has_socket() const { return socket_ != -1; }
 private:
  int* val_;
  int socket_;
};
```

这个资源类有一个严重的 bug：如果构造函数中 socket 分配失败，就会抛出异常，从而导致 int 资源泄露。

如果我们让 int 资源和 socket 资源用两个资源类管理，并让 IntSocket 包含这两个资源类，则不会出现资源泄露。
因为**如果某一个成员构造失败，编译器总会保证它之前构造的成员可以被正确地析构**，这样即使 socket 分配失败了，也可以确保 int 资源被释放。
