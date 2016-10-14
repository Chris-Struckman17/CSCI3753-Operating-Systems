

#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024
#define MAJOR_NUMBER 240
#define DEV_NAME "simple_char_dev"

static char device_buffer[BUFFER_SIZE];
static int numberOpens = 0;
static int numberCloses = 0;


ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_to_user function. source is device_buffer (the buffer defined at the start of the code) and destination is the userspace buffer *buffer */
//if (*offset >= BUFFER_SIZE) *offset = BUFFER_SIZE-1;                                      
//if ((*offset + length) >= BUFFER_SIZE) length = (BUFFER_SIZE - *offset) - 1;
printk(KERN_ALERT "Reading.%d.\n",length);

if(copy_to_user(buffer,device_buffer,BUFFER_SIZE)) return -EFAULT;


return length;
}



ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
	/*  length is the length of the userspace buffer*/
	/*  current position of the opened file*/
	/* copy_from_user function. destination is device_buffer (the buffer defined at the start of the code) and source is the userspace 		buffer *buffer */
*offset = strlen(device_buffer);
if (*offset >= BUFFER_SIZE) *offset = BUFFER_SIZE-1; 
                                     
if ((*offset + length) >= BUFFER_SIZE) length = (BUFFER_SIZE - *offset) - 1; 
printk(KERN_ALERT "Writing..\n");

copy_from_user(&device_buffer[*offset],buffer, length);
printk(KERN_ALERT "Wrote %d bytes to device\n",length);

return length;

}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
numberOpens++;
printk(KERN_INFO "The Device has been opened %d time(s)\n", numberOpens);
/* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	numberCloses++;
	printk(KERN_INFO "The Device has been closed %d time(s)\n", numberCloses);/* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	.write = simple_char_driver_write,
	.read = simple_char_driver_read,
	.open = simple_char_driver_open,
	.release = simple_char_driver_close,
	
	/* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/
};

static int simple_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	/* register the device */
printk(KERN_ALERT "Device Initialization Processing..\n");
register_chrdev(MAJOR_NUMBER,DEV_NAME,&simple_char_driver_file_operations);

return 0;
}

static int simple_char_driver_exit(void)
{
	/* print to the log file that the exit function is called.*/
	/* unregister  the device using the register_chrdev() function. */
printk(KERN_ALERT "Device Removing Sequence Processing...\n");
unregister_chrdev(MAJOR_NUMBER,DEV_NAME);
return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
