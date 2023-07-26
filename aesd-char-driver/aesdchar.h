/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 *    Modified: Salvador Z.
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#include "aesd-circular-buffer.h"

#define AESD_DEBUG 1 // Remove comment on this line to enable debug
#define MAX_BUFFER 1024

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

typedef struct aesd_dev {
  char  *kbuff;
  size_t kbuff_sz;
  struct cdev cdev; /* Char device structure      */
  struct mutex lock;
  aesd_cbuff_t cbuff;
} aesd_dev_t;

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
