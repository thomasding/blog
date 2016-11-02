#include <iostream>

void foo(int a) {
  (void)a;
}

int foo(double a) {
  (void)a;
  return 10;
}

void bar() {
  int x = 10;
  double y = 10.;
  int zz = foo(y);
  (void)x;
  (void)zz;

  char (*t)[10];
  t[0] = 10;
}

template <int N> int bar(int x, char[N % 2 == 0]) { return x; }
template <int N> int bar(int x, char[N % 2 == 1]) { return x + 1; }
