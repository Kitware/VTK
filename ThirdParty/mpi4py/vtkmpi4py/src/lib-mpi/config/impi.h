#ifndef PyMPI_CONFIG_IMPI_H
#define PyMPI_CONFIG_IMPI_H

#include "mpiapi.h"

/* These types may not be available */
#ifndef MPI_REAL2
#undef PyMPI_HAVE_MPI_REAL2
#endif
#ifndef MPI_MPI_COMPLEX4
#undef PyMPI_HAVE_MPI_COMPLEX4
#endif

#if !defined(CIBUILDWHEEL)

#if I_MPI_NUMVERSION >= 20210900300
#define PyMPI_HAVE_MPI_Bcast_c 1
#define PyMPI_HAVE_MPI_Gather_c 1
#define PyMPI_HAVE_MPI_Scatter_c 1
#define PyMPI_HAVE_MPI_Allgather_c 1
#define PyMPI_HAVE_MPI_Alltoall_c 1
#define PyMPI_HAVE_MPI_Reduce_c 1
#define PyMPI_HAVE_MPI_Allreduce_c 1
#define PyMPI_HAVE_MPI_Reduce_scatter_block_c 1
#define PyMPI_HAVE_MPI_Scan_c 1
#define PyMPI_HAVE_MPI_Exscan_c 1
#define PyMPI_HAVE_MPI_Neighbor_allgather_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoall_c 1
#define PyMPI_HAVE_MPI_Ibcast_c 1
#define PyMPI_HAVE_MPI_Igather_c 1
#define PyMPI_HAVE_MPI_Iscatter_c 1
#define PyMPI_HAVE_MPI_Iallgather_c 1
#define PyMPI_HAVE_MPI_Ialltoall_c 1
#define PyMPI_HAVE_MPI_Ireduce_c 1
#define PyMPI_HAVE_MPI_Iallreduce_c 1
#define PyMPI_HAVE_MPI_Ireduce_scatter_block_c 1
#define PyMPI_HAVE_MPI_Iscan_c 1
#define PyMPI_HAVE_MPI_Iexscan_c 1
#define PyMPI_HAVE_MPI_Ineighbor_allgather_c 1
#define PyMPI_HAVE_MPI_Ineighbor_alltoall_c 1
#endif

#if I_MPI_NUMVERSION >= 20211000300
#define PyMPI_HAVE_MPI_Buffer_attach_c 1
#define PyMPI_HAVE_MPI_Buffer_detach_c 1
#define PyMPI_HAVE_MPI_Send_c 1
#define PyMPI_HAVE_MPI_Recv_c 1
#define PyMPI_HAVE_MPI_Sendrecv_c 1
#define PyMPI_HAVE_MPI_Sendrecv_replace_c 1
#define PyMPI_HAVE_MPI_Bsend_c 1
#define PyMPI_HAVE_MPI_Ssend_c 1
#define PyMPI_HAVE_MPI_Rsend_c 1
#define PyMPI_HAVE_MPI_Isend_c 1
#define PyMPI_HAVE_MPI_Irecv_c 1
#define PyMPI_HAVE_MPI_Ibsend_c 1
#define PyMPI_HAVE_MPI_Issend_c 1
#define PyMPI_HAVE_MPI_Irsend_c 1
#define PyMPI_HAVE_MPI_Send_init_c 1
#define PyMPI_HAVE_MPI_Recv_init_c 1
#define PyMPI_HAVE_MPI_Bsend_init_c 1
#define PyMPI_HAVE_MPI_Ssend_init_c 1
#define PyMPI_HAVE_MPI_Rsend_init_c 1
#define PyMPI_HAVE_MPI_Mrecv_c 1
#define PyMPI_HAVE_MPI_Imrecv_c 1
#define PyMPI_HAVE_MPI_Gatherv_c 1
#define PyMPI_HAVE_MPI_Scatterv_c 1
#define PyMPI_HAVE_MPI_Allgatherv_c 1
#define PyMPI_HAVE_MPI_Alltoallv_c 1
#define PyMPI_HAVE_MPI_Alltoallw_c 1
#define PyMPI_HAVE_MPI_Reduce_scatter_c 1
#define PyMPI_HAVE_MPI_Neighbor_allgatherv_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallv_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallw_c 1
#define PyMPI_HAVE_MPI_Igatherv_c 1
#define PyMPI_HAVE_MPI_Iscatterv_c 1
#define PyMPI_HAVE_MPI_Iallgatherv_c 1
#define PyMPI_HAVE_MPI_Ialltoallv_c 1
#define PyMPI_HAVE_MPI_Ialltoallw_c 1
#define PyMPI_HAVE_MPI_Ireduce_scatter_c 1
#define PyMPI_HAVE_MPI_Ineighbor_allgatherv_c 1
#define PyMPI_HAVE_MPI_Ineighbor_alltoallv_c 1
#define PyMPI_HAVE_MPI_Ineighbor_alltoallw_c 1
#endif

