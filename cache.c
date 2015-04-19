// Function to access an L1 cache with a victim cache

unsigned int    /* latency of access in cycles */
cache_access_victim(struct cache_t *cp, /* cache to access */
      enum mem_cmd cmd,  /* access type, Read or Write */
      md_addr_t addr,  /* address of access */
      void *vp,   /* ptr to buffer for input/output */
      int nbytes,  /* number of bytes to access */
      tick_t now,  /* time of access */
      byte_t **udata,  /* for return of user data ptr */
      md_addr_t *repl_addr, /* for address of replaced block */
      struct cache_t *vc,
      byte_t (*state_access_fn) (enum mem_cmd cmd,
       md_addr_t baddr, int bsize,
       struct cache_blk_t *blk, 
       tick_t now)) /* for address of replaced block */

{
  byte_t *p = vp;
  md_addr_t tag = CACHE_TAG(cp, addr);
  md_addr_t set = CACHE_SET(cp, addr);
  md_addr_t bofs = CACHE_BLK(cp, addr);
  md_addr_t addr_repl;
  md_addr_t tag_repl_vc;
  md_addr_t set_repl_vc;
  byte_t state;

  int lat_victim;
  struct cache_blk_t *blk, *repl;
  int lat = 0;

  /* check for a fast hit: access to same block */
  if (CACHE_TAGSET(cp, addr) == cp->last_tagset)
    {
      /* hit in the same block */
      blk = cp->last_blk;
      cp->last_blk->state = 0x03;
      goto cache_fast_hit;
    }

      /* low-associativity cache, linear search the way list */
      for (blk=cp->sets[set].way_head;
    blk;
    blk=blk->way_next)
 {
   if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
     {
       blk->state = 0x03;
    goto cache_hit;
     }
 }
    

  lat_victim = victim_cache_access(vc, Read, addr,
          NULL, nbytes, now,
          NULL, NULL);



  switch (cp->policy) {
    case LRU:
    case FIFO:
      repl = cp->sets[set].way_tail;
      update_way_list(&cp->sets[set], repl, Head);
      break;
    case Random:
      {
        int bindex = myrand() & (cp->assoc - 1);
        repl = CACHE_BINDEX(cp, cp->sets[set].blks, bindex);
      } 
      break;
    default:
      panic("bogus replacement policy");
    }

    addr_repl = CACHE_MK_BADDR(cp, repl->tag, set);

    tag_repl_vc = CACHE_TAG(vc, addr_repl);
    set_repl_vc = CACHE_SET(vc, addr_repl);

    //state=state_access_fn(Read,CACHE_BADDR(cp, addr),cp->bsize,repl, now+lat);

  if (lat_victim == 1)
  {
   //vc->last_blk->tag = tag_repl_vc;
     //vc->last_blk->status = repl->status;
     cp->hits++;
     lat+=cp->hit_latency;
     if (!(repl->state & STICKY_BIT))
     {
      if (cp->hsize)
         unlink_htab_ent(cp, &cp->sets[set], repl);
      /d* blow away the last block to hit */
      cp->last_tagset = 0;
      cp->last_blk = NULL;

      vc->last_blk->tag =  tag_repl_vc;
      vc->last_blk->status = repl->status;
      vc->last_blk->state = repl->state;
      repl->state = 0x03;
      //repl->state = 0x02;
      repl->tag = tag;
      repl->status = CACHE_BLK_VALID;
     /* remove this block from the hash bucket chain, if hash exists */
     }
     else
     {
      if ( !(vc->last_blk->state & HIT_BIT))
       repl->state &= NOT_STICKY_BIT;
      else
      {
     vc->last_blk->tag =  tag_repl_vc;
     vc->last_blk->status = repl->status;
     vc->last_blk->state = repl->state;
     repl->state = 0x02;
     repl->tag = tag;
     repl->status = CACHE_BLK_VALID;
      }
     }
  }
  else
  {
   if ( !(repl->state & STICKY_BIT))
   {
    vc->sets[set_repl_vc].way_head->tag = tag_repl_vc; 
    vc->sets[set_repl_vc].way_head->status = repl->status;
    vc->sets[set_repl_vc].way_head->state = repl->state;
    if (repl->status & CACHE_BLK_VALID)
          cp->replacements++;
    repl->tag = tag;
    repl->status = CACHE_BLK_VALID;
    repl->state = 0x03;
    //repl->state = 0x02;
   }
   else
   {
    state=state_access_fn(Read,CACHE_BADDR(cp, addr),cp->bsize,repl, now+lat);
    if (!(state & HIT_BIT))
    {
       addr_repl = CACHE_MK_BADDR(cp,tag,set);
       tag_repl_vc = CACHE_TAG(vc, addr_repl);
       set_repl_vc = CACHE_SET(vc, addr_repl);
       if (vc->sets[set_repl_vc].way_head->status & CACHE_BLK_VALID)
             cp->replacements++;
    vc->sets[set_repl_vc].way_head->tag = tag_repl_vc;
    vc->sets[set_repl_vc].way_head->status = CACHE_BLK_VALID;
    repl->state &= NOT_STICKY_BIT;
    }
    else
    {
     vc->sets[set_repl_vc].way_head->tag = tag_repl_vc;
     vc->sets[set_repl_vc].way_head->status = repl->status;
     vc->sets[set_repl_vc].way_head->state = repl->state;
     if (repl->status & CACHE_BLK_VALID)
              cp->replacements++;
     repl->tag = tag;
     repl->status = CACHE_BLK_VALID;
     repl->state = 0x02;
    }
   }
   /* cache block not found */

  /* **MISS** */
  cp->misses++;


  /* read data block */
  lat += cp->blk_access_fn(Read, CACHE_BADDR(cp, addr), cp->bsize,
      repl, now+lat);

  }

  /* update block tags */
  //repl->tag = tag;
  //repl->status = CACHE_BLK_VALID; /* dirty bit set on update */

  /* update dirty status */
  if (cmd == Write)
    repl->status |= CACHE_BLK_DIRTY;
 
  /* get user block data, if requested and it exists */
  if (udata)
    *udata = repl->user_data;

  /* update block status */
  repl->ready = now+lat;

  /* link this entry back into the hash table */
  if (cp->hsize)
    link_htab_ent(cp, &cp->sets[set], repl);

  /* return latency of the operation */
  return lat;


 cache_hit: /* slow hit handler */

  /* **HIT** */
  cp->hits++;

  /* update dirty status */
  if (cmd == Write)
    blk->status |= CACHE_BLK_DIRTY;

  /* if LRU replacement and this is not the first element of list, reorder */
  if (blk->way_prev && cp->policy == LRU)
    {
      /* move this block to head of the way (MRU) list */
      update_way_list(&cp->sets[set], blk, Head);
    }

  /* tag is unchanged, so hash links (if they exist) are still valid */

  /* record the last block to hit */
  cp->last_tagset = CACHE_TAGSET(cp, addr);
  cp->last_blk = blk;

  /* return first cycle data is available to access */
  return (int) MAX(cp->hit_latency, (blk->ready - now));

 cache_fast_hit: /* fast hit handler */

  /* **FAST HIT** */
  cp->hits++;

  /* copy data out of cache block, if block exists */
  if (cp->balloc)
    {
      CACHE_BCOPY(cmd, blk, bofs, p, nbytes);
    }

  /* update dirty status */
  if (cmd == Write)
    blk->status |= CACHE_BLK_DIRTY;
 
  /* this block hit last, no change in the way list */

  /* tag is unchanged, so hash links (if they exist) are still valid */

  /* get user block data, if requested and it exists */
  if (udata)
    *udata = blk->user_data;

  /* record the last block to hit */
  cp->last_tagset = CACHE_TAGSET(cp, addr);
  cp->last_blk = blk;

  /* return first cycle data is available to access */
  return (int) MAX(cp->hit_latency, (blk->ready - now));
}


