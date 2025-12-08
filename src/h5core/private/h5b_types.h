#ifndef __PRIVATE_H5B_TYPES_H
#define __PRIVATE_H5B_TYPES_H

struct h5b_partition {
	h5_int64_t i_start;
	h5_int64_t i_end;
	h5_int64_t j_start;
	h5_int64_t j_end;
	h5_int64_t k_start;
	h5_int64_t k_end;
};

struct h5b_fdata {
	h5_id_t iteration_idx;
	h5_size_t i_max;
	h5_size_t j_max;
	h5_size_t k_max;
	struct h5b_partition user_layout[1];
	struct h5b_partition write_layout[1];
	int have_layout;

	MPI_Comm cart_comm;
	h5_size_t i_grid;
	h5_size_t j_grid;
	h5_size_t k_grid;
	int have_grid;

	hid_t shape;
	hid_t memshape;
	hid_t diskshape;
	hid_t block_gid;
	hid_t field_gid;
	hid_t dcreate_prop;

	MPI_Datatype partition_mpi_t;
};
typedef struct h5b_fdata h5b_fdata_t;
typedef struct h5b_partition h5b_partition_t;
#endif
