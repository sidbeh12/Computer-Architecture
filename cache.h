/* block status values */ 
#define CACHE_BLK_VALID  0x00000001 /* block in valid, in use */
#define CACHE_BLK_DIRTY  0x00000002 /* dirty block */
#define HIT_BIT  0x01    /*hit bit*/
#define STICKY_BIT 0x02     /*sticky*/
#define NOT_HIT_BIT   0xFE    /*remove hit bit*/
#define NOT_STICKY_BIT 0xFD   /*remove sticky bit*/

/* cache block (or line) definition */
struct cache_blk_t
{
  struct cache_blk_t *way_next; /* next block in the ordered way chain, used
       to order blocks for replacement */
  struct cache_blk_t *way_prev; /* previous block in the order way chain */
  struct cache_blk_t *hash_next;/* next block in the hash bucket chain, only
       used in highly-associative caches */
  /* since hash table lists are typically small, there is no previous
     pointer, deletion requires a trip through the hash table bucket list */
  md_addr_t tag;  /* data block tag value */
  unsigned int status;  /* block status, see CACHE_BLK_* defs above */
  tick_t ready;  /* time when block will be accessible, field
       is set when a miss fetch is initiated */
  byte_t *user_data;  /* pointer to user defined data, e.g.,
       pre-decode data or physical page address */
  /* DATA should be pointer-aligned due to preceeding field */
  /* NOTE: this is a variable-size tail array, this must be the LAST field
     defined in this structure! */
  byte_t data[1];  /* actual data block starts here, block size
       should probably be a multiple of 8 */
  byte_t state;      // Stores state bits
};
