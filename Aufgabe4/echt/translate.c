#include "translate.h"

// global variables 
int translate_major;    // recieved major number
struct translate_dev *translate_devs;    // translate device

// translate parameters (and default values)
static char *translate_subst = STD_TRANSLATE_SUBSTR;
static int translate_bufsize = STD_BUFFER_SIZE;

// make variables into module params
module_param(translate_subst, charp, S_IRUGO);
module_param(translate_bufsize, int, S_IRUGO);

// TODO: refactor debug prints
// TODO: better naming of Anwendungsfunktionen

void encode(char *write_pos) {
    int index = encodeIndexFromChar(*write_pos);
    if (index != NEUTRAL_CHAR_INDEX) {
        *write_pos = translate_subst[index];
    }
}

void decode(char *read_pos) {
    char * pchar = strchr(translate_subst, *read_pos);
    // if the char was found in substr (aka, had been encoded)
    if (pchar != NULL) {
	// then get the original char according to
	// the position of the finding
	
	// calc the index using both pointers
        int index = pchar - translate_subst;
	*read_pos = decodeFromIndex(index);
    }
}

char decodeFromIndex(int index) {
    if( IS_IN_LOWER_CASE_SUBSTR(index) ) {
	return (LOWER_CASE_ASCII + (index -LOWER_CASE_SUBSTR_OFFSET));
    }
    else {
	return (UPPER_CASE_ASCII + (index - UPPER_CASE_SUBSTR_OFFSET));
    }
}

int encodeIndexFromChar(char c) {
    int result = NEUTRAL_CHAR_INDEX;
    if (IS_UPPER_CASE(c)) {
        result = c - UPPER_CASE_ASCII + UPPER_CASE_SUBSTR_OFFSET;
    } else if (IS_LOWER_CASE(c)) {
        result = c - LOWER_CASE_ASCII + LOWER_CASE_SUBSTR_OFFSET;
    }
    return result;
}

// open operation (taken from scull)
int translate_open(struct inode *inode, struct file *filp) {
    struct translate_dev *dev = container_of(inode->i_cdev, struct translate_dev, cdev);
    // makes a pointer to the device from a 'complicated' inode
    filp->private_data = dev;
    
    DEBUG(printk(KERN_NOTICE "translate_open()\n"));
    
    // we check whether the user is in write or read mode.
    // then we try to access the corresponding part of the device
    // if the device is free for writing/reading
    if( (filp->f_mode & FMODE_WRITE) == FMODE_WRITE) {
	// try to access the device. for that we try to decrease the
	// semaphore value. If we succeed, we may use it.
	// else we say, that the device is busy.
        if (down_trylock(&dev->writer_open_lock) != 0) {
            DEBUG(printk(KERN_NOTICE "translate_open: device already being written onto.\n"));
            return -EBUSY;
        }
    } else {
	// same goes for reading
        if (down_trylock(&dev->reader_open_lock) != 0) {
            DEBUG(printk(KERN_NOTICE "translate_open: device already being read from.\n"));
            return -EBUSY;
        }
    }
    
    return EXIT_SUCCESS;
}

// close file operation.
// simply check which mode the user was in and decrease the corresponding
// semaphore. this then allows others to read/write
int translate_close(struct inode *inode, struct file *filp) {
    struct translate_dev *dev = filp->private_data;
    
    DEBUG(printk(KERN_NOTICE "translate_close()\n"));
    
    if ((filp->f_mode & FMODE_WRITE) == FMODE_WRITE) {
        
        up(&dev->writer_open_lock);
    } else {
        up(&dev->reader_open_lock);
    }
    return EXIT_SUCCESS;
}

