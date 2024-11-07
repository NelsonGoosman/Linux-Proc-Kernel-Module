#ifndef __KMLAB_GIVEN_INCLUDE__
#define __KMLAB_GIVEN_INCLUDE__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#define MAX_BUFF_SIZE 2048       /*Procfile size*/

extern int procfs_buff_size;            /*Keeps track of procfile buffer size*/
extern char data_buffer[MAX_BUFF_SIZE]; /*Holds procfile data*/
void work_handler(struct work_struct* work);
void timer_callback(struct timer_list *timer);
int __init kmlab_init(void);
void __exit kmlab_exit(void);


#endif
