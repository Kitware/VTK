/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "H5PTprivate.h"
#include "H5TBprivate.h"
#include <stdlib.h>

/*  Packet Table private data */

typedef struct
{
  hid_t dset_id;  /* The ID of the dataset containing this table */
  hid_t type_id;  /* The ID of the packet table's native datatype */
  hsize_t current_index;  /* The index of the packet that get_next_packet will read next */
  hsize_t size;  /* The number of packets currently contained in this table */
} htbl_t;

static hsize_t H5PT_ptable_count = 0;
static H5I_type_t H5PT_ptable_id_type = H5I_UNINIT;

#define H5PT_HASH_TABLE_SIZE 64

/* Packet Table private functions */
static herr_t H5PT_close( htbl_t* table );
static herr_t H5PT_create_index(htbl_t *table_id);
static herr_t H5PT_set_index(htbl_t *table_id, hsize_t pt_index);
static herr_t H5PT_get_index(htbl_t *table_id, hsize_t *pt_index);

/*-------------------------------------------------------------------------
 *
 * Create/Open/Close functions
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: H5PTcreate_fl
 *
 * Purpose: Creates a dataset containing a table and returns the Identifier
 *          of the table.
 *
 * Return: Success: table ID, Failure: Negative
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments: This function does not handle fill data
 *           currently.  Fill data is not necessary because the
 *           table is initially of size 0.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

hid_t H5PTcreate_fl ( hid_t loc_id,
                              const char *dset_name,
                              hid_t dtype_id,
                              hsize_t chunk_size,
                              int compression )
{
  htbl_t * table = NULL;
  hid_t dset_id = H5I_BADID;
  hid_t space_id = H5I_BADID;
  hid_t plist_id = H5I_BADID;
  hsize_t dims[1];
  hsize_t dims_chunk[1];
  hsize_t maxdims[1];
  hid_t ret_value;

  /* Register the packet table ID type if this is the first table created */
  if(H5PT_ptable_id_type < 0)
    if((H5PT_ptable_id_type = H5Iregister_type((size_t)H5PT_HASH_TABLE_SIZE, 0, (H5I_free_t)free)) < 0)
      goto out;

  /* Get memory for the table identifier */
  table = (htbl_t *)malloc(sizeof(htbl_t));

  /* Create a simple data space with unlimited size */
  dims[0] = 0;
  dims_chunk[0] = chunk_size;
  maxdims[0] = H5S_UNLIMITED;
  if((space_id = H5Screate_simple(1, dims, maxdims)) < 0)
    goto out;

  /* Modify dataset creation properties to enable chunking  */
  plist_id = H5Pcreate(H5P_DATASET_CREATE);
  if(H5Pset_chunk(plist_id, 1, dims_chunk) < 0)
    goto out;
  if(compression >= 0 && compression <= 9)
    if(H5Pset_deflate(plist_id, (unsigned)compression) < 0)
        goto out;

  /* Create the dataset. */
  if((dset_id = H5Dcreate2(loc_id, dset_name, dtype_id, space_id, H5P_DEFAULT, plist_id, H5P_DEFAULT)) < 0)
    goto out;

  /* Terminate access to the data space. */
  if(H5Sclose(space_id) < 0)
    goto out;

  /* End access to the property list */
  if(H5Pclose(plist_id) < 0)
    goto out;

  /* Create the table identifier */
  table->dset_id = dset_id;

  if((table->type_id = H5Tcopy(dtype_id)) < 0)
    goto out;

  H5PT_create_index(table);
  table->size = 0;

  /* Get an ID for this table */
  ret_value = H5Iregister(H5PT_ptable_id_type, table);

  if(ret_value != H5I_INVALID_HID)
    H5PT_ptable_count++;
  else
    H5PT_close(table);

  return ret_value;

  out:
    H5E_BEGIN_TRY
    H5Sclose(space_id);
    H5Pclose(plist_id);
    H5Dclose(dset_id);
    if(table)
      free(table);
    H5E_END_TRY
    return H5I_INVALID_HID;
}

