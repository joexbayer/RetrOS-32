#ifndef __SYSCALL_H
#define __SYSCALL_H

#ifdef __cplusplus
extern "C"
{
#endif

void screen_put(int x, int y, unsigned char c);
void print_put(unsigned char c);
void exit();
void sleep(int seconds);

void gfx_create_window(int width, int height);

int gfx_draw_syscall(int option, void* data);
int gfx_set_title(char* title);

#ifdef __cplusplus
}
#endif

#endif /* __SYSCALL_H */
