#ifndef TRANSLATE_H_
#define TRANSLATE_H_

// includes
#include <linux/init.h>     	/*für module_init module_exit*/
#include <linux/module.h>	/*muss in jedes modul*/
#include <linux/kernel.h>   	/*kernelfunktionen*/
#include <linux/slab.h>     	/*im kernel speicher allozieren*/
#include <linux/fs.h>       	/*major beziehen , fileOps,file ...*/
#include <linux/proc_fs.h>  	/*fürs Ablegen im Ordner /proc*/
#include <linux/errno.h>    	/*errorcodes zurückgeben*/
#include <linux/types.h>    	/* size_t */
#include <linux/fcntl.h>	/*fileOPs f_mode*/
#include <linux/cdev.h>    	/*für das struct cdev, was die minor praesentiert*/
#include <linux/kdev_t.h>	/*für typ dev_t*/
#include <asm/uaccess.h>	/*copy_from_user*/

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
