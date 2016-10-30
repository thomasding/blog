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
  int z = foo(x);
  double y = 10.;
  int zz = foo(y);
}
