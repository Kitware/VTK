#include "private/h5t_octree.h"
#include "private/h5t_core.h"

#ifdef WITH_PARALLEL_H5GRID

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "private/h5_mpi.h"

h5_oct_dta_types_t h5_oct_dta_types;


static h5_err_t
get_bounding_box_of_octant (
		h5t_octree_t* const octree,
		h5_oct_idx_t oct_idx,
		h5_float64_t* bounding_box
		);

#if 0

/*** code copied from http://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
 * author http://stackoverflow.com/users/429544/techplexengineer
 */
static int printf_arginfo_M(const struct printf_info *info, size_t n, int *argtypes, int *avoidwarning) {
	/* "%M" always takes one argument, a pointer to uint8_t[6]. */
	if (n > 0) {
		argtypes[0] = PA_POINTER;
	}
	return 1;
}     /* printf_arginfo_M */

static int printf_output_M(FILE *stream, const struct printf_info *info, const void *const *args) {
	int value = 0;
	int len;
	value = *(int *) (args[0]);
//	value = *(int **) (args[0]);
	//Begin My Code ------------------------------------------------------------
	char buffer [2] = ""; //Is this bad?
	char buffer2 [40] = ""; //Is this bad?
	int bits = 32; //info->width;
//        if (bits <= 0)
//            bits = 8; //Default to 8 bits
	int counter = 1;
	uint mask = 1 << (bits-1);
	while (mask > 0) {
		sprintf(buffer, "%s", (((value & mask) > 0) ? "1" : "0"));
		strcat(buffer2, buffer);
		if (counter %8 == 0) {
			sprintf(buffer, " ");
			strcat(buffer2, buffer);
		}
		mask >>= 1;
		counter++;
	}
	strcat(buffer2, "\n");
	//End my code --------------------------------------------------------------
	len = fprintf(stream, "%s", buffer2);
	return len;
}     /* printf_output_M */
static int
printf_output_B(FILE* stream,
		const struct printf_info* info,
		const void* const* args
		) {
	int value = 0;
	int length = 0;
	value = *(int *) (args[0]);
	char buffer [2] = "";
	char buffer2 [40] = "";
	int bits = 32;
	int counter = 1;
	uint mask = 1 << (bits-1);
	while (mask > 0) {
		sprintf(buffer, "%s", (((value & mask) > 0) ? "1" : "0"));
		strcat(buffer2, buffer);
		if (counter %8 == 0) {
			sprintf(buffer, " ");
			strcat(buffer2, buffer);
		}
		mask >>= 1;
		counter++;
	}
	strcat(buffer2, "\n");
	length = fprintf(stream, "%s", buffer2);
	return length;
}

static int
printf_arginfo_B (const struct printf_info* info,
		size_t n,
		int* argtypes,
		int* args
		) {
	if (n > 0) {
		argtypes[0] = PA_POINTER;
	}
	return 1;
}
#endif

static inline h5_err_t
create_mpi_type_octant (
        void
        ) {
	h5t_octant_t octant;
	H5_PRIV_FUNC_ENTER (h5_err_t, "%s", "void");
	int i = 0;
	const int count = 6;
	int blocklens[count];
	MPI_Aint indices[count];
	MPI_Datatype old_types[count];
	MPI_Aint base;
	MPI_Aint addr;
	TRY (mpi_get_address (&octant, &base));

	// idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// parent_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.parent_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// child_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.child_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// level_idx
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.level_idx, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_SHORT;

//	// bounding_box
//	blocklens[i] = 6;
//	TRY (mpi_get_address (&octant.bounding_box, &addr));
//	indices[i] = addr - base;
//	old_types[i++] = MPI_DOUBLE;

	// processor
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.processor, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;

	// userlevels
	blocklens[i] = 1;
	TRY (mpi_get_address (&octant.userlevels, &addr));
	indices[i] = addr - base;
	old_types[i++] = MPI_INT;


	// create new type
	assert (i == count);
	TRY (mpi_create_type_struct (count, blocklens, indices, old_types,
	                             &h5_oct_dta_types.mpi_octant));
	// commit new type
	TRY (h5priv_mpi_type_commit (&h5_oct_dta_types.mpi_octant));

	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_create_mpi_type_octant (void) {
	return create_mpi_type_octant ();
}
/*
 * Get parent index of octant
 */
static h5_oct_idx_t
get_parent (
        h5t_octree_t* const octree,
        const h5_oct_idx_t oct_idx
        ) {
	if (oct_idx <= 0) {
		return -1;
	}
	return (octree->octants[oct_idx].parent_idx);
};
h5_oct_idx_t
H5t_get_parent (h5t_octree_t* const octree, const h5_oct_idx_t oct_idx) {
	return get_parent (octree, oct_idx);
};

static int
get_maxpoints (
		h5t_octree_t* const octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p,", octree);
	H5_RETURN (octree->maxpoints);
}
int  H5t_get_maxpoints (h5t_octree_t* const octree) {
	return get_maxpoints (octree);
}

static h5_err_t
set_maxpoints (
		h5t_octree_t* const octree,
        int maxpoints
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, maxpoints=%d", octree, maxpoints);
	assert (maxpoints > 0);
	octree->maxpoints = maxpoints;
	H5_RETURN (H5_SUCCESS);
}

h5_err_t H5t_set_maxpoints (h5t_octree_t* const octree, int maxpoints) {
	return set_maxpoints (octree, maxpoints);
}

/*
 * Check if octant is on level
 * return value is 0 if it is NOT on level
 */
h5_oct_level_t
oct_has_level (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_level_t level
        ) {
	return octree->octants[oct_idx].userlevels &  1 << level;
}
h5_oct_level_t H5t_oct_has_level (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_level_t level) {
	return oct_has_level (octree, oct_idx, level);
}
/*
 * set internal data changed
 */
static h5_err_t
set_intdata_chg (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);

	octree->octants[oct_idx].level_idx |= ( 1 << OCT_CHG_INTERNAL);

	H5_RETURN (H5_SUCCESS);
}
/*
 * get new proc
 */
static h5_int32_t
get_proc (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	return octree->octants[oct_idx].processor;

}
h5_int32_t
H5t_get_proc (h5t_octree_t* octree, h5_oct_idx_t oct_idx) {
	return get_proc (octree, oct_idx);
}
/*
 * set new proc
 */
static h5_err_t
set_proc (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_int32_t proc
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d proc=%d", octree, oct_idx, proc);

	if (octree->octants[oct_idx].processor != proc) {   // to avoid unnecessary internal updates
		octree->octants[oct_idx].processor = proc;
		TRY (set_intdata_chg (octree, oct_idx));
	}
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_set_proc (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_int32_t proc) {
	return set_proc (octree, oct_idx, proc);
}
/*
 * set new proc without triggering update
 * WARNING if not all proc do the same the state gets inconsistent!
 */
static h5_err_t
set_proc_int (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_int32_t proc
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d proc=%d", octree, oct_idx, proc);

	octree->octants[oct_idx].processor = proc;

	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_set_proc_int (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_int32_t proc) {
	return set_proc_int (octree, oct_idx, proc);
}

/*
 * set userlevel
 */
static h5_err_t
set_userlevel (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d level=%d", octree, oct_idx, level);

	octree->octants[oct_idx].userlevels |=  ( 1 << level);

	TRY (set_intdata_chg (octree, oct_idx));

	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_set_userlevel (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_level_t level) {
	return set_userlevel (octree, oct_idx, level);
}
/*
 * get userlevel
 */
static h5_oct_level_t
get_userlevel (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);
	h5_oct_level_t level;
	level = octree->octants[oct_idx].userlevels ;
	level = ((level | 1 << (OCT_USERLEV_LENGTH - 1)) ^ (1 << (OCT_USERLEV_LENGTH - 1))); // remove leave level bit

	H5_RETURN (level);
}
h5_oct_level_t H5t_get_userlevel (h5t_octree_t* octree, h5_oct_idx_t oct_idx) {
	return get_userlevel (octree, oct_idx);
}

/*
 * set userlevel
 */
static h5_err_t
set_userlevel_int (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d level=%d", octree, oct_idx, level);

	octree->octants[oct_idx].userlevels |=  ( 1 << level);

	H5_RETURN (H5_SUCCESS);
}
// is an internal function - maybe if there is a usecase one could make it API but is dangerous
// to use since it could create a missmatch in level definitions an different procs
//h5_err_t H5t_set_userlevel_int (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_oct_level_t level) {
//	return set_userlevel_int (octree, oct_idx, level);
//}

#if 0
/*
 * remove userlevel
 */
static h5_err_t
remove_userlevel (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d level=%d", octree, oct_idx, level);
	if (oct_has_level (octree,oct_idx,level)){
	octree->octants[oct_idx].userlevels ^=  ( 1 << level);
	TRY (set_intdata_chg (octree, oct_idx));
	}
	H5_RETURN (H5_SUCCESS);
}
#endif

static h5_err_t
remove_userlevel_int (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d level=%d", octree, oct_idx, level);
	if (oct_has_level (octree,oct_idx,level)){
	octree->octants[oct_idx].userlevels ^=  ( 1 << level);

	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * set leave level
 */
static h5_err_t
set_leave_level (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);

	for (int i = 0; i <= octree->current_oct_idx; i++) {
		// check if leave level bit needs to be changed
		if (((octree->octants[i].child_idx == -1) &&
			((octree->octants[i].userlevels & 1 << (OCT_USERLEV_LENGTH - 1)) != 1 << (OCT_USERLEV_LENGTH - 1) )) ||
		    ((octree->octants[i].child_idx != -1) &&
		    ((octree->octants[i].userlevels & 1 << (OCT_USERLEV_LENGTH - 1)) == 1 << (OCT_USERLEV_LENGTH - 1) ))) {
			octree->octants[i].userlevels ^= 1 << (OCT_USERLEV_LENGTH - 1);
			//TRY (set_intdata_chg (octree, i));
		}
	}

	H5_RETURN (H5_SUCCESS);
}

#if 0
/*
 * Clear all levels
 */
static h5_err_t
clear_all_levels (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);

	for (int i = 0; i <= octree->current_oct_idx; i++) {
		octree->octants[i].userlevels = 0;
		//TRY (set_intdata_chg (octree, i));
	}
	set_leave_level (octree);

	H5_RETURN (H5_SUCCESS);
}
#endif

static h5_err_t
clear_level_internal (
		h5t_octree_t* octree
		) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		if ((octree->octants[i].level_idx & (1 << OCT_CHG_INTERNAL)) == (1 << OCT_CHG_INTERNAL) ) {
			octree->octants[i].level_idx ^= (1 << OCT_CHG_INTERNAL);
		}
	}
	H5_RETURN (H5_SUCCESS);
}


/*
 * get orientation of octant
 */
static h5_oct_orient_t
get_orient (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	if (oct_idx != 0) {
		// WARNING Right shift may not work as expected (c standart is not clear)
		// implementation now without right shift
		h5_oct_orient_t orient = 0;
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_X)) != 0) {
			orient += 1;
		}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_Y)) != 0) {
			orient += 2;
		}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_Z)) != 0) {
			orient += 4;
		}
		return orient ;
	} else {
		return -1;
	}

}
/*
 * get orientation of octant
 */
static h5_lvl_idx_t
get_oct_level (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	assert (oct_idx > -1);
	if (oct_idx != 0) {
		// WARNING Right shift may not work as expected (c standart is not clear)
		// implementation now without right shift
		h5_oct_orient_t orient = 0;
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_CHG_INTERNAL)) != 0) {
					orient |= 1 << OCT_CHG_INTERNAL;
				}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_CHG_USERDATA)) != 0) {
					orient |= 1 << OCT_CHG_USERDATA;
				}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_X)) != 0) {
			orient |= 1 << OCT_X;
		}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_Y)) != 0) {
			orient |= 1 << OCT_Y;
		}
		if ((octree->octants[oct_idx].level_idx & (1 << OCT_Z)) != 0) {
			orient |= 1 << OCT_Z;
		}
		return octree->octants[oct_idx].level_idx ^ orient ;
	} else {
		return 0;
	}

}


#if 0
/*
 * Get total number of octants
 */
static h5_oct_idx_t
get_nbr_octants (
        h5t_octree_t* const octree
        ) {
	return (octree->current_oct_idx) +1;
};

/*
 * Make global update because user data changed
 */
#endif

