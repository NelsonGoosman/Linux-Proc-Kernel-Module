
#include "ll.h"
#define find_task_by_pid(nr) pid_task(find_vpid(nr), PIDTYPE_PID)
#define DEBUG_L 1
spinlock_t sl_dynamic;  
struct list_head pid_list;

void init_sl(){
    spin_lock_init(&sl_dynamic);
}

int add_pid(int pid){
    unsigned long new_cpu_time;
    if(get_cpu_use(pid, &new_cpu_time) == -1){
        return 1;
    }

    unsigned long flags;
    // allocate space for new node
    struct process_struct *new_node = kmalloc((size_t) (sizeof(struct process_struct)), GFP_KERNEL);
    if (!new_node){
        printk(KERN_ERR "Memory allocation failed\n");
        return -ENOMEM;
    }
    new_node->pid = pid;
    new_node->cpu_time = 0;
    // insert
    spin_lock_irqsave(&sl_dynamic, flags);
    list_add_tail(&(new_node->list), &pid_list);
    spin_unlock_irqrestore(&sl_dynamic, flags);
    return 0;
}

int remove_pid(int pid){
    // This function is not currently used however it may be useful in more
    // extensive implimentations of this program
    struct process_struct *entry = NULL;
    unsigned long flags;
    spin_lock_irqsave(&sl_dynamic, flags);
    // iterate through list, look for pid and delete
    list_for_each_entry(entry, &pid_list, list){
        if (entry->pid == pid){
            list_del(&(entry->list));
            kfree(entry);
            spin_unlock_irqrestore(&sl_dynamic, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&sl_dynamic, flags);
    printk(KERN_INFO "Could not find element\n");
    return 1;
}

void destruct_list(){
    printk(KERN_INFO "Destructing list\n");
    struct process_struct *cur = NULL, *temp = NULL;
    list_for_each_entry_safe(cur, temp, &pid_list, list){
        printk(KERN_INFO "Removing PID: %d\n", cur->pid);
        list_del(&cur->list);
        kfree(cur);
    }
}

int get_cpu_use(int pid, unsigned long *cpu_use)
{
    // given a PID, writes that pids use time into cpu_use pointer and returns 0
    // returns -1 for an invalid or terminated pid
   struct task_struct* task;
   rcu_read_lock();
   task=find_task_by_pid(pid);
   if (task!=NULL)
   {  
	*cpu_use=task->utime;
        rcu_read_unlock();
        return 0;
   }
   else 
   {
     rcu_read_unlock();
     pr_info("PID invalid: %d", pid);
     return -1;
   }
}

void update_list_and_proc(){
    #ifdef DEBUG_L
    pr_info("Updating List...\n");
    #endif

    unsigned long flags;
    unsigned long new_cpu_time;
    int chars_written;
    struct process_struct *cur = NULL, *temp = NULL;

    spin_lock_irqsave(&sl_dynamic, flags);
    list_for_each_entry_safe(cur, temp, &pid_list, list){
        if(get_cpu_use(cur->pid, &new_cpu_time) == -1){
            // Removing the node if the PID has been terminated
            #ifdef DEBUG_L
            pr_info("PID %d terminated", cur->pid);
            #endif
            list_del(&cur->list);
            kfree(cur);
        }else{
            // update cpu time
            cur->cpu_time = new_cpu_time;
            if (procfs_buff_size >= MAX_BUFF_SIZE){
                printk(KERN_WARNING "Procfile buffer is full\n");
                break;
            }
            // snprintf prints to a buffer with printf formatting. This appends a string in the format <PID>: <CPU_TIME> 
            // to databuffer at the offset of the current buffer size. Returns the amount of characters written
            chars_written = snprintf(data_buffer + procfs_buff_size, MAX_BUFF_SIZE - procfs_buff_size, 
            "%d: %lu\n", cur->pid, cur->cpu_time);
            // adds chars written to proc buffer size. If there is an issue in snprintf it is reported to KERN_ERR
            if (chars_written > 0 && chars_written < (MAX_BUFF_SIZE - procfs_buff_size)){
                procfs_buff_size += chars_written;
            }else if (chars_written < 0){
                printk(KERN_ERR "Error in witing pid to /proc/kmlab/status in update_list_and_proc()\n");
            }else if (chars_written == 0){
                printk(KERN_ERR "Nothing written to procfile\n");
            }
        }
    }
    spin_unlock_irqrestore(&sl_dynamic, flags);
}
