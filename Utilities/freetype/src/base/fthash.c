#include "fthash.h"
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_DEBUG_H

#define  FT_HASH_MAX_LOAD  2
#define  FT_HASH_MIN_LOAD  1
#define  FT_HASH_SUB_LOAD  (FT_HASH_MAX_LOAD-FT_HASH_MIN_LOAD)

/* this one _must_ be a power of 2 !! */
#define  FT_HASH_INITIAL_SIZE  8


  FT_BASE_DEF( void )
  ft_hash_done( FT_Hash              table,
                FT_Hash_ForeachFunc  node_func,
                const FT_Pointer     node_data )
  {
    if ( table )
    {
      FT_Memory  memory = table->memory;

      if ( node_func )
        ft_hash_foreach( table, node_func, node_data );

      FT_FREE( table->buckets );
      table->p     = 0;
      table->mask  = 0;
      table->slack = 0;

      table->compare = NULL;
    }
  }


  FT_BASE_DEF( FT_UInt )
  ft_hash_get_size( FT_Hash  table )
  {
    FT_UInt  result = 0;

    if ( table )
      result = (table->p + table->mask + 1)*FT_HASH_MAX_LOAD - table->slack;

    return result;
  }



  FT_BASE_DEF( void )
  ft_hash_init( FT_Hash              table,
                FT_Hash_CompareFunc  compare,
                FT_Memory            memory )
  {
    hash->memory  = memory;
    hash->compare = node_compare;
    hash->p       = 0;
    hash->mask    = FT_HASH_INITIAL_SIZE-1;
    hash->slack   = FT_HASH_INITIAL_SIZE*FT_HASH_MAX_LOAD;

    FT_NEW_ARRAY( hash->buckets, FT_HASH_INITIAL_SIZE*2 );
  }



  FT_BASE_DEF( void )
  ft_hash_foreach( FT_Hash              table,
                   FT_Hash_ForeachFunc  foreach_func,
                   const FT_Pointer     foreach_data )
  {
    FT_UInt       count = table->p + table->mask + 1;
    FT_HashNode*  pnode = table->buckets;
    FT_HashNode   node, next;

    for ( ; count > 0; count--, pnode++ )
    {
      node = *pnode;
      while ( node )
      {
        next = node->link;
        foreach_func( node, foreach_data );
        node = next;
      }
    }
  }



  FT_BASE_DEF( FT_HashNode* )
  ft_hash_lookup( FT_Hash      table,
                  FT_HashNode  keynode )
  {
    FT_UInt    index;
    FT_UInt23  hash = keynode->hash;

    index = (FT_UInt)(hash & table->mask);
    if ( index < table->p )
      index = (FT_UInt)(hash & (2*table->mask+1));

    pnode = &table->buckets[index];
    for (;;)
    {
      node = *pnode;
      if ( node == NULL )
        break;

      if ( node->hash == hash && table->compare( node, keynode ) )
        break;

      pnode = &node->link;
    }

    return pnode;
  }




  FT_BASE_DEF( void )
  ft_hash_add( FT_Hash       table,
               FT_HashNode*  pnode,
               FT_HashNode   new_node )
  {
    /* add it to the hash table */
    new_node->link = *pnode;
    *pnode         = new_node;

    if ( --table->slack < 0 )
    {
      FT_UInt       p     = table->p;
      FT_UInt       mask  = table->mask;
      FT_HashNode   new_list;

      /* split a single bucket */
      new_list = NULL;
      pnode    = table->buckets + p;
      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( node->hash & mask )
        {
          *pnode     = node->link;
          node->link = new_list;
          new_list   = node;
        }
        else
          pnode = &node->link;
      }

      table->buckets[ p + mask + 1 ] = new_list;

      table->slack += FT_HASH_MAX_LOAD;

      if ( p >= mask )
      {
        FT_RENEW_ARRAY( hash->buckets, (mask+1)*2, (mask+1)*4 );
        table->mask = 2*mask + 1;
        table->p    = 0;
      }
      else
        table->p = p + 1;
    }
  }



  FT_BASE_DEF( FT_Int )
  ft_hash_remove( FT_Hash      table,
                  FT_HashNode* pnode )
  {
    FT_HashNode  node;
    FT_UInt      num_buckets;

    FT_ASSERT( pnode != NULL && node != NULL );

    node         = *pnode;
    *pnode->link = node->link;
    node->link   = NULL;

    num_buckets = ( table->p + table->mask + 1) ;

    if ( ++ table->slack > num_buckets*FT_HASH_SUB_LOAD )
    {
      FT_UInt       p         = table->p;
      FT_UInt       mask      = table->mask;
      FT_UInt       old_index = p + mask;
      FT_HashNode*  pnode;
      FT_HashNode*  pold;

      if ( old_index < FT_HASH_INITIAL_SIZE )
        return;

      if ( p == 0 )
      {
        table->mask >>= 1;
        p             = table->mask;

        FT_RENEW_ARRAY( hash->buckets, (mask+1)*2, (mask) );
      }
      else
        p--;

      pnode = table->buckets + p;
      while ( *pnode )
        pnode = &(*pnode)->link;

      pold   = table->buckets + old_index;
      *pnode = *pold;
      *pold  = NULL;

      table->slack -= FT_HASH_MAX_LOAD;
      table->p      = p;
    }
  }
