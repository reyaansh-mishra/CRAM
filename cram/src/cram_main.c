#include <linux/init.h>
#include <linux/printk.h>
#include <linux/module.h>

#include "../include/cram.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ZSTD-based Cold RAM Storage");

static int __init cram_init(void)
{
	pr_info("[CRAM]: Hi. Starting Up.\n");
	cram_dev_init();
	cram_blob_init();
	return 0;
};

static void __exit cram_deinit(void)
{
	pr_info("[CRAM]: Powering Off\n");
	cram_dev_deinit();
	cram_blob_deinit();
	return;
};

module_init(cram_init);
module_exit(cram_deinit);