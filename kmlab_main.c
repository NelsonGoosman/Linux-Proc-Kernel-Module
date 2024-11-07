#define LINUX

#include <linux/timer.h>
#include <linux/workqueue.h>
#include "kmlab_given.h"
#include "ll.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Goosman"); 
MODULE_DESCRIPTION("CPTS360 KM PA");

#define DEBUG 1
#define PROC_DIR_NAME "kmlab"
#define PROC_FILE_NAME "status"
#define PROC_FULL_PATH "/proc/kmlab/status"
#define PROCFILE_PERMISSION 0666
#define TIMER_UPDATE_FREQ 5                  /*Frequencey of timer in seconds*/
#define TIMER_INTERVAL msecs_to_jiffies(TIMER_UPDATE_FREQ * 1000)    /*Convert seconds to jiffies*/

static struct timer_list pid_timer;          /*Timer to call every 5 seconds to update the PID list*/
static struct proc_dir_entry* proc_kmlab_d;  /*proc/kmlab directory*/     
static struct proc_dir_entry* proc_status_f; /*proc/kmlab/status file*/
static struct workqueue_struct* my_wq;       /*Work queue*/
struct work_struct my_work;                  /*Work manager for update function*/
int procfs_buff_size;            
char data_buffer[MAX_BUFF_SIZE]; 


/*
Work handler function which is called to be executed immediatly upon timer callback. This
calls update_list_and_proc which update the list of running processes and writes their
statuses to the proc file.
*/
void work_handler(struct work_struct* work){
   if (!list_empty(&pid_list)){
      update_list_and_proc();
   }
}

/*
This function is called every time the timer goes off and schedules the abover work handler
function, then resets to timer to go off
*/
void timer_callback(struct timer_list *timer){
   queue_work(my_wq, &my_work);
   //schedule timer to run again in TIMER_INTERVAL seconds
   mod_timer(timer, jiffies + TIMER_INTERVAL);
}

/*
Function that appends data from userspace to /proc/kmlab/status. Checks if the length of the data being passed in is greater
than the remaining procfile buffer size, and if it is not then it writes the buffer data to the procfile
using copy_from_user. Returns the amount of bytes written to the proc buffer.
*/
static ssize_t procfile_write(struct file* fp, const char __user * buffer, size_t count, loff_t* pos){
   int pid;
   size_t space_left = MAX_BUFF_SIZE - procfs_buff_size;
   size_t bytes_to_write = count;

   if (bytes_to_write > space_left){
      bytes_to_write = space_left;
      if (bytes_to_write == 0){
         printk(KERN_WARNING "Procfile Buffer is full");
         return -ENOSPC; //no space left
      }
   }

   #ifdef DEBUG
   printk(KERN_INFO "Appending to /proc/kmlab/status\n");
   #endif

   if (copy_from_user(data_buffer + procfs_buff_size, buffer, bytes_to_write)){
      return -EFAULT;
   }

   procfs_buff_size += bytes_to_write;
   data_buffer[procfs_buff_size] = '\0';

   // convert user buffer to integer PID then add it to list
   if(kstrtoint_from_user(buffer, count, 10, &pid) != 0){
      printk(KERN_ERR "Could not cast user buffer to unsigned long\n");
      return -EINVAL;
   }

   add_pid(pid);

   *pos += bytes_to_write;
   return bytes_to_write;
}

/*
This reads from /proc/kmlab/status and sends the data to userspace. Returns the length of data read
*/
static ssize_t procfile_read(struct file* fp, char* buf, size_t count, loff_t* offp){
   
   int len = procfs_buff_size;
   //ssize_t ret = len;

   if (*offp >= len){
      return 0;
   }

   if (*offp + count > len){
      count = len + *offp;
   }
   if(copy_to_user(buf, data_buffer + *offp, count)){
      return -EFAULT;
   }
   *offp += count;

   return count;
}

/* Setting procfile_read and procfile_write as procfile operations */
 struct proc_ops proc_fops = {
   .proc_read = procfile_read,
   .proc_write = procfile_write
};

/*
Initialization procedures
*/
int __init kmlab_init(void)
{
   #ifdef DEBUG
   printk(KERN_INFO "KMLAB MODULE LOADING\n");
   #endif
   procfs_buff_size = 0;
   proc_kmlab_d = proc_mkdir(PROC_DIR_NAME, NULL);
   if (!proc_kmlab_d){
      printk(KERN_ERR "Error: unable to create kmlab directory");
      return -ENOMEM;
   }

   proc_status_f = proc_create(PROC_FILE_NAME, PROCFILE_PERMISSION, proc_kmlab_d, &proc_fops);
   if (!proc_status_f){
      printk(KERN_ERR "Error: unable to create status file");
      remove_proc_entry(PROC_DIR_NAME, NULL);
      return -ENOMEM;
   }

   /*
   Initialize spinlock, linked list, workqueue, and timer.
   */
   init_sl();
   INIT_LIST_HEAD(&pid_list);
   my_wq = alloc_workqueue("LIST OPERATION WQ", WQ_UNBOUND, 1);
   INIT_WORK(&my_work, work_handler);
   timer_setup(&pid_timer, timer_callback, 0);
   mod_timer(&pid_timer, jiffies + TIMER_INTERVAL);

   pr_info("KMLAB MODULE LOADED\n");
   return 0;   
}

/*
Exit procedures. Removes the procfs directory and files, linked list, timer, and work queue
*/
void __exit kmlab_exit(void)
{
   #ifdef DEBUG
   pr_info("KMLAB MODULE UNLOADING\n");
   #endif

   remove_proc_entry(PROC_FILE_NAME, proc_kmlab_d);
   remove_proc_entry(PROC_DIR_NAME, NULL);
   destruct_list();
   del_timer(&pid_timer);
   destroy_workqueue(my_wq);
   pr_info("KMLAB MODULE UNLOADED\n");
}

module_init(kmlab_init);
module_exit(kmlab_exit);