static h5_err_t
update_userdata (
        h5t_octree_t* const octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	h5_oct_idx_t* oct_idx;
	TRY (oct_idx = h5_calloc (octree->current_oct_idx + 1, sizeof (*oct_idx)));
	h5_oct_idx_t nbr_loc_oct_changed = 0;
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		if ((octree->octants[i].level_idx & (1 << OCT_CHG_USERDATA)) == (1 << OCT_CHG_USERDATA)) {
			oct_idx[nbr_loc_oct_changed] = i;
			nbr_loc_oct_changed++;
		}
	}

	// exchange the number of changed octants
	int size;
	TRY (h5priv_mpi_comm_size (octree->comm, &size));
	h5_int32_t* nbr_oct_changed;
	TRY (nbr_oct_changed = h5_calloc (size, sizeof (*nbr_oct_changed)));

	TRY (h5priv_mpi_allgather (&nbr_loc_oct_changed,
	                          1,
	                          MPI_INT,
	                          nbr_oct_changed,
	                          1,
	                          MPI_INT,
	                          octree->comm));


	// exchange the changed octant_id's
	h5_oct_idx_t* changed_oct_idx;
	h5_oct_idx_t* recv_displs;
	h5_oct_idx_t* recv_counts = nbr_oct_changed;    // just for simpler naming
	h5_oct_idx_t nbr_glb_oct_changed = nbr_oct_changed[0];

	TRY (recv_displs = h5_calloc(size,sizeof(*recv_displs)));

	recv_displs[0] = 0;

	for (int i = 1; i < size; i++) {
		recv_displs[i] = recv_displs[i-1] + nbr_oct_changed[i-1];
		nbr_glb_oct_changed += nbr_oct_changed[i];
	};
	TRY (changed_oct_idx = h5_calloc (nbr_glb_oct_changed, sizeof (*changed_oct_idx)));
	TRY (mpi_allgatherv (oct_idx,
	                     nbr_loc_oct_changed,
	                     MPI_INT,
	                     changed_oct_idx,
	                     recv_counts,
	                     recv_displs,
	                     MPI_INT,
	                     octree->comm
	                     ));

	// check if an octant has been changed on multiple procs
	h5_oct_idx_t oct_idx_to_check;
	for (int i = 0; i < nbr_glb_oct_changed - 1; i++) {
		oct_idx_to_check = changed_oct_idx[i];
		for (int j = i + 1; j < nbr_glb_oct_changed; j++) {
			if (oct_idx_to_check == changed_oct_idx[j]) {
				/*** an octant was changed twice! ***/
				H5_RETURN_ERROR (
					H5_ERR_INVAL,
					"Multiple cores tried to update the same userdata with idx: %d",
					oct_idx_to_check);
			};
		};
	};
	/*** exchange changed userdata ***/
	// new mpi datatype for userdata
	MPI_Datatype userdata_type;

	TRY (h5priv_mpi_type_contiguous (octree->size_userdata, MPI_BYTE, &userdata_type));
	TRY (h5priv_mpi_type_commit (&userdata_type));

	void* sendbuf;
	void* recvbuf;
	TRY (sendbuf = h5_calloc (nbr_loc_oct_changed, octree->size_userdata));
	TRY (recvbuf = h5_calloc (nbr_glb_oct_changed, octree->size_userdata));

//	recv_counts[0] *=  octree->size_userdata;
//	for (int i = 1; i < size; i++) {
//		recv_displs[i] *=  octree->size_userdata;
//		recv_counts[i] *=  octree->size_userdata;
//	};
	char* charp_to = (char*) sendbuf;
	char* charp_from = (char*) octree->userdata;
	// memcpy to send buffer
	for (int i = 0; i < nbr_loc_oct_changed; i++) {
		memcpy( &(charp_to[i*octree->size_userdata]),
		        &(charp_from[oct_idx[i]*octree->size_userdata]),
		        octree->size_userdata);
	};

	TRY (mpi_allgatherv (
	            sendbuf,
	            nbr_loc_oct_changed,
	            userdata_type,
	            recvbuf,
	            recv_counts,
	            recv_displs,
	            userdata_type,
	            octree->comm
	            ));
	// copy user data to local memory location
	charp_to = octree->userdata;
	charp_from = recvbuf;
	//TODO could be improved by not copying the already local updated octants
	for (int i = 0; i < nbr_glb_oct_changed; i++) {
		memcpy( &(charp_to[changed_oct_idx[i] * octree->size_userdata]),
		        &(charp_from[i * octree->size_userdata]),
		        octree->size_userdata);
	}
	// set changed user data to 0
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		if (i == octree->nbr_alloc_oct) { h5_debug ("THIS shoul not happen");}
		octree->octants[i].level_idx &= ~(1 << OCT_CHG_USERDATA);
	}
	// free memory

	TRY (h5_free (oct_idx));
	TRY (h5_free (nbr_oct_changed));
	TRY (h5_free (sendbuf));
	TRY (h5_free (recvbuf));
	TRY (h5_free (recv_displs));
	TRY (h5_free (changed_oct_idx));
	//MPI_Type_free(&userdata_type);

	H5_RETURN (H5_SUCCESS);
};
h5_err_t
H5t_update_userdata (h5t_octree_t* const octree) {
	return update_userdata (octree);
}

/*
 * Make global update because internal data changed
 */

static h5_err_t
update_internal (
        h5t_octree_t* const octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	int nprocs =-1;
	h5priv_mpi_comm_size (octree->comm, &nprocs);
	if (nprocs == 1 ) {
		// set changed internal data to 0
		clear_level_internal (octree);
		set_leave_level (octree);
	}

	h5_oct_idx_t* oct_idx;
	TRY (oct_idx = h5_calloc (octree->current_oct_idx + 1, sizeof (*oct_idx)));
	h5_oct_idx_t nbr_loc_oct_changed = 0;
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		if ((octree->octants[i].level_idx & (1 << OCT_CHG_INTERNAL)) == (1 << OCT_CHG_INTERNAL)) {
			oct_idx[nbr_loc_oct_changed] = i;
			nbr_loc_oct_changed++;
		}
	}

	// exchange the number of changed octants
	int size;
	TRY (h5priv_mpi_comm_size (octree->comm, &size));
	h5_int32_t* nbr_oct_changed;
	TRY (nbr_oct_changed = h5_calloc (size, sizeof (*nbr_oct_changed)));

	TRY (h5priv_mpi_allgather (&nbr_loc_oct_changed,
	                          1,
	                          MPI_INT,
	                          nbr_oct_changed,
	                          1,
	                          MPI_INT,
	                          octree->comm));

	// exchange the changed octant_id's
	h5_oct_idx_t* changed_oct_idx;
	h5_oct_idx_t* recv_displs;
	h5_oct_idx_t* recv_counts = nbr_oct_changed;    // just for simpler naming
	h5_oct_idx_t nbr_glb_oct_changed = nbr_oct_changed[0];

	TRY (recv_displs = h5_calloc(size,sizeof(*recv_displs)));

	recv_displs[0] = 0;

	for (int i = 1; i < size; i++) {
		recv_displs[i] = recv_displs[i-1] + nbr_oct_changed[i-1];
		nbr_glb_oct_changed += nbr_oct_changed[i];
	};
	if (nbr_glb_oct_changed == 0) {
		// set changed internal data to 0
		clear_level_internal (octree);
		// free memory

		TRY (h5_free (oct_idx));
		TRY (h5_free (nbr_oct_changed));
		TRY (h5_free (recv_displs));
		H5_LEAVE (H5_SUCCESS);
	}
	TRY (changed_oct_idx = h5_calloc (nbr_glb_oct_changed, sizeof (*changed_oct_idx)));
	TRY (mpi_allgatherv (oct_idx,
	                     nbr_loc_oct_changed,
	                     MPI_INT,
	                     changed_oct_idx,
	                     recv_counts,
	                     recv_displs,
	                     MPI_INT,
	                     octree->comm
	                     ));

	// check if an octant has been changed on multiple procs
	h5_oct_idx_t oct_idx_to_check;
	for (int i = 0; i < nbr_glb_oct_changed - 1; i++) {
		oct_idx_to_check = changed_oct_idx[i];
		for (int j = i + 1; j < nbr_glb_oct_changed; j++) {
			if (oct_idx_to_check == changed_oct_idx[j]) {
				/*** an octant was changed twice! ***/
				H5_LEAVE(H5_ERR_INVAL)
			};
		};
	};
	/*** exchange changed octants ***/
	// new mpi datatype for userdata


	void* sendbuf;
	void* recvbuf;
	TRY (sendbuf = h5_calloc (nbr_loc_oct_changed, sizeof (*(octree->octants))));
	TRY (recvbuf = h5_calloc (nbr_glb_oct_changed, sizeof (*(octree->octants))));

	h5t_octant_t* charp_to = (h5t_octant_t*) sendbuf;
	h5t_octant_t* charp_from =  octree->octants;
	// memcpy to send buffer
	for (int i = 0; i < nbr_loc_oct_changed; i++) {
		memcpy( &(charp_to[i]),
		        &(charp_from[oct_idx[i]]),
		        sizeof (*octree->octants));
	};

	TRY (mpi_allgatherv (
	            sendbuf,
	            nbr_loc_oct_changed,
	            h5_oct_dta_types.mpi_octant,
	            recvbuf,
	            recv_counts,
	            recv_displs,
	            h5_oct_dta_types.mpi_octant,
	            octree->comm
	            ));
	// copy user data to local memory location
	charp_to = octree->octants;
	charp_from = recvbuf;
	//TODO could be improved by not copying the already local updated octants
	for (int i = 0; i < nbr_glb_oct_changed; i++) {
		memcpy( &(charp_to[changed_oct_idx[i]]),
		        &(charp_from[i]),
		        sizeof (*octree->octants));
	}
	// set changed internal data to 0
	clear_level_internal (octree);
	set_leave_level (octree);
	// free memory

	TRY (h5_free (oct_idx));
	TRY (h5_free (nbr_oct_changed));
	TRY (h5_free (sendbuf));
	TRY (h5_free (recvbuf));
	TRY (h5_free (recv_displs));
	TRY (h5_free (changed_oct_idx));


	H5_RETURN (H5_SUCCESS);
};
h5_err_t
H5t_update_internal (h5t_octree_t* const octree) {
	return update_internal (octree);
}

/*
 * Get first children index of octant
 */
static h5_oct_idx_t
get_children (
        h5t_octree_t* const octree,
        const h5_oct_idx_t oct_idx
        ) {
	return octree->octants[oct_idx].child_idx;
};
h5_oct_idx_t
H5t_get_children (h5t_octree_t* const octree, const h5_oct_idx_t oct_idx) {
	return get_children (octree, oct_idx);
};
/*
 * Get pointer to userdata of octant for reading
 * This Pointer is only valid till the next call of the Library!
 */
static h5_err_t
get_userdata_r (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        void** userdata
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, userdata=%p", octree, oct_idx, userdata);

	char* userdata_int = (char*) octree->userdata;
	*userdata = &userdata_int[oct_idx * octree->size_userdata];

	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_get_userdata_r (h5t_octree_t* octree, h5_oct_idx_t oct_idx, void** userdata) {
	return get_userdata_r (octree, oct_idx, userdata);
}

/*
 * Get pointer to userdata of octant for reading/writing
 * This Pointer is only valid till the next call of the Library!
 * To make the write visible to all procs call update_userdata(h5t_octree_t* octree);
 */
static h5_err_t
get_userdata_rw (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        void** userdata
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, userdata=%p", octree, oct_idx, userdata);

	octree->octants[oct_idx].level_idx |= (1 << OCT_CHG_USERDATA);
	char* userdata_int;
	userdata_int = octree->userdata;
	*userdata = &userdata_int[oct_idx * octree->size_userdata];

	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_get_userdata_rw (h5t_octree_t* octree, h5_oct_idx_t oct_idx, void** userdata) {
	return get_userdata_rw (octree, oct_idx, userdata);
}


/*
 * grow data structure of octree
 */
static h5_err_t
grow_octree (
        h5t_octree_t* octree,
        h5_int32_t additional_size
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, additional_size=%d", octree, additional_size);

	if (additional_size == -1) {
		additional_size = octree->nbr_alloc_oct;
	}
	if (additional_size < 8 ) {
		additional_size = 8;
	}
	// check if data structures already exist
	if (octree->octants == NULL) {
		// allocate data structures
		TRY (octree->octants = h5_calloc (additional_size, sizeof (*(octree->octants))));
		if (octree->size_userdata > 0) {
			TRY (octree->userdata = h5_calloc (additional_size, octree->size_userdata));
			memset (octree->userdata, -1, additional_size * octree->size_userdata);
		}
		octree->nbr_alloc_oct = additional_size;
	} else {
		// grow data structure
		TRY (octree->octants = h5_alloc (octree->octants, (octree->nbr_alloc_oct + additional_size) * sizeof (*(octree->octants))));
		if (octree->size_userdata > 0) {
			TRY (octree->userdata = h5_alloc (octree->userdata, (octree->nbr_alloc_oct+additional_size) * octree->size_userdata));
			memset (((char*)octree->userdata) + octree->nbr_alloc_oct * octree->size_userdata , -1, additional_size * octree->size_userdata);
		}
		octree->nbr_alloc_oct += additional_size;
	}
	H5_RETURN (H5_SUCCESS);

}

#if 0
/*
 * Reset user data
 */
static h5_err_t
reset_userdata (
        h5t_octree_t* octree,
        h5_int32_t size_userdata
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, size_userdata=%d", octree, size_userdata);

	// make sure all procs call the function
	TRY (h5priv_mpi_barrier (octree->comm));

	// delete userdata
	TRY (h5_free(octree->userdata));

	// allocate new userdata
	if (size_userdata > 0) {
		octree->size_userdata = size_userdata;
		TRY (octree->userdata = h5_calloc (octree->nbr_alloc_oct, octree->size_userdata));
	} else {
		octree->size_userdata = -1;
	}
	// set changed user data to 0
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		octree->octants[i].level_idx &= ~(1 << OCT_CHG_USERDATA);
	}

	H5_RETURN (H5_SUCCESS);
}
#endif
/*
 * Create octant
 */
static h5_err_t
create_octant (
        h5t_octree_t* octree,
        h5_oct_idx_t parent_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, parent_idx=%d",
			octree,
			parent_idx);

	octree->current_oct_idx +=1;

	// check if there is enough space to insert one octant more.
	if (octree->current_oct_idx >= octree->nbr_alloc_oct) {
		TRY (grow_octree (octree,-1));
	}
	// set parameters
	h5t_octant_t* current_octant = &(octree->octants[octree->current_oct_idx]);
	current_octant->idx = octree->current_oct_idx;
	current_octant->parent_idx = parent_idx;
	current_octant->child_idx = -1;
	if (parent_idx >= 0) {

		 current_octant->level_idx = ((octree->current_oct_idx - octree->octants[parent_idx].child_idx) << OCT_X)
				 + get_oct_level (octree, parent_idx) + 1;


	}

	if (parent_idx != -1) {
		current_octant->processor = octree->octants[parent_idx].processor;
	} else {
		current_octant->processor = 0;
	}
	current_octant->userlevels = 0;
	H5_RETURN (H5_SUCCESS);
};

