#include <errors.h>
#include <assert.h>

static char* error_string[] = {
    "Unspecified generic error.",
    "Unable to allocate memory.",
    "Invalid index.",
    "File not found.",
    "No more free PCBs.",
    "Unable to create kernel thread.",
    "Unable to start kernel thread."

};

char* error_get_string(error_t err)
{
    assert(err < 0);
    return error_string[-err];
}
