#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/printk.h>

#include "../include/cram.h"

static int cram_major_ident;
struct gendisk *cram_disk;
struct cram_dev_fs cram_dev = {
    .device_id = 0,
};

static void cram_submit_bio(struct bio *bio)
{

	switch (bio_op(bio)) {
		case REQ_OP_READ: {
			bio_io_error(bio);
			break;
		};
		
		case REQ_OP_WRITE: {
				pr_debug("[CRAM]: Write RECVD\n");
			    struct bio_vec bvec;
				struct bvec_iter iter;
				void *kaddr;
				void *src;

				bio_for_each_segment(bvec, bio, iter) {
					if (bvec.bv_len != PAGE_SIZE) {
						pr_err("[CRAM]: ERR_BAD_IO_SIZE: %u\n", bvec.bv_len);
						bio_io_error(bio);
						return;
						break;
					}

					kaddr = kmap_local_page(bvec.bv_page);
					src   = (u8 *)kaddr + bvec.bv_offset;

					sector_t sector = bio->bi_iter.bi_sector;
					u32 page_id = sector >> (PAGE_SHIFT - SECTOR_SHIFT);

					int ret = cram_blob_ingest_page(src, page_id);

					if (ret != 0) {
						pr_info("[CRAM]: ERR_BLOB_INGESTION_FAILED\n");
						kunmap_local(kaddr);
						bio_io_error(bio);
						return;
						break;
					}
					
					kunmap_local(kaddr);
				};

			break;
		};

		default:
			bio_io_error(bio);
			return;
			break;
	};

	bio_endio(bio);
}

static const struct block_device_operations cram_fops = {
	.submit_bio	= cram_submit_bio,
	.owner 		= THIS_MODULE,
};

int cram_dev_deinit(void)
{
	pr_info("[CRAM]: cram_dev_deinit\n");
	del_gendisk(cram_disk);
    put_disk(cram_disk);
    unregister_blkdev(cram_major_ident, "cram");

    return 0;
};

int cram_dev_init(void)
{
	struct queue_limits block_limits = { 0 };

	pr_info("[CRAM]: REGISTER BLOCKDEVICE\n");
	cram_major_ident = register_blkdev(0, "cram");
	if (cram_major_ident < 0) {
		pr_err("[CRAM]: ERR: %d\n", cram_major_ident);
		return cram_major_ident;
	};

	pr_info("[CRAM]: REGISTERED blockdevice WITH major_id: %d\n", cram_major_ident);

	pr_info("[CRAM]: BUILDING gendisk cram_disk\n");
	block_limits.logical_block_size 	= 4096;
	block_limits.physical_block_size 	= 4096;
	block_limits.io_opt					= 8388608;
	block_limits.io_min					= 8388608;
	block_limits.max_sectors			= (unsigned int)(block_limits.io_opt >> 9);

	cram_disk = blk_alloc_disk(&block_limits, NUMA_NO_NODE);

		if (!cram_disk) {
		pr_err("[CRAM]: blk_alloc_disk() failed\n");
		unregister_blkdev(cram_major_ident, "cram");
		return -ENOMEM;
	}

	cram_disk->major = cram_major_ident;
	cram_disk->first_minor = 0;
	cram_disk->minors = 1;
	cram_disk->private_data = &cram_dev;
	cram_disk->fops = &cram_fops;

	snprintf(cram_disk->disk_name, DISK_NAME_LEN, "cram0");

	set_capacity(cram_disk, 5ULL * 1024 * 1024 * 1024 / 512);
	set_bit(GD_SUPPRESS_PART_SCAN, &cram_disk->state);

	int err = add_disk(cram_disk);
	if (err != 0) {
		pr_err("[CRAM]: add_disk() failed, error: %d\n", err);
			
		// Clean up resources if add_disk fails
		cram_dev_deinit();
			
		return err;
	};

	pr_info("[CRAM]: BUILT gendisk cram_disk\n");

	return 0;
};