/*
 * Create root octant
 */
static h5_err_t
create_root_octant (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	if (octree->octants == NULL || octree->nbr_alloc_oct < 1) {
		H5_LEAVE (H5_ERR_INTERNAL);
	}
	TRY (create_octant (octree,-1));
	octree->octants[0].level_idx = 0;
	TRY (set_leave_level(octree));
	H5_RETURN (H5_SUCCESS);
};
/*
 * Initialize an octree
 */
static h5_err_t
init_octree (
        h5t_octree_t** octree,
        h5_int32_t size_userdata,
        h5_float64_t* const bounding_box,
        h5_int32_t maxpoints,
        const MPI_Comm comm
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
	                    "octree=%p, size_userdata=%d, "
			    "bounding_box=%p, "
			    "maxpoints=%d, comm=?",
	                    octree,
	                    size_userdata,
	                    bounding_box,
	                    maxpoints);

	TRY (h5priv_mpi_barrier (comm));

	/*** assert of input data ***/
	//TODO maybe there is a more thorough check needed that input data is valid (i.e. the same on all proc)
	if (bounding_box != NULL && (bounding_box[0] >= bounding_box[3] ||
		bounding_box[1] >= bounding_box[4] ||
		bounding_box[2] >= bounding_box[5])) {
		H5_LEAVE (H5_ERR_INVAL);
	};

	/*** allocate octree ***/
	TRY (*octree = h5_calloc (1, sizeof (**octree)));

	(*octree)->comm = comm;
	(*octree)->size_userdata = size_userdata;
	TRY (create_mpi_type_octant ());
	(*octree)->octants = NULL;
	(*octree)->userdata = NULL;
	(*octree)->maxpoints = maxpoints;

	/*** allocate datastructures***/
	TRY (grow_octree (*octree,9));

	(*octree)->nbr_alloc_oct = 9;
	(*octree)->ref_oct_idx = -1;
	(*octree)->current_oct_idx = -1;
	TRY (create_root_octant (*octree));

	if (bounding_box != NULL) {
		TRY (H5t_set_bounding_box (*octree, bounding_box));
	}


	H5_RETURN (H5_SUCCESS);
};

h5_err_t H5t_init_octree (h5t_octree_t** octree, h5_int32_t size_userdata, h5_float64_t* const bounding_box, h5_int32_t maxpoints, const MPI_Comm comm) {
	return init_octree (octree, size_userdata, bounding_box, maxpoints, comm);
}

static h5_err_t
read_octree (
	h5t_octree_t** octree,
	h5_oct_idx_t current_oct_idx,
	h5_int32_t size_userdata,
	h5_int32_t maxpoints,
	h5t_octant_t** octants,
	void** userdata,
	const MPI_Comm comm
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			    "octree=%p, current_oct_idx=%d, size_userdata=%d, "
			    "maxpoints=%d, octants=%p, userdata=%p, comm=?",
			    octree,
			    current_oct_idx,
			    size_userdata,
			    maxpoints,
			    octants,
			    userdata);

	TRY (h5priv_mpi_barrier (comm)); // delete for speed up?

	/*** allocate octree ***/
	TRY (*octree = h5_calloc (1, sizeof (**octree)));
	(*octree)->comm = comm;
	(*octree)->size_userdata = size_userdata;
	TRY (create_mpi_type_octant ());
	(*octree)->octants = NULL;
	(*octree)->userdata = NULL;
	(*octree)->maxpoints = maxpoints;

	/*** allocate datastructures***/
	TRY (grow_octree (*octree,current_oct_idx + 1));

	(*octree)->nbr_alloc_oct = current_oct_idx + 1;
	(*octree)->ref_oct_idx = -1;
	(*octree)->current_oct_idx = current_oct_idx;
	(*octants) = (*octree)->octants;
	(*userdata) = (*octree)->userdata;
	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_read_octree (h5t_octree_t** octree, h5_oct_idx_t current_oct_idx, h5_int32_t size_userdata, h5_int32_t maxpoints, h5t_octant_t** octants, void** userdata, const MPI_Comm comm) {
	return read_octree (octree, current_oct_idx, size_userdata, maxpoints, octants, userdata, comm);
}
/*
 * free octree
 */
static h5_err_t
free_oct (
	h5t_octree_t* octree) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	if (octree != NULL) {
		if (octree->userdata != NULL) {
			TRY (h5_free (octree->userdata));
		}
		MPI_Type_free(&h5_oct_dta_types.mpi_octant);
		TRY (h5_free (octree->octants));
		TRY (h5_free (octree));
		octree = NULL;
	}
	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_free_octree (h5t_octree_t* octree) {
	return free_oct (octree);
}
static h5_err_t
write_octree (
	h5t_mesh_t* const m
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "m=%p", m);


	H5_RETURN (h5_error_not_implemented ());
}
h5_err_t H5t_write_octree ( h5t_mesh_t* const m) {
	return write_octree (m);
}

#if 0
/*
 * Print octree in human readable form
 */
static h5_err_t
print_octree_dbg (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,"octree=%p", octree);
	int rank;
	TRY (h5priv_mpi_comm_rank (octree->comm, &rank));
	h5_debug ("\nproc %d: Octree\n"
			"proc %d:    current_oct_idx:    %d \n"
			"proc %d:    nbr_alloc_oct:      %d \n\n",
			rank,
			rank,
			octree->current_oct_idx,
			rank,
			octree->nbr_alloc_oct);
	H5_RETURN (H5_SUCCESS);
}
/*
 * Print octree in human readable form
 */
static h5_err_t
print_octree (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,"octree=%p", octree);
	int rank;
	TRY (h5priv_mpi_comm_rank (octree->comm, &rank));
	printf ("\nproc %d: Octree\n"
			"proc %d:    current_oct_idx:    %d \n"
			"proc %d:    nbr_alloc_oct:      %d \n\n",
			rank,
			rank,
			octree->current_oct_idx,
			rank,
			octree->nbr_alloc_oct);
	H5_RETURN (H5_SUCCESS);
}
/*
 * Traverse through octants and print them in human readable form
 */
static h5_err_t
print_octants (
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
#if 0
	register_printf_specifier ('B', printf_output_M, printf_arginfo_M);
	int rank;
	TRY (h5priv_mpi_comm_rank (octree->comm, &rank));
	for (int i = 0; i <= octree->current_oct_idx; i++) {
		printf ("\n"
//		h5_debug ("\n"
				"    Octant\n    idx:          %d \n"
				"    parent_idx:   %d \n"
				"    child_idx:    %d \n"
//				"    bounding_box: %4.4f, %4.4f, %4.4f \n"
//				"                  %4.4f, %4.4f, %4.4f \n"
				"    processor:    %d \n"
				"    level_idx:    %B \n"
				"    userlevels:    %B \n\n",
				octree->octants[i].idx,
				octree->octants[i].parent_idx,
				octree->octants[i].child_idx,
//				octree->octants[i].bounding_box[0],
//				octree->octants[i].bounding_box[1],
//				octree->octants[i].bounding_box[2],
//				octree->octants[i].bounding_box[3],
//				octree->octants[i].bounding_box[4],
//				octree->octants[i].bounding_box[5],
				octree->octants[i].processor,
				octree->octants[i].level_idx,
				octree->octants[i].userlevels);

	}
#endif
	H5_RETURN (H5_SUCCESS);
}
#endif

#if 0
static h5_err_t
print_octants_arr (
        h5t_octant_t* octants,
        h5_int32_t nbr_oct,
        h5_int32_t rank
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octants);
#if 0
	register_printf_specifier ('B', printf_output_M, printf_arginfo_M);

	for (int i = 0; i < nbr_oct; i++) {
		h5_debug ("\n"
				"    Octant\n    idx:          %d \n"
				"    parent_idx:   %d \n"
				"    child_idx:    %d \n"
//				"    bounding_box: %4.4f, %4.4f, %4.4f \n"
//				"                  %4.4f, %4.4f, %4.4f \n"
				"    processor:    %d \n"
				"    level_idx:    %B \n"
				"    userlevels:    %B \n\n",
				octants[i].idx,
				octants[i].parent_idx,
				octants[i].child_idx,
//				octants[i].bounding_box[0],
//				octants[i].bounding_box[1],
//				octants[i].bounding_box[2],
//				octants[i].bounding_box[3],
//				octants[i].bounding_box[4],
//				octants[i].bounding_box[5],
				octants[i].processor,
				octants[i].level_idx,
				octants[i].userlevels);

	}
#endif
	H5_RETURN (H5_SUCCESS);
}
#endif
///*
// * Traverse through octants and plot the for gnuplot
// */

int print_octant_for_gnuplot (
		h5_oct_idx_t oct_idx,
		h5_float64_t* bounding_box
	) {
#if 0
	printf (		"#    Octant    idx:          %d \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n",
					oct_idx,
					bounding_box[0], bounding_box[1], bounding_box[2],
					bounding_box[3], bounding_box[1], bounding_box[2],
					bounding_box[3], bounding_box[4], bounding_box[2],
					bounding_box[0], bounding_box[4], bounding_box[2],
					bounding_box[0], bounding_box[1], bounding_box[2]);
			printf (
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n",
					bounding_box[0], bounding_box[1], bounding_box[5],
					bounding_box[3], bounding_box[1], bounding_box[5],
					bounding_box[3], bounding_box[4], bounding_box[5],
					bounding_box[0], bounding_box[4], bounding_box[5],
					bounding_box[0], bounding_box[1], bounding_box[5]);
			printf (
					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n"

					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n"

					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n"

					" %4.4f, %4.4f, %4.4f \n"
					" %4.4f, %4.4f, %4.4f \n\n",
					bounding_box[0], bounding_box[1], bounding_box[2],
					bounding_box[0], bounding_box[1], bounding_box[5],

					bounding_box[3], bounding_box[1], bounding_box[2],
					bounding_box[3], bounding_box[1], bounding_box[5],

					bounding_box[3], bounding_box[4], bounding_box[2],
					bounding_box[3], bounding_box[4], bounding_box[5],

					bounding_box[0], bounding_box[4], bounding_box[2],
					bounding_box[0], bounding_box[4], bounding_box[5]);
#endif
	return 0;
}

h5_err_t
h5priv_plot_octants (
        h5t_octree_t* octree
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "octree=%p", octree);
//	h5_debug ("# start plot octants","");
	h5_float64_t bounding_box[6];
	int i;
	for (i = 0; i <= octree->current_oct_idx; i++) {
//		h5_debug (
		TRY (get_bounding_box_of_octant (octree, i, bounding_box));
		print_octant_for_gnuplot (i, bounding_box);
	}
	for (i = 0; i <= octree->current_oct_idx; i++) {
		TRY (get_bounding_box_of_octant (octree, i, bounding_box));
		printf (
				"set label \" %d \"  at first  %4.4f, first %4.4f  font \"Helvetica,7\"\n",octree->octants[i].idx, bounding_box[0] + 0.02 + 0.6 * bounding_box[1], bounding_box[2] + 0.02 + 0.4 * bounding_box[1]);
	}
//	h5_debug ("# end plot octants","");
	H5_RETURN (H5_SUCCESS);
}

static h5_err_t
get_siblings(
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_idx_t* siblings_idx
        );

