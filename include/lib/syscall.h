#ifndef __SYSCALL_H
#define __SYSCALL_H

#ifdef __cplusplus
extern "C"
{
#endif

int invoke_syscall(int i, int arg1, int arg2, int arg3);

void screen_put(int x, int y, unsigned char c);
void print_put(unsigned char c);
void exit();
void sleep(int seconds);

void gfx_create_window(int width, int height, int flags);

int gfx_draw_syscall(int option, void* data, int flags);
int gfx_set_title(const char* title);
int gfx_set_header(const char* header);

int open(const char* name, int flags);
int write(int fd, void* buffer, int size);
int read(int fd, void* buffer, int size);
int fclose(int fd);

void* malloc(int size);
void free(void* ptr);

int thread_create(void* entry, void* arg, int flags);
void yield();


#ifdef __cplusplus
}
#endif

#endif /* __SYSCALL_H */
