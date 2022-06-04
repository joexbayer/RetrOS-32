#include <process.h>
#include <pci.h>

void list_net_devices()
{

}

void main()
{

}

PROGRAM(networking, &main)
ATTACH("lsnet", &list_net_devices)
PROGRAM_END