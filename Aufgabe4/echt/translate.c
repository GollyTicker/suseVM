#include "translate.h"

/**
 * globale variablen
 */
int translate_major; /*erhaltene major nummer*/
struct translate_dev *translate_devs; /*devices pointer*/


/*parameter*/
static char *translate_subst = TRANSLATE_DEF_SUBST;
static int translate_bufsize = TRANSLATE_DEF_BUF_SIZE;

/*definition der modulparameter*/
module_param(translate_subst, charp , S_IRUGO|S_IWUSR);
module_param(translate_bufsize, int, S_IRUGO|S_IWUSR);

/*beschreibung der modulparameter*/
MODULE_PARM_DESC(translate_bufsize, "buffersize, def 40 \n");
MODULE_PARM_DESC(translate_subst, "subst, def 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz' \n"); //Standard subst

/* Metainformation, können mit modinfo aufgerufen werden*/
MODULE_AUTHOR("Asmatullah Noor & Maschhood Ahmad");
MODULE_DESCRIPTION("replace character");
MODULE_SUPPORTED_DEVICE("none");
MODULE_LICENSE("GPL");

/*Dieses Makro sorgt dafür, dass im Fall eines Modultreibers der frei gewählte Funktionsname der Initialisierungsfunktion in den Namen init_module umgesetzt wird.*/
module_init( translate_init);
/*Dieses Makro sorgt dafür, dass im Fall eines Modultreibers der frei gewählte Funktionsname der Deinitialisierungsfunktion in den Namen cleanup_module umgesetzt wird.*/
module_exit( translate_cleanup);


void encode(char *write_pos) {
	int index=0;

	index = getEncodedCharIndex(*write_pos);
	if (index >= 0 && index < strlen(translate_subst)) {
		*write_pos = translate_subst[index];
	}
}


void decode(char *read_pos) {
	char * pch = NULL;
	int index=0;

	pch = strchr(translate_subst, *read_pos);
	if (pch != NULL) {
		index = pch - translate_subst;
		if (('a' + index) > 'z') {
			*read_pos = '\'' + index;
		} else {
			*read_pos = 'a' + index;
		}
	}
}


int getEncodedCharIndex(char c) {
	int result = -1;

	if (IS_UPPER_CASE(c)) {
		result = c - UPPER_OFFSET;
	} else if (IS_LOWER_CASE(c)) {
		result = c - LOWER_OFFSET;
	}
	return result;
}

//fileoperation method for tag "open"
int translate_open(struct inode *inode, struct file *filp) {
	int result = SUCCESS; 
    /*The container_of macro takes the inode->i_cdev pointer to a field of type cdev,
    within a structure of type translate_dev,
    and returns a pointer to the translate_dev structure.*/
	struct translate_dev *dev = container_of(inode->i_cdev, struct translate_dev, cdev);
	filp->private_data = dev;

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_open called --- \n");
	#endif

	try_module_get(THIS_MODULE);/*increment that module's usage count*/
	if ((filp->f_mode & FMODE_WRITE) == FMODE_WRITE) {
        /*try to decrement the value of the semaphore and get write access*/
		if (down_trylock(&dev->writer_open_lock) != 0) {

			#ifdef DEBUG_MESSAGES
			printk(KERN_NOTICE "translate_open: sending -EBUSY on write request \n");
			#endif

			module_put(THIS_MODULE);
			result = -EBUSY;
		} else {

			#ifdef DEBUG_MESSAGES
			printk(KERN_NOTICE "translate_open: write access OK \n");
			#endif
		}
	} else {
        /*try to decrement the value of the semaphore and get read access*/
		if (down_trylock(&dev->reader_open_lock) != 0) {

			#ifdef DEBUG_MESSAGES
			printk(KERN_NOTICE "translate_open: sending -EBUSY on read request \n");
			#endif

			module_put(THIS_MODULE);
			result = -EBUSY;
		} else {

			#ifdef DEBUG_MESSAGES
			printk(KERN_NOTICE "translate_open: read access OK \n");
			#endif

		}
	}

	return result;
}

