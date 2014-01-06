#include "translate.h"

    //Parameter
static char *tl_subst = TL_DEFAULT_SUBSTR;
static int tl_buffersize = TL_BUFFERSIZE;

    //Modulparameter
module_param(tl_subst, charp, S_IRUGO);
module_param(tl_buffersize, int, S_IRUGO);

    //Erhaltene major number
int tl_majornumber;
    //Erhaltene devices
struct tl_device *devices;

    //Encoding des chars auf den der Pointer zeigt
void encode(char *plain)
{
    if (IS_NOT_TO_BE_ENCODED(*plain))
    {
        return;
    }
    *plain = tl_subst[CHIFFRE_INDEX(*plain)];
}
    //Decoding des chars auf den der Pointer zeigt
void decode(char *cipher)
{
    if (IS_NOT_TO_BE_ENCODED(*cipher))
    {
        return;
    }
    *cipher = CHAR_FOR_CHIFFRE_INDEX(strchr(tl_subst, *cipher));
}

    //Open operation aus scull
int tl_open(struct inode *inode, struct file *filp)
{
    struct tl_device *device;

    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_open\n"));

        //Erhalte einen Pointer auf das Device.
    device = container_of(inode->i_cdev, struct tl_device, cdev);
    filp->private_data = device;

        //Abhängig vom Modus des Users, dekrementiere den Wert der entsprechenden Semaphore.
    int result;
    if ((filp->f_mode & FMODE_WRITE) == FMODE_WRITE)
    {
        result = down_trylock(&device->openLockWriting);
    }
    else
    {
        result = down_trylock(&device->openLockReading);
    }

        //Falls nicht erfolgreich, gebe busy zurück.
    if (result!= 0)
    {
        DEBUG_LOG(printk(KERN_NOTICE "In tl_open: Device is already in use.\n"));
    }
    
    return EXIT_SUCCESS;
}

    //Release operation
int tl_release(struct inode *inode, struct file *filp)
{
    struct tl_device *device = filp->private_data;
    
    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_release\n"));

    //Abhängig vom Modus des Users, inkrementiere den Wert der entsprechenden Semaphore.
    if ((filp->f_mode & FMODE_WRITE) == FMODE_WRITE)
    {
        up(&device->openLockWriting);
    }
    else
    {
        up(&device->openLockReading);
    }
    return EXIT_SUCCESS;
}

    //Schreibzugriff
ssize_t tl_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
    struct tl_device *device = filp->private_data;

    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_write\n"));

        //Die Schreibposition im Buffer, in chars
    int writePosition = (device->writePointer - device->buffer) / sizeof(char);

        //Anzahl der bisher kopierten chars
    int alreadyCopied = 0;

        //Solange weniger chars kopiert sind, als gefordert:
    while (alreadyCopied < count)
    {
            //Dekrementiere die Semaphore für freien Buffer.
        if (down_trylock(&device->freeBuffer) != 0)
        {
                //Falls dies nicht geht, ist der Buffer voll. Gebe die Anzahl der kopierten chars zurück.
            DEBUG_LOG(printk(KERN_NOTICE "In tl_write: Buffer is full. Already copied: %d\n",alreadyCopied));
            return alreadyCopied;
        }

            //Kopiere einen char in den buffer. Da im kernel space, wird copy_from_user benötigt
        unsigned long result = copy_from_user(device->writePointer, buf, 1) != 0
        if (result != 0)
        {
                //Falls dies fehlschlägt, inkrementiere die Semaphore wieder und gebe fault zurück.
            DEBUG_LOG(printk(KERN_NOTICE "In tl_write: %d chars could not be copied.\n", result));
            up(&device->freeBuffer);
                return -EFAULT;
        }

            //Als Device 0, encode beim Schreiben.
        if (MINOR(device->cdev.dev) == MINORNUMBER_MIN)
        {
            encode(device->writePointer);
        }
            //Inkrementiere die Position im Buffer (Ring).
        device->writePointer = device->buffer + ((writePosition + 1) % tl_buffersize) * sizeof(char);
            //Erhalte die neue Schreibposition
        writePosition = (device->writePointer - device->buffer) / sizeof(char);
            //TODO
        buf += sizeof(char);

        alreadyCopied++;
        device->bufferContent++;
	
            //Inkrementiere die Semaphore für den Bufferinhalt
        up(&device->filledBuffer);
    }

    return alreadyCopied;
}

    //Lesezugriff
