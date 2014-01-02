#ifndef TRANSLATE_H_
#define TRANSLATE_H_

// aus dem scull-main
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

// weiter notwenidige incudes
#include <linux/kernel.h>       // printk()
#include <linux/slab.h>         // kmalloc()
#include <linux/fs.h>           // vieles...
#include <linux/errno.h>        // error codes
#include <linux/types.h>        // size_t
#include <linux/fcntl.h>	// file operations
#include <linux/cdev.h>         // cdev
#include <linux/kdev_t.h>	// dev_t
#include <asm/uaccess.h>	// copy_from_user()

// Translate DEFINE
// standardmaessig ist der Buffer 40
#define STD_BUFFER_SIZE 40

// der default translate substring
#define STD_TRANSLATE_SUBSTR "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"    //standard subst

// die minor nummern beginnen ab 0
#define MINOR_BEGINNING 0

// wir haben die devices translate0 und translate1
#define NO_OF_DEVICES 2

// helper macros
#define IS_LOWER_CASE(c) (c >= 'a' && c <= 'z')
#define IS_UPPER_CASE(c) (c >= 'A' && c <= 'Z')

// offset for lower case characters
#define LOWER_CASE_OFFSET 97
#define UPPER_OFFSET 39	/*entspricht offsetwert 39*/
#define SUCCESS 0	/*result value 0 == success*/

// unser Device
struct translate_dev {
	char *buffer;
	int items;
	char *read_pos;
	char *write_pos;
	struct cdev cdev;
	struct semaphore writer_open_lock;
	struct semaphore reader_open_lock;
	struct semaphore itemsInBuffer;
	struct semaphore freeBufferSpace;
};


// Translate fileoperations
int translate_open(struct inode *inode, struct file *filp);
int translate_release(struct inode *inode, struct file *filp);
ssize_t translate_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t translate_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);


// Install/Uninstall
static int translate_init(void);
static void translate_cleanup(void);
static void translate_setup_cdev(struct translate_dev *dev, int index);

// Echte Anwendungsfunktionen
int getEncodedCharIndex(char c);
void encode(char *write_pos);
void decode(char *read_pos);


// file_operations interface implementieren
struct file_operations translate_ops = {
    .owner = THIS_MODULE,
    .open  = translate_open,
    .release = translate_release,
    .write = translate_write,
    .read  = translate_read
};


#endif
