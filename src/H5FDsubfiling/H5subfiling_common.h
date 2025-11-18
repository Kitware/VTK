/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Header file for shared code between the HDF5 Subfiling VFD and IOC VFD
 */

#ifndef H5_SUBFILING_COMMON_H
#define H5_SUBFILING_COMMON_H

#include <stdatomic.h>

#include "H5private.h"
#include "H5FDprivate.h"
#include "H5Iprivate.h"
#include "H5Pprivate.h"

#include "H5FDsubfiling.h"
#include "H5FDioc.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/*
 * Some definitions for debugging the Subfiling feature
 */
/* #define H5_SUBFILING_DEBUG */

/*
 * Some definitions for controlling performance across
 * different machines where some types of MPI operations
 * may be better optimized than others
 */
/* #define H5_SUBFILING_PREFER_ALLGATHER_TOPOLOGY */
#ifndef H5_SUBFILING_PREFER_ALLGATHER_TOPOLOGY
#if !H5_CHECK_MPI_VERSION(3, 0)
#error "MPI 3 required for MPI_Comm_split_type"
#endif
#endif

/*
 * Name of the HDF5 FAPL property that the Subfiling VFD
 * uses to pass its configuration down to the underlying
 * IOC VFD
 */
#define H5FD_SUBFILING_CONFIG_PROP "H5FD_SUBFILING_CONFIG_PROP"

/*
 * Name of the HDF5 FAPL property that the Subfiling VFD
 * uses to pass the HDF5 stub file's Inode value to the
 * underlying IOC VFD
 */
#define H5FD_SUBFILING_STUB_FILE_ID "H5FD_SUBFILING_STUB_FILE_ID"

/*
 * MPI Tags are 32 bits, we treat them as unsigned
 * to allow the use of the available bits for RPC
 * selections, i.e. a message from the VFD read or write functions
 * to an IO Concentrator.  The messages themselves are in general
 * ONLY 3 int64_t values which define a) the data size to be read
 * or written, b) the file offset where the data will be read from
 * or stored, and c) the context_id allows the IO concentrator to
 * locate the IO context for the new IO transaction.
 *
 *    0000
 *    0001 READ_OP  (Independent)
 *    0010 WRITE_OP (Independent)
 *    0011 /////////
 *    0100 CLOSE_OP (Independent)
 *    -----
 *    1000
 *    1001 COLLECTIVE_READ
 *    1010 COLLECTIVE_WRITE
 *    1011 /////////
 *    1100 COLLECTIVE_CLOSE
 *
 *   31    28      24      20      16      12       8       4       0|
 *   +-------+-------+-------+-------+-------+-------+-------+-------+
 *   |       |       |              ACKS             |      OP       |
 *   +-------+-------+-------+-------+-------+-------+-------+-------+
 *
 */

/* Bit 3 SET indicates collectives */
#define COLL_FUNC (0x1 << 3)

#define ACK_PART  (0x01 << 8)
#define DATA_PART (0x02 << 8)
#define READY     (0x04 << 8)
#define COMPLETED (0x08 << 8)

#define INT32_MASK 0x07FFFFFFFFFFFFFFF

#define READ_INDEP  (READ_OP)
#define READ_COLL   (COLL_FUNC | READ_OP)
#define WRITE_INDEP (WRITE_OP)
#define WRITE_COLL  (COLL_FUNC | WRITE_OP)

#define GET_EOF_COMPLETED (COMPLETED | GET_EOF_OP)
#define TRUNC_COMPLETED   (COMPLETED | TRUNC_OP)

#define SET_LOGGING (LOGGING_OP)

/* MPI tag values for data communicator */
#define WRITE_INDEP_ACK 0
#define READ_INDEP_ACK  1
#define READ_INDEP_DATA 2
#define WRITE_DATA_DONE 3
#define IO_TAG_BASE     4

/*
 * Object type definitions for subfiling objects.
 * Used when generating a new subfiling object ID
 * or accessing the cache of stored subfiling
 * objects.
 */
typedef enum {
    SF_BADID    = (-1),
    SF_TOPOLOGY = 1,
    SF_CONTEXT  = 2,
    SF_NTYPES /* number of subfiling object types, MUST BE LAST */
} sf_obj_type_t;