h5_err_t
h5priv_plot_octant_anc (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "octree=%p", octree);
//	h5_debug ("# start plot octants","");
	h5_float64_t bounding_box[6];
	do {
		TRY (get_siblings(octree, oct_idx, &oct_idx));
		for (int i = 0; i < NUM_OCTANTS; i++) {
			TRY (get_bounding_box_of_octant (octree, oct_idx + i, bounding_box));
			print_octant_for_gnuplot (oct_idx + i, bounding_box);

			printf (
				"set label \" %d \"  at first  %4.4f, first %4.4f  font \"Helvetica,7\"\n",octree->octants[oct_idx+i].idx, bounding_box[0] + 0.02 + 0.6 * bounding_box[1], bounding_box[2] + 0.02 + 0.4 * bounding_box[1]);
		}
	} while ((oct_idx = get_parent(octree, oct_idx)) != -1);

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
h5priv_plot_leaf_octants (
        h5t_octree_t* octree
        ) {
	H5_PRIV_API_ENTER (h5_err_t, "octree=%p", octree);

	h5_float64_t bounding_box[6];
	h5t_oct_iterator_t* iter;
	TRY (H5t_init_leafoct_iterator (octree, &iter));
	h5_oct_idx_t oct_idx = -1;
	while ((oct_idx = H5t_iterate_oct(iter)) != -1) {

		TRY (get_bounding_box_of_octant (octree, oct_idx, bounding_box));
		print_octant_for_gnuplot (oct_idx, bounding_box);
	}
	printf ("\n #real oct labels \n");
	TRY (H5t_init_leafoct_iterator (octree, &iter));
	while ((oct_idx = H5t_iterate_oct(iter)) != -1) {
		TRY (get_bounding_box_of_octant (octree, oct_idx, bounding_box));
		printf (
				"set label \" %d \"  at first  %4.4f, first %4.4f  font \"Helvetica,7\"\n",
				octree->octants[oct_idx].idx, bounding_box[0] + 0.02 + 0.6 * bounding_box[1], bounding_box[2] + 0.02 + 0.4 * bounding_box[1]);
	}
	printf ("\n #leaf oct labels \n");
	TRY (H5t_init_leafoct_iterator (octree, &iter));
	int counter = 0;
	while ((oct_idx = H5t_iterate_oct(iter)) != -1) {
		TRY (get_bounding_box_of_octant (octree, oct_idx, bounding_box));
		printf (
				"set label \" %d \"  at first  %4.4f, first %4.4f  font \"Helvetica,7\"\n",
				counter++, bounding_box[0] + 0.02 + 0.6 * bounding_box[1], bounding_box[2] + 0.02 + 0.4 * bounding_box[1]);
	}
	TRY (H5t_end_iterate_oct(iter));
	H5_RETURN (H5_SUCCESS);
}

void print_array (
				h5_int32_t* neigh,
				h5_oct_idx_t nbr_neigh,
				int rank
					) {
			printf ("proc %d: array \n",rank);
			for (int i = 0; i < nbr_neigh; i++) {
				printf (" %d, ", neigh[i]);
			}

			printf ("\n");
		}


/*
 * begin refine octants
 */
static h5_err_t
begin_refine_octants(
        h5t_octree_t* octree
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);

	TRY (h5priv_mpi_barrier (octree->comm));
	update_internal (octree);
	octree->ref_oct_idx = octree->current_oct_idx;
	H5_RETURN (H5_SUCCESS);
}
/*
 * Refine an octant
 */
static h5_err_t
refine_octant(
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);
	int rank;
	MPI_Comm_rank (octree->comm, &rank);

	if (octree->octants[oct_idx].processor != rank ) {
		h5_debug ("Trying to refine an octant that doesn't belong to proc");
		H5_LEAVE (H5_ERR_INVAL);
	}

	if (octree->octants[oct_idx].child_idx != -1 || octree->ref_oct_idx == -1) {
		h5_debug ("Either octant is already refined or begin_refine_octants() was not invoked");
		H5_LEAVE (H5_ERR_INVAL);
	}
	if( octree->current_oct_idx + 1 + 8 >= octree->nbr_alloc_oct) {
		/*** need to allocate more memory for octants ***/
		TRY (grow_octree (octree, -1));
	}


	octree->octants[oct_idx].child_idx = octree->current_oct_idx + 1;
	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	TRY (create_octant (octree, oct_idx));

	set_leave_level (octree);

	H5_RETURN (H5_SUCCESS);
}
/*
 * end refine octants
 */
static h5_err_t
end_refine_octants(
        h5t_octree_t* octree,
        h5_oct_point_t* midpoints,
        int num_midpoints
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	if (octree->ref_oct_idx == -1) {
		H5_LEAVE (H5_ERR_INVAL);
	}

	// exchange how many new octants per proc where created
	int size,rank;
	TRY (h5priv_mpi_comm_size (octree->comm, &size));
	TRY (h5priv_mpi_comm_rank (octree->comm, &rank));
	h5_int32_t* nbr_oct_new;
	TRY (nbr_oct_new = h5_calloc (size,sizeof (*nbr_oct_new)));
	h5_int32_t total_oct_new = 0;
	h5_oct_idx_t loc_oct_new = octree->current_oct_idx - octree->ref_oct_idx;
	TRY (h5priv_mpi_allgather (&loc_oct_new,
		                          1,
		                          MPI_INT,
		                          nbr_oct_new,
		                          1,
		                          MPI_INT,
		                          octree->comm));

	// calculate own range (i.e. offset)
	h5_oct_idx_t* offset;
	TRY (offset = h5_calloc (size,sizeof(*offset)));
	total_oct_new = nbr_oct_new[0];
	for (int i = 1; i < size; i++ ) {
		total_oct_new += nbr_oct_new[i];
		offset[i] = nbr_oct_new[i-1] + offset[i-1];
	}
	// allocate enough memory for all new octants
	if (octree->ref_oct_idx + 1 + total_oct_new >= octree->nbr_alloc_oct) {
		TRY (grow_octree (octree, total_oct_new ));
	}

	// update octants with offset
	if (offset[rank] > 0) {
		for (int i =  octree->ref_oct_idx+1; i <= octree->current_oct_idx; i++) {
			if (octree->octants[i].idx > octree->ref_oct_idx) {
				octree->octants[i].idx += offset[rank];
			}
			if (octree->octants[i].parent_idx > octree->ref_oct_idx) {
				octree->octants[i].parent_idx += offset[rank];
			}
			if (octree->octants[i].child_idx > octree->ref_oct_idx) {
				octree->octants[i].child_idx += offset[rank];
			}
		}
		// update midpoints list with offset
		if (num_midpoints > 0) {
			for (int i = 0; i < num_midpoints; i++) {
				if (midpoints[i].oct > octree->ref_oct_idx) {
					midpoints[i].oct += offset[rank];
				}
			}
		}

	}
	void* sendbuf;
	void* recvbuf;
	if ((loc_oct_new) > 0) {
		TRY (sendbuf = h5_calloc (loc_oct_new ,sizeof (h5t_octant_t)));
	} else {
		sendbuf = NULL;
	}
	TRY (recvbuf = h5_calloc (total_oct_new ,sizeof (h5t_octant_t)));
	// copy new octants to send buffer
	memmove (sendbuf,
			&(octree->octants[octree->ref_oct_idx + 1]),
			loc_oct_new * sizeof (h5t_octant_t));
	octree->current_oct_idx = octree->ref_oct_idx + total_oct_new;
	// commit new type

//	TRY (h5priv_mpi_type_commit (&h5_oct_dta_types.mpi_octant));

	TRY (mpi_allgatherv (
			sendbuf,
	        nbr_oct_new[rank],
	        h5_oct_dta_types.mpi_octant,
	        recvbuf,
	        nbr_oct_new,
	        offset,
	        h5_oct_dta_types.mpi_octant,
	        octree->comm
	        ));

	memcpy (&(octree->octants[octree->ref_oct_idx + 1]), recvbuf, total_oct_new * sizeof (h5t_octant_t));

	// update parents child_idx
	//TODO could be more efficient
	for (int i = octree->current_oct_idx; i > octree->ref_oct_idx ; i--) {
		h5_oct_idx_t parent_idx = get_parent (octree, i);
		if (parent_idx < 0 || parent_idx > octree->current_oct_idx) {
			H5_LEAVE (H5_ERR_INTERNAL);
		}
		octree->octants[parent_idx].child_idx = i;

	}

	octree->ref_oct_idx = -1;

	TRY (h5_free (nbr_oct_new));
	if (sendbuf != NULL) {
		TRY (h5_free (sendbuf));
	}
	TRY (h5_free (recvbuf));
	TRY (h5_free (offset));
	update_userdata (octree);
	clear_level_internal (octree);
	update_internal (octree); // maybe update leave level would be enough
	H5_RETURN (H5_SUCCESS);
}

/*
 * compare func for qsort bsearch
 */
int compare_points_x(const void *p_a, const void *p_b)
{
return ((*(h5_oct_point_t*)p_a).x - (*(h5_oct_point_t*)p_b).x) >= 0 ? 1 : 0;
}
int compare_points_y(const void *p_a, const void *p_b)
{
return ((*(h5_oct_point_t*)p_a).y - (*(h5_oct_point_t*)p_b).y) >= 0 ? 1 : 0;
}
int compare_points_z(const void *p_a, const void *p_b)
{
return ((*(h5_oct_point_t*)p_a).z - (*(h5_oct_point_t*)p_b).z) >= 0 ? 1 : 0;
}
int sort_points_x(const void *p_a, const void *p_b)
{

	if (((*(h5_oct_point_t*)p_a).x - (*(h5_oct_point_t*)p_b).x) < 0 )
		return -1 ;
	if (((*(h5_oct_point_t*)p_a).x - (*(h5_oct_point_t*)p_b).x) == 0 )
		return 0;
	if (((*(h5_oct_point_t*)p_a).x - (*(h5_oct_point_t*)p_b).x) > 0 )
		return 1 ;
	 return -1; // will never be executed
}
int sort_points_y(const void *p_a, const void *p_b)
{
	if (((*(h5_oct_point_t*)p_a).y - (*(h5_oct_point_t*)p_b).y) < 0 )
		return -1 ;
	if (((*(h5_oct_point_t*)p_a).y - (*(h5_oct_point_t*)p_b).y) == 0 )
		return 0;
	if (((*(h5_oct_point_t*)p_a).y - (*(h5_oct_point_t*)p_b).y) > 0 )
		return 1 ;
	 return -1; // will never be executed
}
int sort_points_z(const void *p_a, const void *p_b)
{
	if (((*(h5_oct_point_t*)p_a).z - (*(h5_oct_point_t*)p_b).z) < 0 )
		return -1 ;
	if (((*(h5_oct_point_t*)p_a).z - (*(h5_oct_point_t*)p_b).z) == 0 )
		return 0;
	if (((*(h5_oct_point_t*)p_a).z - (*(h5_oct_point_t*)p_b).z) > 0 )
		return 1 ;
 return -1; // will never be executed
}


/*
 * multidimensional sort of array of points
 */
static h5_err_t
sort_array (
	h5_oct_point_t* key,
	h5_oct_point_t* points,
	h5_int32_t nbr_points,
	h5_oct_point_t** split,
	h5_int32_t* nbr_in_split
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "key=%p, points=%p, nbr_points=%d, split=%p, nbr_in_split=%p",
			key, points, nbr_points, split, nbr_in_split);

	// sort in z
	comparison_fn_t compare;
	compare.compare = compare_points_z;
	split[0] = points;
	qsort (points, nbr_points, sizeof (*points), sort_points_z);
	split[4] = linsearch (key, points, nbr_points, sizeof (*points), compare);
	if (split[4] == NULL) {
		nbr_in_split[0] = nbr_points;
	} else {
		nbr_in_split[0] = split[4] - split[0];
	}
	nbr_in_split[4] = nbr_points - nbr_in_split[0];


	// sort in y
	compare.compare = compare_points_y;
	qsort (split[0], nbr_in_split[0], sizeof (*points), sort_points_y);
	qsort (split[4], nbr_in_split[4], sizeof (*points), sort_points_y);
	split[2] = linsearch (key, split[0], nbr_in_split[0], sizeof (*points), compare);
	split[6] = linsearch (key, split[4], nbr_in_split[4], sizeof (*points), compare);
	if (split[2] == NULL) {
		nbr_in_split[2] = 0;
	} else {
		nbr_in_split[2] = nbr_in_split[0] - (split[2] - split[0]);
	}
	nbr_in_split[0] -= nbr_in_split[2];

	if (split[6] == NULL) {
		nbr_in_split[6] = 0;
	} else {
		nbr_in_split[6] = nbr_in_split[4] - (split[6] - split[4]);
	}
	nbr_in_split[4] -= nbr_in_split[6];

	// sort in x
	compare.compare = compare_points_x;
	qsort (split[0], nbr_in_split[0], sizeof (*points), sort_points_x);
	qsort (split[2], nbr_in_split[2], sizeof (*points), sort_points_x);
	qsort (split[4], nbr_in_split[4], sizeof (*points), sort_points_x);
	qsort (split[6], nbr_in_split[6], sizeof (*points), sort_points_x);
	split[1] = linsearch (key, split[0], nbr_in_split[0], sizeof (*points), compare);
	split[3] = linsearch (key, split[2], nbr_in_split[2], sizeof (*points), compare);
	split[5] = linsearch (key, split[4], nbr_in_split[4], sizeof (*points), compare);
	split[7] = linsearch (key, split[6], nbr_in_split[6], sizeof (*points), compare);

	if (split[1] == NULL) {
		nbr_in_split[1] = 0;
	} else {
		nbr_in_split[1] = nbr_in_split[0] - (split[1] - split[0]);
	}
	nbr_in_split[0] -= nbr_in_split[1];

	if (split[3] == NULL) {
		nbr_in_split[3] = 0;
	} else {
		nbr_in_split[3] = nbr_in_split[2] - (split[3] - split[2]);
	}
	nbr_in_split[2] -= nbr_in_split[3];

	if (split[5] == NULL) {
		nbr_in_split[5] = 0;
	} else {
		nbr_in_split[5] = nbr_in_split[4] - (split[5] - split[4]);
	}
	nbr_in_split[4] -= nbr_in_split[5];

	if (split[7] == NULL) {
		nbr_in_split[7] = 0;
	} else {
		nbr_in_split[7] = nbr_in_split[6] - (split[7] - split[6]);
	}
	nbr_in_split[6] -= nbr_in_split[7];

	H5_RETURN (H5_SUCCESS);
}
/*
 * get_midpoint
 */
