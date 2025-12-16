/*
  Copyright (c) 2006-2016, The Regents of the University of California,
  through Lawrence Berkeley National Laboratory (subject to receipt of any
  required approvals from the U.S. Dept. of Energy) and the Paul Scherrer
  Institut (Switzerland).  All rights reserved.

  License: see file COPYING in top level of source distribution.
*/

#ifndef __PRIVATE_H5_MPI_H
#define __PRIVATE_H5_MPI_H

#ifdef H5_HAVE_PARALLEL

#include "h5core/h5_types.h"
#include "h5core/h5_err.h"
#include "private/h5_log.h"

#define MPI_WRAPPER_ENTER(type, fmt, ...)				\
	__FUNC_ENTER(type, H5_DEBUG_MPI, fmt, __VA_ARGS__ )

static inline h5_err_t
h5priv_mpi_alltoall (
        void* sendbuf,
        const int sendcount,
        const MPI_Datatype sendtype,
        void* recvbuf,
        const int recvcount,
        const MPI_Datatype recvtype,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, sendcount=%d, sendtype=?, recvbuf=%p, "
	                   "recvcount=%d, recvtype=?, comm=?",
	                   sendbuf, sendcount, recvbuf, recvcount);
	int err = MPI_Alltoall (
	        sendbuf,
	        sendcount,
	        sendtype,
	        recvbuf,
	        recvcount,
	        recvtype,
	        comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform all to all communication");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_alltoallv (
        void* sendbuf,
        int* sendcounts,
        int* senddispls,
        const MPI_Datatype sendtype,
        void* recvbuf,
        int* recvcounts,
        int* recvdispls,
        const MPI_Datatype recvtype,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, sendcounts=%p, senddispls=%p, sendtype=?, "
	                   "recvbuf=%p, recvcounts=%p, recvdispls=%p, recvtype=?, "
	                   "comm=?",
	                   sendbuf, sendcounts, senddispls,
	                   recvbuf, recvcounts, recvdispls);
	int err = MPI_Alltoallv (
	        sendbuf,
	        sendcounts,
	        senddispls,
	        sendtype,
	        recvbuf,
	        recvcounts,
	        recvdispls,
	        recvtype,
	        comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform all to all communication");
	H5_RETURN (H5_SUCCESS);
}

// barrier
static inline h5_err_t
h5priv_mpi_barrier (
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "comm=%p",&comm);
	int err = MPI_Barrier(comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"MPI Barrier was not successful");
	H5_RETURN (H5_SUCCESS);
}

// communication routines
static inline h5_err_t
h5priv_mpi_recv(
        void* buf,
        const int count,
        const MPI_Datatype type,
        const int from,
        const int tag,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "buf=%p, count=%d, type=?, from=%d, tag=%d, comm=?",
	                   buf, count, from, tag);
	int err = MPI_Recv(
	        buf,
	        count,
	        type,
	        from,
	        tag,
	        comm,
	        MPI_STATUS_IGNORE
	        );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot receive data");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_send(
        void* buf,
        const int count,
        const MPI_Datatype type,
        const int to,
        const int tag,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "buf=%p, count=%d, type=?, to=%d, tag=%d, comm=?",
	                   buf, count, to, tag);
	int err = MPI_Send(
	        buf,
	        count,
	        type,
	        to,
	        tag,
	        comm
	        );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot send data");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_bcast (
        void* buf,
        const int count,
        const MPI_Datatype type,
        const int root,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "buf=%p, count=%d, type=?, root=%d, comm=?",
	                   buf, count, root);
	int err = MPI_Bcast(
	        buf,
	        count,
	        type,
	        root,
	        comm
	        );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform broadcast");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_sum (
        void* sendbuf,
        void* recvbuf,
        const int count,
        const MPI_Datatype type,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, recvbuf=%p, count=%d, type=?, comm=?",
	                   sendbuf, recvbuf, count);
	int err = MPI_Allreduce(
	        sendbuf,
	        recvbuf,
	        count,
	        type,
	        MPI_SUM,
	        comm
	        );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform MPI_SUM reduction");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_allreduce_max (
        void* sendbuf,
        void* recvbuf,
        const int count,
        const MPI_Datatype type,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, recvbuf=%p, count=%d, type=?, comm=?",
	                   sendbuf, recvbuf, count);
	if (MPI_Allreduce(
		    sendbuf,
		    recvbuf,
		    count,
		    type,
		    MPI_MAX,
		    comm
		    ) != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform MPI_MAX reduction");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_prefix_sum (
        void* sendbuf,
        void* recvbuf,
        const int count,
        const MPI_Datatype type,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, recvbuf=%p, count=%d, type=?, comm=?",
	                   sendbuf, recvbuf, count);
	int err = MPI_Scan(
	        sendbuf,
	        recvbuf,
	        count,
	        type,
	        MPI_SUM,
	        comm
	        );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot perform prefix sum");
	H5_RETURN (H5_SUCCESS);
}

