#include <asm-generic/errno-base.h>
#include <asm-generic/int-ll64.h>
#include <linux/gfp_types.h>
#include <linux/highmem.h>
#include <linux/mempool.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/zstd.h>


#include "../include/cram.h"

/* Compression Workspace */
static void* 			compression_workspace_ptr;
static zstd_cctx* 		compression_cram_cctx;
static zstd_parameters 	compression_params;

/*  Compression Scratchpad */
struct cram_page 		cram_page[CRAM_SCRATCHPAD_MAX_PAGES];
static uint 			compression_page_id = 0;
static u8*				scratchpad;

/* Decompression Workspace */
static void* 			decompression_workspace_ptr;
static zstd_dctx* 		decompression_cram_cctx;

/* Dispatch Workspace */
static struct blob			cram_blob[CRAM_MAX_BLOBS];
static uint 				cram_blob_id 		= 0;


/* cram_blob inits and deinits */

int cram_blob_init(void)
{
	pr_info("\n[CRAM]: cram_blob_init()\n");
	
	pr_info("[CRAM]: SETTING UP cram_zstd_workspace");

	/* --------------------- Setup Compression Workspace ------------------------------------------------------- */
	compression_params = zstd_get_params(CRAM_ZSTD_LEVEL_DEFAULT, CRAM_SCRATCHPAD_SIZE);
	size_t compression_workspace_size = zstd_cctx_workspace_bound(&compression_params.cParams);

	compression_workspace_ptr = kvmalloc(compression_workspace_size, GFP_KERNEL);
	if (compression_workspace_ptr == NULL) { return -ENOMEM; };

	compression_cram_cctx = zstd_init_cctx(compression_workspace_ptr, compression_workspace_size);
	
	
	scratchpad = kvmalloc(CRAM_SCRATCHPAD_SIZE, GFP_KERNEL);
	if (!scratchpad) {
		pr_err("[CRAM]: ERR_FAILED_ALLOC: scratchpad");
		return -ENOMEM;
	};


	pr_info("[CRAM]: ZSTD COMPRESSION WORKSPACE ALLOC'D: %zu BYTES\n", compression_workspace_size);

	/* --------------------- Setup Decompression Workspace ------------------------------------------------------- */
	size_t decompression_workspace_size = zstd_dctx_workspace_bound();

	decompression_workspace_ptr = kvmalloc(decompression_workspace_size, GFP_KERNEL);
	if (decompression_workspace_ptr == NULL) { return -ENOMEM; };

	decompression_cram_cctx = zstd_init_dctx(decompression_workspace_ptr, decompression_workspace_size);

	pr_info("[CRAM]: ZSTD DECOMPRESSION WORKSPACE ALLOC'D: %zu BYTES\n", decompression_workspace_size);

	/* ------------------------------------------------------------------------------------------------------------------------ */
	pr_info("\n[CRAM]: cram_blob_init() EXITED\n");
	
	return 0;
};


int cram_blob_deinit(void)
{
	pr_info("\n[CRAM]: cram_blob_deinit\n");

	kvfree(compression_workspace_ptr);
	kvfree(decompression_workspace_ptr);
	kvfree(scratchpad);

	for (size_t i = 0; i < CRAM_MAX_BLOBS; i++) {
		// Action #1: Are We at the end of the blobs?
		if (cram_blob[i].active == false) break;

		// Action #2: FREEE
		kvfree(cram_blob[i].blob);

		// Action #3: MARK AS NON ACTIVE AND ZERO EVERYTHING
		cram_blob[i].blob 	= NULL;
		cram_blob[i].size 	= 0;
		cram_blob[i].active = false;
	};

	cram_blob_id = 0;
	compression_page_id = 0;

	pr_info("\n[CRAM]: cram_blob_deinit() EXIT\n");
	return 0;
};

static int cram_blob_dispatch(void)
{
	pr_info("[CRAM]: BLOB_DISPATCH");
	if (cram_blob_id >= CRAM_MAX_BLOBS) {
    	pr_err("[CRAM]: ERR_BLOB_OVERFLOW: DISREGARDING DISPATCH.");
		return -ENOSPC;
	};

	size_t cram_max_blob_size = zstd_compress_bound(CRAM_SCRATCHPAD_SIZE);
	void *dst = kvmalloc(cram_max_blob_size, GFP_KERNEL);
	if (!dst) {
		pr_err("[CRAM]: ERR_FAILED_ALLOC: dispatch_dst");
		return -ENOMEM;
	}

	size_t compressed_size = zstd_compress_cctx(compression_cram_cctx, dst, cram_max_blob_size, scratchpad, CRAM_SCRATCHPAD_SIZE, &compression_params);
	if (zstd_is_error(compressed_size)) {
		kvfree(dst);
		pr_err("[CRAM]: ERR_FAILED_ZSTD_COMPRESSION: zstd_compress_blob");
		return -EIO;   /* or another appropriate errno */
	}

	cram_blob[cram_blob_id].size 	= compressed_size;
	cram_blob[cram_blob_id].blob 	= dst;
	cram_blob[cram_blob_id].active 	= true;	/* COMMIT */

	cram_blob_id++;
	compression_page_id = 0;

	return 0;
};


int cram_blob_ingest_page(u8 *page)		/* QUICK NOTE ON OWNERSHIP: WE OWN THE PAGE NOW.*/
{
	BUG_ON(compression_page_id >= CRAM_SCRATCHPAD_MAX_PAGES);
	BUG_ON(compression_page_id * PAGE_SIZE >= CRAM_SCRATCHPAD_SIZE);

	/* page MUST point to exactly PAGE_SIZE bytes */

	if (sizeof(*page) != PAGE_SIZE) { return -EINVAL; }

	pr_alert("[CRAM]: Page Ingestion Requested");
	if (compression_page_id >= CRAM_SCRATCHPAD_MAX_PAGES) {
		int ret = cram_blob_dispatch();
		if (ret)
			return ret;
	};

	pr_info("[CRAM] page_id=%u\n", compression_page_id);

	memcpy(
		scratchpad + compression_page_id * PAGE_SIZE,
		page,
		PAGE_SIZE
	);

	// cram_page[compression_page_id].page;
	compression_page_id++;

	BUG_ON(compression_page_id >= CRAM_SCRATCHPAD_MAX_PAGES);
	BUG_ON(compression_page_id * PAGE_SIZE >= CRAM_SCRATCHPAD_SIZE);


	return 0;
};