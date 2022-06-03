#include <process.h>
#include <pci.h>

void list_net_devices()
{

}

void main()
{

}

void init_networking()
{
    int id = ATTACH_PROCESS("Network", &main);
    ATTACH_FUNCTION(id, "lsnet", &list_net_devices);
}