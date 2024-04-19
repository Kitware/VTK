/*!
  \defgroup h5block_c_api H5Block C API
*/

/*!
  \internal

  \defgroup h5block_kernel H5Block Kernel
*/

/*!
  \internal

  \defgroup h5block_private H5Block Private
*/

/*!
  \ingroup h5block_c_api
  \defgroup h5block_model	Setting up the Data Model
*/  
/*!
  \ingroup h5block_c_api
  \defgroup h5block_data	Reading and Writing Datasets
*/  
/*!
  \ingroup h5block_c_api
  \defgroup h5block_attrib	Reading and Writing Attributes
*/

/*!
  \note
  Different field sizes are allowed in the same time-step.

  \note
  The same layout can be used, if the size of the field matches the
  size of the layout.  If the size of the layout doesn't match the
  size of the field, an error will be indicated. 
 
  \note
  In write mode partitions are shrinked to make them non-overlaping. This 
  process may shrink the partitions more than required.

  \note
  In read-mode partitions may not cross boundaries. This means, if the grid
  size is (X, Y, Z), all partitions must fit into this grid.


  \todo
  check whether layout is reasonable

  API function names
*/


#include <stdlib.h>
#include <string.h>

#include <vtk_hdf5.h>
#include "H5Part.h"
#include "H5PartErrors.h"
#include "H5PartPrivate.h"

#include "H5BlockTypes.h"
#include "H5Block.h"
#include "H5BlockPrivate.h"
#include "H5BlockErrors.h"

/********************** declarations *****************************************/


/********************** misc *************************************************/

/*!
  \ingroup h5block_private

  \internal

  Check whether \c f points to a valid file handle.

  \return	H5PART_SUCCESS or error code
*/

static h5part_int64_t
_H5Block_file_is_valid (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	if ( f == NULL )
		return H5PART_ERR_BADFD;
	if ( f->file == 0 )
		return H5PART_ERR_BADFD;
	if ( f->block == NULL )
		return H5PART_ERR_BADFD;
	return H5PART_SUCCESS;
}


/********************** file open and close **********************************/

