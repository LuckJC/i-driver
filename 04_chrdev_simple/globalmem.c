#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>

#define GLOBALMEM_SIZE		0x1000	/*全局内存大小4K*/
#define MEM_CLEAR			0x01	/*清零全局内存*/
#define GLOBALMEM_MAJOR	250		/*预设globalmem的主设备号*/

static int globalmem_major = GLOBALMEM_MAJOR;

/*globalmem设备结构体*/
struct globalmem_dev
{
	struct cdev cdev;		/*cdev结构体*/
	unsigned char mem[GLOBALMEM_SIZE];	/*全局内存*/
};

struct globalmem_dev dev;

static void globalmem_setup_cdev(void);

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;

	/*分析和获取有效的读长度*/
	if(p >= GLOBALMEM_SIZE)
		return 0;

	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	/*内核空间 -> 用户空间*/
	if(copy_to_user(buf, (void *)(dev.mem + p), count))
		ret = - EFAULT;
	else
	{
		*ppos += count;
		ret = count;

		printk(KERN_INFO "read %d byte(s) from %d\n", count, (int)p);
	}

	return ret;
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	int ret = 0;

	/*分析和获取有效的读长度*/
	if(p >= GLOBALMEM_SIZE)
		return 0;

	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	/*用户空间->  内核空间**/
	if(copy_from_user((void *)(dev.mem + p), buf, count))
		ret = - EFAULT;
	else
	{
		*ppos += count;
		ret = count;

		printk(KERN_INFO "written %d byte(s) from %d\n", count, (int)p);
	}

	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t ret;
	switch(orig){
	case 0:	/*文件开始*/
		if(offset < 0 || (unsigned int)offset > GLOBALMEM_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:	/*文件当前位置*/
		if(filp->f_pos + offset < 0 || filp->f_pos + offset  > GLOBALMEM_SIZE)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += (unsigned int)offset;
		ret = filp->f_pos;
		break;\
	case 2:	/*文件结束*/
		if(offset > 0 || offset + GLOBALMEM_SIZE<  0)
		{
			ret = -EINVAL;
			break;
		}
		filp->f_pos += GLOBALMEM_SIZE + offset;
		ret = filp->f_pos;
		break;
	default:
		ret = - EINVAL;
		break;
	}

	return ret;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
	case MEM_CLEAR:
		memset(dev.mem, 0, GLOBALMEM_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct file_operations globalmem_fops= {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
};

static void globalmem_setup_cdev(void)
{
	int err, devno = MKDEV(globalmem_major, 0);

	cdev_init(&dev.cdev, &globalmem_fops);
	dev.cdev.owner = THIS_MODULE;
	err = cdev_add(&dev.cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Erro %d adding flobalmem", err);
}

static int __init globalmem_init(void)
{
	int result;
	dev_t devno = MKDEV(globalmem_major, 0);

	/*申请字符设备驱动区域*/
	if(globalmem_major)
	{
		result = register_chrdev_region(devno, 1, "globalmem");
	}
	else
	{
		result = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}

	if(result < 0)
		return result;

	globalmem_setup_cdev();

	return 0;
}

static void __exit globalmem_exit(void)
{
	cdev_del(&dev.cdev);	/*删除cdev结构*/
	printk(KERN_INFO "globalmem_major  = %d\n", globalmem_major);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);		/*注销设备区域*/
}

module_init(globalmem_init);
module_exit(globalmem_exit);

MODULE_AUTHOR("Jovec <958028483@qq.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A simple chrdev driver");
MODULE_VERSION("V1.0");
MODULE_ALIAS("a simplest driver");