/* The following are the basic 'op codes' used when
 * constructing a RPC message for IO Concentrators.
 * These are defined in the low 8 bits of the
 * message.
 */
typedef enum io_ops {
    READ_OP    = 1,
    WRITE_OP   = 2,
    OPEN_OP    = 3,
    CLOSE_OP   = 4,
    TRUNC_OP   = 5,
    GET_EOF_OP = 6,
    FINI_OP    = 8,
    LOGGING_OP = 16
} io_op_t;

/*
 * Every MPI rank in a file's communicator will
 * record their MPI rank for the file communicator
 * and their node-local MPI rank for the node's
 * communicator. Then the resulting information
 * will be broadcast to all MPI ranks and will
 * provide a basis for determining which MPI ranks
 * will host an I/O concentrator.
 *
 * - rank
 *     The MPI rank value for this processor
 *
 * - node_local_rank
 *     The MPI rank value for this processor in an MPI
 *     communicator that only involves MPI ranks on the
 *     same node as this processor
 *
 * - node_local_size
 *     The number of MPI ranks on the same node as this
 *     processor, including this processor itself
 *
 * - node_lead_rank
 *     The lowest MPI rank value for processors on the
 *     same node as this processor (possibly the MPI
 *     rank value for this processor); Denotes a "lead"
 *     MPI rank for certain operations.
 *
 */
typedef struct {
    int rank;
    int node_local_rank;
    int node_local_size;
    int node_lead_rank;
} layout_t;

/*
 * This typedef defines a fixed process layout which
 * can be reused for any number of file open operations
 */
typedef struct app_layout_t {
    layout_t *layout;          /* Array of (rank, node local rank, node local size) values */
    int      *node_ranks;      /* Array of lowest MPI rank values on each node             */
    int       node_count;      /* Total number of nodes                                    */
    int       world_rank;      /* MPI rank in file communicator                            */
    int       world_size;      /* Size of file communicator                                */
    int       node_local_rank; /* MPI rank on node                                         */
    int       node_local_size; /* Size of node intra-communicator                          */
} app_layout_t;

/*  This typedef defines things related to IOC selections */
typedef struct topology {
    app_layout_t               *app_layout;         /* Pointer to our layout struct       */
    MPI_Comm                    app_comm;           /* MPI communicator for this topology */
    bool                        rank_is_ioc;        /* Indicates that we host an IOC      */
    int                         ioc_idx;            /* Valid only if rank_is_ioc          */
    int                         n_io_concentrators; /* Number of I/O concentrators        */
    int                        *io_concentrators;   /* Vector of ranks which are IOCs     */
    H5FD_subfiling_ioc_select_t selection_type;     /* Cache our IOC selection criteria   */
} sf_topology_t;

typedef struct {
    int64_t        sf_context_id;           /* Generated context ID which embeds the cache index     */
    uint64_t       h5_file_id;              /* GUID (basically the inode value)                      */
    bool           threads_inited;          /* Whether the IOC threads for this context were started */
    int            file_ref;                /* Reference count held by files using this context      */
    int           *sf_fids;                 /* Array of file IDs for subfiles this rank owns         */
    int            sf_num_fids;             /* Number of subfiles this rank owns                     */
    int            sf_num_subfiles;         /* Total number of subfiles for logical HDF5 file        */
    size_t         sf_write_count;          /* Statistics: write_count                               */
    size_t         sf_read_count;           /* Statistics: read_count                                */
    haddr_t        sf_eof;                  /* File eof                                              */
    int64_t        sf_stripe_size;          /* Stripe-depth                                          */
    int64_t        sf_blocksize_per_stripe; /* Stripe-depth X n_IOCs                                 */
    int64_t        sf_base_addr;            /* For an IOC, our base address                          */
    MPI_Comm       sf_msg_comm;             /* MPI comm used to send RPC msg                         */
    MPI_Comm       sf_data_comm;            /* MPI comm used to move data                            */
    MPI_Comm       sf_eof_comm;             /* MPI comm used to communicate EOF                      */
    MPI_Comm       sf_node_comm;            /* MPI comm used for intra-node comms                    */
    MPI_Comm       sf_group_comm;           /* Not used: for IOC collectives                         */
    int            sf_group_size;           /* IOC count (in sf_group_comm)                          */
    int            sf_group_rank;           /* IOC rank  (in sf_group_comm)                          */
    char          *subfile_prefix;          /* If subfiles are node-local                            */
    char          *config_file_prefix;      /* Prefix added to config file name                      */
    char          *h5_filename;             /* The user supplied file name                           */
    void          *ioc_data;                /* Private data for underlying IOC                       */
    sf_topology_t *topology;                /* Pointer to our topology                               */

#ifdef H5_SUBFILING_DEBUG
    char  sf_logfile_name[PATH_MAX];
    FILE *sf_logfile;
#endif

} subfiling_context_t;

