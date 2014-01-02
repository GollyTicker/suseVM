#ifndef TRANSLATE_H_
#define TRANSLATE_H_

// aus dem scull-main
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>   	// printk()
#include <linux/slab.h>     	// kmalloc()
#include <linux/fs.h>       	// vieles...
#include <linux/proc_fs.h>  	/*fürs Ablegen im Ordner /proc*/
#include <linux/errno.h>    	// error codes
#include <linux/types.h>    	// size_t
#include <linux/fcntl.h>	    // file operations
#include <linux/cdev.h>     	// cdev
#include <linux/kdev_t.h>	    // dev_t
#include <asm/uaccess.h>	    // copy_from_user()

// defines
#define TRANSLATE_DEF_BUF_SIZE 40       /*standard buffergröße 40*/
#define TRANSLATE_FIRST_MINOR 0         /*minor anfang*/
#define TRANSLATE_NUMBER_OF_DEVS 2      /*anzahl devices*/
#define TRANSLATE_DEF_SUBST "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"    //standard subst
#define IS_LOWER_CASE(c) (c >= 'a' && c <= 'z')	/*prüfe kleinen buchstaben*/
#define IS_UPPER_CASE(c) (c >= 'A' && c <= 'Z') /*prüfe großen buchstaben*/
#define LOWER_OFFSET 97      /*entspricht offsetwert 97*/
#define UPPER_OFFSET 39	/*entspricht offsetwert 39*/
#define SUCCESS 0	/*result value 0 == success*/

// structure
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


// prototypes
//fileopeartions
int translate_open(struct inode *inode, struct file *filp);
int translate_release(struct inode *inode, struct file *filp);
ssize_t translate_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t translate_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
//module
static int translate_init(void);
static void translate_cleanup(void);
//devices
static void translate_setup_cdev(struct translate_dev *dev, int index);
//own
int getEncodedCharIndex(char c);
void encode(char *write_pos);
void decode(char *read_pos);


// Tag Initialisierungssyntax
struct file_operations translate_ops = {
	.owner = THIS_MODULE,
	.open =	translate_open,
	.release = translate_release,
	.write = translate_write,
	.read = translate_read, };


#endif
