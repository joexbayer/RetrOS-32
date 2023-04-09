#include <stdio.h>
#include <mocks.h>

int main()
{
    testprintf(0 == 0, "Template test!");

    return failed > 0 ? -1 : 0;
}