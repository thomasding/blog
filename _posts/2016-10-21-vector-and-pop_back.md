---
layout: post
title: Vector and pop_back
tags: [c++, exception-safe]
comments: true
---

In vector, there is a function **push\_back** that extends the vector size by 1
and put the object onto the last element, while there is no such function
**pop\_back** that shrinks the size by 1 and returns the last element. As is
implemented by STL, vector provides function **back** to access the last element
and another function **pop_back** that shrinks the size by 1 but returns
nothing, respectively.

<!--more-->

This implementation takes into account the chance of throwing exceptions during
the copy of the last element.

The following code demonstrates a very simple and straightforward implementation
of vector.

```c++
template< typename T >
class vector {
public:
    // Omit constructors and access functions.
    T pop_back() {
        --len_;
        return std::move(data_[len_]);
    }
private:
    T* data_;              // The element array.
    size_t len_;           // The number of elements.
    size_t capacity_;      // The length of the array.
};
```

The return statement of **pop\_back** returns a new object by copying from the
last element, which may throw exceptions. If an exception is thrown, the last
element will be lost, due to the decrement of len\_.

One may try solving the problem by returning a lvalue reference, like this:

```c++
T& pop_back() {
    --len_;
    return data_[len_];
}
```

This implementation won't work completely, because the referenced object may be
overwritten by the next **push\_back**. However, **pop\_back** implies a
transfer of object ownership from the vector to the caller, but the
implementation in this way doesn't transfer the ownership at all.

One may try returning a rvalue reference, like the following code:

```c++
T&& pop_back() {
    --len_;
    return std::move(data_[len_]);
}
```

But it is no better than the two previous versions. If the caller assigns the
return value of **pop\_back** to another rvalue reference or lvalue reference to
const, it suffers from the same problem as the lvalue version. If the caller
otherwise constructs a new object from it by calling the move constructor, it
will be same as the first version.

Therefore, there is no good ways to implement an exception-safe **pop\_back**.

Splitting the function into two makes the operation exception-safe.

```c++
T& back() {
    return data_[len_ - 1];
}
void pop_back() {
    data_[len_ - 1].~T();
    --len_;
}
```

In the implementation, **pop\_back** not only decreases the length of the array
by one, but manually calls the destructor of the object to destroy it without
deallocating the memory. It brings a new problem that you cannot free the memory
by calling delete[] afterwards, for it will call the destructor again, causing
the mysterious undefined behavior. Details about how vectors manage memory are
irrelevant to the topic of this post, so we are not going to discuss more
deeply about it. 

The function **back** itself won't throw any exception. Hence, it is the
designer of class T's responsibility to write exception-safe copy or move
operations. Even if the copy or move operation on the object fails, the vector
itself is not affected. As long as the object keeps its original value, it can
be moved or copied again after the exception is properly handled. No memory
leak, no data loss. The implementation becomes exception-safe.
