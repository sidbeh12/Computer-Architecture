// fn to get state on il1 miss

byte_t
il1_fetch_state(enum mem_cmd cmd,  /* access cmd, Read or Write */
       md_addr_t baddr,  /* block address to access */
       int bsize,  /* size of block to access */
       struct cache_blk_t *blk, /* ptr to block in upper level */
       tick_t now)  /* time of access */
{
  byte_t state;


if (cache_il2)
    {
      /* access next level of inst cache hierarchy */
      state = get_state(cache_il2, cmd, baddr, NULL, bsize,
    /* now */now, /* pudata */NULL, /* repl addr */NULL);
      if (cmd == Read)
 return state;
      else
 panic("use il1_write_state fn");
    } 
  else
    {
   state = 0x00;
      /* access main memory */
      if (cmd == Read)
 return state;
      else
 panic("use il1_write_state fn");
    }
}

// fn to write back il1 block state bits

void
il1_write_state(enum mem_cmd cmd,  /* access cmd, Read or Write */
       md_addr_t baddr,  /* block address to access */
       int bsize,  /* size of block to access */
       struct cache_blk_t *blk, /* ptr to block in upper level */
       byte_t state)  /* time of access */
{

if (cache_il2)
    {
  if (cmd == Read)
  panic("use il1_fetch_state fn");

 /* access next level of inst cache hierarchy */
      write_state(cache_il2, cmd, baddr, bsize,state);

 }
}


static unsigned int   /* latency of block access */
victim_cache_access_fn(enum mem_cmd cmd,  /* access cmd, Read or Write */
       md_addr_t baddr,  /* block address to access */
       int bsize,  /* size of block to access */
       struct cache_blk_t *blk, /* ptr to block in upper level */
       tick_t now)  /* time of access */
{
  unsigned int lat;
  lat = 2;
  if (cmd == Write)
  il1_write_state(Write,baddr,
           bsize,blk,blk->state);
 // if (cmd == Read)
   return lat;
 // else
  // panic("writes to instruction memory not supported");
}