#if I_MPI_NUMVERSION >= 20211100300
#define PyMPI_HAVE_MPI_Session 1
#define PyMPI_HAVE_MPI_ERRORS_ABORT 1
#define PyMPI_HAVE_MPI_SESSION_NULL 1
#define PyMPI_HAVE_MPI_MAX_PSET_NAME_LEN 1
#define PyMPI_HAVE_MPI_Session_init 1
#define PyMPI_HAVE_MPI_Session_finalize 1
#define PyMPI_HAVE_MPI_Session_get_num_psets 1
#define PyMPI_HAVE_MPI_Session_get_nth_pset 1
#define PyMPI_HAVE_MPI_Session_get_info 1
#define PyMPI_HAVE_MPI_Session_get_pset_info 1
#define PyMPI_HAVE_MPI_Group_from_session_pset 1
#define PyMPI_HAVE_MPI_Session_errhandler_function 1
#define PyMPI_HAVE_MPI_Session_create_errhandler 1
#define PyMPI_HAVE_MPI_Session_get_errhandler 1
#define PyMPI_HAVE_MPI_Session_set_errhandler 1
#define PyMPI_HAVE_MPI_Session_call_errhandler 1
#define PyMPI_HAVE_MPI_MAX_STRINGTAG_LEN 1
#define PyMPI_HAVE_MPI_Comm_create_from_group 1
#define PyMPI_HAVE_MPI_COMM_TYPE_HW_GUIDED 1
#define PyMPI_HAVE_MPI_ERR_SESSION 1
#define PyMPI_HAVE_MPI_Session_c2f 1
#define PyMPI_HAVE_MPI_Session_f2c 1
#endif

