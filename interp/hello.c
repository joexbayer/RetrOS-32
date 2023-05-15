#include <stdio.h>

int add(int a, int b)
{
  return a + b;
}

int main()
{
  int result;
  result = add(10, 13);

  printf("Hello world %d\n", result);
  return 0;
}