#include <util.h>
#include <memory.h>
#include <serial.h>
#include <ksyms.h>
#include <terminal.h>
#include <gfx/gfxlib.h>
#include <vbe.h>
#include <pcb.h>

static char *units[] = {"bytes", "kb", "mb"};

unsigned char* run_length_encode(const unsigned char* data, int length, unsigned char* out, int* encodedLength)
{
    unsigned char* encodedData = out;
    int index = 0;
    unsigned short count = 1;

    for (int i = 1; i < length; i++) {
        if (data[i] == data[i - 1]) {
            count++;
        } else {
            encodedData[index++] = data[i - 1];
            encodedData[index++] = (count & 0xFF);           // Lower byte of count
            encodedData[index++] = ((count >> 8) & 0xFF);    // Upper byte of count
            count = 1;
        }
    }

    // Store the last run
    encodedData[index++] = data[length - 1];
    encodedData[index++] = (count & 0xFF);
    encodedData[index++] = ((count >> 8) & 0xFF);

    *encodedLength = index;
    return encodedData;
}

unsigned char* run_length_decode(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength)
{
    unsigned char* decodedData = out;
    int index = 0;

    for (int i = 0; i < encodedLength; i += 3) {
        unsigned char bit = encodedData[i];
        unsigned short count = encodedData[i + 1] | (encodedData[i + 2] << 8);

        for (int j = 0; j < count; j++) {
            decodedData[index++] = bit;
        }
    }

    *decodedLength = index;
    return decodedData;
}

/* Function to align a given size to the size of a void* */
int align_to_pointer_size(int size)
{
    /* Calculate the alignment requirement */
    int alignment = sizeof(void*);

    /* Align the size */
    int aligned_size = (size + alignment - 1) & ~(alignment - 1);

    return aligned_size;
}

int exec_cmd(char* str)
{
    struct args args = {
        .argc = 0
    };

    for (int i = 0; i < 10; i++){
        args.argv[i] = args.data[i];
    }
    
	args.argc = parse_arguments(str, args.data);
	if(args.argc == 0) return -1;

    for (int i = 0; i < args.argc; i++){
        dbgprintf("%d: %s\n", args.argc, args.argv[i]);
    }

	void (*ptr)(int argc, char* argv[]) = (void (*)(int argc, char* argv[])) ksyms_resolve_symbol(args.argv[0]);
	if(ptr == NULL){
		return -1;
	}


    /* execute command */
	ptr(args.argc, args.argv);

	gfx_commit();

	return 0;
}

struct unit calculate_size_unit(int bytes)
{
    int index = 0;
    double size = bytes;

    while (size >= 1024 && index < 2) {
        size /= 1024;
        index++;
    }

    struct unit unit = {
        .size = size,
        .unit = units[index]
    };

    return unit;
}

void kernel_panic(const char* reason)
{
    ENTER_CRITICAL();

    const char* message = "KERNEL PANIC";
    int message_len = strlen(message);
    //vesa_fillrect((uint8_t*)vbe_info->framebuffer, 0, 0, vbe_info->width, vbe_info->height, 1);

    for (int i = 0; i < message_len; i++){
        vesa_put_char16((uint8_t*)vbe_info->framebuffer, message[i], 16+(i*16), vbe_info->height/3 - 24, 15);
    }
    
    struct pcb* pcb = current_running;
    vesa_printf((uint8_t*)vbe_info->framebuffer, 16, vbe_info->height/3, 15, "A critical error has occurred and your system is unable to continue operating.\nThe cause of this failure appears to be an essential system component.\n\nReason:\n%s\n\n###### PCB ######\npid: %d\nname: %s\nesp: 0x%x\nebp: 0x%x\nkesp: 0x%x\nkebp: 0x%x\neip: 0x%x\nstate: %s\nstack limit: 0x%x\nstack size: 0x%x (0x%x - 0x%x)\nPage Directory: 0x%x\nCS: %d\nDS:%d\n\n\nPlease power off and restart your device.\nRestarting may resolve the issue if it was caused by a temporary problem.\nIf this screen appears again after rebooting, it indicates a more serious issue.",
    reason, pcb->pid, pcb->name, pcb->ctx.esp, pcb->ctx.ebp, pcb->kesp, pcb->kebp, pcb->ctx.eip, pcb_status[pcb->state], pcb->stackptr, (int)((pcb->stackptr+0x2000-1) - pcb->ctx.esp), (pcb->stackptr+0x2000-1), pcb->ctx.esp,  pcb->page_dir, pcb->cs, pcb->ds);

    PANIC();
}

int kref_init(struct kref* ref)
{
    ref->refs = 0;
    ref->spinlock = 0;

    return 0;
}

int kref_get(struct kref* ref)
{
    spin_lock(&ref->spinlock);

    ref->refs++;

    spin_unlock(&ref->spinlock);

    return ref->refs;
}

int kref_put(struct kref* ref)
{
    spin_lock(&ref->spinlock);

    ref->refs--;

    spin_unlock(&ref->spinlock);

    return ref->refs;
}

#define MAX_FMT_STR_SIZE 256

/* Custom sprintf function */
int32_t csprintf(char *buffer, const char *fmt, ...)
{
    va_list args;
    int written = 0; // Number of characters written
    char str[MAX_FMT_STR_SIZE];
    int num = 0;

    va_start(args, fmt);

    while (*fmt != '\0' && written < MAX_FMT_STR_SIZE) {
        if (*fmt == '%') {
            memset(str, 0, MAX_FMT_STR_SIZE); // Clear the buffer
            fmt++; // Move to the format specifier

            if (written < MAX_FMT_STR_SIZE - 1) {
                switch (*fmt) {
                    case 'd':
                    case 'i':
                        num = va_arg(args, int);
                        itoa(num, str);
                        break;
                    case 'x':
                    case 'X':
                        num = va_arg(args, unsigned int);
                        written += itohex(num, str);
                        break;
                    case 's':
                        {
                            char *str_arg = va_arg(args, char*);
                            while (*str_arg != '\0' && written < MAX_FMT_STR_SIZE - 1) {
                                buffer[written++] = *str_arg++;
                            }
                        }
                        break;
                    case 'c':
                        if (written < MAX_FMT_STR_SIZE - 1) {
                            buffer[written++] = (char)va_arg(args, int);
                        }
                        break;
                    // Add additional format specifiers as needed
                }

                // Copy formatted string to buffer
                for (int i = 0; str[i] != '\0'; i++) {
                    buffer[written++] = str[i];
                }
            }
        } else {
            // Directly copy characters that are not format specifiers
            if (written < MAX_FMT_STR_SIZE - 1) {
                buffer[written++] = *fmt;
            }
        }
        fmt++;
    }

    va_end(args);

    // Ensure the buffer is null-terminated
    buffer[written < MAX_FMT_STR_SIZE ? written : MAX_FMT_STR_SIZE - 1] = '\0';

    return written;
}