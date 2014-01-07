#ifndef TRANSLATE_H_
#define TRANSLATE_H_

    //Die folgenden zwei Absätze sind aus sculls main.c übernommen
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

    //Für verschiedene Hilfsfunktionen
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#include <linux/string.h>

    //Die default-Einstellungen
#define TL_BUFFERSIZE 40
#define TL_DEFAULT_SUBST
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

    //Es gibt zwei Devices, minor number startet bei 0
#define DEVICE_COUNT 2
#define MINORNUMBER_MIN 0

    //Helfermakros für Ver- und Entschlüsseln
#define LOWERCASE_START 'a'
#define LOWERCASE_END 'z'
#define LOWERCASE_LENGTH (LOWERCASE_END - LOWERCASE_START + 1)
#define UPPERCASE_START 'A'
#define UPPERCASE_END 'Z'
#define LOWERCASE(C) ((C) >= LOWERCASE_START && (C) <= LOWERCASE_END)
#define UPPERCASE(C) ((C) >= LOWERCASE_START && (C) <= UPPERCASE_END)
#define CHIFFRE_INDEX(C) ((LOWERCASE(C)) ? ((C) - LOWERCASE_START) : ((C) - UPPERCASE_START + LOWERCASE_LENGTH))
#define CHAR_FOR_CHIFFRE_INDEX(C) (((C) < LOWERCASE_LENGTH) ? ((C) + LOWERCASE_START) : ((C) - LOWERCASE_LENGTH + UPPERCASE_START))
#define IS_NOT_TO_BE_ENCODED(C) (!(LOWERCASE(C)) && !(UPPERCASE(C))

    //Debugausgaben mit Präfix
#ifdef DEBUG_MESSAGES
#define DEBUG_LOG(E) printk(KERN_NOTICE "In translate: ");E
#else
#define DEBUG_LOG(E) ()
#endif

    //Struct für ein Device
struct tl_device {
        //Position des Buffers
	char *buffer;
	int bufferContent;
    struct semaphore freeBuffer;
	struct semaphore filledBuffer;
    struct semaphore openLockWriting;
	struct semaphore openLockReading;
	char *readPointer;
    char *writePointer;
	struct cdev cdev;
};

    //Ver- und Entschlüsseln
void encode(char *plain);
void decode(char *cipher);

    //Dateioperationen aus scull
int tl_open(struct inode *inode, struct file *filp);
int tl_release(struct inode *inode, struct file *filp);
ssize_t tl_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t tl_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);

    //Setup, modifiziert aus scull
static int tl_init(void);
static void tl_cleanup(void);
static void tl_cleanup_device(int i);
static void tl_setup_cdev(struct translate_dev *dev, int index);

    //Interface für Dateioperationen
struct file_operations tl_operations = {
    .owner = THIS_MODULE,
    .open  = tl_open,
    .release = tl_release,
    .write = tl_write,
    .read  = tl_read
};

    //Modulsetup aus scull
module_init(tl_init);
module_exit(tl_cleanup);

#endif
