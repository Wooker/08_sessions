#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

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
  struct session_data *session = file->private_data;

  if (read) {
    read = false;
    return 0;
  }
  read = true;

  printk(KERN_INFO "kernel_mooc In read. Session id: %d", session->id);

  if (session->position == -1) {
    printk(KERN_INFO "kernel_mooc First read, return the session ID as a string: %d", session->id);

    char session_id_str[2];
    snprintf(session_id_str, sizeof(session_id_str), "%d", session->id);
    size_t len = strlen(session_id_str);

    res = copy_to_user(buf, session_id_str, len);
    if (res != 0) {
      printk(KERN_ALERT "kernel_mooc Failed to copy data to user buffer\n");
      return -EFAULT;
    }

    *ppos += len;
    session->position += len;

    return len;
  } else {
    printk(KERN_INFO "kernel_mooc Subsequent reads, return the string stored in the buffer: %d", session->id);

    size_t remaining_length = strlen(session->buffer) - session->position;
    size_t read_length = min(lbuf, remaining_length);

    res = copy_to_user(buf, session->buffer + session->position, read_length);
    if (res != 0) {
      printk(KERN_ALERT "kernel_mooc Failed to copy data to user buffer\n");
      return -EFAULT;
    }

    session->position += read_length;
    return read_length;
  }
}

static ssize_t solution_write(struct file *file, const char __user *buf,
                              size_t lbuf, loff_t *ppos) {
  struct session_data *session = file->private_data;

  if (lbuf > MAX_BUFFER_SIZE) {
    printk(KERN_ALERT "kernel_mooc Data exceeds maximum buffer size\n");
    return -EINVAL;
  }

  if (copy_from_user(session->buffer, buf, lbuf) != 0) {
    printk(KERN_ALERT "kernel_mooc Failed to copy data from user buffer\n");
    return -EFAULT;
  }

  session->position = lbuf;
  session->buffer[lbuf] = '\0';

  return lbuf;
}

static loff_t solution_llseek(struct file *file, loff_t off, int whence) {
  char *kbuf = file->private_data;
  loff_t new_pos;

  printk(KERN_INFO "kernel_mooc In llseek, off: %llu, wehnce: %d", off,
         whence);

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
    new_pos = strlen(kbuf) + off;
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