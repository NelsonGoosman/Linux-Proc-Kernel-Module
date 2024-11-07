#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include "kmlab_given.h"

// Linked list node 
struct process_struct{
    struct list_head list;
    int pid;
    unsigned long cpu_time;
};

extern struct list_head pid_list;

//THIS FUNCTION RETURNS 0 IF THE PID IS VALID AND THE CPU TIME IS SUCCESFULLY RETURNED BY THE PARAMETER CPU_USE. OTHERWISE IT RETURNS -1
int get_cpu_use(int pid, unsigned long *cpu_use);


/*
This function is called whenever the userapp writes a new pid to the list.
It will add a new node to the list with the PID.
*/
int add_pid(int pid);


/*
When a process dies/ends, this function will remove it.
*/
int remove_pid(int pid);


/*
Called in module exit function to free kernel memory
*/
void destruct_list(void);


/*
This iterates over the list and updates the PIDs runtime as well as writes the new runtime to the 
proc file. If the PID is expired, then it is removed from the linked list 
*/
void update_list_and_proc(void);


/*
initilizes spinlock for linked list operations
*/
void init_sl(void);

#endif