static h5_err_t
get_midpoint (
        h5_oct_point_t* midpoint,
        h5_float64_t* bounding_box
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "midpoint=%p, bounding_box=%p", midpoint, bounding_box);
	midpoint->x = (bounding_box[3] + bounding_box[0]) / 2.0;
	midpoint->y = (bounding_box[4] + bounding_box[1]) / 2.0;
	midpoint->z = (bounding_box[5] + bounding_box[2]) / 2.0;
	H5_RETURN (H5_SUCCESS);
}
/*
 * function to calculate bounding box of child with orientation orient
 */

static h5_err_t
get_new_bounding_box (
	h5_float64_t* bb,
	h5_float64_t* new_bb,
	h5_oct_orient_t orient
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "midpoint=%p, bounding_box=%p, orient=%d", bb, new_bb, orient);
	h5_float64_t xmin, xmid, xmax, ymin, ymid, ymax, zmin, zmid, zmax;

	// store all values such that bb and new_bb can be the same
	xmin = bb[0];
	xmax = bb[3];
	xmid = (xmin + xmax) / 2;

	ymin = bb[1];
	ymax = bb[4];
	ymid = (ymin + ymax) / 2;

	zmin = bb[2];
	zmax = bb[5];
	zmid = (zmin + zmax) / 2;

	// x direction
	if ((orient & 1) == 1) {
		new_bb[0] = xmid;
		new_bb[3] = xmax;
	} else {
		new_bb[0] = xmin;
		new_bb[3] = xmid;
	}
	// y direction
	if ((orient & 2) == 2) {
		new_bb[1] = ymid;
		new_bb[4] = ymax;
	} else {
		new_bb[1] = ymin;
		new_bb[4] = ymid;
	}
	// z direction
	if ((orient & 4) == 4) {
		new_bb[2] = zmid;
		new_bb[5] = zmax;
	} else {
		new_bb[2] = zmin;
		new_bb[5] = zmid;
	}
	H5_RETURN (H5_SUCCESS);
}
void print_array_p (
			h5_oct_point_t* neigh,
			h5_oct_idx_t nbr_neigh,
			int rank
				) {
		printf ("proc %d: array \n",rank);
		for (int i = 0; i < nbr_neigh; i++) {
			printf (" %4.4f, %4.4f, %4.4f / %d, %lld --", neigh[i].x, neigh[i].y, neigh[i].z, neigh[i].oct, (long long) neigh[i].elem);
		}

		printf ("\n");
	}
/*
 * recursive refine oct w points
 */
static h5_err_t
recursive_ref_points (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_float64_t* bounding_box,
	h5_oct_point_t* points,
	h5_int32_t nbr_points,
	h5_int32_t max_points
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, oct_idx=%d points=%p, bounding_box=%p, nbr_points=%d, max_points=%d",
			octree,
			oct_idx,
			bounding_box,
			points,
			nbr_points,
			max_points);

	h5_oct_point_t midpoint;
	TRY (get_midpoint (&midpoint, bounding_box));
	if (octree->octants[oct_idx].child_idx == -1) {
		// octant needs to be refined
		TRY (refine_octant (octree, oct_idx));
	}
	h5_oct_point_t* split_arr[8];
	h5_oct_point_t** split = split_arr;
	h5_int32_t nbr_in_split_arr[8];
	h5_int32_t* nbr_in_split = nbr_in_split_arr;
	TRY (sort_array (&midpoint, points, nbr_points, split, nbr_in_split));

	for (int i = 0; i < 8; i++) {
		if (nbr_in_split[i] > max_points ||
			(h5tpriv_octant_is_full (octree, octree->octants[oct_idx].child_idx + i)
			&& nbr_in_split[i] > 0)
			) {
			h5_float64_t new_bounding_box[6];
			TRY (get_new_bounding_box (bounding_box, new_bounding_box, i));
			// call itself with points belonging to childrens octants
			TRY (recursive_ref_points (octree, octree->octants[oct_idx].child_idx + i, new_bounding_box, split[i], nbr_in_split[i], max_points));

		} else {
			// assign oct_idx to points
		//	print_array_p (points,nbr_points,32);
			for (int j = 0; j < nbr_in_split[i]; j++) {
				(split[i] + j)->oct = octree->octants[oct_idx].child_idx + i;

			}
			//print_array_p (points,nbr_points,32);
		}
	}

	H5_RETURN (H5_SUCCESS);
}

/*
 * refine with points
 */
static h5_err_t
refine_w_points (
	h5t_octree_t* octree,
	h5_oct_point_t* points,
	h5_int32_t nbr_points,
	h5_int32_t max_points
	) {
	H5_PRIV_FUNC_ENTER (
		h5_err_t,
		"octree=%p, points=%p, nbr_points=%d, max_points=%d",
		octree, points, nbr_points, max_points);
	if (nbr_points < 1) {
		TRY (begin_refine_octants (octree));
		TRY (end_refine_octants (octree, points, nbr_points));
		H5_LEAVE (H5_SUCCESS);
	}

	TRY (begin_refine_octants (octree));

	if (points[0].oct == -1) { // adding points for the first time
		if (nbr_points > max_points || h5tpriv_octant_is_full (octree, 0)) {
			TRY (recursive_ref_points (octree, 0, octree->bounding_box, points, nbr_points, max_points));
		} else {
			for (int j = 0; j < nbr_points; j++) {
				points[j].oct = 0;
			}
		}
	} else { // midpoints from refining elements
		int counter = 0;
		h5_float64_t bounding_box[6];
		while (counter < nbr_points) {
			// find all points that are together in a leaf oct at the moment
			h5_oct_idx_t oct_idx = points[counter].oct;
			int first_point = counter++;
			assert ( oct_has_level (octree, oct_idx, OCT_USERLEV_LENGTH -1));
			while (counter < nbr_points && oct_idx == points[counter].oct) {
				counter++;
			}
			// check if refinement of octree is necessary (either octant is full or more than maxpoints are in leaf octant)
			if (counter - first_point > max_points || h5tpriv_octant_is_full (octree, points[first_point].oct)) {

				TRY (get_bounding_box_of_octant(octree, points[first_point].oct, bounding_box));
				TRY (recursive_ref_points (
						octree,
						points[first_point].oct,
						bounding_box,
						&points[first_point],
						counter - first_point,
						max_points));
			}

		}
	}
	TRY (end_refine_octants (octree, points, nbr_points));

	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_refine_w_points (h5t_octree_t* octree, h5_oct_point_t* points, h5_int32_t nbr_points,	h5_int32_t max_points) {
	return refine_w_points (octree, points, nbr_points,	max_points);
}
static int
bounding_box_contains_point (
	h5_float64_t* bounding_box,
	h5_oct_point_t* point
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "bounding_box=%p, point=%p", bounding_box, point);
	if ( 	bounding_box[0] <= point->x &&
			bounding_box[3] > point->x &&
			bounding_box[1] <= point->y &&
			bounding_box[4] > point->y &&
			bounding_box[2] <= point->z &&
			bounding_box[5] > point->z
			) {
		H5_LEAVE (1);
	} else {
		H5_LEAVE (0);
	}
	H5_RETURN (H5_ERR);
}
static h5_err_t
get_bounding_box_of_octant (
	h5t_octree_t* const octree,
	h5_oct_idx_t oct_idx,
	h5_float64_t* bounding_box
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, bounding_box=%p", octree, oct_idx, bounding_box);

	// get level and allocate memory to store all ancestors
	h5_lvl_idx_t level = get_oct_level (octree, oct_idx);
	h5_oct_idx_t* ancestors = NULL;
	TRY (ancestors = h5_calloc (level + 1, sizeof (*ancestors)));

	// retrieve all ancestors
	int i = 0;
	ancestors[i++] = oct_idx;
	h5_oct_idx_t parent = get_parent (octree, oct_idx);

	while (parent >= 0) {
		ancestors[i++] = parent;
		parent = get_parent (octree, parent);
	}
	// get bounding box of octree
	memcpy (bounding_box, octree->bounding_box, 6 * sizeof (*bounding_box));

	// calculate recursively the bounding box of oct_idx
	h5_oct_orient_t direction = 0;
	while (--i > 0) {
		direction = ancestors[i-1] - get_children(octree, ancestors[i]);
		TRY (get_new_bounding_box(bounding_box, bounding_box, direction));
	}
	TRY (h5_free (ancestors));
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_get_bounding_box_of_octant (h5t_octree_t* const octree, h5_oct_idx_t oct_idx, h5_float64_t* bounding_box) {
	return get_bounding_box_of_octant (octree, oct_idx, bounding_box);
}

static h5_oct_idx_t
find_leafoctant_of_point (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_float64_t* bounding_box,
	h5_oct_point_t* point
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, point=%p", octree, oct_idx, point);

	// if point is not contained in octree return error
	if (!bounding_box_contains_point (bounding_box, point)) {
		H5_LEAVE (H5_ERR_INVAL);
	}
	h5_oct_idx_t ret_oct_idx = oct_idx;
	// if octant has children find the child that contains it and search -> recursively
	if (octree->octants[oct_idx].child_idx != -1) {
		oct_idx = octree->octants[oct_idx].child_idx;
		h5_float64_t new_bb[6];
		for (int i = 0; i < 8; i++) {
			TRY (get_new_bounding_box(bounding_box, new_bb, i));
		if (bounding_box_contains_point (new_bb, point)) {
				TRY (ret_oct_idx = find_leafoctant_of_point (octree, oct_idx + i, new_bb, point));
				break;
			}
		}
	}

	H5_RETURN (ret_oct_idx);
}
h5_oct_idx_t
H5t_find_leafoctant_of_point (h5t_octree_t* octree, h5_oct_idx_t oct_idx, h5_float64_t* bounding_box, h5_oct_point_t* point) {
	return find_leafoctant_of_point (octree, oct_idx, bounding_box, point);
}


/*
 * if points are assigned to octants which have children, points get assigned to leaf level octants
 */
static h5_err_t
add_points_to_leaf (
	h5t_octree_t* octree,
	h5_oct_point_t* points,
	h5_int32_t nbr_points
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, points=%p, nbr_points=%d", octree, points, nbr_points);
	h5_float64_t bounding_box[6];
	for (int i = 0; i < nbr_points; i++) {
		TRY (get_bounding_box_of_octant (octree, points[i].oct, bounding_box));
		TRY (points[i].oct = find_leafoctant_of_point (octree, points[i].oct, bounding_box, &points[i]));
	}

	H5_RETURN (H5_SUCCESS);
}

h5_err_t H5t_add_points_to_leaf (h5t_octree_t* octree, h5_oct_point_t* points, h5_int32_t nbr_points) {
	return add_points_to_leaf (octree, points, nbr_points);
}
/*
 * Get siblings
 */
static h5_err_t
get_siblings(
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_idx_t* siblings_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, siblings_idx=%p", octree, oct_idx, siblings_idx);


	if (oct_idx == 0) {
		/*** root node has no siblings***/
		H5_LEAVE (H5_ERR_INVAL);
	} else {
		h5_oct_idx_t parent_idx = octree->octants[oct_idx].parent_idx;
		*siblings_idx = octree->octants[parent_idx].child_idx;
	}

	H5_RETURN (H5_SUCCESS);
}

/*
 * Get sibling
 */
static h5_oct_idx_t
get_sibling(
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);
	assert (oct_idx >0);
	H5_RETURN (
		octree->octants[octree->octants[oct_idx].parent_idx].child_idx);
}

h5_oct_idx_t H5t_get_sibling(h5t_octree_t* octree, h5_oct_idx_t oct_idx) {
	return get_sibling(octree, oct_idx);
}

#if 0
/*
 * returns memory usage per processor
 */
static h5_err_t
mem_usage (
        h5t_octree_t* octree,
        h5_int32_t* mem_use
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, mem_use=%p", octree, mem_use);
	*mem_use = sizeof (*octree) + (sizeof (*(octree->octants)) + octree->size_userdata) * octree->nbr_alloc_oct;
	H5_RETURN (H5_SUCCESS);
}
#endif

#if 0
/*
 * free neigh memory
 */
static h5_err_t
free_neigh (
        h5_oct_idx_t* neighbors,
        h5_oct_idx_t* ancestor_of_neigh
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "neighbors=%p, ancestor_of_neigh=%p", neighbors, ancestor_of_neigh);
	TRY (h5_free (neighbors));
	TRY (h5_free (ancestor_of_neigh));
	H5_RETURN (H5_SUCCESS);
}
#endif

/*
 * compare func for qsort bsearch
 */
int compare_oct_idx(const void *p_a, const void *p_b)
{
return *(h5_oct_idx_t*)p_a - *(h5_oct_idx_t*)p_b;
}
/*
 * add oct_idx to array if it doesn't exists yet (without sorting)
 */