#ifdef H5_VLPT_ENABLED
/*-------------------------------------------------------------------------
 * Function: H5PTcreate_vl
 *
 * Purpose: Creates a dataset containing a table of variable length records
 *          and returns the Identifier of the table.
 *
 * Return: Success: table ID, Failure: Negative
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: April 12, 2004
 *
 * Comments: This function does not handle compression or fill data
 *           currently.  Fill data is not necessary because the
 *           table is initially of size 0.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t H5PTcreate_vl ( hid_t loc_id,
                                 const char*dset_name,
                                 hsize_t chunk_size)
{
  hid_t ret_value=H5I_BADID;
  hid_t vltype;

  /* Create a variable length type that uses single bytes as its base type */
  vltype = H5Tvlen_create(H5T_NATIVE_UCHAR);
  if(vltype < 0)
    goto out;

  if((ret_value=H5PTcreate_fl(loc_id, dset_name, vltype, chunk_size, 0)) < 0)
    goto out;

  /* close the vltype */
  if(H5Tclose(vltype) < 0)
    goto out;

  return ret_value;

out:
  if(ret_value != H5I_BADID)
    H5PTclose(ret_value);
  return H5I_BADID;
}
#endif /* H%_VLPT_ENABLED */

/*-------------------------------------------------------------------------
 * Function: H5PTopen
 *
 * Purpose: Opens a dataset containing a table and returns the Identifier
 *          of the table.
 *
 * Return: Success: table ID, Failure: Negative
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 10, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 * 		John Mainzer -- 4/23/08
 * 		Added error check on malloc of table, initialized fields
 * 		in table to keep lower level code from choking on bogus
 * 		data in error cases.
 *
 *-------------------------------------------------------------------------
 */
hid_t H5PTopen( hid_t loc_id,
                             const char *dset_name )
{
  hid_t type_id=H5I_BADID;
  hid_t space_id=H5I_BADID;
  htbl_t * table = NULL;
  hid_t ret_value;
  hsize_t dims[1];

  /* Register the packet table ID type if this is the first table created */
  if( H5PT_ptable_id_type < 0)
    if((H5PT_ptable_id_type = H5Iregister_type((size_t)H5PT_HASH_TABLE_SIZE, 0, (H5I_free_t)free)) < 0)
      goto out;

  table = (htbl_t *)malloc(sizeof(htbl_t));

  if ( table == NULL ) {
    goto out;
  }
  table->dset_id = H5I_BADID;
  table->type_id = H5I_BADID;

  /* Open the dataset */
  if((table->dset_id = H5Dopen2(loc_id, dset_name, H5P_DEFAULT)) < 0)
      goto out;
  if(table->dset_id < 0)
    goto out;

  /* Get the dataset's disk datatype */
  if((type_id = H5Dget_type(table->dset_id)) < 0)
    goto out;

  /* Get the table's native datatype */
  if((table->type_id = H5Tget_native_type(type_id, H5T_DIR_ASCEND)) < 0)
    goto out;

  if(H5Tclose(type_id) < 0)
    goto out;

  /* Initialize the current record pointer */
  if((H5PT_create_index(table)) < 0)
    goto out;

  /* Get number of records in table */
  if((space_id=H5Dget_space(table->dset_id)) < 0)
    goto out;
  if( H5Sget_simple_extent_dims( space_id, dims, NULL) < 0)
    goto out;
  if(H5Sclose(space_id) < 0)
    goto out;
  table->size = dims[0];

  /* Get an ID for this table */
  ret_value = H5Iregister(H5PT_ptable_id_type, table);

  if(ret_value != H5I_INVALID_HID)
    H5PT_ptable_count++;
  else
    H5PT_close(table);

  return ret_value;

out:
  H5E_BEGIN_TRY
  H5Tclose(type_id);
  H5Sclose(space_id);
  if(table)
  {
    H5Dclose(table->dset_id);
    H5Tclose(table->type_id);
    free(table);
  }
  H5E_END_TRY
  return H5I_INVALID_HID;
}


