// Test C program
#include "lib/std.c"

struct hello {
  char* hello;
  int b;
};

int main() {
  struct hello h;
  h.hello = "Hello world!\n";
  h.b = 14;

  print(h.hello);

  return 0;
}