//fileoperation method for tag "release"
int translate_release(struct inode *inode, struct file *filp) {
	struct translate_dev *dev = filp->private_data;
	int result = SUCCESS;

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_release called ---\n");
	#endif

	if ((filp->f_mode & FMODE_WRITE) == FMODE_WRITE) {
        /*increase the value of the semaphore to unlock the monitor for writer*/
		up(&dev->writer_open_lock);

		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_release: another writer may enter now \n");
		#endif

	} else {
        /*increase the value of the semaphore to unlock the monitor for reader*/
		up(&dev->reader_open_lock);

		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_release: another reader may enter now \n");
		#endif
	}

	module_put(THIS_MODULE);

	return result;
}

// fileoperation method for tag "write"
ssize_t translate_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    /*allocate an fill the struct to fileoperations*/
	struct translate_dev *dev = filp->private_data;
	int writePointerIndex = (dev->write_pos - dev->buffer) / sizeof(char);
	
	
	
	ssize_t result = 0;
	int itemsCopied = 0;

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_write called ---\n");
	#endif
	
	
	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_write: writePointerIndex= %d \n",writePointerIndex);
	#endif
	
	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_write: count= %d \n",count);
	#endif
	
	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_write: param buffer= %s \n",buf);
	#endif

	while (itemsCopied < count) {
		if (itemsCopied == 0) {
            /*decrement the value of the semaphore freeBufferSpace, is interruptable*/
			if (down_interruptible(&dev->freeBufferSpace)) {

				#ifdef DEBUG_MESSAGES
				printk(KERN_NOTICE "translate_write: down_interruptible failed. sending -ERESTARTSYS \n");
				#endif

				result = -ERESTARTSYS;/*wenn die Schleife durch ein Signal unterbrochen wird*/
				goto out;
			}
		} else {
            /*try to decrement the value of the semaphore freeBufferSpace*/
			if (down_trylock(&dev->freeBufferSpace) != 0) {

				#ifdef DEBUG_MESSAGES
				printk(KERN_NOTICE "translate_write: buffer full, return number of copied chars: %d \n",itemsCopied);
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
				printk(KERN_NOTICE "translate_write: copy_from_user failed! already copied items \n");
				#endif

				result = itemsCopied;
			}else {/* coulnt wrote, cant writing, throw fault*/

				#ifdef DEBUG_MESSAGES
				printk(KERN_NOTICE "translate_write: copy_from_user failed: sending -EFAULT (BAD ADDRESS) \n");
				#endif

				result = -EFAULT;/*BAD ADDRESS*/
			}
			/* increment the value of the semaphore of freeBufferSpace*/
			up(&dev->freeBufferSpace);
			goto out;
		}
		
		
		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_write: user buffer befor encode= %s \n",buf);
		#endif
		
		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_write: device%d buffer befor encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
		#endif
		
		/*if devices is translate0 then encode*/
		if (MINOR(dev->cdev.dev)== 0) {
			encode(dev->write_pos);
		}

		
		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_write: user buffer after encode= %s \n",buf);
		#endif
		
		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_write: device%d buffer after encode= %s \n",MINOR(dev->cdev.dev),dev->buffer);
		#endif
		
		itemsCopied++;
		dev->write_pos = dev->buffer + ((writePointerIndex + 1) % translate_bufsize)* sizeof(char);
		writePointerIndex = (dev->write_pos - dev->buffer) / sizeof(char);
		dev->items++;
		buf += sizeof(char);
                /* increment the value of the semaphore itemsInBuffer*/
		up(&dev->itemsInBuffer);

		#ifdef DEBUG_MESSAGES
		printk(KERN_NOTICE "translate_write: \n\t free=%d \n\t used=%d \n\t items=%d \n\t copiedItems=%d \n",dev->freeBufferSpace.count, dev->itemsInBuffer.count,dev->items, itemsCopied);
		#endif

	}

	result = itemsCopied;
	out:

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_write: \n\t return=%d \n\t free=%d \n\t used=%d \n\t items=%d \n",result,dev->freeBufferSpace.count, dev->itemsInBuffer.count, dev->items);
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

