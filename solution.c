#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "solution_node"
#define MAJOR_NUMBER 212
#define MAX_BUFFER_SIZE 255

struct session_data {
  char buffer[MAX_BUFFER_SIZE];
  loff_t position;
  int id;
};

static int counter = 0;

static int solution_open(struct inode *inode, struct file *file) {
  struct session_data *session =
      kmalloc(sizeof(struct session_data), GFP_KERNEL);

  printk(KERN_INFO "kernel_mooc Openening new session\n");

  if (!session) {
    printk(KERN_ALERT "Unable to allocate memory for session\n");
    return -ENOMEM;
  }

  session->position = 0;
  session->id = counter++;
  file->private_data = session;

  printk(KERN_INFO "kernel_mooc Opened session %d\n", session->id);

  return 0;
}

static int solution_release(struct inode *inode, struct file *file) {
  struct session_data *session = file->private_data;

  printk(KERN_INFO "kernel_mooc Closing session %d\n", session->id);

  kfree(session);
  file->private_data = NULL;

  printk(KERN_INFO "kernel_mooc Closed session\n");

  return 0;
}

static bool read = false;
static ssize_t solution_read(struct file *file, char __user *buf, size_t lbuf,
                             loff_t *ppos) {
  int res;
  size_t len;
  struct session_data *session = file->private_data;

  if (read) {
    read = false;
    return 0;
  }

  printk(KERN_INFO "kernel_mooc Reading with session %d, lbuf: %d, ppos: %llu",
         session->id, lbuf, *ppos);

  if (session->position == 0) {
    printk(KERN_INFO
           "kernel_mooc First read, return the session ID as a string: %d",
           session->id);

    char session_id_str[2];
    snprintf(session_id_str, sizeof(session_id_str), "%d", session->id);

    len = strlen(session_id_str);

    res = copy_to_user(buf, session_id_str, len);
    if (res != 0) {
      printk(KERN_ALERT "kernel_mooc Failed to copy data to user buffer\n");
      return -EFAULT;
    }

    *ppos += len;
    file->f_pos += len;
    session->position += len;
    pr_info("kernel_mooc sid: %d, ppos: %llu, f_pos: %llu", session->id, *ppos, file->f_pos);
  } else {
    printk(KERN_INFO "kernel_mooc Subsequent reads, return the string stored "
                     "in the buffer: %d",
           session->id);

    size_t remaining_length = strlen(session->buffer) - session->position;
    len = min(lbuf, remaining_length);

    res = copy_to_user(buf, session->buffer + session->position, len);
    if (res != 0) {
      printk(KERN_ALERT "kernel_mooc Failed to copy data to user buffer\n");
      return -EFAULT;
    }

    session->position += len;
  }

  read = true;
  return len;
}

static ssize_t solution_write(struct file *file, const char __user *buf,
                              size_t lbuf, loff_t *ppos) {
  struct session_data *session = file->private_data;

  printk(KERN_INFO "kernel_mooc Writing with session %d, lbuf: %d, ppos: %llu",
         session->id, lbuf, *ppos);

  if (lbuf > MAX_BUFFER_SIZE) {
    printk(KERN_ALERT "kernel_mooc Data exceeds maximum buffer size\n");
    return -EINVAL;
  }

  if (copy_from_user(session->buffer, buf, lbuf) != 0) {
    printk(KERN_ALERT "kernel_mooc Failed to copy data from user buffer\n");
    return -EFAULT;
  }

  session->position += lbuf;
  session->buffer[lbuf] = '\0';

  printk(KERN_INFO "kernel_mooc Wrote %s with session %d", session->buffer,
         session->id);

  return lbuf;
}

static loff_t solution_llseek(struct file *file, loff_t off, int whence) {
  struct session_data *session = file->private_data;

  loff_t new_pos;

  printk(KERN_INFO "kernel_mooc In llseek, off: %llu, wehnce: %d", off, whence);

  switch (whence) {
  case 0:
    printk(KERN_INFO "kernel_mooc SEEK_SET");
    new_pos = off;
    break;
  case 1:
    printk(KERN_INFO "kernel_mooc SEEK_CUR");
    new_pos = session->position + off;
    break;
  case 2:
    printk(KERN_INFO "kernel_mooc SEEK_END");
    new_pos = strlen(session->buffer) + off;
    break;
  default:
    return -EINVAL;
  }

  if (new_pos < 0)
    return -EINVAL;

  session->position = new_pos;
  // file->f_pos = new_pos;
  // printk(KERN_INFO "kernel_mooc f_pos is set to %llu", new_pos);

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