ssize_t tl_read(struct file *filp, char __user *buf,
		       size_t count,loff_t *f_pos)
{
    struct tl_device *device = filp->private_data;

    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_read\n"));

        //Die Leseposition im Buffer, in chars
    int readPosition = (device->readPointer - device->buffer) / sizeof(char);

        //Anzahl der bisher kopierten chars
    int alreadyCopied = 0;

        //Solange weniger chars kopiert sind, als gefordert:
    while (alreadyCopied < count)
    {
            //Dekrementiere die Semaphore für chars im Buffer.
        if (down_trylock(&device->filledBuffer) != 0)
        {
                //Falls dies nicht geht, ist der Buffer leer. Gebe die Anzahl der kopierten chars zurück.
            DEBUG_LOG(printk(KERN_NOTICE "In tl_read: Buffer is empty. Already copied: %d\n",alreadyCopied));
            return alreadyCopied;
        }

            //Als Device 1, decode beim Lesen.
        if (MINOR(device->cdev.dev) == (MINORNUMBER_MIN + 1))
        {
            decode(dev->readPointer);
        }

            //Kopiere einen char aus dem buffer. Da aus kernel space, wird copy_to_user benötigt
        unsigned long result = copy_to_user(buf, device->readPointer, 1)
        if (result != 0)
        {
                //Falls dies fehlschlägt, inkrementiere die Semaphore wieder und gebe fault zurück.
            DEBUG_LOG(printk(KERN_NOTICE "In tl_write: %d chars could not be copied.\n", result));
            up(&device->filledBuffer);
            return -EFAULT;
        }

            //Inkrementiere die Position im Buffer (Ring).
        device->readPointer = device->buffer + ((readPosition + 1) % tl_buffersize) * sizeof(char);
            //Erhalte die neue Leseposition.
        readPosition = (dev->readPointer - dev->buffer) / sizeof(char);
        buf += sizeof(char);

        alreadyCopied++;
        dev->bufferContent--;

            //Inkrementiere die Semaphore für freien Buffer.
        up(&device->freeBuffer);
    }
    
    return alreadyCopied;
}

    //Wird vom Kernel zum Initialisieren des Moduls aufgerufen.
static int tl_init(void)
{
    int result, i;
    dev_t dev;
    
    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_init\n"));
    
    result = alloc_chrdev_region(&dev, MINORNUMBER_MIN, DEVICE_COUNT,"translate\n");
    tl_majornumber = MAJOR(dev);

    if (result != EXIT_SUCCESS)
    {
        DEBUG_LOG(printk(KERN_ALERT "In tl_init: Can't get major number %d \n",
		result, tl_majornumber));
        return result;
    }

        //Allokiere Speicher für die devices.
    devices = kmalloc(DEVICE_COUNT * sizeof(struct tl_device),GFP_KERNEL);
    if (!devices)
    {
        result = -ENOMEM;
            //We refused to make this more graceful
        goto fail;
    }
    memset(devices, 0, DEVICE_COUNT * sizeof(struct tl_device));

        //Initialisieren der Devices
    for (i = 0; i < DEVICE_COUNT; i++)
    {
        struct tl_device *dev = &devices[i];
            //Allokiere Speicher für die Buffer der Devices.
        dev->buffer = kmalloc(translate_bufsize, GFP_KERNEL);
        if (!(dev->buffer))
        {
            result = -ENOMEM;
            goto fail;
        }

            //Am Anfang ist der Buffer leer, Lese- und Schreibposition sind am Anfang
        dev->bufferContent = 0;
        dev->readPointer = dev->buffer;
        dev->writePointer = dev->buffer;

            //Initialisierung der Semaphoren
        sema_init(&dev->filledBuffer, 0);
        sema_init(&dev->freeBuffer, tl_buffersize);
        sema_init(&dev->openLockReading, 1);
        sema_init(&dev->openLockWriting, 1);
	
        tl_setup_cdev(&devices[i], i);
        DEBUG_LOG(printk(KERN_NOTICE "In tl_init: Device %d was initialized", i));
    }
    
    DEBUG_LOG(printk(KERN_NOTICE "In tl_init: All devices were initialized"));
    
    return EXIT_SUCCESS;

    fail: tl_cleanup();
    return result;
}

    //Setup der char_dev structure aus scull
static void tl_setup_cdev(struct tl_device *dev, int index)
{
    int result;

    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_setup_cdev\n"));

    int devno = MKDEV(tl_majornumber, MINORNUMBER_MIN + index);
    
    cdev_init(&dev->cdev, &tl_operations);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &tl_operations;
    
    result = cdev_add(&dev->cdev, devno, 1);

    if (result)
    {
            //Even more graceful
        DEBUG_LOG(printk(KERN_NOTICE "In tl_setup_cdev: (Error %d) Couldn't add device %d\n", result, index));
    }
}

    //Cleanup für ein einzelnes Device
static void tl_cleanup_device(int index)
{
    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_cleanup_device\n"));

    struct tl_device *device = &(devices[index]);
    kfree(device->buffer);
    cdev_del(&device->cdev);
    DEBUG_LOG(printk(KERN_NOTICE "In tl_cleanup_device: Cleaned up device %d\n", index));
}

    //Cleanup aus scull. Wird auch bei Initialisierungsfehlern aufgerufen
static void tl_cleanup(void)
{
    int i;

    DEBUG_LOG(printk(KERN_NOTICE "Entering tl_cleanup\n"));

    dev_t devno = MKDEV(tl_majornumber, MINORNUMBER_MIN);

    if (devices)
    {
        for (i = 0; i < DEVICE_COUNT; i++)
        {
            tl_cleanup_device(i);
        }
        kfree(devices);
    }
        //Wenn tl_cleanup gecalled wurde, war das Registrieren schon erfolgreich, unregistriere.
    unregister_chrdev_region(dev, DEVICE_COUNT);
}