//regestriert jede vom Modul bereitgestellte Fähigkeiten
static int translate_init(void) {
	int result;
	dev_t dev;
	int i;

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_init called ---\n");
	#endif

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_init: subst = %s \n",translate_subst);
	#endif

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "translate_init: bufsize = %d \n",translate_bufsize);
	#endif

    //Majonummer holen
	result = alloc_chrdev_region(&dev, TRANSLATE_FIRST_MINOR, TRANSLATE_NUMBER_OF_DEVS,"translate\n");
	translate_major = MAJOR(dev);

	if (result < 0) {

		#ifdef DEBUG_MESSAGES
		printk(KERN_ALERT "translate: can`t get major %d \n",translate_major);
		#endif

                return result;
	}

	/*allocate memory for both structs an get the pointer*/
	translate_devs = kmalloc(TRANSLATE_NUMBER_OF_DEVS * sizeof(struct translate_dev),GFP_KERNEL);
	if (!translate_devs) {
		result = -ENOMEM;
	goto fail;
	}

	/*overwrite the device memory with 0*/
	memset(translate_devs, 0, TRANSLATE_NUMBER_OF_DEVS * sizeof(struct translate_dev));

	/*init the member of both devices (translate0,translate1)*/
	for (i = 0; i < TRANSLATE_NUMBER_OF_DEVS; i++) {

		/*init the semaphores*/
		sema_init(&translate_devs[i].reader_open_lock, 1);
		sema_init(&translate_devs[i].writer_open_lock, 1);
		sema_init(&translate_devs[i].itemsInBuffer, 0);
		sema_init(&translate_devs[i].freeBufferSpace, translate_bufsize);

		/*allocate memory for the buffer an set the pointer*/
		translate_devs[i].buffer = kmalloc(translate_bufsize, GFP_KERNEL);
		if (!translate_devs[i].buffer) {
			result = -ENOMEM;
			goto fail;
		}

		/*init the other member of the struct*/
		translate_devs[i].items = 0;
		translate_devs[i].read_pos = translate_devs[i].buffer;
		translate_devs[i].write_pos = translate_devs[i].buffer;
		translate_setup_cdev(&translate_devs[i], i);
	}

	return 0;

	fail: translate_cleanup();
	return result;
}

//setup method for character devices from skull driver
static void translate_setup_cdev(struct translate_dev *dev, int index) {
	int result;
    /*set the devicenumber*/
	int devno = MKDEV(translate_major, TRANSLATE_FIRST_MINOR + index);

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_setup_cdev called --- \n");
	#endif
    /*init the characterdevice*/
	cdev_init(&dev->cdev, &translate_ops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &translate_ops;
    /*add the characterdevice*/
	result = cdev_add(&dev->cdev, devno, 1);

	if (result) {

		#ifdef DEBUG_MESSAGES
		printk (KERN_NOTICE "Error %d adding translatedev%d \n", result, index);
		#endif

	}
}

// Jedes Modul desregistrieren, das das Modul vorher registriert hat
static void translate_cleanup(void) {
	int i;
	dev_t dev;

	#ifdef DEBUG_MESSAGES
	printk(KERN_NOTICE "--- translate_cleanup called ---\n");
	#endif

	if (translate_devs != NULL) {
		for (i = 0; i < TRANSLATE_NUMBER_OF_DEVS; i++) {
            //lese und schreiben pointer auf NULL setzten
			translate_devs[i].read_pos = NULL;
			translate_devs[i].write_pos = NULL;
            //Buffer speciher freigeben
			kfree(translate_devs[i].buffer);

			#ifdef DEBUG_MESSAGES
			printk(KERN_NOTICE "translate_cleanup: kfree buffer %d OK \n",i);
			#endif
            //Buffer pointer auf NULL setzten
			translate_devs[i].buffer = NULL;
            //Character devices löschen
			cdev_del(&translate_devs[i].cdev);
		}
        //Speicher freigeben
		kfree(translate_devs);

		#ifdef DEBUG_MESSAGES
		printk (KERN_NOTICE "translate_cleanup: kfree translate_devs OK \n");
		#endif

	}
    //deregistrieren des Moduls
	dev = MKDEV(translate_major, TRANSLATE_FIRST_MINOR);
	unregister_chrdev_region(dev, TRANSLATE_NUMBER_OF_DEVS);
}