// fileoperation method for tag "write"
ssize_t translate_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    /*allocate an fill the struct to fileoperations*/
    struct translate_dev *dev = filp->private_data;
    int writePointerIndex = (dev->write_pos - dev->buffer) / sizeof(char);
    
    
    
    ssize_t result = 0;
    int itemsCopied = 0;

    #ifdef DEBUG_MESSAGES
    //printk(KERN_NOTICE "--- translate_write called ---\n");
    #endif
    
    
    #ifdef DEBUG_MESSAGES
    //printk(KERN_NOTICE "translate_write: writePointerIndex= %d \n",writePointerIndex);
    #endif
    
    #ifdef DEBUG_MESSAGES
    //printk(KERN_NOTICE "translate_write: count= %d \n",count);
    #endif
    
    #ifdef DEBUG_MESSAGES
    //printk(KERN_NOTICE "translate_write: param buffer= %s \n",buf);
    #endif

    while (itemsCopied < count) {
        if (itemsCopied == 0) {
            /*decrement the value of the semaphore freeBufferSpace, is interruptable*/
            if (down_interruptible(&dev->freeBufferSpace)) {

                #ifdef DEBUG_MESSAGES
                //printk(KERN_NOTICE "translate_write: down_interruptible failed. sending -ERESTARTSYS \n");
                #endif

                result = -ERESTARTSYS;/*wenn die Schleife durch ein Signal unterbrochen wird*/
                goto out;
            }
        } else {
            /*try to decrement the value of the semaphore freeBufferSpace*/
            if (down_trylock(&dev->freeBufferSpace) != 0) {

                #ifdef DEBUG_MESSAGES
                //printk(KERN_NOTICE "translate_write: buffer full, return number of copied chars: %d \n",itemsCopied);
                #endif

                result = itemsCopied;
                goto out;
            }
        }
        /*copy from device to user space*/
        if (copy_from_user(dev->write_pos, buf, 1)) {
            /*has wrote, cant writing now, process will try again*/
            if (itemsCopied > 0) {

                #ifdef DEBUG_MESSAGES
                //printk(KERN_NOTICE "translate_write: copy_from_user failed! already copied items \n");
                #endif

                result = itemsCopied;
            }else {/* coulnt wrote, cant writing, throw fault*/

                #ifdef DEBUG_MESSAGES
                //printk(KERN_NOTICE "translate_write: copy_from_user failed: sending -EFAULT (BAD ADDRESS) \n");
                #endif

                result = -EFAULT;/*BAD ADDRESS*/
            }
            /* increment the value of the semaphore of freeBufferSpace*/
            up(&dev->freeBufferSpace);
            goto out;
        }
        
        
        #ifdef DEBUG_MESSAGES
        //printk(KERN_NOTICE "translate_write: user buffer befor encode= %s \n",buf);
        #endif
        
        #ifdef DEBUG_MESSAGES
        //printk(KERN_NOTICE "translate_write: device%d buffer befor encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
        #endif
        
        /*if devices is translate0 then encode*/
        if (MINOR(dev->cdev.dev)== 0) {
            encode(dev->write_pos);
        }

        
        #ifdef DEBUG_MESSAGES
        //printk(KERN_NOTICE "translate_write: user buffer after encode= %s \n",buf);
        #endif
        
        #ifdef DEBUG_MESSAGES
        //printk(KERN_NOTICE "translate_write: device%d buffer after encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
        #endif
        
        itemsCopied++;
        dev->write_pos = dev->buffer + ((writePointerIndex + 1) % translate_bufsize)* sizeof(char);
        writePointerIndex = (dev->write_pos - dev->buffer) / sizeof(char);
        dev->items++;
        buf += sizeof(char);
                /* increment the value of the semaphore itemsInBuffer*/
        up(&dev->itemsInBuffer);

        #ifdef DEBUG_MESSAGES
        //printk(KERN_NOTICE "translate_write: \n\t free=%d \n\t used=%d \n\t items=%d \n\t copiedItems=%d \n",dev->freeBufferSpace.count, dev->itemsInBuffer.count,dev->items, itemsCopied);
        #endif

    }

    result = itemsCopied;
    out:

    #ifdef DEBUG_MESSAGES
    //printk(KERN_NOTICE "translate_write: \n\t return=%d \n\t free=%d \n\t used=%d \n\t items=%d \n",result,dev->freeBufferSpace.count, dev->itemsInBuffer.count, dev->items);
    #endif

    return result;
}