#define h5priv_mpi_allgather mpi_allgather
static inline h5_err_t
mpi_allgather (
        void* sendbuf,
        const int sendcount,
        const MPI_Datatype sendtype,
        void* recvbuf,
        const int recvcount,
        const MPI_Datatype recvtype,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, sendcount=%d, sendtype=?, recvbuf=%p, "
	                   "recvcount=%d, recvtype=?, comm=?",
	                   sendbuf, sendcount, recvbuf, recvcount);
	int err = MPI_Allgather (
	        sendbuf,
	        sendcount,
	        sendtype,
	        recvbuf,
	        recvcount,
	        recvtype,
	        comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot gather data");
	H5_RETURN (H5_SUCCESS);
}
#define h5priv_mpi_allgatherv mpi_allgatherv
static inline h5_err_t
mpi_allgatherv (
        void* sendbuf,
        const int sendcount,
        const MPI_Datatype sendtype,
        void* recvbuf,
        int* recvcounts,
        int* recvdispls,
        const MPI_Datatype recvtype,
        const MPI_Comm comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "sendbuf=%p, sendcount=%d, sendtype=?, "
	                   "recvbuf=%p, recvcounts=%p, recvtype=?, recvdispls=%p, "
	                   "&comm=%p",
	                   sendbuf, sendcount, recvbuf, recvcounts, recvdispls, &comm);
	int err = MPI_Allgatherv (
	        sendbuf,
	        sendcount,
	        sendtype,
	        recvbuf,
	        recvcounts,
	        recvdispls,
	        recvtype,
	        comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot gather data");
	H5_RETURN (H5_SUCCESS);
}

///
static inline h5_err_t
h5priv_mpi_comm_size (
        MPI_Comm comm,
        int* size
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "comm=?, size=%p", size);
	int err = MPI_Comm_size (comm, size);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot get communicator size");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_comm_rank (
        MPI_Comm comm,
        int* rank
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "comm=?, rank=%p", rank);
	int err = MPI_Comm_rank (comm, rank);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot get this task's rank");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_type_contiguous (
        const size_t nelems,
        const MPI_Datatype oldtype,
        MPI_Datatype *const newtype
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
			   "nelems=%lu, oldtype=?, newtype=?",
			   (long unsigned)nelems);
	int err;
	err = MPI_Type_contiguous ( nelems, oldtype, newtype );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot create new MPI type");
	err = MPI_Type_commit ( newtype );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot commit new MPI type");
	H5_RETURN (H5_SUCCESS);
}

#define h5priv_mpi_get_address mpi_get_address
static inline h5_err_t
mpi_get_address (
        void* location,
        MPI_Aint* address
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "location=%p, address=%p", location, address);
	int err = MPI_Get_address (location, address);
	if (err != MPI_SUCCESS) {
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"Cannot get MPI address of location=%p", location);
	}
	H5_RETURN (H5_SUCCESS);
}

#define h5priv_mpi_create_type_struct  mpi_create_type_struct
static inline h5_err_t
mpi_create_type_struct (
        int count,
        int blocklens[],
        MPI_Aint indices[],
        MPI_Datatype old_types[],
        MPI_Datatype* new_type
        ) {
	MPI_WRAPPER_ENTER (h5_err_t,
	                   "count=%d, blocklens=%p, indices=%p, old_types=%p, new_type=%p",
	                   count, blocklens, indices, old_types, new_type);
	int err = MPI_Type_create_struct (count, blocklens, indices, old_types, new_type);
	if (err != MPI_SUCCESS) {
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot create new MPI struct");
	}
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_type_commit (
		MPI_Datatype* type
		) {
	MPI_WRAPPER_ENTER (h5_err_t, "type=%p", type);
	int err = MPI_Type_commit (type);
	if (err != MPI_SUCCESS) {
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot commit MPI datatype");
	}
	H5_RETURN (H5_SUCCESS);
}


static inline h5_err_t
h5priv_mpi_type_free (
        MPI_Datatype *type
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "type=%p", type);
	int err = MPI_Type_free( type );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot free MPI type");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_cart_create (
        MPI_Comm old_comm,
        int ndims,
        int *dims,
        int *period,
        int reorder,
        MPI_Comm *new_comm
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "old_comm=?, ndims=%d, dims=%p, period=%p, "
	                   "reorder=%d, new_comm=%p",
	                   ndims, dims, period, reorder, new_comm);
	int err = MPI_Cart_create(
	        old_comm, ndims, dims, period, reorder, new_comm);
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot create cartesian grid");
	H5_RETURN (H5_SUCCESS);
}

static inline h5_err_t
h5priv_mpi_cart_coords (
        MPI_Comm comm,
        int rank,
        int maxdim,
        int *coords
        ) {
	MPI_WRAPPER_ENTER (h5_err_t, "comm=?, rank=%d, maxdim=%d, coords=%p",
	                   rank, maxdim, coords);
	int err = MPI_Cart_coords( comm, rank, maxdim, coords );
	if (err != MPI_SUCCESS)
		H5_RETURN_ERROR (
			H5_ERR_MPI,
			"%s",
			"Cannot create cartesian grid");
	H5_RETURN (H5_SUCCESS);
}

#endif
#endif
