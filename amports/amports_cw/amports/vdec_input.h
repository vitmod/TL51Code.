#ifndef VDEC_INPUT_H
#define VDEC_INPUT_H

#define DEBUG_SWAP_BUG

struct vdec_s;
struct vdec_input_s;

struct vframe_block_list_s {
	u32 magic;
	int id;
	struct list_head list;
	ulong start;
	void *start_virt;
	ulong addr;
	int type;
	u32 size;
	u32 wp;
	u32 rp;
	int data_size;
	int chunk_count;
	struct vdec_input_s *input;
};

#define VFRAME_CHUNK_FLAG_CONSUMED  0x0001

struct vframe_chunk_s {
	u32 magic;
	struct list_head list;
	int flag;
	u32 offset;
	u32 size;
	u32 pts;
	u32 pading_size;
	u64 pts64;
	bool pts_valid;
	u64 sequence;
	struct vframe_block_list_s *block;
};

#define VDEC_INPUT_TARGET_VLD           0
#define VDEC_INPUT_TARGET_HEVC          1

struct vdec_input_s {
	struct list_head vframe_block_list;
	struct list_head vframe_chunk_list;
	struct list_head vframe_block_free_list;
	struct vframe_block_list_s *wr_block;
	int have_free_blocks;
	int no_mem_err_cnt;/*when alloc no mem cnt++*/
	int block_nums;
	int block_id_seq;
	int id;
	spinlock_t lock;
	int type;
	int target;
	struct vdec_s *vdec;
	bool swap_valid;
	bool swap_needed;
	bool eos;
	struct page *swap_page;
	unsigned long swap_page_phys;
#ifdef DEBUG_SWAP_BUG
	struct page *swap_page_bak;
	unsigned long swap_page_phys_bak;
#endif
	u64 total_wr_count;
	u64 total_rd_count;
	u64 streaming_rp;
	u32 swap_rp;
	bool last_swap_slave;
	int dirty_count;
	u64 sequence;
	unsigned start;
	unsigned size;
	int default_block_size;
	int data_size;
	int frame_max_size;
	int prepare_level;
/*for check frame delay.*/
	u64 last_inpts_u64;
	u64 last_comsumed_pts_u64;
	int last_in_nopts_cnt;
	int last_comsumed_no_pts_cnt;
	int last_duration;
/*for check frame delay.*/
	int have_frame_num;
	int stream_cookie; /* wrap count for vld_mem and
			      HEVC_SHIFT_BYTE_COUNT for hevc */
};

struct vdec_input_status_s {
	int size;
	int data_len;
	int free_len;
	int read_pointer;
};

#define input_frame_based(input) \
	(((input)->type == VDEC_TYPE_FRAME_BLOCK) || \
	 ((input)->type == VDEC_TYPE_FRAME_CIRCULAR))
#define input_stream_based(input) \
	(((input)->type == VDEC_TYPE_STREAM_PARSER) || \
	 ((input)->type == VDEC_TYPE_SINGLE))

/* Initialize vdec_input structure */
extern void vdec_input_init(struct vdec_input_s *input, struct vdec_s *vdec);
extern int vdec_input_prepare_bufs(struct vdec_input_s *input,
	int frame_width, int frame_height);

/* Get available input data size */
extern int vdec_input_level(struct vdec_input_s *input);

/* Set input type and target */
extern void vdec_input_set_type(struct vdec_input_s *input, int type,
			int target);

/* Set stream buffer information for stream based input */
extern int vdec_input_set_buffer(struct vdec_input_s *input, u32 start,
			u32 size);

/* Add enqueue video data into decoder's input */
extern int vdec_input_add_frame(struct vdec_input_s *input, const char *buf,
			size_t count);

/* Peek next frame data from decoder's input */
extern struct vframe_chunk_s *vdec_input_next_chunk(
			struct vdec_input_s *input);

/* Peek next frame data from decoder's input, not marked as consumed */
extern struct vframe_chunk_s *vdec_input_next_input_chunk(
			struct vdec_input_s *input);

/* Consume next frame data from decoder's input */
extern void vdec_input_release_chunk(struct vdec_input_s *input,
			struct vframe_chunk_s *chunk);

/* Get decoder input buffer status */
extern int vdec_input_get_status(struct vdec_input_s *input,
			struct vdec_input_status_s *status);

extern unsigned long vdec_input_lock(struct vdec_input_s *input);

extern void vdec_input_unlock(struct vdec_input_s *input, unsigned long lock);

/* release all resource for decoder's input */
extern void vdec_input_release(struct vdec_input_s *input);
int vdec_input_dump_chunks(struct vdec_input_s *input,
	char *bufs, int size);
int vdec_input_dump_blocks(struct vdec_input_s *input,
	char *bufs, int size);

/*for profile*/
int vdec_input_chunk_size(struct vdec_input_s *input, int *size);


#endif /* VDEC_INPUT_H */