static h5_err_t
add_neigh (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t* neigh,
	h5_oct_idx_t* num_neigh
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, oct_idx=%d, ancestor_of_neigh=%p, nbr_anc_of_neigh=%d",
			octree, oct_idx,
			neigh, *num_neigh);
	int i = 0;
	for (; i < *num_neigh; i++) {
		if (oct_idx == neigh[i]) {
			H5_LEAVE (H5_SUCCESS);
		}
	}

	neigh[*num_neigh] = oct_idx;
	(*num_neigh)++;

	H5_RETURN (H5_SUCCESS);
}
/*
 * add ancestor to array if it doesn't exists yet (sort ancestors)
 */
static h5_err_t
add_ancestor (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t* ancestor_of_neigh,
	h5_oct_idx_t* nbr_anc_of_neigh
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, oct_idx=%d, ancestor_of_neigh=%p, nbr_anc_of_neigh=%d",
			octree, oct_idx,
			ancestor_of_neigh, *nbr_anc_of_neigh);


	if ( bsearch (&oct_idx, ancestor_of_neigh, (size_t) *nbr_anc_of_neigh, sizeof (*nbr_anc_of_neigh), compare_oct_idx) == NULL) {
		 ancestor_of_neigh[*nbr_anc_of_neigh] = oct_idx;
		 (*nbr_anc_of_neigh)++;
		 qsort(ancestor_of_neigh, (size_t) *nbr_anc_of_neigh, sizeof (*nbr_anc_of_neigh), compare_oct_idx);
	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * get ancestors of octant
 */
static h5_err_t
get_ancestors (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t* ancestor_of_neigh,
	h5_oct_idx_t* nbr_anc_of_neigh
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, oct_idx=%d, ancestor_of_neigh=%p, nbr_anc_of_neigh=%d",
			octree, oct_idx,
			ancestor_of_neigh, *nbr_anc_of_neigh);
	h5_oct_idx_t parent_idx = oct_idx;
	while ((parent_idx = get_parent (octree, parent_idx)) != -1 ) {
			TRY (add_ancestor (octree, parent_idx, ancestor_of_neigh, nbr_anc_of_neigh));
	}
	H5_RETURN (H5_SUCCESS);
}
/*
 * get children with given orientation on certain level
 */
static h5_err_t
get_kids_with_orient (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_idx_t* neighbors,
        h5_oct_idx_t* nbr_neigh,
        h5_oct_idx_t* ancestor_of_neigh,
        h5_oct_idx_t* nbr_anc_of_neigh,
        h5_oct_level_t userlevel,
        h5_int32_t orient,
        h5_int32_t direction,
        h5_int32_t ifsibling
        );

static h5_err_t
get_kids_with_orient (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_idx_t* neighbors,
        h5_oct_idx_t* nbr_neigh,
        h5_oct_idx_t* ancestor_of_neigh,
        h5_oct_idx_t* nbr_anc_of_neigh,
        h5_oct_level_t userlevel,
        h5_int32_t orient,
        h5_int32_t direction,
        h5_int32_t ifsibling
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t,
			"octree=%p, oct_idx=%d, neighbors=%p, nbr_neigh=%d,"
			" ancestor_of_neigh=%p, nbr_anc_of_neigh=%d,"
			" userlevel=%d, orient=%d, direction=%d",
			octree, oct_idx, neighbors, *nbr_neigh,
			ancestor_of_neigh, *nbr_anc_of_neigh,
			userlevel, orient, direction);
	assert (*nbr_neigh > -1);
	assert (*nbr_neigh < OCT_MAX_NEIGHBORS);
	assert (*nbr_anc_of_neigh > -1);
	assert (*nbr_anc_of_neigh < OCT_MAX_NEIGHBORS);
	assert (oct_idx > 0);

	if (oct_has_level (octree, oct_idx, userlevel)) {
		// octant is neighbor on correct level
		neighbors[*nbr_neigh] = oct_idx;
		(*nbr_neigh)++;
		TRY (get_ancestors (octree, oct_idx, ancestor_of_neigh, nbr_anc_of_neigh));

	} else {

		// add octant as ancestor and go to children
		h5_oct_idx_t children_oct_idx = get_children (octree, oct_idx);
		if (children_oct_idx == -1) {
			H5_LEAVE (H5_ERR_INTERNAL);
		}
		for (int i = 0; i < 8; i++) {
			if ( (ifsibling && (i & direction) == (orient & direction)) ||
				(!ifsibling && (i & direction) != (orient & direction))) {
				// check chlidrens children
				TRY (get_kids_with_orient (octree,
								children_oct_idx + i,
								neighbors,
								nbr_neigh,
								ancestor_of_neigh,
								nbr_anc_of_neigh,
								userlevel,
								orient,
								direction,
								ifsibling));
				}


		}
	}


	H5_RETURN (H5_SUCCESS);
}


/*
 * get nearest common ancestor
 * returns -1 on geom boundary
 */
static h5_err_t
get_nca (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_dir_t direction,
        h5_oct_orient_t* orient_child_nca,
        h5_oct_idx_t* nca
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, direction=%d, orient_child_nca=%p nca=%p",
			octree, oct_idx, direction, orient_child_nca, nca);
	// TODO maybe check that siblings don't fullfill requirement
	h5_oct_idx_t parent_idx = get_parent (octree, oct_idx);
	assert (parent_idx > -1);
	h5_oct_orient_t orient = get_orient (octree, oct_idx);
	h5_oct_orient_t parent_orient = get_orient (octree, parent_idx);
	memcpy (&orient_child_nca[0],&orient, sizeof (orient));
	*nca = -1;
	int counter = 1;
	while (octree->octants[parent_idx].parent_idx != -1) {
		parent_orient = get_orient (octree, parent_idx);
		memcpy (&orient_child_nca[counter++],&parent_orient, sizeof (parent_orient));
		if ((orient & direction) != (parent_orient & direction)) {
			// oct_idx is sibling of neighbors
			*nca = get_parent (octree, parent_idx);
			parent_orient = get_orient (octree, *nca);
			memcpy (&orient_child_nca[counter++],&parent_orient, sizeof (parent_orient));
			break;
		}
		parent_idx = get_parent (octree, parent_idx);
	}

	H5_RETURN (H5_SUCCESS);
}
/*
 * get the child with given orientation, return -1 if no children
 */

static h5_oct_idx_t
get_child_with_orient (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_orient_t orient
        ) {
	if (octree->octants[oct_idx].child_idx == -1) {
		return -1;
	} else {
		return octree->octants[oct_idx].child_idx + orient;
	}
}


/*
 * get the smallest possible neighbor up to the same octree level & userlevel
 */
static h5_err_t
get_equal_sized_neigh (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_dir_t direction,
        h5_oct_orient_t* sibling_orient,
        h5_oct_idx_t nca,
        h5_oct_idx_t* neigh,
        h5_oct_level_t userlevel
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, direction=%d, sibling_orient=%p"
			", nca=%d, neigh=%p, userlevel=%d",
			octree,
			oct_idx,
			direction,
			sibling_orient,
			nca,
			neigh,
			userlevel);
	int orient_idx = get_oct_level (octree, oct_idx) - get_oct_level (octree, nca) -1;
	h5_oct_idx_t child = get_child_with_orient (octree, nca, sibling_orient[orient_idx--] ^ direction);
	h5_oct_orient_t child_orient= 0;

	while ((get_oct_level (octree, oct_idx) > get_oct_level (octree, child)) // neighbor is still coarser
			&& (oct_has_level (octree, child, userlevel ) == 0) ) { // userlevel not reached

		child_orient = sibling_orient[orient_idx--] ^ direction;
		if ( (child = get_child_with_orient (octree, child, child_orient)) == -1 ) {
			H5_LEAVE (H5_ERR_INVAL);
		}
	}
	*neigh = child;

	H5_RETURN (H5_SUCCESS);
}
static h5_err_t
check_neigh_cand (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t* neig,
	h5_oct_idx_t* num_neigh
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, neig=%p, num_neigh=%p, ",
			octree, oct_idx, neig, num_neigh);
	int if_check_fails_is_int_error = 0;
	if (*num_neigh > 1) {
		if_check_fails_is_int_error = 1;
	}
	h5_float64_t bb[6];
	get_bounding_box_of_octant(octree, oct_idx, bb);

	for (int i = 0; i < *num_neigh; i++) {
		h5_float64_t n_bb[6];
		get_bounding_box_of_octant(octree, neig[i], n_bb);
		if (bb[0] > n_bb[3] || bb[3] < n_bb[0] ||
			bb[1] > n_bb[4] || bb[4] < n_bb[1] ||
			bb[2] > n_bb[5] || bb[5] < n_bb[2] 	) {
			// should be deleted
			if (if_check_fails_is_int_error) {
				H5_LEAVE (H5_ERR_INTERNAL);
			} else {
				*num_neigh = 0;
			}
		}

	}


	H5_RETURN (H5_SUCCESS);
}


static h5_err_t
add_common_neigh (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t* neigh,
	h5_oct_idx_t* num_neigh,
	h5_oct_idx_t* x_neigh,
	h5_oct_idx_t num_x_neigh,
	h5_oct_idx_t* y_neigh,
	h5_oct_idx_t num_y_neigh
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, neig=%p, num_neigh=%p, ",
				octree, oct_idx, neigh, num_neigh);
	h5_oct_idx_t num_cand = 0;
	for (int i = 0; i < num_x_neigh; i++) {
			h5_oct_idx_t neigh_idx = x_neigh[i];
			for (int j = 0; j < num_y_neigh; j++) {
				if (neigh_idx == y_neigh[j] && neigh_idx != oct_idx) {
					// edge neighbor candidate is found
					num_cand = 1;
					TRY (check_neigh_cand (octree, oct_idx, &neigh_idx, &num_cand));
					if (num_cand == 1) {
					// add to neigh
					TRY (add_neigh (octree,neigh_idx, neigh, num_neigh));
					break;
					}
				}
			}
	}
	H5_RETURN (H5_SUCCESS);
}
//		// TODO put in separate function
//		h5_oct_idx_t num_cand = 0;
		// find entries that are contained in x, y and z




h5_err_t
static get_tmp_dir_neigh (
		h5t_octree_t* octree,
		h5_oct_idx_t size,
		h5_oct_idx_t* oct_idxs,
		h5_oct_idx_t** neigh,
		h5_oct_idx_t* num_neigh,
		h5_oct_idx_t** anc,
		h5_oct_idx_t* num_anc,
		h5_oct_idx_t* num_alloc,
		h5_oct_level_t userlevel
		);

#if 0
static h5_err_t
get_edge_dir_neigh (
		h5t_octree_t* octree,
		h5_oct_idx_t oct_idx,
		h5_oct_idx_t tmp_oct_idx,
		h5_oct_dir_t tmp_dir,
		h5_oct_idx_t* neighbors,
		h5_oct_idx_t* num_neigh,
		h5_oct_idx_t* anc,
		h5_oct_idx_t* num_anc,
		h5_oct_level_t userlevel
	);
#endif

/*
 * get face neighbors of octant
 */
static h5_err_t
get_neighbors (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx,
        h5_oct_idx_t** neighbors,
        h5_oct_idx_t* nbr_neigh,
        h5_oct_idx_t** ancestor_of_neigh,
        h5_oct_idx_t* nbr_anc_of_neigh,
        h5_oct_idx_t kind_of_neigh, // subdim of neighborhood 1 face neigh, 2 edge neigh, 3 vertex neigh WARNING 2 may also return some vertex edges
        h5_oct_level_t userlevel
        ) {

	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d, neighbors=%p, nbr_neigh=%d, "
			"ancestor_of_neigh=%p, nbr_anc_of_neigh=%d, userlevel=%d",
			octree, oct_idx, neighbors, *nbr_neigh,
			ancestor_of_neigh, *nbr_anc_of_neigh, userlevel);

	assert (kind_of_neigh > 0);
	assert (kind_of_neigh < 4);
	// check if oct_idx has userlevel
	if (oct_has_level (octree, oct_idx, userlevel) == 0 ) {
		H5_LEAVE (H5_ERR_INVAL);
	}

	h5_oct_orient_t orient = get_orient (octree, oct_idx);
//	h5_oct_idx_t parent_oct_idx = octree->octants[oct_idx].parent_idx;
	h5_oct_idx_t siblings;

	h5_oct_orient_t* orient_child_nca = NULL;
	TRY (orient_child_nca = h5_calloc (get_oct_level(octree, oct_idx) + 1, sizeof (*orient_child_nca)));

	TRY (get_siblings (octree, oct_idx, &siblings));
	TRY (*neighbors = h5_alloc (*neighbors, OCT_MAX_NEIGHBORS * sizeof (**neighbors)));
	memset (*neighbors, 0, OCT_MAX_NEIGHBORS * sizeof (**neighbors) );
	TRY (*ancestor_of_neigh = h5_alloc (*ancestor_of_neigh, OCT_MAX_NEIGHBORS * sizeof (**neighbors)));
	memset (*ancestor_of_neigh, 0, OCT_MAX_NEIGHBORS * sizeof (**neighbors) );
	*nbr_neigh = 0;
	*nbr_anc_of_neigh = 0;
	h5_oct_idx_t tmp_oct_idx, nearest_common_anc;
	h5_oct_dir_t tmp_dir;
	h5_oct_idx_t old_num_neigh = 0;
	h5_oct_idx_t num_neigh_per_dir[6];
	h5_int32_t ifsibling = -1;
	// handle siblings

	//sibling in X
	tmp_oct_idx = siblings + (orient ^ 1);
	tmp_dir = 1;
	ifsibling = 1;
	TRY (get_kids_with_orient (octree,
			tmp_oct_idx,
			*neighbors,
			nbr_neigh,
			*ancestor_of_neigh,
			nbr_anc_of_neigh,
			userlevel,
			orient,
			tmp_dir,
			ifsibling));

