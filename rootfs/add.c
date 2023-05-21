#include <stdio.h>

int add(int a, int b)
{
  return a + b;
}

int main()
{
  int c;
  c = add(10, 12);

  printf("Hello %d\n", c);
  return 0;
}