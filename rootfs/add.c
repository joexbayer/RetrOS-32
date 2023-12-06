// Test C program

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

  printf("Hello %d\n", d);
  return 0;
}