#include <utility>

template <typename T>
class unique_ptr {
 public:
  // Constructors that makes an empty unique_ptr.
  constexpr unique_ptr() noexcept : ptr_(nullptr) {}
  constexpr unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  // Constructor that makes a unique_ptr that holds the given object.
  explicit unique_ptr(T* p) noexcept : ptr_(p) {}

  // Implicit type conversions between unique_ptrs.
  template <typename U>
  unique_ptr(unique_ptr<U>&& u) noexcept : ptr_(u.ptr_) {
    u.ptr_ = nullptr;
  }

  // Move constructor. Transfer the object from u to this object.
  unique_ptr(unique_ptr&& u) noexcept : ptr_(u.ptr_) { u.ptr_ = nullptr; }

  // Move assignment operator can be implemented by swapping.
  unique_ptr& operator=(unique_ptr&& u) noexcept {
    swap(u);
    return *this;
  }

  // Swap with another unique_ptr object.
  void swap(unique_ptr& other) noexcept {
    using std::swap;
    swap(ptr_, other.ptr_);
  }

  // Explicitly delete the copy constructor and copy assignment operator.
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;

  // Destructor.
  ~unique_ptr() {
    // Delete the object if the unique_ptr is holding one.
    if (ptr_) {
      delete ptr_;
    }
  }

  // Access the object.
  T* get() const noexcept { return ptr_; }

  // Overloads dereferencing operators so that a unique_ptr resembles a raw
  // pointer, like *uptr and uptr->member.
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }

 private:
  T* ptr_;
};

template <typename T>
void swap(unique_ptr<T>& a, unique_ptr<T>& b) noexcept {
  a.swap(b);
}

struct A {
  int a;
};

void test() {
  unique_ptr<A> a;
  unique_ptr<A> b;
  unique_ptr<A> c = nullptr;
  unique_ptr<A> d = unique_ptr<A>(new A);
  using std::swap;
  swap(a, b);
}

struct add2 {
  int value;
  constexpr add2(int a, int b) : value(a + b) {}
};

int bar();

template <int N>
struct XX {
  static const int value = N;
};

void foo() {
  int b = XX<add2(1, 3).value>::value;
  (void)b;
}

struct Person {
  int age;
  Person(int a) : age(a) {}
}
