#define CRAM_MAX_SIZE_BYTES   					(5ULL * 1024 * 1024 * 1024)
#define CRAM_MAX_PAGES        					(CRAM_MAX_SIZE_BYTES / PAGE_SIZE)
#define CRAM_MAX_BLOBS 							640

#define CRAM_SCRATCHPAD_SIZE				 	(8 * 1024 * 1024)
#define CRAM_SCRATCHPAD_MAX_PAGES 				CRAM_SCRATCHPAD_SIZE/PAGE_SIZE

#define CRAM_DISPATCH_BUFFER_SIZE 				(CRAM_SCRATCHPAD_SIZE + (CRAM_SCRATCHPAD_SIZE / 8))

#define CRAM_ZSTD_LEVEL_DEFAULT 19

/* -------------------------------------------------------------------------------------------- */

#include <linux/types.h>

/* -------------------------------------------------------------------------------------------- */

struct cram_dev_fs {
	uint device_id;		/* N, where N is /dev/cramN */
};

struct cram_page {		/* Page IDs are built into how cram_page is used. The max possible page_id is CRAM_MAX_PAGES */
    u32 blob_id;
    u16 page_index_in_blob;
};

struct blob {			/* Blob IDs are built into how scratchpad_blob is used. The max possible blob_id is CRAM_MAX_BLOBS */
    u8* blob;
    size_t size;
	bool active;
};

extern struct cram_dev_fs cram_dev;

int cram_dev_init(void);
int cram_dev_deinit(void);

int cram_blob_init(void);
int cram_blob_deinit(void);
int cram_blob_ingest_page(u8 *page);