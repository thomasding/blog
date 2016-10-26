#include <utility>

struct T {};

class unique_ptr {
 public:
  // Constructors that makes an empty unique_ptr.
  unique_ptr() noexcept : ptr_(nullptr) {}
  unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

  // Constructor that makes a unique_ptr that holds the given object.
  explicit unique_ptr(T* p) noexcept : ptr_(p) {}

  // Swap with another unique_ptr object.
  void swap(unique_ptr& other) noexcept {
    using std::swap;
    swap(ptr_, other.ptr_);
  }

  // Move constructor. Transfer the object from u to this object.
  unique_ptr(unique_ptr&& u) noexcept : ptr_(u.ptr_) { u.ptr_ = nullptr; }

  // Move assignment operator can be implemented by swapping.
  unique_ptr& operator=(unique_ptr&& u) noexcept {
    swap(u);
    return *this;
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
