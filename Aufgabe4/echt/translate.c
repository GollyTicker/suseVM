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

void encode_char(char *write_pos) {
    int index = encode_index_from_char(*write_pos);
    if (index != NEUTRAL_CHAR_INDEX) {
        *write_pos = translate_subst[index];
    }
}

void decode_char(char *read_pos) {
    char * pchar = strchr(translate_subst, *read_pos);
    // if the char was found in substr (aka, had been encoded)
    if (pchar != NULL) {
	// then get the original char according to
	// the position of the finding
	
	// calc the index using both pointers
        int index = pchar - translate_subst;
	*read_pos = decode_from_index(index);
    }
}

char decode_from_index(int index) {
    if( IS_IN_LOWER_CASE_SUBSTR(index) ) {
	return (LOWER_CASE_ASCII + (index -LOWER_CASE_SUBSTR_OFFSET));
    }
    else {
	return (UPPER_CASE_ASCII + (index - UPPER_CASE_SUBSTR_OFFSET));
    }
}

int encode_index_from_char(char c) {
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

// implementation of what happens when someone now wants to write
// into our device. we got much help form others groups here.
ssize_t translate_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos) {
    struct translate_dev *dev = filp->private_data;
    
    // position of the writer on the buffer
    // it abstracts from the type char
    int p_writer = (dev->write_pos - dev->buffer) / sizeof(char);
    
    int num_copied_items = 0;	// tracks the progress of the # of copied items
    
    DEBUG(printk(KERN_NOTICE "translate_write()\n"));
    
    while (num_copied_items < count) {
	// decrease the semaphore
	// if the buffer is full, this fails.
	if (dev->items == translate_bufsize) {
	    DEBUG(printk(KERN_NOTICE "translate_write: buffer is full. copied %d items \n",num_copied_items));
	    return num_copied_items;
	}
	
        // at this point, the buffer isnt full and
        // there are still items to be copied.
        
        // now copy a single character from the user
        if (copy_from_user(dev->write_pos, buf, 1)){
                DEBUG(printk(KERN_NOTICE "translate_write: copy_from_user failed \n"));
		// free semaphore again and end
                return -EFAULT;
        }
        
        // if we're the translate0 device
        // then encode during writing from user into device
        if (MINOR(dev->cdev.dev) == MINOR_BEGINNING) {
            encode_char(dev->write_pos);
        }
        
        // now that we've succesfully copied (encoded)
        // a char, we increment all loop-dependent-variables.
        
        // update pointers
        dev->write_pos = dev->buffer + ((p_writer + 1) % translate_bufsize)* sizeof(char);
        p_writer = (dev->write_pos - dev->buffer) / sizeof(char);
        buf += sizeof(char);
	
	// update counters
        num_copied_items++;
	dev->items++;
    }

    return num_copied_items;
}

// implementation of what happens when someone now wants to read
// into our device. we got much help form others groups here.
// this is very similar to the write process
ssize_t translate_read(struct file *filp, char __user *buf,
		       size_t count,loff_t *f_pos) {
    struct translate_dev *dev = filp->private_data;
    int num_copied_items = 0;
    int p_reader = (dev->read_pos - dev->buffer) / sizeof(char);

    DEBUG(printk(KERN_NOTICE "translate_read()\n"));
    
    while (num_copied_items < count) {
        if (dev->items == 0) {
	    DEBUG(printk(KERN_NOTICE "translate_read: buffer empty, read %d chars \n",num_copied_items));
	    return num_copied_items;
	}
	
	// because we're got the information now and the user want to read them
	// we'll decode our buffer (if translate1) and THEN hand it over to the user.
        
        // if the device is translate1, then decode
        if (MINOR(dev->cdev.dev) == (MINOR_BEGINNING + 1)) {
            decode_char(dev->read_pos);
        }
        
        if (copy_to_user(buf, dev->read_pos, 1)) {
            return -EFAULT;
        }

        // update pointers
        dev->read_pos = dev->buffer + ((p_reader + 1) % translate_bufsize)* sizeof(char);
        p_reader = (dev->read_pos - dev->buffer) / sizeof(char);
        buf += sizeof(char);
	
	// update counters
        num_copied_items++;
        dev->items--;
    }
    
    return num_copied_items;
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
