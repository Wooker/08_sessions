#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "solution_node"
#define MAJOR_NUMBER 212
#define MAX_BUFFER_SIZE 255

struct session_data {
  char buffer[MAX_BUFFER_SIZE];
  int id;
};

static int counter = 0;

static int solution_open(struct inode *inode, struct file *file) {
  struct session_data *session_data =
      kzalloc(sizeof(struct session_data), GFP_KERNEL);

  printk(KERN_INFO "kernel_mooc Openening new session");

  if (!session_data) {
    printk(KERN_ALERT "Unable to allocate memory for session");
    return -ENOMEM;
  }

  session_data->id = counter++;
  snprintf(session_data->buffer, sizeof(session_data->id), "%d",
           session_data->id);

  file->private_data = session_data;

  printk(KERN_INFO "kernel_mooc %d Opened session", session_data->id);
  return 0;
}

static int solution_release(struct inode *inode, struct file *file) {
  struct session_data *session_data = file->private_data;

  printk(KERN_INFO "kernel_mooc %d Closing session", session_data->id);

  kfree(session_data);
  file->private_data = NULL;

  printk(KERN_INFO "kernel_mooc Closed session\n");
  return 0;
}

static ssize_t solution_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
  int res;
  size_t len;
  char session_id_str[2];
  size_t remaining_length;
  loff_t pos = *ppos;

  struct session_data *session_data = file->private_data;

  printk(KERN_INFO "kernel_mooc %d Reading, lbuf: %lu, ppos: %llu",
         session_data->id, lbuf, *ppos);

  len = strlen(session_data->buffer + *ppos);

  res = copy_to_user(buf, session_data->buffer + *ppos, len);
  if (res != 0) {
    printk(KERN_ALERT "kernel_mooc Failed to copy data to user buffer");
    return -EFAULT;
  }

  *ppos += len;

  pr_info("kernel_mooc %d Read \"%s\", len: %lu ppos: %llu", session_data->id,
          session_data->buffer + pos, len, *ppos);
  return len;
}

static ssize_t solution_write(struct file *file, const char __user *buf,
                              size_t lbuf, loff_t *ppos) {
  loff_t pos = *ppos;
  struct session_data *session_data = file->private_data;

  printk(KERN_INFO "kernel_mooc %d Writing, lbuf: %lu, ppos: %llu",
         session_data->id, lbuf, *ppos);

  if (*ppos + lbuf > MAX_BUFFER_SIZE) {
    printk(KERN_ALERT "kernel_mooc Data exceeds maximum buffer size");
    return -EINVAL;
  }

  if (copy_from_user(session_data->buffer + *ppos, buf, lbuf) != 0) {
    printk(KERN_ALERT "kernel_mooc Failed to copy data from user buffer\n");
    return -EFAULT;
  }

  *ppos += lbuf;

  printk(KERN_INFO "kernel_mooc %d Wrote \"%s\" (ppos: %llu)", session_data->id,
         session_data->buffer + pos, *ppos);

  return lbuf;
}

static loff_t solution_llseek(struct file *file, loff_t off, int whence) {
  struct session_data *session_data = file->private_data;

  loff_t new_pos;

  printk(KERN_INFO "kernel_mooc %d Seeking, off: %llu, wehnce: %d", session_data->id, off, whence);

  if (-off > file->f_pos || off >= MAX_BUFFER_SIZE) {
    return 0;
  }

  switch (whence) {
  case 0:
    printk(KERN_INFO "kernel_mooc SEEK_SET");
    new_pos = off;
    break;
  case 1:
    printk(KERN_INFO "kernel_mooc SEEK_CUR");
    new_pos = file->f_pos + off;
    break;
  case 2:
    printk(KERN_INFO "kernel_mooc SEEK_END");
    new_pos = strlen(session_data->buffer) + off;
    break;
  default:
    return -EINVAL;
  }

  if (new_pos < 0)
    return -EINVAL;

  file->f_pos = new_pos;
  printk(KERN_INFO "kernel_mooc f_pos is set to %llu", new_pos);

  return new_pos;
}

// Register operations
static struct file_operations solution_fops = {
    .owner = THIS_MODULE,
    .open = solution_open,
    .read = solution_read,
    .write = solution_write,
    .release = solution_release,
    .llseek = solution_llseek,
};

static int __init solution_init(void) {
  int result;

  // Register the character device driver
  result = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &solution_fops);
  if (result < 0) {
    printk(KERN_ALERT "kernel_mooc Failed to register solution module %d\n",
           result);
    return result;
  }

  printk(KERN_INFO "kernel_mooc Solution module is loaded\n");
  return 0;
}

static void __exit solution_exit(void) {
  // Unregister the character device driver
  unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);

  printk(KERN_INFO "kernel_mooc Solution module is unloaded\n");
}

module_init(solution_init);
module_exit(solution_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zakhar Semenov");
MODULE_DESCRIPTION("Solution");