/* The following is a somewhat augmented input (by the IOC) which captures
 * the basic RPC from a 'source'.   The fields are filled out to allow
 * an easy gathering of statistics by the IO Concentrator.
 */
typedef struct {
    int64_t header[3];  /* The basic RPC input             */
    int     tag;        /* the supplied OPCODE tag         */
    int     source;     /* Rank of who sent the message    */
    int     ioc_idx;    /* The IOC rank                    */
    int64_t context_id; /* context to be used to complete  */
    double  start_time; /* the request, + time of receipt  */
                        /* from which we calc Time(queued) */
} sf_work_request_t;

/* MPI Datatype used to send/receive an RPC message */
extern MPI_Datatype H5_subfiling_rpc_msg_type;

/*
 * Utility routine to hack around casting away const
 */
static inline void *
H5FD__subfiling_cast_to_void(const void *data)
{
    union {
        const void *const_ptr_to_data;
        void       *ptr_to_data;
    } eliminate_const_warning;
    eliminate_const_warning.const_ptr_to_data = data;
    return eliminate_const_warning.ptr_to_data;
}

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL herr_t H5FD__subfiling_open_stub_file(const char *name, unsigned flags, MPI_Comm file_comm,
                                             H5FD_t **file_ptr, uint64_t *file_id);
H5_DLL herr_t H5FD__subfiling_open_subfiles(const char *base_filename, uint64_t file_id,
                                            H5FD_subfiling_params_t *subfiling_config, int file_acc_flags,
                                            MPI_Comm file_comm, int64_t *context_id_out);
H5_DLL herr_t H5FD__subfiling_close_subfiles(int64_t subfiling_context_id, MPI_Comm file_comm);

H5_DLL void  *H5FD__subfiling_get_object(int64_t object_id);
H5_DLL herr_t H5FD__subfiling_free_object(int64_t object_id);
H5_DLL herr_t H5FD__subfiling_get_config_from_file(FILE *config_file, int64_t *stripe_size,
                                                   int64_t *num_subfiles);
H5_DLL herr_t H5FD__subfiling_resolve_pathname(const char *filepath, MPI_Comm comm, char **resolved_filepath);

H5_DLL herr_t H5FD__subfiling_set_config_prop(H5P_genplist_t                *plist_ptr,
                                              const H5FD_subfiling_params_t *vfd_config);
H5_DLL herr_t H5FD__subfiling_get_config_prop(H5P_genplist_t *plist_ptr, H5FD_subfiling_params_t *vfd_config);
H5_DLL herr_t H5FD__subfiling_set_file_id_prop(H5P_genplist_t *plist_ptr, uint64_t file_id);
H5_DLL herr_t H5FD__subfiling_get_file_id_prop(H5P_genplist_t *plist_ptr, uint64_t *file_id);
H5_DLL herr_t H5FD__subfile_fid_to_context(uint64_t file_id, int64_t *context_id_out);

H5_DLL herr_t H5FD__subfiling_validate_config_params(const H5FD_subfiling_params_t *subf_config);
H5_DLL herr_t H5FD__subfiling_get_default_ioc_config(H5FD_ioc_config_t *config);

H5_DLL herr_t H5FD__subfiling_terminate(void);

#ifdef H5_SUBFILING_DEBUG
H5_DLL void H5FD__subfiling_log(int64_t sf_context_id, const char *fmt, ...);
H5_DLL void H5FD__subfiling_log_nonewline(int64_t sf_context_id, const char *fmt, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H5_SUBFILING_COMMON_H */
