#include <stddef.h>

#define PROG_NAME "task4"
#define TAG PROG_NAME ": "

int  init_device(unsigned int major_num, size_t in_buf_size);
void deinit_device(void);