#if I_MPI_NUMVERSION >= 20211200300
#define PyMPI_HAVE_MPI_Type_contiguous_c 1
#define PyMPI_HAVE_MPI_Type_vector_c 1
#define PyMPI_HAVE_MPI_Type_indexed_c 1
#define PyMPI_HAVE_MPI_Type_create_indexed_block_c 1
#define PyMPI_HAVE_MPI_Type_create_subarray_c 1
#define PyMPI_HAVE_MPI_Type_create_darray_c 1
#define PyMPI_HAVE_MPI_Type_create_hvector_c 1
#define PyMPI_HAVE_MPI_Type_create_hindexed_c 1
#define PyMPI_HAVE_MPI_Type_create_hindexed_block_c 1
#define PyMPI_HAVE_MPI_Type_create_struct_c 1
#define PyMPI_HAVE_MPI_Type_create_resized_c 1
#define PyMPI_HAVE_MPI_Type_size_c 1
#define PyMPI_HAVE_MPI_Type_get_extent_c 1
#define PyMPI_HAVE_MPI_Type_get_true_extent_c 1
#define PyMPI_HAVE_MPI_Type_get_envelope_c 1
#define PyMPI_HAVE_MPI_Type_get_contents_c 1
#define PyMPI_HAVE_MPI_Pack_c 1
#define PyMPI_HAVE_MPI_Unpack_c 1
#define PyMPI_HAVE_MPI_Pack_size_c 1
#define PyMPI_HAVE_MPI_Pack_external_c 1
#define PyMPI_HAVE_MPI_Unpack_external_c 1
#define PyMPI_HAVE_MPI_Pack_external_size_c 1
#define PyMPI_HAVE_MPI_Get_count_c 1
#define PyMPI_HAVE_MPI_Get_elements_c 1
#define PyMPI_HAVE_MPI_Status_set_elements_c 1
#define PyMPI_HAVE_MPI_Barrier_init 1
#define PyMPI_HAVE_MPI_Bcast_init 1
#define PyMPI_HAVE_MPI_Gather_init 1
#define PyMPI_HAVE_MPI_Gatherv_init 1
#define PyMPI_HAVE_MPI_Scatter_init 1
#define PyMPI_HAVE_MPI_Scatterv_init 1
#define PyMPI_HAVE_MPI_Allgather_init 1
#define PyMPI_HAVE_MPI_Allgatherv_init 1
#define PyMPI_HAVE_MPI_Alltoall_init 1
#define PyMPI_HAVE_MPI_Alltoallv_init 1
#define PyMPI_HAVE_MPI_Alltoallw_init 1
#define PyMPI_HAVE_MPI_Reduce_init 1
#define PyMPI_HAVE_MPI_Allreduce_init 1
#define PyMPI_HAVE_MPI_Reduce_scatter_block_init 1
#define PyMPI_HAVE_MPI_Reduce_scatter_init 1
#define PyMPI_HAVE_MPI_Scan_init 1
#define PyMPI_HAVE_MPI_Exscan_init 1
#define PyMPI_HAVE_MPI_Neighbor_allgather_init 1
#define PyMPI_HAVE_MPI_Neighbor_allgatherv_init 1
#define PyMPI_HAVE_MPI_Neighbor_alltoall_init 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallv_init 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallw_init 1
#if defined(__linux__)
#define PyMPI_HAVE_MPI_Bcast_init_c 1
#define PyMPI_HAVE_MPI_Gather_init_c 1
#define PyMPI_HAVE_MPI_Gatherv_init_c 1
#define PyMPI_HAVE_MPI_Scatter_init_c 1
#define PyMPI_HAVE_MPI_Scatterv_init_c 1
#define PyMPI_HAVE_MPI_Allgather_init_c 1
#define PyMPI_HAVE_MPI_Allgatherv_init_c 1
#define PyMPI_HAVE_MPI_Alltoall_init_c 1
#define PyMPI_HAVE_MPI_Alltoallv_init_c 1
#define PyMPI_HAVE_MPI_Alltoallw_init_c 1
#define PyMPI_HAVE_MPI_Reduce_init_c 1
#define PyMPI_HAVE_MPI_Allreduce_init_c 1
#define PyMPI_HAVE_MPI_Reduce_scatter_block_init_c 1
#define PyMPI_HAVE_MPI_Reduce_scatter_init_c 1
#define PyMPI_HAVE_MPI_Scan_init_c 1
#define PyMPI_HAVE_MPI_Exscan_init_c 1
#define PyMPI_HAVE_MPI_Neighbor_allgather_init_c 1
#define PyMPI_HAVE_MPI_Neighbor_allgatherv_init_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoall_init_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallv_init_c 1
#define PyMPI_HAVE_MPI_Neighbor_alltoallw_init_c 1
#endif
#define PyMPI_HAVE_MPI_Win_create_c 1
#define PyMPI_HAVE_MPI_Win_allocate_c 1
#define PyMPI_HAVE_MPI_Win_allocate_shared_c 1
#define PyMPI_HAVE_MPI_Win_shared_query_c 1
#define PyMPI_HAVE_MPI_Get_c 1
#define PyMPI_HAVE_MPI_Put_c 1
#define PyMPI_HAVE_MPI_Accumulate_c 1
#define PyMPI_HAVE_MPI_Get_accumulate_c 1
#define PyMPI_HAVE_MPI_Rget_c 1
#define PyMPI_HAVE_MPI_Rput_c 1
#define PyMPI_HAVE_MPI_Raccumulate_c 1
#define PyMPI_HAVE_MPI_Rget_accumulate_c 1
#if defined(__linux__)
#define PyMPI_HAVE_MPI_File_read_at_c 1
#define PyMPI_HAVE_MPI_File_read_at_all_c 1
#define PyMPI_HAVE_MPI_File_write_at_c 1
#define PyMPI_HAVE_MPI_File_write_at_all_c 1
#define PyMPI_HAVE_MPI_File_iread_at_c 1
#define PyMPI_HAVE_MPI_File_iread_at_all_c 1
#define PyMPI_HAVE_MPI_File_iwrite_at_c 1
#define PyMPI_HAVE_MPI_File_iwrite_at_all_c 1
#define PyMPI_HAVE_MPI_File_read_c 1
#define PyMPI_HAVE_MPI_File_read_all_c 1
#define PyMPI_HAVE_MPI_File_write_c 1
#define PyMPI_HAVE_MPI_File_write_all_c 1
#define PyMPI_HAVE_MPI_File_iread_c 1
#define PyMPI_HAVE_MPI_File_iread_all_c 1
#define PyMPI_HAVE_MPI_File_iwrite_c 1
#define PyMPI_HAVE_MPI_File_iwrite_all_c 1
#define PyMPI_HAVE_MPI_File_read_shared_c 1
#define PyMPI_HAVE_MPI_File_write_shared_c 1
#define PyMPI_HAVE_MPI_File_iread_shared_c 1
#define PyMPI_HAVE_MPI_File_iwrite_shared_c 1
#define PyMPI_HAVE_MPI_File_read_ordered_c 1
#define PyMPI_HAVE_MPI_File_write_ordered_c 1
#define PyMPI_HAVE_MPI_File_read_at_all_begin_c 1
#define PyMPI_HAVE_MPI_File_write_at_all_begin_c 1
#define PyMPI_HAVE_MPI_File_read_all_begin_c 1
#define PyMPI_HAVE_MPI_File_write_all_begin_c 1
#define PyMPI_HAVE_MPI_File_read_ordered_begin_c 1
#define PyMPI_HAVE_MPI_File_write_ordered_begin_c 1
#define PyMPI_HAVE_MPI_File_get_type_extent_c 1
#define PyMPI_HAVE_MPI_Datarep_conversion_function_c 1
#define PyMPI_HAVE_MPI_CONVERSION_FN_NULL_C 1
#define PyMPI_HAVE_MPI_Register_datarep_c 1
#endif
#endif

#if I_MPI_NUMVERSION >= 20211300300
#define PyMPI_HAVE_MPI_User_function_c 1
#define PyMPI_HAVE_MPI_Op_create_c 1
#define PyMPI_HAVE_MPI_Reduce_local_c 1
#endif

#endif

#endif /* !PyMPI_CONFIG_IMPI_H */
