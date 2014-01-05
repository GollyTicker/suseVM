#ifndef TRANSLATE_H_
#define TRANSLATE_H_

// aus dem scull-main
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>       // printk()
#include <linux/slab.h>         // kmalloc()
#include <linux/fs.h>           // vieles...
#include <linux/errno.h>        // error codes
#include <linux/types.h>        // size_t
#include <linux/fcntl.h>	// file operations
#include <linux/cdev.h>         // cdev
#include <linux/kdev_t.h>	// dev_t
#include <asm/uaccess.h>	// copy_from_user()
#include <linux/string.h>       // strchr(), strlen()

// Translate DEFINE
// standardmaessig ist der Buffer 40
#define STD_BUFFER_SIZE 40

// der default translate substring
#define STD_TRANSLATE_SUBSTR "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"    //standard subst
#define TRANSLATE_SUBSTR_LEN (strlen(translate_subst))

// die minor nummern beginnen ab 0
#define MINOR_BEGINNING 0

// wir haben die devices translate0 und translate1
#define NO_OF_DEVICES 2

// helper macros
#define IS_LOWER_CASE(c) ((c) >= 'a' && (c) <= 'z')
#define IS_UPPER_CASE(c) ((c) >= 'A' && (c) <= 'Z')

// only one user may use a devices read/write 'port' at a time
#define NUM_SIMULT_ACCESS_USERS 1

// offset for lower case characters
#define LOWER_CASE_ASCII 'a'
#define UPPER_CASE_ASCII 'A'

// the encoding for lower case chars being at 0
// the encoding for upper case chars begins at the middle
#define LOWER_CASE_SUBSTR_OFFSET 0
#define UPPER_CASE_SUBSTR_OFFSET (TRANSLATE_SUBSTR_LEN/2)
#define IS_IN_LOWER_CASE_SUBSTR(A) ((A) >= LOWER_CASE_SUBSTR_OFFSET && (A) < UPPER_CASE_SUBSTR_OFFSET)

// misc
#define EXIT_SUCCESS 0
#define NEUTRAL_CHAR_INDEX -1

// for debugging
#ifdef DEBUG_MESSAGES
#define DEBUG(A) printk(KERN_NOTICE "<translate>:");A
#endif

#ifndef DEBUG_MESSAGES
#define DEBUG(A) 
#endif

// each of our devices have this
struct translate_dev {
	char *buffer;	// pointer to the buffer
	int items;	// # of items
	char *read_pos;	// current reading position
	char *write_pos;// current writing position
	struct cdev cdev;// char device
	struct semaphore writer_open_lock;	// mutex for writing
	struct semaphore reader_open_lock;	// mutex for reading
};


// Translate fileoperations (auch aus scull ueberneommen)
int translate_open(struct inode *inode, struct file *filp);
int translate_close(struct inode *inode, struct file *filp);
ssize_t translate_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t translate_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);


// Install/Uninstall (oriented on scull)
static int translate_init(void);
static void translate_cleanup(void);
static void translate_setup_cdev(struct translate_dev *dev, int index);
static void cleanup_single_translate_dev(int i);  // cleanup the device of the given index

// Echte Anwendungsfunktionen
int encode_index_from_char(char c);
void encode_char(char *write_pos);
char decode_from_index(int index);
void decode_char(char *read_pos);


// file_operations interface implementieren
struct file_operations translate_ops = {
    .owner = THIS_MODULE,
    .open  = translate_open,
    .release = translate_close,
    .write = translate_write,
    .read  = translate_read
};

// Module things
// Metainformation
MODULE_AUTHOR("Swaneet Sahoo and Ivan Morozov");
MODULE_LICENSE("Beer Licence");

// module init and module exit procedures taken from scull
module_init(translate_init);
module_exit(translate_cleanup);


#endif
