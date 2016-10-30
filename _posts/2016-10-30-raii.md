---
layout: post
title: RAII
tags: [c++]
comments: true
---

RAII is a very important technique in C++, although its full name is a little
obscure: Resource Acquisition Is Initialization. While RAII can be briefly
described as: **Bind a resource to an object, acquiring it in the constructor,
releasing it in the destructor, so that its life cycle is bound to that of the
object.**

To write a good RAII class, the following implementations should be **avoided**:

1. to manually acquire or release the resource like open()/close()
   method;

2. to transfer the ownership of the resource by manual methods like move() or
   transfer(), rather than move constructor or move assignment.
   
By conforming to RAII, many situations that introduces resource leaks can be
easily reduced, and at the same time, with clearer code.

<!--more-->

Prior to C++11, it is the programmer's responsibility to remember to release the
resources in consideration of all the incidents that may occur, such as
exceptions and runtime errors. As the complexity of program grows, it is harder
for the programmer to properly release all the resources, at the same time
the code becoming more tangled. 

However, C++ compiler guarantees all the local object be destroyed when exiting the
current scope, no matter what happens. All that we need to do is to manage the
resource with a RAII class, and after that, it's the compiler's responsibility
to release them. Not only is the implementation more robust, but usually more
readable and shorter.

# Bad designs #

Assume that you've written a class to wrap the UNIX socket. Here is a bad
design (all the details about implementation have been omitted for brevity).

```c++
class Socket {
public:
    // Empty constructors and destructors.
    Socket() {}
    ~Socket() {}
    // We forbid the Socket object from being copied or moved,
    // for it's usually hard to copy or move resources correctly.
    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    // Connect to a given address. May throw bad_connection exception.
    void connect(const std::string& dst);
    // Close the connection.
    void close();

    // Read/write operations. May throw bad_connection exception.
    std::vector<uint8_t> read(size_t maxlen);
    void write(std::vector<uint8_t> data);
};

void use_socket() {
    Socket s;
    s.connect("10.101.30.2:1002");
    // Do some I/O operations.
    s.close();
}
```

The deadliest problem above is that an exception may be thrown when doing I/O
operations. On that condition, the socket will not be closed, resulting in
resource leaks. By wrapping the I/O operations with a try-catch block, it is
guaranteed that the socket will always be closed.

```c++
void use_socket() {
    Socket s;
    // connect may fail. If connect fails, the exception is thrown to the
    // caller of the function.
    s.connect("10.101.30.2:1002");
    try {
        // Do some I/O operations.
    } catch (...) {
        s.close();
        // Throw the exception to the upper level of the function.
        throw;
    }
    s.close();
}
```

Now, assume that we need to open two sockets. The code becomes:

```c++
void use_socket() {
    Socket s1, s2;
    s1.connect("10.101.30.2:1002");
    try {
        s2.connect("10.101.30.3:1004");
        try {
            // Do some I/O operations.
        } catch (...) {
            s2.close();
            throw;
        }
        s2.close();
    } catch (...) {
        s1.close();
        throw;
    }
    s1.close();
}
```

With nested try-catch blocks, the function immediately becomes a little
unreadable, although we do nothing but open two sockets.

# Good design with RAII #

Now we redesign the Socket class with RAII technique.

```c++
class Socket {
public:
    // Connect to a given address. May throw bad_connection exception.
    Socket(const std::string& dst);
    // Close the connection.
    ~Socket();

    // We forbid the Socket object from being copied or moved.
    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    // Read/write operations. May throw bad_connection exception.
    std::vector<uint8_t> read(size_t maxlen);
    void write(std::vector<uint8_t> data);
};
```

By simply establishing the connection in constructor and closing in destructor,
the Socket class is much easier to use.

```c++
void use_socket() {
    Socket s1("10.101.30.2:1002");
    Socket s2("10.101.30.3:1004");
    // Do some I/O operations.
}
```

In comparison to the try-catch style above, the RAII style is shorter, clearer,
safer, easier and overall better. You can simply open three sockets in the same
function without consideration of manually closing any of them.

```c++
void use_socket() {
    Socket s1("10.101.30.2:1002");
    // Do something first. May throw.
    Socket s2("10.101.30.3:1004");
    // Do something next. May throw.
    Socket s3("10.101.30.4:1005");
    // Do something then. May throw, too.
}
```

You can transfer the socket out of the function by wrapping it in a unique\_ptr.

```c++
std::unique_ptr<Socket> make_http_connection(
        const std::string& host, 
        const std::string& path) {
    // auto keyword can be used to replace the long type name.
    auto connection = std::unique_ptr<Socket>(new Socket(host));
    // Send HTTP request.
    return connection;
}
```

When the unique\_ptr object is destroyed and the socket has not been moved to
another unique\_ptr object, the socket will be closed. Therefore, you can simply
forget about when to close it.
