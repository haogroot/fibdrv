#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <uapi/linux/time.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

#define MAX_LENGTH 100

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static char kbuffer[176];
#define WORD_SIZE 4
#define ARR_SIZE 3
#define DTYPE unsigned int
#define BIGGER_DTYPE unsigned long
#define MAX_VAL (BIGGER_DTYPE) 0xffffffff
struct bn {
    // each element is 32 bits size
    DTYPE array[ARR_SIZE];
};

void bignum_init(struct bn *num)
{
    int i;
    for (i = 0; i < ARR_SIZE; ++i) {
        num->array[i] = 0;
    }
}

void bignum_from_int(struct bn *num, BIGGER_DTYPE i)
{
    bignum_init(num);
    num->array[0] = i;
    BIGGER_DTYPE num_32 = 32;
    BIGGER_DTYPE tmp = i >> num_32;
    num->array[1] = tmp;
    int j;
    for (j = 2; j < ARR_SIZE; j++) {
        num->array[j] = 0;
    }
}

void bignum_add(struct bn *input_a, struct bn *input_b, struct bn *output)
{
    int carry = 0;
    int i;

    for (i = 0; i < ARR_SIZE; ++i) {
        BIGGER_DTYPE tmp =
            (BIGGER_DTYPE) input_a->array[i] + input_b->array[i] + carry;
        carry = (tmp > MAX_VAL);
        output->array[i] = (tmp & MAX_VAL);
    }
}

void bignum_sub(struct bn *input_a, struct bn *input_b, struct bn *output)
{
    int borrow = 0;
    int i;
    for (i = 0; i < ARR_SIZE; i++) {
        /* Borrow a higher byte */
        BIGGER_DTYPE tmp1 = (BIGGER_DTYPE) input_a->array[i] + (MAX_VAL + 1);
        BIGGER_DTYPE tmp2 = (BIGGER_DTYPE) input_b->array[i] + borrow;
        BIGGER_DTYPE sub_result = tmp1 - tmp2;
        output->array[i] = (DTYPE)(sub_result & MAX_VAL);
        borrow = (sub_result <= MAX_VAL);
    }
}

void _lshift_word(struct bn *a, int nwords)
{
    int i;
    /* Shift whole words */
    for (i = ARR_SIZE - 1; i >= nwords; --i) {
        a->array[i] = a->array[i - nwords];
    }
    /* Zero pad shifted words */
    for (; i >= 0; --i) {
        a->array[i] = 0;
    }
}

void bignum_mul(struct bn *input_a, struct bn *input_b, struct bn *output)
{
    struct bn row;
    struct bn tmp;
    int i, j;
    bignum_init(output);
    for (i = 0; i < ARR_SIZE; i++) {
        if (input_a->array[i] != 0) {
            bignum_init(&row);
            for (j = 0; j < ARR_SIZE; j++) {
                if (i + j < ARR_SIZE && input_b->array[j] != 0) {
                    bignum_init(&tmp);
                    BIGGER_DTYPE mul_result = (BIGGER_DTYPE) input_a->array[i] *
                                              (BIGGER_DTYPE) input_b->array[j];
                    bignum_from_int(&tmp, mul_result);

                    _lshift_word(&tmp, i + j);
                    bignum_add(&tmp, &row, &row);
                }
            }
            bignum_add(output, &row, output);
        }
    }
}

void bignum_copy(struct bn *src, struct bn *dst)
{
    int i;
    for (i = 0; i < ARR_SIZE; i++) {
        dst->array[i] = src->array[i];
    }
}

/* convert bignum array to hex string */
void bignum_to_string(struct bn *num, char *str, int nbytes)
{
    int j = ARR_SIZE - 1; /* index into array */
    int i = 0;            /* index into string */
    /* nbytes needs to be multiples of 8 */
    while ((j >= 0) && (nbytes > (i + 1))) {
        snprintf(&str[i], (2 * WORD_SIZE) + 1, "%08x", num->array[j]);
        i += (2 * WORD_SIZE); /* step 8 bytes forward in the string. */
        j -= 1;               /* step one element back in the array. */
    }

    /* Count leading zeros: */
    j = 0;
    while (str[j] == '0') {
        j += 1;
    }
    /* Move string j places ahead, effectively skipping leading zeros */
    for (i = 0; i < (nbytes - j); i++) {
        str[i] = str[i + j];
    }
    /* Zero-terminate string */
    str[i] = 0;
}

static int fib_sequence_fast_doubling_highest_bit(int k)
{
    if (k == 0) {
        kbuffer[8] = 48;
        kbuffer[9] = 0;
        return 1;
    }
    /* Calculate the position of the highest bit of n */
    unsigned int h = 0;
    h = (WORD_SIZE * 8) - __builtin_clz(k);

    struct bn a, b, c, d;
    bignum_init(&a);
    bignum_from_int(&b, 1);

    for (int j = h - 1; j >= 0; j--) {
        /* c = a * (2 * b - a) */
        struct bn tmp, tmp2;
        bignum_from_int(&tmp, 2);
        bignum_init(&tmp2);
        bignum_mul(&tmp, &b, &tmp2);
        bignum_sub(&tmp2, &a, &tmp);
        bignum_mul(&tmp, &a, &c);
        /* d = a * a + b * b */
        bignum_mul(&a, &a, &tmp);
        bignum_mul(&b, &b, &tmp2);
        bignum_add(&tmp, &tmp2, &d);

        if ((k >> j) & 1) {
            /* a = d */
            bignum_copy(&d, &a);
            /* b = c + d */
            bignum_add(&c, &d, &b);
        } else {
            /* a = c */
            bignum_copy(&c, &a);
            /* b = d */
            bignum_copy(&d, &b);
        }
    }
    /* return a */
    char hex_buf[40];
    bignum_to_string(&a, hex_buf, sizeof(hex_buf));
    for (int i = 0; i < 40; i++) {
        kbuffer[i + 8] = hex_buf[i];
        if (hex_buf[i] == 0) {
            return i;
        }
    }
    return 0;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    ktime_t kt;
    ssize_t retval;

    kt = ktime_get();
    retval = fib_sequence_fast_doubling_highest_bit(*offset);
    kt = ktime_sub(ktime_get(), kt);
    snprintf(kbuffer, sizeof(kbuffer), "%lld\n", kt);
    copy_to_user(buf, kbuffer, retval + 9);

    return retval;
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