//	//get other neighbors in X
	TRY (get_nca (octree, oct_idx, tmp_dir, orient_child_nca, &nearest_common_anc));
	if (nearest_common_anc != -1) {

		TRY (get_equal_sized_neigh (octree, oct_idx, tmp_dir, orient_child_nca, nearest_common_anc, &tmp_oct_idx, userlevel));
		if (get_sibling (octree, tmp_oct_idx) == get_sibling (octree, oct_idx)) {
			ifsibling = 1;
		} else {
			ifsibling = 0;
		}

		TRY (get_kids_with_orient (octree,
				tmp_oct_idx,
				*neighbors,
				nbr_neigh,
				*ancestor_of_neigh,
				nbr_anc_of_neigh,
				userlevel,
				orient,
				tmp_dir,
				ifsibling));
	}
	num_neigh_per_dir[0] = *nbr_neigh;
	memset (orient_child_nca, 0, (get_oct_level(octree, oct_idx) + 1) * sizeof (*orient_child_nca));

	//sibling in Y
	tmp_oct_idx = siblings + (orient ^ 2);
	tmp_dir = 2;
	ifsibling = 1;
	old_num_neigh = *nbr_neigh;
	TRY (get_kids_with_orient (octree,
				tmp_oct_idx,
				*neighbors,
				nbr_neigh,
				*ancestor_of_neigh,
				nbr_anc_of_neigh,
				userlevel,
				orient,
				tmp_dir,
				ifsibling));

//	//get other neighbors in Y

	TRY (get_nca (octree, oct_idx, tmp_dir, orient_child_nca, &nearest_common_anc));

	if (nearest_common_anc != -1) {
	TRY (get_equal_sized_neigh (octree, oct_idx, tmp_dir, orient_child_nca, nearest_common_anc, &tmp_oct_idx, userlevel));
	if (get_sibling (octree, tmp_oct_idx) == get_sibling (octree, oct_idx)) {
		ifsibling = 1;
	} else {
		ifsibling = 0;
	}
	TRY (get_kids_with_orient (octree,
				tmp_oct_idx,
				*neighbors,
				nbr_neigh,
				*ancestor_of_neigh,
				nbr_anc_of_neigh,
				userlevel,
				orient,
				tmp_dir,
				ifsibling));
	}
	num_neigh_per_dir[1] = *nbr_neigh - old_num_neigh;
	memset (orient_child_nca, 0, (get_oct_level(octree, oct_idx) + 1) * sizeof (*orient_child_nca));

	//sibling in Z
	tmp_oct_idx = siblings + (orient ^ 4);
	tmp_dir = 4;
	ifsibling = 1;
	old_num_neigh = *nbr_neigh;
	TRY (get_kids_with_orient (octree,
				tmp_oct_idx,
				*neighbors,
				nbr_neigh,
				*ancestor_of_neigh,
				nbr_anc_of_neigh,
				userlevel,
				orient,
				tmp_dir,
				ifsibling));

//	//get other neighbors in Z
		TRY (get_nca (octree, oct_idx, tmp_dir, orient_child_nca, &nearest_common_anc));
	if (nearest_common_anc != -1) {
		TRY (get_equal_sized_neigh (octree, oct_idx, tmp_dir, orient_child_nca, nearest_common_anc, &tmp_oct_idx, userlevel));
		if (get_sibling (octree, tmp_oct_idx) == get_sibling (octree, oct_idx)) {
			ifsibling = 1;
		} else {
			ifsibling = 0;
		}
		TRY (get_kids_with_orient (octree,
					tmp_oct_idx,
					*neighbors,
					nbr_neigh,
					*ancestor_of_neigh,
					nbr_anc_of_neigh,
					userlevel,
					orient,
					tmp_dir,
					ifsibling));
	}
	num_neigh_per_dir[2] = *nbr_neigh - old_num_neigh;
	TRY (h5_free (orient_child_nca));

	// get ancestors of siblings
	TRY (get_ancestors (octree, oct_idx, *ancestor_of_neigh, nbr_anc_of_neigh));

	if (kind_of_neigh > 1) {
		// edge neighbors
		h5_oct_idx_t* x_neigh = NULL;
		h5_oct_idx_t* y_neigh = NULL;
		h5_oct_idx_t* z_neigh = NULL;

		h5_oct_idx_t* x_anc = NULL;
		h5_oct_idx_t* y_anc = NULL;
		h5_oct_idx_t* z_anc = NULL;

		h5_oct_idx_t num_x_neigh = 0;
		h5_oct_idx_t num_y_neigh = 0;
		h5_oct_idx_t num_z_neigh = 0;

		h5_oct_idx_t num_x_anc = 0;
		h5_oct_idx_t num_y_anc = 0;
		h5_oct_idx_t num_z_anc = 0;

		h5_oct_idx_t num_x_alloc = 0;
		h5_oct_idx_t num_y_alloc = 0;
		h5_oct_idx_t num_z_alloc = 0;

		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[0],
				&(*neighbors)[0],
				&x_neigh, &num_x_neigh,
				&x_anc,	&num_x_anc,
				&num_x_alloc,
				userlevel
				));


		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[1],
				&(*neighbors)[num_neigh_per_dir[0]],
				&y_neigh, &num_y_neigh,
				&y_anc,	&num_y_anc,
				&num_y_alloc,
				userlevel
				));

		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[2],
				&(*neighbors)[num_neigh_per_dir[0]+ num_neigh_per_dir[1]],
				&z_neigh, &num_z_neigh,
				&z_anc,	&num_z_anc,
				&num_z_alloc,
				userlevel
				));

		old_num_neigh = *nbr_neigh;
		TRY (add_common_neigh (octree,oct_idx, *neighbors, nbr_neigh, x_neigh, num_x_neigh, y_neigh, num_y_neigh));
		num_neigh_per_dir[3] = *nbr_neigh - old_num_neigh;
		old_num_neigh = *nbr_neigh;
		TRY (add_common_neigh (octree,oct_idx, *neighbors, nbr_neigh, x_neigh, num_x_neigh, z_neigh, num_z_neigh));
		num_neigh_per_dir[4] = *nbr_neigh - old_num_neigh;
		old_num_neigh = *nbr_neigh;
		TRY (add_common_neigh (octree,oct_idx, *neighbors, nbr_neigh, z_neigh, num_z_neigh, y_neigh, num_y_neigh));
		num_neigh_per_dir[5] = *nbr_neigh - old_num_neigh;
		// check that all anc have been added
		for (int i = 0; i < *nbr_neigh; i++) {
			TRY (get_ancestors (octree, (*neighbors)[i], *ancestor_of_neigh, nbr_anc_of_neigh));
		}
		TRY (h5_free (x_neigh));
		TRY (h5_free (y_neigh));
		TRY (h5_free (z_neigh));

		TRY (h5_free (x_anc));
		TRY (h5_free (y_anc));
		TRY (h5_free (z_anc));
	}

	if (kind_of_neigh > 2) {
		// idea
		// get all neig of edge neigh, if one is neigh of every edge direction it should be a point neighbor


		h5_oct_idx_t* x_neigh = NULL;
		h5_oct_idx_t* y_neigh = NULL;
		h5_oct_idx_t* z_neigh = NULL;

		h5_oct_idx_t* x_anc = NULL;
		h5_oct_idx_t* y_anc = NULL;
		h5_oct_idx_t* z_anc = NULL;

		h5_oct_idx_t num_x_neigh = 0;
		h5_oct_idx_t num_y_neigh = 0;
		h5_oct_idx_t num_z_neigh = 0;

		h5_oct_idx_t num_x_anc = 0;
		h5_oct_idx_t num_y_anc = 0;
		h5_oct_idx_t num_z_anc = 0;

		h5_oct_idx_t num_x_alloc = 0;
		h5_oct_idx_t num_y_alloc = 0;
		h5_oct_idx_t num_z_alloc = 0;

		h5_oct_idx_t num_face_neigh = num_neigh_per_dir[0] + num_neigh_per_dir[1] + num_neigh_per_dir[2];

		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[3],
				&(*neighbors)[num_face_neigh],
				&x_neigh, &num_x_neigh,
				&x_anc,	&num_x_anc,
				&num_x_alloc,
				userlevel
		));

		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[4],
				&(*neighbors)[num_face_neigh + num_neigh_per_dir[3]],
				&y_neigh, &num_y_neigh,
				&y_anc,	&num_y_anc,
				&num_y_alloc,
				userlevel
		));

		TRY (get_tmp_dir_neigh (octree,
				num_neigh_per_dir[5],
				&(*neighbors)[num_face_neigh + num_neigh_per_dir[3]+ num_neigh_per_dir[4]],
				&z_neigh, &num_z_neigh,
				&z_anc,	&num_z_anc,
				&num_z_alloc,
				userlevel
		));


		// TODO put in separate function
		h5_oct_idx_t num_cand = 0;
		// find entries that are contained in x, y and z
		for (int i = 0; i < num_x_neigh; i++) {
			h5_oct_idx_t neigh_idx = x_neigh[i];
			for (int j = 0; j < num_y_neigh; j++) {
				if (neigh_idx == y_neigh[j]) {
					for (int k = 0; k < num_z_neigh; k++) {
						if (neigh_idx == z_neigh[k] && neigh_idx != oct_idx) {
							// edge neighbor candidate is found
							num_cand = 1;
							TRY (check_neigh_cand (octree, oct_idx, &neigh_idx, &num_cand));
							if (num_cand == 1) {
								// add to neigh
								TRY (add_neigh (octree,neigh_idx, *neighbors, nbr_neigh));
								TRY (get_ancestors(octree, neigh_idx, *ancestor_of_neigh, nbr_anc_of_neigh));
							}
						}
					}
				}
			}
		}
		// There is a very strange case, where it is possible to miss vertex neighbor
		// namely if on one edge there is no edge neighbor but there still exists a vertex only neighbor
		// therefore a dirty hack:
		for (int i = 0; i < num_x_neigh; i++) {
			num_cand = 1;
			h5_oct_idx_t neigh_idx = x_neigh[i];
			TRY (check_neigh_cand (octree, oct_idx, &neigh_idx, &num_cand));
			if (num_cand == 1 && neigh_idx != oct_idx) {
				// add to neigh
				TRY (add_neigh (octree,neigh_idx, *neighbors, nbr_neigh));
				TRY (get_ancestors(octree, neigh_idx, *ancestor_of_neigh, nbr_anc_of_neigh));
			}
		}
		for (int i = 0; i < num_y_neigh; i++) {
			num_cand = 1;
			h5_oct_idx_t neigh_idx = y_neigh[i];
			TRY (check_neigh_cand (octree, oct_idx, &neigh_idx, &num_cand));
			if (num_cand == 1 && neigh_idx != oct_idx) {
				// add to neigh
				TRY (add_neigh (octree,neigh_idx, *neighbors, nbr_neigh));
				TRY (get_ancestors(octree, neigh_idx, *ancestor_of_neigh, nbr_anc_of_neigh));
			}
		}
		for (int i = 0; i < num_z_neigh; i++) {
			num_cand = 1;
			h5_oct_idx_t neigh_idx = z_neigh[i];
			TRY (check_neigh_cand (octree, oct_idx, &neigh_idx, &num_cand));
			if (num_cand == 1 && neigh_idx != oct_idx) {
				// add to neigh
				TRY (add_neigh (octree,neigh_idx, *neighbors, nbr_neigh));
				TRY (get_ancestors(octree, neigh_idx, *ancestor_of_neigh, nbr_anc_of_neigh));
			}
		}

		// check that all anc have been added
		for (int i = 0; i < *nbr_neigh; i++) {
			TRY (get_ancestors (octree, (*neighbors)[i], *ancestor_of_neigh, nbr_anc_of_neigh));
		}
		TRY (h5_free (x_neigh));
		TRY (h5_free (y_neigh));
		TRY (h5_free (z_neigh));

		TRY (h5_free (x_anc));
		TRY (h5_free (y_anc));
		TRY (h5_free (z_anc));
	}
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
H5t_get_neighbors (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t** neighbors,
	h5_oct_idx_t* nbr_neigh,
	h5_oct_idx_t** ancestor_of_neigh,
	h5_oct_idx_t* nbr_anc_of_neigh,
	h5_oct_idx_t kind_of_neigh,
	h5_oct_level_t userlevel
	) {
	return get_neighbors (
		octree,
		oct_idx,
		neighbors,
		nbr_neigh,
		ancestor_of_neigh,
		nbr_anc_of_neigh,
		kind_of_neigh,
		userlevel);
}

#if 0
/*
 * function used to get edge neighbors
 */