// cache access for victim cache

unsigned int    /* latency of access in cycles */
victim_cache_access(struct cache_t *cp, /* cache to access */
      enum mem_cmd cmd,  /* access type, Read or Write */
      md_addr_t addr,  /* address of access */
      void *vp,   /* ptr to buffer for input/output */
      int nbytes,  /* number of bytes to access */
      tick_t now,  /* time of access */
      byte_t **udata,  /* for return of user data ptr */
      md_addr_t *repl_addr ) /* for address of replaced block */


{
  byte_t *p = vp;
  md_addr_t tag = CACHE_TAG(cp, addr);
  md_addr_t set = CACHE_SET(cp, addr);
  md_addr_t bofs = CACHE_BLK(cp, addr);
  struct cache_blk_t *blk, *repl;
  int lat = 1; 

// Skipping code for checking for cache hit 

  /* cache block not found */

  /* **MISS** */
  cp->misses++;

  lat+=1;

  /* select the appropriate block to replace, and re-link this entry to
     the appropriate place in the way list */
  switch (cp->policy) {
  case LRU:
  case FIFO:
    repl = cp->sets[set].way_tail; 
    update_way_list(&cp->sets[set], repl, Head);
    break;
  case Random:
    {
      int bindex = myrand() & (cp->assoc - 1);
      repl = CACHE_BINDEX(cp, cp->sets[set].blks, bindex);
    }
    break;
  default:
    panic("bogus replacement policy");
  }

  /* remove this block from the hash bucket chain, if hash exists */
  if (cp->hsize)
    unlink_htab_ent(cp, &cp->sets[set], repl);

  /* blow away the last block to hit */
  cp->last_tagset = 0;
  cp->last_blk = NULL;

  /* write back replaced block data */
  if (repl->status & CACHE_BLK_VALID)
    {
      cp->replacements++;

      if (repl_addr)
 *repl_addr = CACHE_MK_BADDR(cp, repl->tag, set);

      /* track bus resource usage */
      cp->bus_free = MAX(cp->bus_free, (now + lat)) + 1;

   lat += cp->blk_access_fn(Write,
       CACHE_MK_BADDR(cp, repl->tag, set),
       cp->bsize, repl, now+lat);
 }


// Skipping code for hit handling..same as normal cache

  /* return latency of the operation */
  return lat;


}

