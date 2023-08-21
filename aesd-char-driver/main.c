/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h> // mem alloc
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Salvador Z");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev g_aesd_device;

int aesd_open(struct inode *inode, struct file *filp) {
  aesd_dev_t *dev;

  if (!inode)
    return -EACCES;
  
  dev = container_of(inode->i_cdev, aesd_dev_t, cdev);
  filp->private_data = dev;

  PDEBUG("open");
  return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{

    PDEBUG("release");

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
  ssize_t retval = 0;
  struct aesd_buffer_entry *rd_entry;
  size_t rd_entry_offset;
  char *k_rdbuff = NULL; //stores the commands retireved from the entries
  u8 it;
  u8 n_elements;
  u64 ret_copy = 0;
  u64 bytes_to_copy = 0;
  u64 bytes_in_entry;

  struct aesd_dev *dev = filp->private_data;

  // lock mutex before doing anything
  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;


  n_elements = aesd_buffer_size(&dev->cbuff);
  PDEBUG("read %zu bytes offset %ld elements:%d", count, *f_pos, n_elements);
  if (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED < n_elements)
    n_elements = AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

  // get the node from the buffer based on the given position
  for (it = 0; it < n_elements; it++) {
    rd_entry = aesd_circular_buffer_find_entry_offset_for_fpos(
        &dev->cbuff, *f_pos, &rd_entry_offset);

    if (NULL == rd_entry) {
      if (k_rdbuff) { // buffer has nothing else
        //PDEBUG("No more elements present on the buffer, leaving the iter");
        break;
      } else {
        PDEBUG("Cbuf empty");
        *f_pos = 0;
        goto release;
      }
    }

    bytes_in_entry = rd_entry->size - rd_entry_offset;
    if ((count <= bytes_in_entry) && (!k_rdbuff)) {
      bytes_to_copy = count;
      *f_pos += count;
      // now copy to user
      PDEBUG("Case1. offset: %ld count:%ld same as bytes_to_give:%zd",*f_pos, count, bytes_to_copy);
      break;
    }
    // if asks for more than the current entry
    else if ((NULL == k_rdbuff) && (count > bytes_in_entry)) {
      size_t ammount = (count > MAX_BUFFER) ? MAX_BUFFER : count;
      //PDEBUG("allocating %zu bytes in kbuff", ammount);
      k_rdbuff = kzalloc(ammount * sizeof(char), GFP_KERNEL);
      if (!k_rdbuff) {
        *f_pos = 0;
        PDEBUG("Error alloc");
        goto release;
      }
      memcpy(k_rdbuff, &rd_entry->buffptr[rd_entry_offset], bytes_in_entry);
      *f_pos += bytes_in_entry;
      count -= bytes_in_entry;
      bytes_to_copy = bytes_in_entry;
      PDEBUG("C2. offset: %ld, count:%ld, bytes_to_give:%zd and entry %ld",
             *f_pos, count, bytes_to_copy, rd_entry_offset);
      continue;
    }
    // keep adding to the buffer
    else if (k_rdbuff && (count > bytes_in_entry)) {
      strncat(k_rdbuff, &rd_entry->buffptr[rd_entry_offset], bytes_in_entry);
      count  -= bytes_in_entry;
      *f_pos += bytes_in_entry;
      bytes_to_copy += bytes_in_entry;
      PDEBUG("C3. offset: %ld, count:%ld, bytes_to_give:%zd rd_ofsset:%ld", *f_pos, count,
             bytes_to_copy, rd_entry_offset);
      continue;
    }
    // count is less than entry and/or buffer will be full
    else {

      PDEBUG("C4");
      strncat(k_rdbuff, &rd_entry->buffptr[rd_entry_offset], count);
      *f_pos += count;
      bytes_to_copy += count;
      break;
    }
  }

  if (bytes_to_copy && k_rdbuff) {
    PDEBUG("1 Sending %zd bytes and offset:%ld with:%s", bytes_to_copy, *f_pos, k_rdbuff);
    ret_copy = copy_to_user(buf, k_rdbuff, bytes_to_copy);
    kfree(k_rdbuff);
  } else if (bytes_to_copy && rd_entry) {
    PDEBUG("2 Sending %zd bytes and offset:%ld with:%s", bytes_to_copy, *f_pos,
           &rd_entry->buffptr[rd_entry_offset]);
    ret_copy = copy_to_user(buf, &rd_entry->buffptr[rd_entry_offset], bytes_to_copy);
  }

  if (ret_copy) {
    PDEBUG("fault");
    retval = -EFAULT;
    goto release;
  }

  // update return value to number of bytes read
  retval = bytes_to_copy;

// release mutex, always
release:
  mutex_unlock(&dev->lock);

  return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
  ssize_t retval = -ENOMEM;
  size_t bytes_not_copied = 0;
  bool nl_found = false;
  struct aesd_buffer_entry wr_entry;

  aesd_dev_t * const dev = filp->private_data;

  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;

  // attempt to allocate enough buffer space for data, if the last command
  // didn't finish, get more memory
  if (dev->kbuff) {
    char *holding_old_ref = dev->kbuff;
    dev->kbuff = kzalloc(dev->kbuff_sz + count + 1, GFP_KERNEL);
    if (dev->kbuff) {
      memcpy(dev->kbuff, holding_old_ref, dev->kbuff_sz);
      kfree(holding_old_ref);
    }
  }
  // empty kbuff
  else {
    dev->kbuff = kzalloc(dev->kbuff_sz + count + 1, GFP_KERNEL);
  }

  if (!dev->kbuff) {
    PDEBUG("Failure to allocate enough memory in aesd_write");
    retval = -ENOMEM;
    goto release_mutex;
  }

  // copy data to kernel space
  PDEBUG("Got %d bytes, size %d", count, dev->kbuff_sz);
  // Not required, only to habe better debug for the %s printing
  dev->kbuff[dev->kbuff_sz + count] = '\0';
  bytes_not_copied = copy_from_user(&dev->kbuff[dev->kbuff_sz], buf, count);
  dev->kbuff_sz += count;
  if (0 < bytes_not_copied) {
    // failed to copy all of the user buffer
    PDEBUG("Failure to copy %ld bytes from user in aesd_write",
           bytes_not_copied);
    retval = -EFAULT;
    goto free_mem;
  }

  retval = count;

  // check the the entry for a newline
  if ('\n' == dev->kbuff[dev->kbuff_sz -1]) {
    nl_found = true;
  }

  // add entry to circular buffer if the newline was found, otherwise release
  // mutex and move on
  if (nl_found) {
    PDEBUG("Adding entry to circular buffer, length %zu", dev->kbuff_sz);
    // if full, then free the oldest prior to override
    if (dev->cbuff.full) {
      PDEBUG("Buffer was full, freeing oldest entry");
      kfree(dev->cbuff.entry[dev->cbuff.out_offs].buffptr);
    }
    wr_entry.buffptr = dev->kbuff;
    wr_entry.size = dev->kbuff_sz;
    aesd_circular_buffer_add_entry(&dev->cbuff, &wr_entry);

  } else
    goto release_mutex;
  
  goto release_ptr;

// free any allocated memory, in error cases
free_mem:
  kfree(dev->kbuff);
release_ptr:
  dev->kbuff = NULL; // avoid wild pointer
  dev->kbuff_sz = 0;

// release mutex, always
release_mutex:
  PDEBUG("wrote %zu bytes with offset %lld, elements written %d with %s", retval, *f_pos, aesd_buffer_size(&dev->cbuff), wr_entry.buffptr);
  mutex_unlock(&dev->lock);
  /**
   * TODO: handle write
   */
  return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence) {
  loff_t cbuf_length;
  loff_t retval = -EINVAL;
  struct aesd_dev *dev = filp->private_data;

  PDEBUG("seek to offset %lld, whence %d", offset, whence);

  // lock mutex
  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;

  cbuf_length = aesd_circular_buffer_data_length(&dev->cbuff);

  // use fixed size llseek
  retval = fixed_size_llseek(filp, offset, whence, cbuf_length);
  PDEBUG("fixed size llseek returned: %lld with data len %lld", retval, cbuf_length);

release_mutex:
  mutex_unlock(&dev->lock);

  return retval;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
  long retval = 0;

  switch (cmd) {
  case AESDCHAR_IOCSEEKTO: {
    struct aesd_seekto seekto;
    if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) {
      retval = -EFAULT;
    } else {
      struct aesd_dev *dev = filp->private_data;

      if (!aesd_circular_buffer_is_valid_seekto(&dev->cbuff, seekto.write_cmd,
                                                seekto.write_cmd_offset))
        return -EINVAL;
      filp->f_pos = aesd_circular_buffer_base_offset(
          &dev->cbuff, seekto.write_cmd, seekto.write_cmd_offset);
      PDEBUG("ioctl adjusted f_pos %ld based on cmd %d with offset %d",
             filp->f_pos, seekto.write_cmd, seekto.write_cmd_offset);
      if (0 == filp->f_pos && (seekto.write_cmd || seekto.write_cmd_offset))
        return -EFAULT;
    }
    break;
  }

  default: {
    // cmd not implemented of do no belong to this driver
    retval = -ENOTTY;
    break;
  }
  } /*end of switch*/
  return retval;
}
struct file_operations aesd_fops = {
    .owner          = THIS_MODULE,
    .read           = aesd_read,
    .write          = aesd_write,
    .open           = aesd_open,
    .release        = aesd_release,
    .llseek         = aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&g_aesd_device,0,sizeof(struct aesd_dev));

    aesd_circular_buffer_init(&g_aesd_device.cbuff);
    mutex_init(&g_aesd_device.lock);
    g_aesd_device.kbuff = NULL;
    g_aesd_device.kbuff_sz = 0;

    result = aesd_setup_cdev(&g_aesd_device);
    PDEBUG("Init with major %d\n", aesd_major);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void) {
  u8 idx = 0;
  struct aesd_buffer_entry *entry;

  dev_t devno = MKDEV(aesd_major, aesd_minor);

  cdev_del(&g_aesd_device.cdev);

  PDEBUG("cleaned");

  // free memory used by circular buffer
  AESD_CIRCULAR_BUFFER_FOREACH(entry, &g_aesd_device.cbuff, idx) {
    if (entry->buffptr)
      kfree(entry->buffptr);
  }

  if (g_aesd_device.kbuff)
    kfree(g_aesd_device.kbuff);

  unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