/*-------------------------------------------------------------------------
 * Function: H5PT_close
 *
 * Purpose: Closes a table (i.e. cleans up all open resources used by a
 *          table).
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 10, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PT_close( htbl_t* table)
{
  if(table == NULL)
    goto out;

  /* Close the dataset */
  if(H5Dclose(table->dset_id) < 0)
    goto out;

  /* Close the memory datatype */
  if(H5Tclose(table->type_id) < 0)
    goto out;

  free(table);

  return 0;

out:
  if(table)
  {
    H5E_BEGIN_TRY
    H5Dclose(table->dset_id);
    H5Tclose(table->type_id);
    H5E_END_TRY
    free(table);
  }
  return -1;
}

/*-------------------------------------------------------------------------
 * Function: H5PTclose
 *
 * Purpose: Closes a table (i.e. cleans up all open resources used by a
 *          table).
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: April 21, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTclose( hid_t table_id )
{
  htbl_t * table;

  /* Remove the ID from the library */
  if((table = H5Iremove_verify(table_id, H5PT_ptable_id_type)) ==NULL)
    goto out;

  /* If the library found the table, remove it */
  if( H5PT_close(table) < 0)
    goto out;

  /* One less packet table open */
  H5PT_ptable_count--;

  /* Remove the packet table type ID if no more packet */
  /* tables are open                                   */
  if(H5PT_ptable_count == 0)
  {
    H5Idestroy_type(H5PT_ptable_id_type);
    H5PT_ptable_id_type = H5I_UNINIT;
  }

  return 0;

out:
  return -1;
}


/*-------------------------------------------------------------------------
 *
 * Write functions
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: H5PTappend
 *
 * Purpose: Appends packets to the end of a packet table
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTappend( hid_t table_id,
                           size_t nrecords,
                           const void * data )
{
  htbl_t * table;

  /* Find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    goto out;

  /* If we are asked to write 0 records, just do nothing */
  if(nrecords == 0)
    return 0;

  if((H5TB_common_append_records(table->dset_id, table->type_id,
  			nrecords, table->size, data)) < 0)
    goto out;

  /* Update table size */
  table->size += nrecords;
  return 0;

out:
  return -1;
}

/*-------------------------------------------------------------------------
 *
 * Read functions
 *
 *-------------------------------------------------------------------------
 */


/*-------------------------------------------------------------------------
 * Function: H5PTget_next
 *
 * Purpose: Reads packets starting at the current index and updates
 *          that index
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 10, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTget_next( hid_t table_id,
                             size_t nrecords,
                             void * data)
{
  htbl_t * table;

  /* Find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    goto out;

  /* If nrecords == 0, do nothing */
  if(nrecords == 0)
    return 0;

  if((H5TB_common_read_records(table->dset_id, table->type_id,
                              table->current_index, nrecords, table->size, data)) < 0)
    goto out;

  /* Update the current index */
  table->current_index += nrecords;
  return 0;

out:
  return -1;
}

/*-------------------------------------------------------------------------
 * Function: H5PTread_packets
 *
 * Purpose: Reads packets from anywhere in a packet table
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTread_packets( hid_t table_id,
                         hsize_t start,
                         size_t nrecords,
                         void *data)
{
  htbl_t * table;

  /* find the table struct from its ID */
  table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type);
  if(table == NULL)
    goto out;

  /* If nrecords == 0, do nothing */
  if(nrecords == 0)
    return 0;

  if( H5TB_common_read_records(table->dset_id, table->type_id,
                              start, nrecords, table->size, data) < 0)
    goto out;

  return 0;

out:
  return -1;
}

/*-------------------------------------------------------------------------
 *
 * Table attribute functions
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: H5PT_create_index, H5PT_set_index, H5PT_get_index
 *
 * Purpose: Resets, sets, and gets the current record index for a packet table
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *  		   James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PT_create_index(htbl_t *table)
{
  if( table != NULL)
  {
    table->current_index = 0;
    return 0;
  }
  return -1;
}

static herr_t
H5PT_set_index(htbl_t *table, hsize_t index)
{
  /* Ensure index is valid */
  if( table != NULL )
  {
    if( index < table->size )
    {
      table->current_index = index;
      return 0;
    }
  }
  return -1;
}

