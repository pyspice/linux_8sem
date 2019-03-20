#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include "records.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergey Vinokurov");
MODULE_DESCRIPTION("Linux.2: Task1: Phonebook");
MODULE_VERSION("1.0");

#define DEVICE_NAME "task1_vinokurov"
#define MSG_BUFFER_LEN 128
#define is_space(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\v' || (c) == '\f' || (c) == '\r')

static int major_num;
static int device_open_count = 0;
static char msg_buffer[MSG_BUFFER_LEN];
static char *msg_ptr;
struct Records* head = NULL;
struct Records* tail = NULL;

static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char*, size_t, loff_t*);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);

static struct file_operations file_ops = 
{
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

void clear_buffer(void) 
{
	memset(msg_buffer, 0, MSG_BUFFER_LEN);
}

void put_record(const struct Record* record) {
	size_t name_len = 0, num_len = 0;
	for (name_len = 0; record->name[name_len]; ++name_len);
	for (num_len = 0; record->num[num_len]; ++num_len);

	clear_buffer();
	memcpy(msg_buffer, record->name, name_len);
	memcpy(msg_buffer, " ", 1);
	memcpy(msg_buffer, record->num, num_len);
	msg_buffer[name_len + num_len - 1] = 0;
}

void put_not_found(const char* name, size_t name_len)
{
	clear_buffer();
	memcpy(msg_buffer, name, name_len);
	// long name trimming...
	memcpy(msg_buffer + name_len, " not found\n", 11);
}

void put_error(void) {
	clear_buffer();
	memcpy(msg_buffer, "Error parsing query\n", 20);
}

void add_record(const char* name, size_t name_len, const char* num, size_t num_len)
{
    struct Record rec;
    rec.name = (char*)kmalloc(name_len, GFP_KERNEL);
    memcpy(rec.name, name, name_len);
    rec.name[name_len - 1] = 0;
    rec.num = (char*)kmalloc(num_len, GFP_KERNEL);
    memcpy(rec.num, num, num_len);
    rec.num[num_len - 1] = 0;
    
    struct Records* node = (struct Records*)kmalloc(sizeof(struct Records),
GFP_KERNEL);
    node->next = NULL;
    node->record = rec;

    if (head == NULL)
    {
        head = node;
		tail = node;
    }
    else
    {
        tail->next = node;
    }

	clear_buffer();
}

void put_info(const char* name, size_t name_len) {
	struct Records* cur = records;
    while (cur != NULL)
    {
        if (memcmp(cur->record.name, name, name_len) == 0)
        {
            put_record(&cur->record);
            return;
        }
        cur = cur->next;
    }
    put_not_found(name, name_len);
}

void remove_record(const char* name, size_t name_len) {
	struct Records* cur = head;
	struct Records* prev = NULL;
    while (cur != NULL)
    {
        if (memcmp(cur->record.name, name, name_len) == 0)
        {
			if (prev != NULL) {
				prev->next = cur->next;			
			}

			if (cur->next == NULL) {
				head = tail = NULL;
			}

			kfree(cur->record.name);
			kfree(cur->record.num);
			kfree(cur);

            return;
        }
		prev = cur;
        cur = cur->next;
    }
    put_not_found(name, name_len);	
}

void clear_records(struct Records* records)
{
    if (records == NULL)
    {
        return;
    }
    clear_records(records->next);

    kfree(records->record.name);
    kfree(records->record.num);
    kfree(records);
}

void run_query(void)
{
	size_t s_cmd, f_cmd;
	for (s_cmd = 0; s_cmd < MSG_BUFFER_LEN && !is_space(msg_buffer[s_cmd]); ++s_cmd);
	if (s_cmd == MSG_BUFFER_LEN) 
	{
		put_error();
		return;	
	}
	for (f_cmd = s_cmd + 1; f_cmd < MSG_BUFFER_LEN && !is_space(msg_buffer[f_cmd]); ++f_cmd);
	
	size_t s_name, f_name;
	for (s_name = f_cmd + 1; s_name < MSG_BUFFER_LEN && !is_space(msg_buffer[s_name]); ++s_name);
	if (s_name == MSG_BUFFER_LEN) {
		put_error();
		return;
	}
	for (f_name = s_name + 1; f_name < MSG_BUFFER_LEN && !is_space(msg_buffer[f_name]); ++f_name);

	char name[MSG_BUFFER_LEN];
	size_t name_len = f_name - s_name;
	size_t cmd_len = f_cmd - s_cmd;
	memcpy(name, msg_buffer + s_name, name_len);

	if (memcmp("add", msg_buffer + s_cmd, cmd_len > 3 ? 3 : cmd_len) == 0) {
		size_t s_num, f_num;
		for (s_num = f_name + 1; s_num < MSG_BUFFER_LEN && !is_space(msg_buffer[s_num]); ++s_num);
		if (s_num == MSG_BUFFER_LEN) {
			put_error();
			return;
		}
		for (f_num = s_num + 1; f_num < MSG_BUFFER_LEN && !is_space(msg_buffer[f_num]); ++f_num);

		char num[MSG_BUFFER_LEN];
		size_t num_len = f_num - s_num;
		memcpy(num, msg_buffer + s_num, num_len);

		add_record(name, name_len, num, num_len);
		clear_buffer();
	}
	else if (memcmp("info", msg_buffer + s_cmd, cmd_len > 4 ? 4 : cmd_len) == 0) {
		put_info(name, name_len);
	}
	else if (memcmp("remove", msg_buffer + s_cmd, cmd_len > 6 ? 6 : cmd_len) == 0) {
		remove_record(name, name_len);
		clear_buffer();
	}
	else {
		put_error();	
	}
}

static ssize_t device_read(struct file* flip, char* buffer, size_t len, loff_t* offset)
{
	int bytes_read = 0;

	if (*msg_ptr == 0) {
		msg_ptr = msg_buffer;
	}

    printk(KERN_INFO "len: %d\n", len);

	while (len && *msg_ptr) {
	 	put_user(*(msg_ptr++), buffer++);
	 	len--;
	 	bytes_read++;
	}

	return bytes_read;
}

static ssize_t device_write(struct file* flip, const char* buffer, size_t len, loff_t* offset)
{
	size_t to_cpy = len < MSG_BUFFER_LEN ? len : MSG_BUFFER_LEN;
	msg_ptr = msg_buffer;
    memcpy(msg_ptr, buffer, to_cpy);
	run_query();
    return len;
}

static int device_open(struct inode* inode, struct file* file)
{
	if (device_open_count) 
	{
		return -EBUSY;
 	}
 	device_open_count++;
 	try_module_get(THIS_MODULE);
 	return 0;
}

static int device_release(struct inode* inode, struct file* file)
{
	device_open_count--;
 	module_put(THIS_MODULE);
 	return 0;
}

static int __init init(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (major_num < 0)
    {
        printk(KERN_ALERT "Could not register device: %d\n", major_num);
        return major_num;
    }
    else
    {
        printk(KERN_INFO "%s device major number %d\n", DEVICE_NAME, major_num);
        return 0;
    }
}

static void __exit exit(void)
{
    unregister_chrdev(major_num, DEVICE_NAME);
	clear_records(head);
    head = tail = NULL;
}

module_init(init);
module_exit(exit);


