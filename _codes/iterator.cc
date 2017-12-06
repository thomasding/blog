class ArrayIterator {
public:
  ArrayIterator(int* array, int pos): array_(array), pos_(pos) {}
  int& operator*() const { return array_[pos_]; }
  ArrayIterator& operator++() {
    pos_++;
    return *this;
  }
private:
  int* array_;
  int pos_;
};

int main() {
  int a[] = {1,2,3,4};
  ArrayIterator it(a, 0);
  ArrayIterator end(a, 4);
  return *it;
}
