#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergey Vinokurov");
MODULE_DESCRIPTION("Linux.2: Task2: Keyboard interruptor");
MODULE_VERSION("1.0");

#define KEYBOARD_IRQ 1
#define DATA_REG 0x60
#define STATUS_MASK 0x80

int keysPressed;
spinlock_t lock;
struct task_struct* printTask;

static irqreturn_t keyboardInterruption(int irq, void* dev_id)
{
    char scancode;

    scancode = inb(DATA_REG);
    if (scancode & STATUS_MASK)
    {
        spin_lock(&lock);
        ++keysPressed;
        spin_unlock(&lock);
    }

    return IRQ_HANDLED;
}

static int threadPrinter(void* data)
{
    int seconds = 0;
    while (!kthread_should_stop())
    {
        msleep_interruptible(1000);
	++seconds;
        if (seconds > 5)
        {
            spin_lock(&lock);
            printk(KERN_INFO "%d keys pressed for the last 5 seconds\n", keysPressed);
            keysPressed = 0;
            seconds = 0;
            spin_unlock(&lock);
        }
    }
    return 0;
}

static int __init init(void)
{
    request_irq(KEYBOARD_IRQ, keyboardInterruption, IRQF_SHARED, "keyboardInterruption", (void*)keyboardInterruption);
    spin_lock_init(&lock);
    keysPressed = 0;
    printTask = kthread_run(threadPrinter, NULL, "threadPrinter");
    return 0;
}

static void __exit exit(void)
{
    kthread_stop(printTask);
    free_irq(KEYBOARD_IRQ, (void*)keyboardInterruption);
}

module_init(init);
module_exit(exit);

