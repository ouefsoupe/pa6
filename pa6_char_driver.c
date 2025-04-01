#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define BUFFERSIZE 900
#define DEVICENAME "foo"

static char *device_buffer;
static int open_count = 0;
static int close_count = 0;
static int buffer_size = BUFFERSIZE;

struct file_operations my_file_operations = {
    .owner   = THIS_MODULE,
    .open    = my_open,       // int my_open  (struct inode *, struct file *);
    .release = my_close,      // int my_close (struct inode *, struct file *);
    .read    = my_read,       // ssize_t my_read  (struct file *, char __user *, size_t, loff_t *);
    .write   = my_write,      // ssize_t my_write (struct file *, const char __user *, size_t, loff_t *);
    .llseek  = my_seek        // loff_t  my_seek  (struct file *, loff_t, int);
};


int my_open  (struct inode *, struct file *){
    open_count++;
    printk(KERN_ALERT "Open Count: %d\n", open_count);
    return 0;
}

int my_close (struct inode *, struct file *){
    close_count++;
    printk(KERN_ALERT "Close Count: %d\n", close_count);
    return 0;
}

ssize_t my_read  (struct file *file, char __user *user_buffer, size_t size, loff_t *offset){
    // less if size reached end of the buffer
    int bytes_to_read = min(size, buffer_size - *offset);

    if(copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read) != 0){
        printk(KERN_ALERT "Failed to copy to user\n");
        return -1;
    }

    *offset += bytes_to_read;

    printk(KERN_ALERT "Read %zu bytes\n", bytes_to_read);

    return bytes_to_read;
}

ssize_t my_write (struct file *file, const char __user *user_buffer, size_t size, loff_t *offset){

    loff_t bytes_to_write = min(size, buffer_size - *offset);

    if(copy_from_user(device_buffer + *offset, user_buffer, bytes_to_write) != 0){
        printk(KERN_ALERT "Failed to write to user\n");
        return -1;
    }

    *offset += bytes_to_write;

    printk(KERN_ALERT "Wrote %zu bytes\n", bytes_to_write);

    return bytes_to_write;
}

loff_t my_seek (struct file *file, loff_t offset, int whence){
    loff_t new_position = 0;
    
    switch (whence) {
        case 0:
            new_position = offset;
            break;
        case 1:
            new_position = file->f_pos + offset;
            break;
        case 2:
            new_position = buffer_size + offset;
            break;
    }

    if(new_position < 0){
        new_position = 0;
    }
    else if (new_position > buffer_size){
        new_position = buffer_size;
    }

    file->f_pos = new_position;

    printk(KERN_ALERT "New Position: %zu", new_position);
    return new_position;
}

void init_driver(){
    // register your character driver using ​register_chrdev()
    // ​register_chrdev takes: major number, a unique name, and a pointer to a file operations struct
    int major_number = 511;
    int reg;

    // kmalloc to alloc mem
    device_buffer = kmalloc(BUFFERSIZE, GFP_KERNEL);
    if(!device_buffer){
        printk(KERN_ALERT "Failed to allocate memory\n");
        return;
    }

    reg = ​register_chrdev(major_number, DEVICENAME, my_file_operations);
    if(reg != 0){
        printk(KERN_ALERT "Failed to to register a character device driver with the kernel\n");
        return;
    }

    printk(KERN_ALERT "Successful init\n");

    return;
}

void exit_driver(){
    unregister_chrdev(511, DEVICENAME);
    kfree(device_buffer);
    printk(KERN_ALERT "Driver unregistered\n");
}


module_init(init_driver);
module_exit(exit_driver);