static herr_t
H5PT_get_index(htbl_t *table, hsize_t *index)
{
  /* Ensure index is valid */
  if( table != NULL )
  {
    if(index)
      *index = table->current_index;
    return 0;
  }
  return -1;
}

/*-------------------------------------------------------------------------
 * Function: H5PTcreate_index, H5PTset_index, H5PTget_index
 *
 * Purpose: Resets, sets, and gets the current record index for a packet table
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: April 23, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTcreate_index(hid_t table_id)
{
  htbl_t * table;

  /* find the table struct from its ID */
  if((table = (htbl_t *) (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    return -1;

  return H5PT_create_index(table);
}

herr_t H5PTset_index(hid_t table_id, hsize_t pt_index)
{
  htbl_t * table;

  /* find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    return -1;

  return H5PT_set_index(table, pt_index);
}

herr_t H5PTget_index(hid_t table_id, hsize_t *pt_index)
{
  htbl_t * table;

  /* find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    return -1;

  return H5PT_get_index(table, pt_index);
}

/*-------------------------------------------------------------------------
 *
 * Inquiry functions
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: H5PTget_num_packets
 *
 * Purpose: Returns by reference the number of packets in the dataset
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTget_num_packets( hid_t table_id, hsize_t *nrecords)
{
  htbl_t * table;

  /* find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    goto out;

  if(nrecords)
    *nrecords = table->size;

  return 0;
out:
  return -1;
}


/*-------------------------------------------------------------------------
 * Function: H5PTis_valid
 *
 * Purpose: Validates a table identifier
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: March 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTis_valid(hid_t table_id)
{
  /* find the table struct from its ID */
  if(H5Iobject_verify(table_id, H5PT_ptable_id_type) ==NULL)
    return -1;

  return 0;
}

#ifdef H5_VLPT_ENABLED
/*-------------------------------------------------------------------------
 * Function: H5PTis_varlen
 *
 * Purpose: Returns 1 if a table_id corresponds to a packet table of variable-
 *          length records or 0 for fixed-length records.
 *
 * Return: True: 1, False: 0, Failure: -1
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: April 14, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */
herr_t H5PTis_varlen(hid_t table_id)
{
  H5T_class_t type;
  htbl_t * table;

  /* find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    goto out;

  if((type = H5Tget_class( table->type_id )) == H5T_NO_CLASS)
    goto out;

  if( type == H5T_VLEN )
    return 1;
  else
    return 0;
out:
  return -1;
}

/*-------------------------------------------------------------------------
 *
 * Memory Management functions
 *
 *-------------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 * Function: H5PTfree_vlen_readbuff
 *
 * Purpose: Frees memory used when reading from a variable length packet
 *          table.
 *
 * Return: Success: 0, Failure: -1
 *          -2 if memory was reclaimed but another error occurred
 *
 * Programmer: Nat Furrer, nfurrer@ncsa.uiuc.edu
 *             James Laird, jlaird@ncsa.uiuc.edu
 *
 * Date: April 12, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5PTfree_vlen_readbuff( hid_t table_id,
                               size_t _bufflen,
                               void * buff )
{
  hid_t space_id = H5I_BADID;
  htbl_t * table;
  hsize_t bufflen = _bufflen;
  herr_t ret_value;

  /* find the table struct from its ID */
  if((table = (htbl_t *) H5Iobject_verify(table_id, H5PT_ptable_id_type)) == NULL)
    goto out;

  if((space_id = H5Screate_simple(1, &bufflen, NULL)) < 0)
    goto out;

  /* Free the memory.  If this succeeds, ret_value should be 0. */
  if((ret_value = H5Dvlen_reclaim(table->type_id, space_id, H5P_DEFAULT, buff)) < 0)
    goto out;

  /* If the dataspace cannot be closed, return -2 to indicate that memory */
  /* was freed successfully but an error still occurred. */
  if(H5Sclose(space_id) < 0)
    return -2;

  return ret_value;

out:
  H5E_BEGIN_TRY
    H5Sclose(space_id);
  H5E_END_TRY
  return -1;
}

#endif /* H5_VLPT_ENABLED */
