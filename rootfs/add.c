// Test C program
#include "lib/std.c"

int add(int a, int b)
{
  return a + b;
}

int fac(int n)
{
  if (n == 0)
    return 1;
  return fac(n - 1) * n;
}

int main()
{
  int c, d;
  c = add(3, 2);

  d = fac(c);

  print("Hello world!\n");

  return 0;
}
