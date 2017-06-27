/*kbd.h device specific calls for keybarods */

#include <xeroskernel.h>

#ifndef KBD_H
#define KBD_H

/* ascii */
#define ENTER 0x0A
#define EOT 0x04
#define BLOCK_PROC -3

/*need to change EOF name here since xeroskernel also defined it */

extern int kbdwrite(void* buff, int bufflen);
extern int kbd_read(void* buff, int bufflen,void (*cb_func)(void*,int, int),void* currP,int count);
extern int kbdioctl(void* command_addr);
extern int kbdopen(struct pcb* p, int fd_no);
extern int kbdclose(struct pcb* p, int fd_no);
void kbd_int_handler(void);
extern unsigned char read_char(void);
void save_kchar(unsigned char value);
void save_user_buff(void);
void reset_global (void); 

#endif