static h5_err_t
get_edge_dir_neigh (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx,
	h5_oct_idx_t tmp_oct_idx,
	h5_oct_dir_t tmp_dir,
	h5_oct_idx_t* neighbors,
	h5_oct_idx_t* num_neigh,
	h5_oct_idx_t* anc,
	h5_oct_idx_t* num_anc,
	h5_oct_level_t userlevel
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);

	h5_oct_idx_t* neig_candidates = NULL;
	TRY (neig_candidates = h5_calloc (OCT_MAX_NEIGHBORS , sizeof (*neig_candidates)));
	memset (neig_candidates, 0, OCT_MAX_NEIGHBORS * sizeof (*neig_candidates) );
	h5_oct_idx_t* anc_neig_cand = NULL;
	TRY (anc_neig_cand = h5_calloc (OCT_MAX_NEIGHBORS , sizeof (*anc_neig_cand)));
	memset (anc_neig_cand, 0, OCT_MAX_NEIGHBORS * sizeof (*anc_neig_cand) );
	h5_oct_idx_t num_cand = 0;
	h5_oct_idx_t num_anc_cand = 0;
	h5_oct_orient_t orient = get_orient(octree, oct_idx);
	h5_int32_t ifsibling =0; // WARNING is not set properly below
	h5_oct_orient_t* orient_child_nca = NULL;
	TRY (orient_child_nca = h5_calloc (get_oct_level(octree, oct_idx) + 1, sizeof (*orient_child_nca)));

	h5_oct_idx_t nearest_common_anc = -1;

	TRY (get_kids_with_orient (octree,
			tmp_oct_idx,
			neighbors,
			num_neigh,
			anc,
			num_anc,
			userlevel,
			orient,
			tmp_dir,
			ifsibling));

	//get other neighbors in edge direction
	TRY (get_nca (octree, oct_idx, tmp_dir, orient_child_nca, &nearest_common_anc));
	if (nearest_common_anc != -1) {

		TRY (
			get_equal_sized_neigh (
				octree,
				oct_idx,
				tmp_dir,
				orient_child_nca,
				nearest_common_anc,
				&tmp_oct_idx,
				userlevel));

		TRY (
			get_kids_with_orient (
				octree,
				tmp_oct_idx,
				neig_candidates,
				&num_cand,
				anc_neig_cand,
				&num_anc_cand,
				userlevel,
				orient,
				tmp_dir,
				ifsibling));

		check_neigh_cand (octree, oct_idx, neig_candidates, &num_cand);
		if (num_cand == 0) {
			num_anc_cand = 0;
		}
		for (int i = 0; i < num_cand; i++) {
			neighbors[(*num_neigh)++] = neig_candidates[i];
			TRY (get_ancestors(octree, neig_candidates[i], anc, num_anc));
		}
		num_cand = 0;
		num_anc_cand = 0;
	}
	TRY (h5_free (orient_child_nca));
	TRY (h5_free (neig_candidates));
	TRY (h5_free (anc_neig_cand));
	H5_RETURN (H5_SUCCESS);
}
#endif

static h5_err_t
get_tmp_dir_neigh (
	h5t_octree_t* octree,
	h5_oct_idx_t size,
	h5_oct_idx_t* oct_idxs,
	h5_oct_idx_t** neigh,
	h5_oct_idx_t* num_neigh,
	h5_oct_idx_t** anc,
	h5_oct_idx_t* num_anc,
	h5_oct_idx_t* num_alloc,
	h5_oct_level_t userlevel
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	h5_oct_idx_t* tmp_neigh = NULL;
	h5_oct_idx_t num_tmp_neigh = 0;
	h5_oct_idx_t* tmp_anc = NULL;
	h5_oct_idx_t num_tmp_anc = 0;
	// get neigh of x-y edge neigh
	for (int i = 0; i < size; i++) {
		h5_oct_idx_t curr_idx = oct_idxs[i];
		get_neighbors (octree, curr_idx, &tmp_neigh, &num_tmp_neigh, &tmp_anc, &num_tmp_anc, 1, userlevel);

		// alloc enough mem in glb array
		if (num_tmp_neigh + *num_neigh > *num_alloc ||
				num_tmp_anc + *num_anc > *num_alloc) { // alloc enough mem in x_neigh or x_neigh_anc
			TRY (*neigh = h5_alloc (*neigh, (((int)*num_alloc) + OCT_MAX_NEIGHBORS) * sizeof (**neigh)));
			TRY (*anc = h5_alloc (*anc, ((int)(*num_alloc) + OCT_MAX_NEIGHBORS) * sizeof (**anc)));
			*num_alloc += OCT_MAX_NEIGHBORS;
		}
		// copy values to global array
		for (int j = 0; j < num_tmp_neigh; j++) {
			add_neigh (octree, tmp_neigh[j], *neigh, num_neigh);
		}
		// copy values to global array
		for (int j = 0; j < num_tmp_anc; j++) {
			add_ancestor (octree, tmp_anc[j], *anc, num_anc);
		}
		num_tmp_anc = 0;
		num_tmp_neigh = 0;
	}
	TRY (h5_free (tmp_neigh));
	TRY (h5_free (tmp_anc));
	H5_RETURN (H5_SUCCESS);
}


/*
 * iterates octree iterators
 * return oct_idx of next octant or -1 if traverse is finished
 */
static h5_oct_idx_t
iterate_oct (
	h5t_oct_iterator_t* iter
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "iter=%p", iter);
	h5_oct_userlev_t userlevels = 0;
	h5_int32_t found = 0;
	h5t_oct_iter_t* it = (h5t_oct_iter_t*) iter;
	if (it->current_octant != -1) {
		if (it->current_octant == -2) {
			it->current_octant = -1;
		}
		it->current_octant++;
		while (it->current_octant <= it->octree->current_oct_idx  && !found) {

			userlevels = it->octree->octants[it->current_octant].userlevels;
			if ((userlevels & (1 << it->level) )== 1 << it->level) {
				// current oct idx points to a leaf octant
				found = 1;
				it->current_octant--;
			}
			it->current_octant++;
		}
		if ( it->current_octant > it->octree->current_oct_idx) {
			it->current_octant = -1;
		}
	}
	H5_RETURN (it->current_octant);
}

h5_oct_idx_t
H5t_iterate_oct (
	h5t_oct_iterator_t* iter
	) {
	return iterate_oct (iter);
}

static h5_err_t
init_oct_iterator (
	h5t_octree_t* octree,
	h5t_oct_iterator_t** iter,
	h5_oct_level_t level
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, iter=%p, level=%d", octree, iter, level);
	if (*iter != NULL) {
		TRY (h5_free (*iter));
	}
	TRY (*iter = h5_calloc (1, sizeof (h5t_oct_iter_t)));
	h5t_oct_iter_t* it = (h5t_oct_iter_t*) (*iter);
	it->iter = iterate_oct;
	it->current_octant = -2;
	it->octree = octree;
	it->level = level;
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
H5t_init_oct_iterator (
	h5t_octree_t* octree,
	h5t_oct_iterator_t** iter,
	h5_oct_level_t level
	) {
	return init_oct_iterator (octree, iter, level);
}

static h5_err_t
init_leafoct_iterator (
	h5t_octree_t* octree,
	h5t_oct_iterator_t** iter
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, iter=%p", octree, iter);
	init_oct_iterator (octree, iter, OCT_USERLEV_LENGTH - 1);
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_init_leafoct_iterator (
	h5t_octree_t* octree,
	h5t_oct_iterator_t** iter
	) {
	return init_leafoct_iterator (octree, iter);
}

static h5_err_t
end_iterate_oct (
	h5t_oct_iterator_t* iter
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "iter=%p", iter);
	TRY (h5_free (iter));
	H5_RETURN (H5_SUCCESS);
}
h5_err_t
H5t_end_iterate_oct (
	h5t_oct_iterator_t* iter
	) {
	return end_iterate_oct (iter);
}

static h5_oct_idx_t
get_num_oct_leaflevel (
	h5t_octree_t* octree
	) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p", octree);
	h5t_oct_iterator_t* iter = NULL;
	TRY (init_leafoct_iterator (octree, &iter));
	h5_oct_idx_t counter = 0;
	while (iterate_oct (iter) != -1) {
		counter++;
	}
	end_iterate_oct (iter);
	H5_RETURN (counter);
}
h5_oct_idx_t H5t_get_num_oct_leaflevel (h5t_octree_t* octree) {
	return get_num_oct_leaflevel (octree);
}

h5_oct_idx_t H5t_get_num_octants (h5t_octree_t* octree) {
	return octree->current_oct_idx +1;
}
/*
 * complete level
 */
static h5_err_t
complete_level(
        h5t_octree_t* octree,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, level=%d", octree, level);
	// IDEA go through level, and make all siblings & parents +siblings as on level
	// then then just keep finest level
	// check should be successful after complete_level
	update_internal (octree);

	h5t_oct_iterator_t* iterator = NULL;
	init_oct_iterator (octree, &iterator, level);
	h5_oct_idx_t octant = -1;
	h5_oct_idx_t parent = -1;
	h5_oct_idx_t siblings = -1;
	h5_int32_t done;
	set_userlevel_int (octree, 0, level);
	while ((octant = iterate_oct (iterator)) != -1) {

		if (oct_has_level (octree, octant, level) && octant > 0) {
			// set siblings and parent to level
			get_siblings (octree,octant, &siblings);
			if (siblings != -1 && !(siblings < octant && oct_has_level (octree, siblings, level))) {
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			set_userlevel_int (octree, siblings++, level);
			} else {
				// octant and it's parents have already been set to level, therefore avoid setting again
			}
			done = 0;
			parent = octant;
			while ((parent=get_parent(octree, parent)) != -1 && done == 0){
				if (oct_has_level (octree, parent, level)){
					done = 1;
				} else {
					get_siblings (octree, parent, &siblings);
					if(siblings != -1){
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
						set_userlevel_int (octree, siblings++, level);
					}
				}
			}
		}

	}
	end_iterate_oct (iterator);
	// remove multiple levels on parents
	iterator = NULL;
	init_leafoct_iterator (octree, &iterator);
	done = 0;
	while ((octant = iterate_oct(iterator)) != -1) {
		parent = octant;
		while (oct_has_level (octree, parent, level) == 0 && parent != -1) {
			parent = get_parent(octree, parent);
		}
		if (parent == -1){
			H5_LEAVE (H5_ERR_INTERNAL);
		}
		done = 0;
		// mark all parents of parent as not on level
		while ((parent = get_parent (octree, parent)) != -1 && done == 0) {
			if (oct_has_level (octree, parent, level) == 0) {
				done = 1;
			}
			remove_userlevel_int (octree, parent, level);
		}
	}
	end_iterate_oct (iterator);

	clear_level_internal (octree);
	H5_RETURN (H5_SUCCESS);
}

h5_err_t
H5t_complete_userlevel(h5t_octree_t* octree, h5_oct_level_t level) {
	return complete_level(octree,level);
}

#if 0
/*
 * check level
 */
// BUG does not work properly yet
static h5_err_t
check_level (
        h5t_octree_t* octree,
        h5_oct_level_t level
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, level=%d", octree, level);
	// idea go from leaf level octants up to root. exactly one time they should hit the level!
	h5t_oct_iterator_t* iterator = NULL;
	init_leafoct_iterator (octree, &iterator);
	h5_oct_idx_t octant = -1;
	h5_oct_idx_t parent = -1;
	h5_int32_t counter = 0;
	while ((octant = iterate_oct (iterator)) != -1){
		counter = 0;
		if (oct_has_level (octree, octant, level)) {
			counter++;
		}
		while ((parent=get_parent (octree, octant)) != -1){
			if (oct_has_level (octree, parent, level)) {
						counter++;
					}
		}
		if (counter != 1){
			H5_LEAVE (H5_ERR_INVAL);
		}
	}
	end_iterate_oct (iterator);
	H5_RETURN (H5_SUCCESS);
}
#endif

/*
 * set bounding box for root octant
 */
static h5_err_t
set_bounding_box (
        h5t_octree_t* octree,
        h5_float64_t* bounding_box
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, bounding_box=%p", octree, bounding_box);
	if ((bounding_box[0] >= bounding_box[3]) ||
		(bounding_box[1] >= bounding_box[4]) ||
		(bounding_box[2] >= bounding_box[5])
		) {
		H5_LEAVE (H5_ERR_INVAL);
	}

	memcpy(octree->bounding_box, bounding_box, 6 * sizeof (*bounding_box));

	H5_RETURN (H5_SUCCESS);
}
h5_err_t H5t_set_bounding_box ( h5t_octree_t* octree, h5_float64_t* bounding_box) {
	return set_bounding_box (octree, bounding_box);
}

static h5_float64_t*
get_bounding_box (
		h5t_octree_t* const octree
		) {
	return octree->bounding_box;
}
h5_float64_t* H5t_get_bounding_box (h5t_octree_t* const octree) {
	return get_bounding_box (octree);
}
/*
 * func sceleton
 */
static h5_err_t
func_name (
        h5t_octree_t* octree,
        h5_oct_idx_t oct_idx
        ) {
	H5_PRIV_FUNC_ENTER (h5_err_t, "octree=%p, oct_idx=%d", octree, oct_idx);

	H5_RETURN (H5_SUCCESS);
}

h5_err_t
H5t_func_name (
	h5t_octree_t* octree,
	h5_oct_idx_t oct_idx
	) {
	return func_name (octree, oct_idx);
}

#endif