/*!
  \ingroup h5block_private

  \internal

  Initialize H5Block internal structure.

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_init (
	H5PartFile *f			/*!< IN: file handle */
	) {

	h5part_int64_t herr;
	struct H5BlockStruct *b; 

	herr = _H5Block_file_is_valid ( f );
	if ( herr == H5PART_SUCCESS ) return H5PART_SUCCESS;

	if ( (f == 0) || (f->file == 0) ) return HANDLE_H5PART_BADFD_ERR;

	/*
	  hack for non-parallel processing, should be set in H5Part
	*/
	if ( f->nprocs == 0 ) f->nprocs = 1;

	f->block = (struct H5BlockStruct*) malloc( sizeof (*f->block) );
	if ( f->block == NULL ) {
		return HANDLE_H5PART_NOMEM_ERR;
	}
	b = f->block;
	memset ( b, 0, sizeof (*b) );
	b->user_layout = (struct H5BlockPartition*) malloc (
		f->nprocs * sizeof (b->user_layout[0]) );
	if ( b->user_layout == NULL ) {
		return HANDLE_H5PART_NOMEM_ERR;
	}
	b->write_layout = (struct H5BlockPartition*) malloc (
		f->nprocs * sizeof (b->write_layout[0]) );
	if ( b->write_layout == NULL ) {
		return HANDLE_H5PART_NOMEM_ERR;
	}
	b->timestep = -1;
	b->blockgroup = -1;
	b->shape = -1;
	b->diskshape = -1;
	b->memshape = -1;
	b->field_group_id = -1;
	b->have_layout = 0;
	b->chunk[0] = 0;
	b->chunk[1] = 0;
	b->chunk[2] = 0;

	b->create_prop = H5Pcreate ( H5P_DATASET_CREATE );
	if ( b->create_prop < 0 ) return HANDLE_H5P_CREATE_ERR;

	f->close_block = _H5Block_close;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  De-initialize H5Block internal structure.  Open HDF5 objects are 
  closed and allocated memory freed.

  \return	H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_close (
	H5PartFile *f		/*!< IN: file handle */
	) {

	herr_t herr;
	struct H5BlockStruct *b = f->block;

	if ( b->blockgroup >= 0 ) {
		herr = H5Gclose ( b->blockgroup );
		if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
		b->blockgroup = -1;
	}
	if ( b->shape >= 0 ) {
		herr = H5Sclose ( b->shape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		b->shape = -1;
	}
	if ( b->diskshape >= 0 ) {
		herr = H5Sclose ( b->diskshape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		b->diskshape = -1;
	}
	if ( b->memshape >= 0 ) {
		herr = H5Sclose ( b->memshape );
		if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
		b->memshape = -1;
	}

	herr = H5Pclose ( b->create_prop );
	if ( herr < 0 ) return HANDLE_H5P_CLOSE_ERR ( "create_prop" );

	free ( f->block );

	f->block = NULL;
	f->close_block = NULL;

	return H5PART_SUCCESS;
}

/********************** defining the layout **********************************/

/*!
  \note
  A partition must not be part of another partition.

  A partition must not divide another partition into two pieces.

  After handling the ghost zones, the partition must not be empty

  We must track the overall size somewhere. This is a good place to do it. (?)
*/

/*!
  \ingroup h5block_private

  \internal

  Normalize partition.

  \e means that the start coordinates are less or equal the
  end coordinates.
*/
static void
_normalize_partition (
	struct H5BlockPartition *p	/*!< IN/OUT: partition */
	) {
	h5part_int64_t x;

	if ( p->i_start > p->i_end ) {
		x = p->i_start;
		p->i_start = p->i_end;
		p->i_end = x;
	}
	if ( p->j_start > p->j_end ) {
		x = p->j_start;
		p->j_start = p->j_end;
		p->j_end = x;
	}
	if ( p->k_start > p->k_end ) {
		x = p->k_start;
		p->k_start = p->k_end;
		p->k_end = x;
	}
}

/*!
  \ingroup h5block_private

  \internal

  Gather layout to all processors

  \return	H5PART_SUCCESS or error code
*/
#ifdef PARALLEL_IO
static h5part_int64_t
_allgather (
	const H5PartFile *f		/*!< IN: file handle */
	) {
	struct H5BlockPartition *layout = f->block->user_layout;

	MPI_Datatype    partition_m;
	size_t n = sizeof (struct H5BlockPartition) / sizeof (h5part_int64_t);

	MPI_Type_contiguous ( n, MPI_LONG_LONG, &partition_m );
	MPI_Type_commit ( &partition_m );

	MPI_Allgather ( MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
			layout, 1, partition_m, f->comm );

	return H5PART_SUCCESS;
}
#else
static h5part_int64_t
_allgather (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	return H5PART_SUCCESS;
}
#endif

/*!
  \ingroup h5block_private

  \internal

  Get dimension sizes of block.  These informations are stored inside the
  block structure.
*/
static void
_get_dimension_sizes (
	H5PartFile *f			/*!< IN: file handle */
	) {
	int proc;
	struct H5BlockStruct *b = f->block;
	struct H5BlockPartition *partition = b->user_layout;

	b->i_max = 0;
	b->j_max = 0;
	b->k_max = 0;

	for ( proc = 0; proc < f->nprocs; proc++, partition++ ) {
		if ( partition->i_end > b->i_max ) b->i_max = partition->i_end;
		if ( partition->j_end > b->j_max ) b->j_max = partition->j_end;
		if ( partition->k_end > b->k_max ) b->k_max = partition->k_end;
	}
}

#define _NO_GHOSTZONE(p,q) ( (p->i_end < q->i_start) \
  			  ||   (p->j_end < q->j_start) \
			  ||   (p->k_end < q->k_start) )


/*!
  \ingroup h5block_private

  \internal

  Check whether two partitions have a common ghost-zone.

  \return value != \c 0 if yes otherwise \c 0
*/
static int
_have_ghostzone (
	const struct H5BlockPartition *p,	/*!< IN: partition \c p */
	const struct H5BlockPartition *q	/*!< IN: partition \c q */
	) {
	return ( ! ( _NO_GHOSTZONE ( p, q ) || _NO_GHOSTZONE ( q, p ) ) );
}

/*!
  \ingroup h5block_private

  \internal

  Calculate volume of partition.

  \return volume
*/
static h5part_int64_t
_volume_of_partition (
	const struct H5BlockPartition *p	/*!< IN: partition */
	) {
	return (p->i_end - p->i_start)
		* (p->j_end - p->j_start)
		* (p->k_end - p->k_start);

}

#define MIN( x, y ) ( (x) <= (y) ? (x) : (y) )  
#define MAX( x, y ) ( (x) >= (y) ? (x) : (y) )  

/*!
  \ingroup h5block_private

  \internal

  Calc volume of ghost-zone.

  \return volume
*/
static h5part_int64_t
_volume_of_ghostzone (
	const struct H5BlockPartition *p, /*!< IN: ptr to first partition */
	const struct H5BlockPartition *q  /*!< IN: ptr to second partition */
	) {

	h5part_int64_t dx = MIN ( p->i_end, q->i_end )
		- MAX ( p->i_start, q->i_start ) + 1;
	h5part_int64_t dy = MIN ( p->j_end, q->j_end )
		- MAX ( p->j_start, q->j_start ) + 1;
	h5part_int64_t dz = MIN ( p->k_end, q->k_end )
		- MAX ( p->k_start, q->k_start ) + 1;

	return dx * dy * dz;
}

/*!
  \ingroup h5block_private

  \internal

  Dissolve ghost-zone by moving the X coordinates.  Nothing will be changed
  if \c { p->i_start <= q->i_end <= p->i_end }.  In this case \c -1 will be
  returned.

  \return H5PART_SUCCESS or -1
*/
static h5part_int64_t
_dissolve_X_ghostzone (
	struct H5BlockPartition *p,	/*!< IN/OUT: ptr to first partition */
	struct H5BlockPartition *q	/*!< IN/OUT: ptr to second partition */
	) {

	if ( p->i_start > q->i_start )
		return _dissolve_X_ghostzone( q, p );

	if ( q->i_end <= p->i_end )  /* no dissolving		*/
		return -1;

	p->i_end = ( p->i_end + q->i_start ) >> 1;
	q->i_start = p->i_end + 1;
	return 0;
}

/*!
  \ingroup h5block_private

  \internal

  Dissolve ghost-zone by moving the Y coordinates.  Nothing will be changed
  if \c { p->j_start <= q->j_end <= p->j_end }.  In this case \c -1 will be
  returned.

  \return H5PART_SUCCESS or -1
*/
static h5part_int64_t
_dissolve_Y_ghostzone (
	struct H5BlockPartition *p,	/*!< IN/OUT: ptr to first partition */
	struct H5BlockPartition *q	/*!< IN/OUT: ptr to second partition */
	) {

	if ( p->j_start > q->j_start )
		return _dissolve_Y_ghostzone( q, p );

	if ( q->j_end <= p->j_end )    /* no dissolving		*/
		return -1;

	p->j_end = ( p->j_end + q->j_start ) >> 1;
	q->j_start = p->j_end + 1;
	return 0;
}

/*!
  \ingroup h5block_private

  \internal

  Dissolve ghost-zone by moving the Z coordinates.  Nothing will be changed
  if \c { p->k_start <= q->k_end <= p->k_end }.  In this case \c -1 will be
  returned.

  \return H5PART_SUCCESS or -1
*/
static h5part_int64_t
_dissolve_Z_ghostzone (
	struct H5BlockPartition *p,	/*!< IN/OUT: ptr to first partition */
	struct H5BlockPartition *q	/*!< IN/OUT: ptr to second partition */
	) {

	if ( p->k_start > q->k_start )
		return _dissolve_Z_ghostzone( q, p );

	if ( q->k_end <= p->k_end )    /* no dissolving		*/
		return -1;

	p->k_end = ( p->k_end + q->k_start ) >> 1;
	q->k_start = p->k_end + 1;
	return 0;
}

/*!
  \ingroup h5block_private

  \internal

  Dissolve ghost-zone for partitions \p and \q.

  Dissolving is done by moving either the X, Y or Z plane.  We never move
  more than one plane per partition.  Thus we always have three possibilities
  to dissolve the ghost-zone.  The "best" is the one with the largest
  remaining volume of the partitions.

  \return H5PART_SUCCESS or error code.
*/
static h5part_int64_t
_dissolve_ghostzone (
	struct H5BlockPartition *p,	/*!< IN/OUT: ptr to first partition */
	struct H5BlockPartition *q	/*!< IN/OUT: ptr to second partition */
	) {

	struct H5BlockPartition p_;
	struct H5BlockPartition q_;
	struct H5BlockPartition p_best;
	struct H5BlockPartition q_best;
	h5part_int64_t vol;
	h5part_int64_t max_vol = 0;

	p_ = *p;
	q_ = *q;
	if ( _dissolve_X_ghostzone ( &p_, &q_ ) == 0 ) {
		vol = _volume_of_partition ( &p_ ) 
			+ _volume_of_partition ( &q_ );
		if ( vol > max_vol ) {
			max_vol = vol;
			p_best = p_;
			q_best = q_;
		}
	}

	p_ = *p;
	q_ = *q;
	if ( _dissolve_Y_ghostzone ( &p_, &q_ ) == 0 ) {
		vol = _volume_of_partition ( &p_ )
			+ _volume_of_partition ( &q_ );
		if ( vol > max_vol ) {
			max_vol = vol;
			p_best = p_;
			q_best = q_;
		}
	}
	p_ = *p;
	q_ = *q;

	if ( _dissolve_Z_ghostzone ( &p_, &q_ ) == 0 ) {
		vol = _volume_of_partition ( &p_ )
			+ _volume_of_partition ( &q_ );
		if ( vol > max_vol ) {
			max_vol = vol;
			p_best = p_;
			q_best = q_;
		}
	}
	if ( max_vol <= 0 ) {
		return H5PART_ERR_LAYOUT;
	}
	*p = p_best;
	*q = q_best;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  Dissolve all ghost-zones.

  Ghost-zone are dissolved in the order of their magnitude, largest first.

  \note
  Dissolving ghost-zones automaticaly is not trivial!  The implemented 
  algorithmn garanties, that there are no ghost-zones left and that we
  have the same result on all processors.
  But there may be zones which are not assigned to a partition any more.
  May be we should check this and return an error in this case.  Then
  the user have to decide to continue or to abort.

  \b {Error Codes}
  \b H5PART_NOMEM_ERR

  \return H5PART_SUCCESS or error code.
*/
static h5part_int64_t
_dissolve_ghostzones (
	H5PartFile *f	/*!< IN: file handle */
	) {

	struct H5BlockStruct *b = f->block;
	struct H5BlockPartition *p;
	struct H5BlockPartition *q;
	int proc_p, proc_q;

	struct list {
		struct list *prev;
		struct list *next;
		struct H5BlockPartition *p;
		struct H5BlockPartition *q;
		h5part_int64_t vol;
	} *p_begin, *p_el, *p_max, *p_end, *p_save;

	memcpy ( b->write_layout, b->user_layout,
		 f->nprocs * sizeof (*f->block->user_layout) );

	p_begin = p_max = p_end = (struct list*) malloc ( sizeof ( *p_begin ) );
	if ( p_begin == NULL ) return HANDLE_H5PART_NOMEM_ERR;
	
	memset ( p_begin, 0, sizeof ( *p_begin ) );

	for ( proc_p = 0, p = b->write_layout;
		proc_p < f->nprocs-1;
		proc_p++, p++ ) {
		for ( proc_q = proc_p+1, q = &b->write_layout[proc_q];
			proc_q < f->nprocs;
			proc_q++, q++ ) {

			if ( _have_ghostzone ( p, q ) ) {
				p_el = (struct list*) malloc ( sizeof ( *p_el ) );
				if ( p_el == NULL )
					return HANDLE_H5PART_NOMEM_ERR;

				p_el->p = p;
				p_el->q = q;
				p_el->vol = _volume_of_ghostzone ( p, q );
				p_el->prev = p_end;
				p_el->next = NULL;
				
				if ( p_el->vol > p_max->vol )
					p_max = p_el;

				p_end->next = p_el;
				p_end = p_el;
			}
		}
	}
	while ( p_begin->next ) {
		if ( p_max->next ) p_max->next->prev = p_max->prev;
		p_max->prev->next = p_max->next;
		
		_dissolve_ghostzone ( p_max->p, p_max->q );

		free ( p_max );
		p_el = p_max = p_begin->next;

		while ( p_el ) {
			if ( _have_ghostzone ( p_el->p, p_el->q ) ) {
				p_el->vol = _volume_of_ghostzone ( p_el->p, p_el->q );
				if ( p_el->vol > p_max->vol )
					p_max = p_el;
				p_el = p_el->next;
			} else {
				if ( p_el->next )
					p_el->next->prev = p_el->prev;
				p_el->prev->next = p_el->next;
				p_save = p_el->next;
				free ( p_el );
				p_el = p_save;
			}
		}

	}
	free ( p_begin );

	p = &b->user_layout[f->myproc];
	_H5Part_print_debug (
		"PROC[%d]: User layout: %lld:%lld, %lld:%lld, %lld:%lld",
		f->myproc,
		(long long)p->i_start, (long long)p->i_end,
		(long long)p->j_start, (long long)p->j_end,
		(long long)p->k_start, (long long)p->k_end );
	/* more detailed debug output: all procs report their view
	   of all other procs */
	for ( proc_p = 0, p = b->user_layout;
		proc_p < f->nprocs;
		proc_p++, p++ ) {
		_H5Part_print_debug_detail (
			"PROC[%d]: proc[%d]: User layout: %lld:%lld, %lld:%lld, %lld:%lld  ",
			f->myproc, proc_p,
			(long long)p->i_start, (long long)p->i_end,
			(long long)p->j_start, (long long)p->j_end,
			(long long)p->k_start, (long long)p->k_end );
	}

	p = &b->write_layout[f->myproc];
	_H5Part_print_debug (
		"PROC[%d]: Ghost-zone layout: %lld:%lld, %lld:%lld, %lld:%lld",
		f->myproc,
		(long long)p->i_start, (long long)p->i_end,
		(long long)p->j_start, (long long)p->j_end,
		(long long)p->k_start, (long long)p->k_end );
	/* more detailed debug output: all procs report their view
	   of all other procs */
	for ( proc_p = 0, p = b->write_layout;
		proc_p < f->nprocs;
		proc_p++, p++ ) {
		_H5Part_print_debug_detail (
			"PROC[%d]: proc[%d]: Ghost-zone layout: %lld:%lld, %lld:%lld, %lld:%lld  ",
			f->myproc, proc_p,
			(long long)p->i_start, (long long)p->i_end,
			(long long)p->j_start, (long long)p->j_end,
			(long long)p->k_start, (long long)p->k_end );
	}
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

*/
static h5part_int64_t
_release_hyperslab (
	H5PartFile *f			/*!< IN: file handle */
	) {
	herr_t herr;

	if ( f->block->shape > 0 ) {
		herr = H5Sclose ( f->block->shape );
		if ( herr < 0 ) return H5PART_ERR_HDF5;
		f->block->shape = -1;
	}
	if ( f->block->diskshape > 0 ) {
		herr = H5Sclose ( f->block->diskshape );
		if ( herr < 0 ) return H5PART_ERR_HDF5;
		f->block->diskshape = -1;
	}
	if ( f->block->memshape > 0 ) {
		herr = H5Sclose ( f->block->memshape );
		if ( herr < 0 ) return H5PART_ERR_HDF5;
		f->block->memshape = -1;
	}
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Defines the partition of the field that this processor owns, using
  Fortran ordering: the fastest moving index is \c i.

  This routine uses an MPI_Allgather, so at large concurrency it should
  be called as infrequently as possible. For instance, if several timesteps
  use the same field dimensions, set the layout only once before the
  first timestep.

  \return \c H5PART_SUCCESS on success<br>
  \c H5PART_ERR_MPI<br>
  \c H5PART_ERR_HDF5
*/
h5part_int64_t
H5BlockDefine3DFieldLayout(
	H5PartFile *f,			/*!< IN: File handle		*/
	const h5part_int64_t i_start,	/*!< IN: start index of \c i	*/ 
	const h5part_int64_t i_end,	/*!< IN: end index of \c i	*/  
	const h5part_int64_t j_start,	/*!< IN: start index of \c j	*/ 
	const h5part_int64_t j_end,	/*!< IN: end index of \c j	*/ 
	const h5part_int64_t k_start,	/*!< IN: start index of \c k	*/ 
	const h5part_int64_t k_end	/*!< IN: end index of \c k	*/
	) {

	SET_FNAME ( "H5BlockDefine3DFieldLayout" );
	BLOCK_INIT( f );

	struct H5BlockStruct *b = f->block;
	struct H5BlockPartition *p = &b->user_layout[f->myproc];
	p->i_start = i_start;
	p->i_end =   i_end;
	p->j_start = j_start;
	p->j_end =   j_end;
	p->k_start = k_start;
	p->k_end =   k_end;

	_normalize_partition ( p );

	h5part_int64_t herr = _allgather ( f );
	if ( herr < 0 ) return HANDLE_MPI_ALLGATHER_ERR;

	_get_dimension_sizes ( f );

	herr = _dissolve_ghostzones ( f );
	if ( herr < 0 ) return HANDLE_H5PART_LAYOUT_ERR;

	herr = _release_hyperslab ( f );
	if ( herr < 0 )	return HANDLE_H5S_CLOSE_ERR;

	b->have_layout = 1;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Define the chunk dimensions and enable chunking in the underlying
  HDF5 dataset.

  \return \c H5PART_SUCCESS on success
*/
h5part_int64_t
H5BlockDefine3DChunkDims(
	H5PartFile *f,			/*!< IN: File handle */
	const h5part_int64_t i,		/*!< IN: size of \c i */ 
	const h5part_int64_t j,		/*!< IN: size of \c j */  
	const h5part_int64_t k		/*!< IN: size of \c k */ 
	) {

	SET_FNAME ( "H5BlockDefine3DChunkDims" );
	BLOCK_INIT( f );

	struct H5BlockStruct *b = f->block;

	b->chunk[0] = (hsize_t)k;
	b->chunk[1] = (hsize_t)j;
	b->chunk[2] = (hsize_t)i;

	_H5Part_print_debug (
		"PROC[%d]: Chunk dimensions: (%lld,%lld,%lld)",
		f->myproc,
		(long long)b->chunk[2],
		(long long)b->chunk[1],
		(long long)b->chunk[0] );

	herr_t herr = H5Pset_chunk ( b->create_prop, 3, b->chunk );
	if ( herr < 0 ) return HANDLE_H5P_SET_CHUNK_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Lookup the chunk dimensions of the underlying HDF5 dataset.

  \return \c H5PART_SUCCESS on success
*/
h5part_int64_t
H5BlockGet3DChunkDims(
	H5PartFile *f,			/*!< IN: File handle */
	const char *field_name, 	/*!< IN: name of dataset */
	h5part_int64_t *dims    	/*!< OUT: array containing
						  the chunk dimensions */
	) {

	SET_FNAME ( "H5BlockGet3DChunkDims" );
	BLOCK_INIT( f );

	struct H5BlockStruct *b = f->block;

	CHECK_TIMEGROUP ( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	hid_t dataset_id = H5Dopen ( b->field_group_id, "0"
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( field_name );

	hid_t plist_id = H5Dget_create_plist (dataset_id);
	if ( plist_id < 0 ) return HANDLE_H5D_GET_PLIST_ERR;

	hsize_t cdims[3];
	int ndim = H5Pget_chunk ( plist_id, 3, cdims );
	if ( ndim != 3 ) return HANDLE_H5P_GET_CHUNK_ERR;

	H5Dclose (dataset_id);
	H5Pclose (plist_id);

	_H5Part_print_debug (
		"PROC[%d]: Chunk dimensions: (%lld,%lld,%lld)",
		f->myproc,
		(long long)cdims[2],
		(long long)cdims[1],
		(long long)cdims[0] );

	dims[0] = cdims[2];
	dims[1] = cdims[1];
	dims[2] = cdims[0];

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Return partition of processor \c proc as specified with
  \c H5BlockDefine3dLayout().

  \return \c H5PART_SUCCESS on success.<br>
	  \c H5PART_ERR_INVAL if proc is invalid.
*/
h5part_int64_t
H5Block3dGetPartitionOfProc (
	H5PartFile *f,			/*!< IN: File handle */
	const h5part_int64_t proc,	/*!< IN: Processor to get partition from */
	h5part_int64_t *i_start,	/*!< OUT: start index of \c i	*/ 
	h5part_int64_t *i_end,		/*!< OUT: end index of \c i	*/  
	h5part_int64_t *j_start,	/*!< OUT: start index of \c j	*/ 
	h5part_int64_t *j_end,		/*!< OUT: end index of \c j	*/ 
	h5part_int64_t *k_start,	/*!< OUT: start index of \c k	*/ 
	h5part_int64_t *k_end		/*!< OUT: end index of \c k	*/ 
	) {

	SET_FNAME ( "H5Block3dGetProcOf" );
	BLOCK_INIT ( f );
	CHECK_LAYOUT ( f );

	if ( ( proc < 0 ) || ( proc >= f->nprocs ) )
		return H5PART_ERR_INVAL;

	struct H5BlockPartition *p = &f->block->user_layout[(size_t)proc];

	*i_start = p->i_start;
	*i_end =   p->i_end;
	*j_start = p->j_start;
	*j_end =   p->j_end;
	*k_start = p->k_start;
	*k_end =   p->k_end;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Return reduced (ghost-zone free) partition of processor \c proc
  as specified with \c H5BlockDefine3dLayout().

  \return \c H5PART_SUCCESS on success.<br>
	  \c H5PART_ERR_INVAL if proc is invalid.
*/
h5part_int64_t
H5Block3dGetReducedPartitionOfProc (
	H5PartFile *f,			/*!< IN: File handle */
	h5part_int64_t proc,		/*!< IN: Processor to get partition from */
	h5part_int64_t *i_start,	/*!< OUT: start index of \c i */ 
	h5part_int64_t *i_end,		/*!< OUT: end index of \c i */  
	h5part_int64_t *j_start,	/*!< OUT: start index of \c j */ 
	h5part_int64_t *j_end,		/*!< OUT: end index of \c j */ 
	h5part_int64_t *k_start,	/*!< OUT: start index of \c j */ 
	h5part_int64_t *k_end		/*!< OUT: end index of \c j */ 
	) {

	SET_FNAME ( "H5Block3dGetProcOf" );
	BLOCK_INIT ( f );
	CHECK_LAYOUT ( f );

	if ( ( proc < 0 ) || ( proc >= f->nprocs ) )
		return -1;

	struct H5BlockPartition *p = &f->block->write_layout[(size_t)proc];

	*i_start = p->i_start;
	*i_end =   p->i_end;
	*j_start = p->j_start;
	*j_end =   p->j_end;
	*k_start = p->k_start;
	*k_end =   p->k_end;

	return H5PART_SUCCESS;
}


/*!
  \ingroup h5block_model

  Returns the processor computing the reduced (ghostzone-free) 
  partition given by the coordinates \c i, \c j and \c k.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dGetProcOf (
	H5PartFile *f,			/*!< IN: File handle */
	h5part_int64_t i,		/*!< IN: \c i coordinate */
	h5part_int64_t j,		/*!< IN: \c j coordinate */
	h5part_int64_t k		/*!< IN: \c k coordinate */
	) {

	SET_FNAME ( "H5Block3dGetProcOf" );
	BLOCK_INIT ( f );
	CHECK_LAYOUT ( f );

	struct H5BlockPartition *layout = f->block->write_layout;
	int proc;

	for ( proc = 0; proc < f->nprocs; proc++, layout++ ) {
		if ( (layout->i_start <= i) && (i <= layout->i_end) &&
		     (layout->j_start <= j) && (j <= layout->j_end) &&
		     (layout->k_start <= k) && (k <= layout->k_end) ) 
			return (h5part_int64_t)proc;
	}
	
	return -1;
}

/********************** helper functions for reading and writing *************/

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
static h5part_int64_t
_open_block_group (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	struct H5BlockStruct *b = f->block;

	if ( (f->timestep != b->timestep) && (b->blockgroup > 0) ) {
		herr_t herr = H5Gclose ( b->blockgroup );
		if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
		f->block->blockgroup = -1;
	}

	if ( b->blockgroup < 0 ) {
		hid_t herr = H5Gopen (
				f->timegroup,
				H5BLOCK_GROUPNAME_BLOCK
#ifndef H5_USE_16_API
				, H5P_DEFAULT
#endif
				);
		if ( herr < 0 ) return HANDLE_H5G_OPEN_ERR ( H5BLOCK_GROUPNAME_BLOCK );
		b->blockgroup = herr;
	}
	b->timestep = f->timestep;

	return H5PART_SUCCESS;
}

/********************** functions for reading ********************************/

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_open_field_group (
	H5PartFile *f,			/*!< IN: file handle */
	const char *name
	) {

	struct H5BlockStruct *b = f->block;

	h5part_int64_t h5err = _open_block_group ( f );
	if ( h5err < 0 ) return h5err;

	if ( ! _H5Part_have_group ( b->blockgroup, name ) )
		return HANDLE_H5PART_NOENT_ERR ( name );

	herr_t herr = H5Gopen ( b->blockgroup, name
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( herr < 0 ) return HANDLE_H5G_OPEN_ERR ( name );

	b->field_group_id = herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_close_field_group (
	H5PartFile *f			/*!< IN: file handle */
	) {

	herr_t herr = H5Gclose ( f->block->field_group_id );
	if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_select_hyperslab_for_reading (
	H5PartFile *f,			/*!< IN: file handle */
	hid_t dataset
	) {

	struct H5BlockStruct *b = f->block;
	struct H5BlockPartition *p = &b->user_layout[f->myproc];
	int rank;
	hsize_t field_dims[3];
	hsize_t start[3] = {
		p->k_start,
		p->j_start,
		p->i_start };
	hsize_t stride[3] = { 1, 1, 1 };
	hsize_t part_dims[3] = {
		p->k_end - p->k_start + 1,
		p->j_end - p->j_start + 1,
		p->i_end - p->i_start + 1 };

	h5part_int64_t herr = _release_hyperslab ( f );
	if ( herr < 0 )	return HANDLE_H5S_CLOSE_ERR;

 	b->diskshape = H5Dget_space ( dataset );
	if ( b->diskshape < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

	rank = H5Sget_simple_extent_dims ( b->diskshape, NULL, NULL );
	if ( rank < 0 )  return HANDLE_H5S_GET_SIMPLE_EXTENT_DIMS_ERR;
	if ( rank != 3 ) return HANDLE_H5PART_DATASET_RANK_ERR ( rank, 3 );

	rank = H5Sget_simple_extent_dims ( b->diskshape, field_dims, NULL );
	if ( rank < 0 )  return HANDLE_H5S_GET_SIMPLE_EXTENT_DIMS_ERR;
	
	if ( (field_dims[0] < (hsize_t)b->k_max) ||
	     (field_dims[1] < (hsize_t)b->j_max) ||
	     (field_dims[2] < (hsize_t)b->i_max) ) return HANDLE_H5PART_LAYOUT_ERR;

	_H5Part_print_debug (
		"PROC[%d]: field_dims: (%lld,%lld,%lld)",
		f->myproc,
		(long long)field_dims[2],
		(long long)field_dims[1],
		(long long)field_dims[0] );

	b->diskshape = H5Screate_simple ( rank, field_dims,field_dims );
	if ( b->diskshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_3D_ERR ( field_dims );

	f->block->memshape = H5Screate_simple ( rank, part_dims, part_dims );
	if ( b->memshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_3D_ERR ( part_dims );

	herr = H5Sselect_hyperslab (
		b->diskshape,
		H5S_SELECT_SET,
		start,
		stride,
		part_dims,
		NULL );
	if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;

	_H5Part_print_debug (
		"PROC[%d]: Select hyperslab: \n"
		"\tstart:  (%lld,%lld,%lld)\n"
		"\tstride: (%lld,%lld,%lld)\n"
		"\tdims:   (%lld,%lld,%lld)",
		f->myproc,
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_read_data (
	H5PartFile *f,			/*!< IN: file handle */
	const char *name,		/*!< IN: name of dataset to read */
	void *data,			/*!< OUT: ptr to read buffer */
	hid_t type      		/*!< IN: data type */
	) {

	h5part_int64_t herr;
	struct H5BlockStruct *b = f->block;

	hid_t dataset_id = H5Dopen ( b->field_group_id, name
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( name );

	herr = _H5Block_select_hyperslab_for_reading ( f, dataset_id );
	if ( herr < 0 ) return herr;

#ifdef PARALLEL_IO
	herr = _H5Part_start_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dread ( 
		dataset_id,
		type,
		f->block->memshape,
		f->block->diskshape,
		f->xfer_prop,
		data );
	if ( herr < 0 ) return HANDLE_H5D_READ_ERR ( name, f->timestep );

#ifdef PARALLEL_IO
	herr = _H5Part_end_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dclose ( dataset_id );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	return H5PART_SUCCESS;
}

/********************** functions for writing ********************************/

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
static h5part_int64_t
_select_hyperslab_for_writing (
	H5PartFile *f		/*!< IN: file handle */
	) {

	/*
	  re-use existing hyperslab
	*/
	if ( f->block->shape >= 0 ) return H5PART_SUCCESS;

	herr_t herr;
	struct H5BlockStruct *b = f->block;
	struct H5BlockPartition *p = &b->write_layout[f->myproc];
	struct H5BlockPartition *q = &b->user_layout[f->myproc];

	int rank = 3;
	
	hsize_t field_dims[3] = {
		b->k_max+1,
		b->j_max+1,
		b->i_max+1
	};

	hsize_t start[3] = {
		p->k_start,
		p->j_start,
		p->i_start
	};
	hsize_t stride[3] = { 1, 1, 1 };
	hsize_t part_dims[3] = {
		p->k_end - p->k_start + 1,
		p->j_end - p->j_start + 1,
		p->i_end - p->i_start + 1
	};


	b->shape = H5Screate_simple ( rank, field_dims, field_dims );
	if ( b->shape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_3D_ERR ( field_dims );

	b->diskshape = H5Screate_simple ( rank, field_dims,field_dims );
	if ( b->diskshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_3D_ERR ( field_dims );

	_H5Part_print_debug (
		"PROC[%d]: Select hyperslab on diskshape: \n"
		"\tstart:  (%lld,%lld,%lld)\n"
		"\tstride: (%lld,%lld,%lld)\n"
		"\tdims:   (%lld,%lld,%lld)",
		f->myproc,
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	herr = H5Sselect_hyperslab (
		b->diskshape,
		H5S_SELECT_SET,
		start,
		stride,
		part_dims,
		NULL );
	if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;

	field_dims[0] = q->k_end - q->k_start + 1;
	field_dims[1] = q->j_end - q->j_start + 1;
	field_dims[2] = q->i_end - q->i_start + 1;

	f->block->memshape = H5Screate_simple ( rank, field_dims, field_dims );
	if ( b->memshape < 0 )
		return HANDLE_H5S_CREATE_SIMPLE_3D_ERR ( part_dims );

	start[0] = p->k_start - q->k_start;
	start[1] = p->j_start - q->j_start;
	start[2] = p->i_start - q->i_start;

	_H5Part_print_debug (
		"PROC[%d]: Select hyperslab on memshape: \n"
		"\tstart:  (%lld,%lld,%lld)\n"
		"\tstride: (%lld,%lld,%lld)\n"
		"\tdims:   (%lld,%lld,%lld)",
		f->myproc,
		(long long)start[2],
		(long long)start[1],
		(long long)start[0],
		(long long)stride[2],
		(long long)stride[1],
		(long long)stride[0],
		(long long)part_dims[2],
		(long long)part_dims[1],
		(long long)part_dims[0]  );

	herr = H5Sselect_hyperslab (
		b->memshape,
		H5S_SELECT_SET,
		start,
		stride,
		part_dims,
		NULL );
	if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
static h5part_int64_t
_create_block_group (
	const H5PartFile *f		/*!< IN: file handle */
	) {

	herr_t herr;
	struct H5BlockStruct *b = f->block;

	if ( b->blockgroup > 0 ) {
		herr = H5Gclose ( b->blockgroup );
		if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
		f->block->blockgroup = -1;
	}

	herr = H5Gcreate (
		f->timegroup,
		H5BLOCK_GROUPNAME_BLOCK,
#ifndef H5_USE_16_API
		H5P_DEFAULT,
		H5P_DEFAULT,
		H5P_DEFAULT
#else
		0
#endif
		);
	if ( herr < 0 ) return HANDLE_H5G_CREATE_ERR ( H5BLOCK_GROUPNAME_BLOCK );

	f->block->blockgroup = herr;
	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_create_field_group (
	H5PartFile *f,			/*!< IN: file handle */
	const char *name		/*!< IN: name of field group to create */
	) {

	h5part_int64_t h5err;
	struct H5BlockStruct *b = f->block;


	if ( ! _H5Part_have_group ( f->timegroup, H5BLOCK_GROUPNAME_BLOCK ) ) {
		h5err = _create_block_group ( f );
	} else {
		h5err = _open_block_group ( f );
	}
	if ( h5err < 0 ) return h5err;

	h5err = _select_hyperslab_for_writing ( f );
	if ( h5err < 0 ) return h5err;

	if ( _H5Part_have_group ( b->blockgroup, name ) )
		return  HANDLE_H5PART_GROUP_EXISTS_ERR ( name );

	herr_t herr = H5Gcreate (
		b->blockgroup,
		name,
#ifndef H5_USE_16_API
		H5P_DEFAULT,
		H5P_DEFAULT,
		H5P_DEFAULT
#else
		0
#endif
		);
	if ( herr < 0 ) return HANDLE_H5G_CREATE_ERR ( name );
	b->field_group_id = herr;

	return H5PART_SUCCESS;
}	

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_H5Block_write_data (
	H5PartFile *f,			/*!< IN: file handle */
	const char *name,		/*!< IN: name of dataset to write */
	const void *data,		/*!< IN: data to write */
	const hid_t type		/*!< IN: data type */
	) {

	herr_t herr;
	hid_t dataset;
	struct H5BlockStruct *b = f->block;

#ifndef H5_USE_16_API
	htri_t exists = H5Lexists ( b->field_group_id, name, H5P_DEFAULT );
	if ( exists > 0 ) return HANDLE_H5D_EXISTS_ERR ( name, f->timestep );
#endif

	dataset = H5Dcreate (
		b->field_group_id,
		name,
		type,
		b->shape, 
#ifndef H5_USE_16_API
		H5P_DEFAULT,
		b->create_prop,
		H5P_DEFAULT
#else
		b->create_prop
#endif
		);
	if ( dataset < 0 ) return HANDLE_H5D_CREATE_ERR ( name, f->timestep );

#ifdef PARALLEL_IO
	herr = _H5Part_start_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dwrite ( 
		dataset,
		type,
		b->memshape,
		b->diskshape,
		f->xfer_prop,
		data );
	if ( herr < 0 ) return HANDLE_H5D_WRITE_ERR ( name, f->timestep );

#ifdef PARALLEL_IO
	herr = _H5Part_end_throttle ( f );
	if ( herr < 0 ) return herr;
#endif

	herr = H5Dclose ( dataset );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	return H5PART_SUCCESS;
}

/********************** query information about available fields *************/

/*!
  \ingroup h5block_model

  Query number of fields in current time step.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockGetNumFields (
	H5PartFile *f			/*!< IN: file handle */
	) {

	SET_FNAME ( "H5BlockGetNumFields" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	if ( ! _H5Part_have_group ( f->timegroup, H5BLOCK_GROUPNAME_BLOCK ) )
		return 0;

	return _H5Part_get_num_objects ( f->timegroup, H5BLOCK_GROUPNAME_BLOCK, H5G_GROUP );
}

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
static h5part_int64_t
_get_field_info (
	H5PartFile *f,			/*!< IN: file handle */
	const char *field_name,		/*!< IN: field name to get info about */
	h5part_int64_t *grid_rank,	/*!< OUT: rank of grid */
	h5part_int64_t *grid_dims,	/*!< OUT: dimensions of grid */
	h5part_int64_t *field_rank,	/*!< OUT: rank of field  (1 or 3) */
	h5part_int64_t *type		/*!< OUT: datatype */
	) {

	hsize_t dims[16];
	hsize_t _grid_rank, _field_rank;
	h5part_int64_t i, j;

	h5part_int64_t herr = _open_block_group ( f );
	if ( herr < 0 ) return herr;

	hid_t group_id = H5Gopen (
		f->block->blockgroup,
		field_name
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR ( field_name );

	hid_t dataset_id = H5Dopen ( group_id, "0"
#ifndef H5_USE_16_API
		, H5P_DEFAULT
#endif
		);
	if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( "0" );

 	hid_t dataspace_id = H5Dget_space ( dataset_id );
	if ( dataspace_id < 0 ) return HANDLE_H5D_GET_SPACE_ERR;

	_grid_rank = H5Sget_simple_extent_dims ( dataspace_id, dims, NULL );
	if ( _grid_rank < 0 ) return HANDLE_H5S_GET_SIMPLE_EXTENT_DIMS_ERR;
	if ( grid_rank ) *grid_rank = (h5part_int64_t) _grid_rank;
 
	if ( grid_dims ) {
		for ( i = 0, j = _grid_rank-1; i < _grid_rank; i++, j-- )
			grid_dims[i] = (h5part_int64_t)dims[j];
	}

	_field_rank = _H5Part_get_num_objects (
		f->block->blockgroup,
		field_name,
		H5G_DATASET );
	if ( _field_rank < 0 ) return *field_rank;
	if ( field_rank ) *field_rank = (h5part_int64_t) _field_rank;

	hid_t h5type = H5Dget_type ( dataset_id );
	if ( h5type < 0 ) return HANDLE_H5D_GET_TYPE_ERR;

	if ( type ) {
		*type = _H5Part_normalize_h5_type ( h5type );
		if ( *type < 0 ) return *type;
	}

	herr = H5Sclose ( dataspace_id );
	if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

	herr = H5Dclose ( dataset_id );
	if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

	herr = H5Gclose ( group_id ); 
	if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_model

  Get the name, rank and dimensions of the field specified by the
  index \c idx.

  This function can be used to retrieve all fields bound to the
  current time-step by looping from \c 0 to the number of fields
  minus one.  The number of fields bound to the current time-step
  can be queried by calling the function \c H5BlockGetNumFields().

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockGetFieldInfo (
	H5PartFile *f,				/*!< IN: file handle */
	const h5part_int64_t idx,		/*!< IN: index of field */
	char *field_name,			/*!< OUT: field name */
	const h5part_int64_t len_field_name,	/*!< IN: buffer size */
	h5part_int64_t *grid_rank,		/*!< OUT: grid rank */
	h5part_int64_t *grid_dims,		/*!< OUT: grid dimensions */
	h5part_int64_t *field_rank,		/*!< OUT: field rank */
	h5part_int64_t *type			/*!< OUT: datatype */
	) {

	SET_FNAME ( "H5BlockGetFieldInfo" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	h5part_int64_t herr = _H5Part_get_object_name (
		f->timegroup,
		H5BLOCK_GROUPNAME_BLOCK,
		H5G_GROUP,
		idx,
		field_name,
		len_field_name );
	if ( herr < 0 ) return herr;

	return _get_field_info (
		f, field_name, grid_rank, grid_dims, field_rank, type );
}

/*!
  \ingroup h5block_model

  Get the rank and dimensions of the field specified by its name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockGetFieldInfoByName (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	h5part_int64_t *grid_rank,		/*!< OUT: grid rank */
	h5part_int64_t *grid_dims,		/*!< OUT: grid dimensions */
	h5part_int64_t *field_rank,		/*!< OUT: field rank */
	h5part_int64_t *type			/*!< OUT: datatype */
	) {

	SET_FNAME ( "H5BlockGetFieldInfo" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	return _get_field_info (
		f, field_name, grid_rank, grid_dims, field_rank, type );
}

/********************** reading and writing attribute ************************/

/*!
  \ingroup h5block_private

  \internal

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_write_field_attrib (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const hid_t attrib_type,		/*!< IN: attribute type */
	const void *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	h5part_int64_t herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	herr = _H5Part_write_attrib (
		f->block->field_group_id,
		attrib_name,
		attrib_type,
		attrib_value,
		attrib_nelem );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_attrib

  Write \c attrib_value with type \c attrib_type as attribute \c attrib_name
  to field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttrib (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const h5part_int64_t attrib_type,	/*!< IN: attribute type */
	const void *attrib_value,		/*!< IN: attribute value */
	const h5part_int64_t attrib_nelem	/*!< IN: number of elements */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttrib" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name, (const hid_t)attrib_type, attrib_value,
		(const hid_t)attrib_nelem );
}

/*!
  \ingroup h5block_attrib

  Write string \c attrib_value as attribute \c attrib_name to field
  \c field_name..

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockWriteFieldAttribString (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	const char *attrib_value		/*!< IN: attribute value */
	) {

	SET_FNAME ( "H5BlockWriteFieldAttribString" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	return _write_field_attrib (
		f,
		field_name,
		attrib_name, H5T_NATIVE_CHAR, attrib_value,
		strlen ( attrib_value ) + 1 );
}

/*!
  \ingroup h5block_attrib

  Query the number of attributes of field \c field_name.

  \return number of attributes or error code
*/
h5part_int64_t
H5BlockGetNumFieldAttribs (
	H5PartFile *f,				/*<! IN: file handle */
	const char *field_name			/*<! IN: field name */
	) {

	SET_FNAME ( "H5BlockGetNumFieldAttribs" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	h5part_int64_t nattribs = H5Aget_num_attrs (
		f->block->field_group_id );
	if ( nattribs < 0 ) HANDLE_H5A_GET_NUM_ATTRS_ERR;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return nattribs;
}


/*!
  \ingroup h5block_attrib

  Query information about a attribute given by index \c attrib_idx and
  field name \c field_name. The function returns the name of the attribute,
  the type of the attribute and the number of elements of this type.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockGetFieldAttribInfo (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const h5part_int64_t attrib_idx,	/*!< IN: attribute index */
	char *attrib_name,			/*!< OUT: attribute name */
	const h5part_int64_t len_of_attrib_name,/*!< IN: buffer size */
	h5part_int64_t *attrib_type,		/*!< OUT: attribute type */
	h5part_int64_t *attrib_nelem		/*!< OUT: number of elements */
	) {

	SET_FNAME ( "H5BlockGetFieldAttribInfo" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	h5part_int64_t herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	herr = _H5Part_get_attrib_info (
		f->block->field_group_id,
		attrib_idx,
		attrib_name,
		len_of_attrib_name,
		attrib_type,
		attrib_nelem );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_private

  \internal

  Read attribute \c attrib_name of field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
_read_field_attrib (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	void *attrib_value			/*!< OUT: value */
	) {

	struct H5BlockStruct *b = f->block;

	h5part_int64_t herr = _H5Block_open_field_group ( f, field_name );
	if ( herr < 0 ) return herr;

	herr = _H5Part_read_attrib (
		b->field_group_id,
		attrib_name,
		attrib_value );
	if ( herr < 0 ) return herr;

	herr = _H5Block_close_field_group ( f );
	if ( herr < 0 ) return herr;

	return H5PART_SUCCESS;
}

/*!
  \ingroup h5block_attrib

  Read attribute \c attrib_name of field \c field_name.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5BlockReadFieldAttrib (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const char *attrib_name,		/*!< IN: attribute name */
	void *attrib_value			/*!< OUT: value */
	) {

	SET_FNAME ( "H5PartReadFieldAttrib" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );
	
	return _read_field_attrib (
		f, field_name, attrib_name, attrib_value );
}


/*!
  \ingroup h5block_attrib

  Get field origin.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dGetFieldOrigin (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	h5part_float64_t *x_origin,		/*!< OUT: X origin */
	h5part_float64_t *y_origin,		/*!< OUT: Y origin */
	h5part_float64_t *z_origin		/*!< OUT: Z origin */
	) {

	SET_FNAME ( "H5BlockSetFieldOrigin" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	h5part_float64_t origin[3];

	h5part_int64_t herr = _read_field_attrib (
		f,
		field_name,
		H5BLOCK_FIELD_ORIGIN_NAME,
		origin );

	*x_origin = origin[0];
	*y_origin = origin[1];
	*z_origin = origin[2];
	return herr;
}

/*!
  \ingroup h5block_attrib

  Set field origin.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dSetFieldOrigin (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const h5part_float64_t x_origin,	/*!< IN: X origin */
	const h5part_float64_t y_origin,	/*!< IN: Y origin */
	const h5part_float64_t z_origin		/*!< IN: Z origin */
	) {

	SET_FNAME ( "H5BlockSetFieldOrigin" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	h5part_float64_t origin[3] = { x_origin, y_origin, z_origin };

	return _write_field_attrib (
		f,
		field_name,
		H5BLOCK_FIELD_ORIGIN_NAME,
		(const hid_t)H5PART_FLOAT64, 
		origin,
		3 );
}

/*!
  \ingroup h5block_attrib

  Get field spacing for field \c field_name in the current time step.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dGetFieldSpacing (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	h5part_float64_t *x_spacing,		/*!< OUT: X spacing */
	h5part_float64_t *y_spacing,		/*!< OUT: Y spacing */
	h5part_float64_t *z_spacing		/*!< OUT: Z spacing */
	) {

	SET_FNAME ( "H5BlockGetFieldSpacing" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	h5part_float64_t spacing[3];

	h5part_int64_t herr = _read_field_attrib (
		f,
		field_name,
		H5BLOCK_FIELD_SPACING_NAME,
		spacing );

	*x_spacing = spacing[0];
	*y_spacing = spacing[1];
	*z_spacing = spacing[2];
	return herr;
}

/*!
  \ingroup h5block_attrib

  Set field spacing for field \c field_name in the current time step.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5Block3dSetFieldSpacing (
	H5PartFile *f,				/*!< IN: file handle */
	const char *field_name,			/*!< IN: field name */
	const h5part_float64_t x_spacing,	/*!< IN: X spacing */
	const h5part_float64_t y_spacing,	/*!< IN: Y spacing */
	const h5part_float64_t z_spacing	/*!< IN: Z spacing */
	) {

	SET_FNAME ( "H5BlockSetFieldSpacing" );
	BLOCK_INIT ( f );
	CHECK_WRITABLE_MODE( f );
	CHECK_TIMEGROUP( f );

	h5part_float64_t spacing[3] = { x_spacing, y_spacing, z_spacing };

	return _write_field_attrib (
		f,
		field_name,
		H5BLOCK_FIELD_SPACING_NAME,
		(const hid_t)H5PART_FLOAT64, 
		spacing,
		3 );
}

/*!
  \ingroup h5block_model

  Checks whether the current time-step has field data or not.

  \return \c H5PART_SUCCESS if field data is available otherwise \c
  H5PART_ERR_NOENTRY.
*/
h5part_int64_t
H5BlockHasFieldData (
	H5PartFile *f		/*!< IN: file handle */
	) {

	SET_FNAME ( "H5BlockHasFieldData" );
	BLOCK_INIT ( f );
	CHECK_TIMEGROUP( f );

	if ( ! _H5Part_have_group ( f->timegroup, H5BLOCK_GROUPNAME_BLOCK ) ) {
		return H5PART_ERR_NOENTRY;
	}
	return H5PART_SUCCESS;
}