// Function to fetch state bits of incoming block

byte_t    /* latency of access in cycles */
get_state(struct cache_t *cp, /* cache to access */
      enum mem_cmd cmd,  /* access type, Read or Write */
      md_addr_t addr,  /* address of access */
      void *vp,   /* ptr to buffer for input/output */
      int nbytes,  /* number of bytes to access */
      tick_t now,  /* time of access */
      byte_t **udata,  /* for return of user data ptr */ 
      md_addr_t *repl_addr) /* for address of replaced block */

{
  byte_t state = 0x00;
  byte_t *p = vp;
  md_addr_t tag = CACHE_TAG(cp, addr);
  md_addr_t set = CACHE_SET(cp, addr);
  md_addr_t bofs = CACHE_BLK(cp, addr);
  struct cache_blk_t *blk, *repl;
  //int lat = 0;

  /* default replacement address */
  if (repl_addr)
    *repl_addr = 0;

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    fatal("cache: access error: bad size or alignment, addr 0x%08x", addr);

  /* access must fit in cache block */
  /* FIXME:
     ((addr + (nbytes - 1)) > ((addr & ~cp->blk_mask) + (cp->bsize - 1))) */
  if ((addr + nbytes) > ((addr & ~cp->blk_mask) + cp->bsize))
    fatal("cache: access error: access spans block, addr 0x%08x", addr);

  /* permissions are checked on cache misses */

  /* check for a fast hit: access to same block */
  if (CACHE_TAGSET(cp, addr) == cp->last_tagset)
    {
      /* hit in the same block */
      blk = cp->last_blk;
      state = cp->last_blk->state;
      return state;
    }

  if (cp->hsize)
    {
      /* higly-associativity cache, access through the per-set hash tables */
      int hindex = CACHE_HASH(cp, tag);

      for (blk=cp->sets[set].hash[hindex];
    blk;
    blk=blk->hash_next)
 {
   if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
    {
       state = blk->state;
       return state;
    }
 }
    }
  else
    {
      /* low-associativity cache, linear search the way list */ 
      for (blk=cp->sets[set].way_head;
    blk;
    blk=blk->way_next)
 {
   if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
    {
       state = blk->state;
       return state;
    }
 }
    }
  return state;

}

// Function to writeback state bits on eviction

void    /* latency of access in cycles */
write_state(struct cache_t *cp, /* cache to access */
      enum mem_cmd cmd,  /* access type, Read or Write */
      md_addr_t addr,  /* address of access */
      int nbytes,  /* number of bytes to access */
      byte_t state) /* for address of replaced block */

{
  md_addr_t tag = CACHE_TAG(cp, addr);
  md_addr_t set = CACHE_SET(cp, addr);
  md_addr_t bofs = CACHE_BLK(cp, addr);
  struct cache_blk_t *blk, *repl;
 

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    fatal("cache: access error: bad size or alignment, addr 0x%08x", addr);

  /* access must fit in cache block */
  /* FIXME:
     ((addr + (nbytes - 1)) > ((addr & ~cp->blk_mask) + (cp->bsize - 1))) */
  if ((addr + nbytes) > ((addr & ~cp->blk_mask) + cp->bsize))
    fatal("cache: access error: access spans block, addr 0x%08x", addr);

  /* permissions are checked on cache misses */

  /* check for a fast hit: access to same block */
  if (CACHE_TAGSET(cp, addr) == cp->last_tagset)
    {
      /* hit in the same block */
      cp->last_blk->state = state;
      goto end;
    }

  if (cp->hsize)
    {
      /* higly-associativity cache, access through the per-set hash tables */
      int hindex = CACHE_HASH(cp, tag); 

      for (blk=cp->sets[set].hash[hindex];
    blk;
    blk=blk->hash_next)
 {
   if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
    {
       blk->state = state;
       break;
    }
    //goto cache_hit;
 }
    }
  else
    {
      /* low-associativity cache, linear search the way list */
      for (blk=cp->sets[set].way_head;
    blk;
    blk=blk->way_next)
 {
   if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
    {
       blk->state = state;
       break;
    }
 }
    }
  end:;
}
