#include <errors.h>
#include <assert.h>

static char* error_string[] = {
    "No error.",
    "Unspecified generic error.",
    "Null pointer.",
    "Invalid arguments.",
    "Work queue full.",
    "Unable to allocate memory.",
    "Invalid index.",
    "File not found.",
    "No more free PCBs.",
    "Unable to create kernel thread.",
    "Unable to start kernel thread.",
    "Invalid socket.",
    "Invalid socket type.",
    "Invalid size of packet (probably too high).",
    "Ring buffer full.",
    "Ring buffer empty.",
    "PCB is NULL.",
    "PCB queue is NULL.",
    "Unable to create PCB queue.",
    "PCB queue is empty.",
    "Scheduler already exists and is initiated.",
    "Invalid scheduler, probably not initiated.",
    "Window not found.",
    "Window operations are corrupted.",
    "Out of memory.",
};

char* error_get_string(error_t err)
{
    assert(err < 0);
    return error_string[-err];
}
