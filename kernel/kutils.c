#include <util.h>
#include <memory.h>
#include <serial.h>
#include <ksyms.h>
#include <terminal.h>
#include <vbe.h>
#include <pcb.h>

const char *units[] = {"bytes", "kb", "mb"};

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

    for (int i = 0; i < args.argc; i++)
    {
        dbgprintf("%d: %s\n", args.argc, args.argv[i]);
    }
    
    dbgprintf("%s %s\n", args.argv[0], str);

	void (*ptr)(int argc, char* argv[]) = (void (*)(int argc, char* argv[])) ksyms_resolve_symbol(args.argv[0]);
	if(ptr == NULL){
		return -1;
	}

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

    int len = strlen(reason);
    const char* message = "KERNEL PANIC";
    int message_len = strlen(message);
    vesa_fillrect(vbe_info->framebuffer, 0, 0, vbe_info->width, vbe_info->height, 1);

    for (int i = 0; i < message_len; i++){
        vesa_put_char16(vbe_info->framebuffer, message[i], 16+(i*16), vbe_info->height/3 - 24, 15);
    }
    
    struct pcb* pcb = current_running;
    vesa_printf(vbe_info->framebuffer, 16, vbe_info->height/3, 15, "A critical error has occurred and your system is unable to continue operating.\nThe cause of this failure appears to be an essential system component.\n\nReason:\n%s\n\n###### PCB ######\npid: %d\nname: %s\nesp: 0x%x\nebp: 0x%x\nkesp: 0x%x\nkebp: 0x%x\neip: 0x%x\nstate: %s\nstack limit: 0x%x\nstack size: 0x%x (0x%x - 0x%x)\nPage Directory: 0x%x\nCS: %d\nDS:%d\n\n\nPlease power off and restart your device.\nRestarting may resolve the issue if it was caused by a temporary problem.\nIf this screen appears again after rebooting, it indicates a more serious issue.",
    reason, pcb->pid, pcb->name, pcb->ctx.esp, pcb->ctx.ebp, pcb->kesp, pcb->kebp, pcb->ctx.eip, pcb_status[pcb->state], pcb->stack_ptr, (int)((pcb->stack_ptr+0x2000-1) - pcb->ctx.esp), (pcb->stack_ptr+0x2000-1), pcb->ctx.esp,  pcb->page_dir, pcb->cs, pcb->ds);

    PANIC();
}