//fileoperation method for tag "read"
ssize_t translate_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos) {
        /*allocate an fill the struct to fileoperations*/
    struct translate_dev *dev = filp->private_data;
    int itemsCopied = 0;
    ssize_t result = 0;
    int readPointerIndex = (dev->read_pos - dev->buffer) / sizeof(char);

    #ifdef DEBUG_MESSAGES
    printk(KERN_NOTICE "--- translate_read called ---\n");
    #endif

    #ifdef DEBUG_MESSAGES
    printk(KERN_NOTICE "translate_read: readPointerIndex= %d \n",readPointerIndex);
    #endif
    
    #ifdef DEBUG_MESSAGES
    printk(KERN_NOTICE "translate_read: count= %d \n",count);
    #endif
    
    #ifdef DEBUG_MESSAGES
    printk(KERN_NOTICE "translate_read: param buffer= %s \n",buf);
    #endif
    
    while (itemsCopied < count) {
        if (itemsCopied == 0) {
                        /*decrement the value of the semaphore itemsInBuffer, is interruptable*/
            if (down_interruptible(&dev->itemsInBuffer)) {

                #ifdef DEBUG_MESSAGES
                printk(KERN_NOTICE "translate_read: down_interruptible failed. sending -ERESTARTSYS \n");
                #endif

                result = -ERESTARTSYS;
                goto out;
            }
        } else {
                        /*try to decrement the value of the semaphore itemsInBuffer*/
            if (down_trylock(&dev->itemsInBuffer) != 0) {

                #ifdef DEBUG_MESSAGES
                printk(KERN_NOTICE "translate_read: buffer empty, return number of copied chars: %d \n",itemsCopied);
                #endif

                result = itemsCopied;
                goto out;
            }
        }
        
        
        #ifdef DEBUG_MESSAGES
        printk(KERN_NOTICE "translate_read: user buffer befor encode= %s \n",buf);
        #endif
        
        #ifdef DEBUG_MESSAGES
        printk(KERN_NOTICE "translate_read: device%d buffer befor encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
        #endif
        
        /*if devices translate1 then decode*/
        if (MINOR(dev->cdev.dev) == 1) {
            decode(dev->read_pos);
        }
        
        
        #ifdef DEBUG_MESSAGES
        printk(KERN_NOTICE "translate_read: user buffer after encode= %s \n",buf);
        #endif
        
        #ifdef DEBUG_MESSAGES
        printk(KERN_NOTICE "translate_read: device%d buffer after encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
        #endif
        
        
        /*copy from device to user space*/
        if (copy_to_user(buf, dev->read_pos, 1)) {
            /*has read,cant reading, preocess will try again.*/
            if (itemsCopied > 0) {

                #ifdef DEBUG_MESSAGES
                printk(KERN_NOTICE "translate_open: copy_to_user failed! already copied items: %d \n",itemsCopied);
                #endif

                result = itemsCopied;

            }else {/*coulnt read,cant reading, throw fault */

                #ifdef DEBUG_MESSAGES
                printk(KERN_NOTICE "translate_open: copy_to_user failed: sending -EFAULT \n");
                #endif

                result = -EFAULT;
            }
            /* increment the value of the semaphore itemsInBuffer*/
            up(&dev->itemsInBuffer);
            goto out;
        }

        itemsCopied++;
        dev->read_pos = dev->buffer + ((readPointerIndex + 1) % translate_bufsize)* sizeof(char);
        readPointerIndex = (dev->read_pos - dev->buffer) / sizeof(char);
        dev->items--;
        buf += sizeof(char);
                /* increment the value of the semaphore freeBufferSpace*/
        up(&dev->freeBufferSpace);

        #ifdef DEBUG_MESSAGES
        printk (KERN_NOTICE "translate_read: \n\t free=%d \n\t used=%d \n\t items=%d \n\t copiedItems=%d \n",dev->freeBufferSpace.count, dev->itemsInBuffer.count, dev->items, itemsCopied);
        #endif

    }

    result = itemsCopied;
    out:

    #ifdef DEBUG_MESSAGES
    printk(KERN_NOTICE "translate_read: \n\t return=%d \n\t free=%d \n\t used=%d \n\t items=%d \n", result,dev->freeBufferSpace.count, dev->itemsInBuffer.count, dev->items);
    #endif

return result;

}

// called from kernel to initialize translate module. taken from scull
static int translate_init(void) {
    int result = EXIT_SUCCESS, i;
    dev_t dev;
    
    DEBUG(printk(KERN_NOTICE "translate_init()\n"));
    DEBUG(printk(KERN_NOTICE "translate_init: param subst = %s \n",translate_subst));
    DEBUG(printk(KERN_NOTICE "translate_init: param bufsize = %d \n",translate_bufsize));
    
    result = alloc_chrdev_region(&dev, MINOR_BEGINNING, NO_OF_DEVICES,"translate\n");
    translate_major = MAJOR(dev);

    if (result != EXIT_SUCCESS) {
	DEBUG(printk(KERN_ALERT "translate_init: error(%d) getting major %d \n",
		result, translate_major));
	return result;
    }

    // allocate memory for the devices
    translate_devs = kmalloc(NO_OF_DEVICES * sizeof(struct translate_dev),GFP_KERNEL);
    if (!translate_devs) {
        result = -ENOMEM;
	goto fail;
    }
    
    // reset contents of device
    memset(translate_devs, 0, NO_OF_DEVICES * sizeof(struct translate_dev));

    // initialize each device (in translate its only two)
    for (i = 0; i < NO_OF_DEVICES; i++) {
	struct translate_dev *dev = &translate_devs[i];
	// allocate buffer (just like with the device memory)
	dev->buffer = kmalloc(translate_bufsize, GFP_KERNEL);
	if (!(dev->buffer)) {
	    result = -ENOMEM;
	    goto fail;
	}
	
	dev->items = 0;
	dev->read_pos = dev->buffer;
	dev->write_pos = dev->buffer;
	
	// init semaphores
	sema_init(&dev->reader_open_lock, NUM_SIMULT_ACCESS_USERS);
	sema_init(&dev->writer_open_lock, NUM_SIMULT_ACCESS_USERS);
	sema_init(&dev->itemsInBuffer, 0);
	sema_init(&dev->freeBufferSpace, translate_bufsize);
	
	translate_setup_cdev(&translate_devs[i], i);
	DEBUG(printk(KERN_NOTICE "translate_init: translate dev %d initialized", i));
    }
    
    DEBUG(printk(KERN_NOTICE "translate_init: translate initialized"));
    
    return EXIT_SUCCESS;

    fail: translate_cleanup();
    return result;
}

// setup char device (taken form scull)
static void translate_setup_cdev(struct translate_dev *dev, int index) {
    int result = EXIT_SUCCESS, devno = MKDEV(translate_major, MINOR_BEGINNING + index);
    
    DEBUG(printk(KERN_NOTICE "translate_setup_cdev()\n"));
    
    cdev_init(&dev->cdev, &translate_ops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &translate_ops;
    
    result = cdev_add(&dev->cdev, devno, 1);

    if (result) {
        DEBUG(printk(KERN_NOTICE "Error(%d): adding translate dev %d \n", result, index));
    }
}

// cleanup procedure. taken from scull
static void translate_cleanup(void) {
    dev_t dev = MKDEV(translate_major, MINOR_BEGINNING);
    int i;
    DEBUG(printk(KERN_NOTICE "translate_cleanup()\n"));
    if (translate_devs != NULL) {
        for (i = 0; i < NO_OF_DEVICES; i++) {
	    cleanup_single_translate_dev(i);
        }
        // free device memory
        kfree(translate_devs);
    }
    unregister_chrdev_region(dev, NO_OF_DEVICES);
}


static void cleanup_single_translate_dev(int i) {
    struct translate_dev *dev = &(translate_devs[i]);
    // free Buffer
    kfree(dev->buffer);
    // reset pointers
    dev->read_pos = NULL;
    dev->write_pos = NULL;
    dev->buffer = NULL;
    // delete char dev
    cdev_del(&dev->cdev);
    DEBUG(printk(KERN_NOTICE "translate_cleanup: kfree'd translate dev %d\n", i));
}
