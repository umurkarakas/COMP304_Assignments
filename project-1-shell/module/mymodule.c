#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Psvis module for Shellect");

int my_pid;

module_param(my_pid, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(my_pid, "PID: \n");

//Function for depth first search algorithm over tree
void dfs(struct task_struct *task, int parent_pid) {
	struct task_struct *child; //Pointer to the next child
	struct list_head *list; //Children
    struct task_struct *the_thread;//Thread
    char comm[TASK_COMM_LEN];

    if(parent_pid== -1){ //Root
        printk(KERN_INFO "{\"ppid\":-1,\"name\": \"%s\", \"pid\": %d, \"start_time\": %lld}\n", task->comm, task->pid, ktime_get_real()-ktime_get_ns()+task->start_time);

    }
    else{
        printk(KERN_INFO "{\"ppid\":%d,\"name\": \"%s\", \"pid\": %d, \"start_time\": %lld}\n", parent_pid, task->comm, task->pid, ktime_get_real()-ktime_get_ns()+task->start_time);

    }


    for_each_thread(task, the_thread) {
        if(task_pid_nr(the_thread) != task_tgid_nr(the_thread)){
            printk(KERN_INFO "{\"ppid\":%d,\"name\": \"%s\", \"pid\": %d, \"start_time\": %lld}\n",  task_tgid_nr(the_thread), get_task_comm(comm, the_thread), task_pid_nr(the_thread), ktime_get_real()-ktime_get_ns()+task->start_time);

            //pr_info("thread node: %p\t Name: %s\t PID:[%d]\t TGID:[%d]\n",the_thread, get_task_comm(comm, the_thread),task_pid_nr(the_thread), task_tgid_nr(the_thread));

        }
    }


	list_for_each(list, &task->children) { //Loop over children
		child = list_entry(list, struct task_struct, sibling); //Get child
		/* child points to the next child in the list */
        
		dfs(child, task->pid); //  from child
	}
}

// A function that runs when the module is first loaded
int simple_init(void) {
	struct task_struct *task;

    printk(KERN_INFO "Inserting psvis module\n");	

    printk(KERN_INFO "System tries to find task with given PID\n");
	
    //task = pid_task(find_get_pid(my_pid), PIDTYPE_PID);
   	task = get_pid_task(find_get_pid(my_pid), PIDTYPE_PID);


    if (task != NULL) {
        printk(KERN_INFO "Task is found, listing starts\n");
        dfs(task, -1);
    }else{
        printk(KERN_INFO "No task for given pid\n");
        return -1;
    }
       
	printk(KERN_INFO "Listing finished\n");
	return 0;
}

// A function that runs when the module is removed
void simple_exit(void) {
	printk(KERN_INFO "Removing psvis\n");
}

module_init(simple_init);
module_exit(simple_exit);
