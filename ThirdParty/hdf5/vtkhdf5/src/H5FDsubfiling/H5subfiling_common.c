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
 * Generic code for integrating an HDF5 VFD with the subfiling feature
 */

#include "H5subfiling_common.h"

#include "H5Eprivate.h"
#include "H5MMprivate.h"

typedef struct {            /* Format of a context map entry  */
    uint64_t file_id;       /* key value (linear search of the cache) */
    int64_t  sf_context_id; /* The return value if matching file_handle */
} file_map_to_context_t;

/* MPI Datatype used to send/receive an RPC message */
MPI_Datatype H5_subfiling_rpc_msg_type = MPI_DATATYPE_NULL;

static subfiling_context_t **sf_context_cache  = NULL;
static sf_topology_t       **sf_topology_cache = NULL;

static size_t sf_context_cache_size         = 0;
static size_t sf_topology_cache_size        = 0;
static size_t sf_context_cache_next_index   = 0;
static size_t sf_topology_cache_num_entries = 0;

static file_map_to_context_t *sf_open_file_map = NULL;
static int                    sf_file_map_size = 0;

#define DEFAULT_CONTEXT_CACHE_SIZE  16
#define DEFAULT_TOPOLOGY_CACHE_SIZE 4
#define DEFAULT_FILE_MAP_ENTRIES    8

static int64_t H5FD__subfiling_new_object_id(sf_obj_type_t obj_type);
static herr_t  H5FD__subfiling_free_context(subfiling_context_t *sf_context);
static herr_t  H5FD__subfiling_free_topology(sf_topology_t *topology);

static herr_t H5FD__subfiling_setup_context(const char *base_filename, uint64_t file_id,
                                            H5FD_subfiling_params_t *subfiling_config, int file_acc_flags,
                                            MPI_Comm comm, int64_t *context_id_out);
static herr_t H5FD__subfiling_init_app_topology(int64_t                  sf_context_id,
                                                H5FD_subfiling_params_t *subfiling_config, MPI_Comm comm,
                                                MPI_Comm node_comm, sf_topology_t **app_topology_out);
static herr_t H5FD__subfiling_init_context(int64_t context_id, const char *prefix_env,
                                           const char *base_filename, uint64_t file_id,
                                           H5FD_subfiling_params_t *subfiling_config,
                                           sf_topology_t *app_topology, MPI_Comm file_comm,
                                           MPI_Comm node_comm, subfiling_context_t **context);
static herr_t
H5FD__subfiling_get_ioc_selection_criteria_from_env(H5FD_subfiling_ioc_select_t *ioc_selection_type,
                                                    char                       **ioc_sel_info_str);
static herr_t H5FD__subfiling_find_cached_topology_info(MPI_Comm comm, H5FD_subfiling_params_t *subf_config,
                                                        long iocs_per_node, sf_topology_t **app_topology);
static herr_t H5FD__subfiling_init_app_layout(sf_topology_t *app_topology, MPI_Comm comm, MPI_Comm node_comm);
static herr_t H5FD__subfiling_gather_topology_info(app_layout_t *app_layout, MPI_Comm comm,
                                                   MPI_Comm intra_comm);
static int    H5FD__subfiling_compare_layout_nodelocal(const void *layout1, const void *layout2);
static herr_t H5FD__subfiling_identify_ioc_ranks(int64_t sf_context_id, sf_topology_t *app_topology,
                                                 int rank_stride);
static herr_t H5FD__subfiling_init_open_file_map(void);
static herr_t H5FD__subfiling_record_fid_map_entry(uint64_t file_id, int64_t subfile_context_id,
                                                   int *next_index);
static herr_t H5FD__subfiling_clear_fid_map_entry(uint64_t file_id, int64_t sf_context_id);
static herr_t H5FD__subfiling_ioc_open_files(int64_t file_context_id, int file_acc_flags);
static herr_t H5FD__subfiling_create_config_file(subfiling_context_t *sf_context, const char *base_filename,
                                                 const char *config_dir, const char *subfile_dir,
                                                 bool truncate_if_exists);
static herr_t H5FD__subfiling_open_config_file(const char *base_filename, const char *config_dir,
                                               uint64_t file_id, const char *mode, FILE **config_file_out);

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_new_object_id
 *
 * Purpose:     Given a subfiling object type and an index value, generates
 *              a new subfiling object ID.
 *
 * Return:      Non-negative object ID on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static int64_t
H5FD__subfiling_new_object_id(sf_obj_type_t obj_type)
{
    int64_t index_val = 0;
    int64_t ret_value;

    FUNC_ENTER_PACKAGE

    if (obj_type == SF_CONTEXT)
        index_val = (int64_t)sf_context_cache_next_index;
    else if (obj_type == SF_TOPOLOGY)
        index_val = (int64_t)sf_topology_cache_num_entries;
    else
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid object type");

    if (index_val < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, FAIL, "invalid object index");

    ret_value = ((int64_t)obj_type << 32) | index_val;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_get_object
 *
 * Purpose:     Given a subfiling object ID, returns a pointer to the
 *              underlying object, which can be either a subfiling context
 *              object (subfiling_context_t) or a subfiling topology
 *              object (sf_topology_t).
 *
 *              A subfiling object ID contains the object type in the upper
 *              32 bits and an index value in the lower 32 bits.
 *
 *              Subfiling contexts are 1 per open file. If only one file is
 *              open at a time, then only a single subfiling context cache
 *              entry will be used.
 *
 * Return:      Pointer to underlying subfiling object if subfiling object
 *              ID is valid
 *
 *              NULL if subfiling object ID is invalid or an internal
 *              failure occurs
 *
 *-------------------------------------------------------------------------
 */
void *
H5FD__subfiling_get_object(int64_t object_id)
{
    int64_t obj_type  = (object_id >> 32) & 0x0FFFF;
    int64_t obj_index = object_id & 0x0FFFF;
    void   *ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if (obj_index < 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "invalid object index for subfiling object ID %" PRId64,
                    object_id);

    if (obj_type == SF_CONTEXT) {
        /* Contexts provide information principally about the application and how
         * the data layout is managed over some number of subfiles.  The important
         * parameters are the number of subfiles (or in the context of IOCs, the
         * MPI ranks and counts of the processes which host an I/O Concentrator.
         * We also provide a map of IOC rank to MPI rank to facilitate the
         * communication of I/O requests.
         */

        /* Create subfiling context cache if it doesn't exist */
        if (!sf_context_cache) {
            if (NULL ==
                (sf_context_cache = H5MM_calloc(DEFAULT_CONTEXT_CACHE_SIZE * sizeof(*sf_context_cache))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                            "couldn't allocate space for subfiling context cache");
            sf_context_cache_size       = DEFAULT_CONTEXT_CACHE_SIZE;
            sf_context_cache_next_index = 0;
        }

        /* Make more space in context cache if needed */
        if ((size_t)obj_index >= sf_context_cache_size) {
            size_t old_num_entries;
            size_t new_size;
            void  *tmp_realloc;

            old_num_entries = sf_context_cache_size;

            new_size = (sf_context_cache_size * 3) / 2;

            if (NULL == (tmp_realloc = H5MM_realloc(sf_context_cache, new_size * sizeof(*sf_context_cache))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                            "couldn't allocate space for subfiling context cache");

            sf_context_cache      = tmp_realloc;
            sf_context_cache_size = new_size;

            /* Clear newly-allocated entries */
            memset(&sf_context_cache[old_num_entries], 0,
                   (sf_context_cache_size - old_num_entries) * sizeof(*sf_context_cache));

            /*
             * If we had to make more space, the given object index
             * should always fall within range after a single re-allocation
             */
            assert((size_t)obj_index < sf_context_cache_size);
        }

        ret_value = sf_context_cache[obj_index];
        if (!ret_value) {
            size_t next_idx;

            /* Allocate a new subfiling context object */
            if (NULL == (sf_context_cache[obj_index] = H5MM_calloc(sizeof(subfiling_context_t))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "couldn't allocate subfiling context object");

            ret_value = sf_context_cache[obj_index];

            /* Set index for next available cache entry. If all available
             * slots are filled, the index will be set to sf_context_cache_size
             * and cause a reallocation of the cache the next time a new
             * cache entry is created.
             */
            next_idx = (size_t)obj_index + 1;
            while (next_idx < sf_context_cache_size && sf_context_cache[next_idx])
                next_idx++;

            sf_context_cache_next_index = next_idx;
        }
    }
    else if (obj_type == SF_TOPOLOGY) {
        /* Create subfiling topology cache if it doesn't exist */
        if (!sf_topology_cache) {
            if (NULL ==
                (sf_topology_cache = H5MM_calloc(DEFAULT_TOPOLOGY_CACHE_SIZE * sizeof(*sf_topology_cache))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                            "couldn't allocate space for subfiling topology cache");
            sf_topology_cache_size        = DEFAULT_TOPOLOGY_CACHE_SIZE;
            sf_topology_cache_num_entries = 0;
        }

        /* Make more space in topology cache if needed */
        if ((size_t)obj_index >= sf_topology_cache_size) {
            size_t old_num_entries;
            size_t new_size;
            void  *tmp_realloc;

            old_num_entries = sf_topology_cache_num_entries;

            new_size = (sf_topology_cache_size * 3) / 2;

            if (NULL ==
                (tmp_realloc = H5MM_realloc(sf_topology_cache, new_size * sizeof(*sf_topology_cache))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL,
                            "couldn't allocate space for subfiling topology cache");

            sf_topology_cache      = tmp_realloc;
            sf_topology_cache_size = new_size;

            /* Clear newly-allocated entries */
            memset(&sf_topology_cache[old_num_entries], 0,
                   (sf_topology_cache_size - old_num_entries) * sizeof(*sf_topology_cache));

            /*
             * If we had to make more space, the given object index
             * should always fall within range after a single re-allocation
             */
            assert((size_t)obj_index < sf_topology_cache_size);
        }

        /*
         * Since this cache currently just keeps all entries until
         * application exit, topology entry indices should just be
         * consecutive
         */
        assert((size_t)obj_index <= sf_topology_cache_num_entries);
        if ((size_t)obj_index < sf_topology_cache_num_entries)
            ret_value = sf_topology_cache[obj_index];
        else {
            assert(!sf_topology_cache[sf_topology_cache_num_entries]);

            /* Allocate a new subfiling topology object */
            if (NULL == (ret_value = H5MM_malloc(sizeof(sf_topology_t))))
                HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, NULL, "couldn't allocate subfiling topology object");

            sf_topology_cache[sf_topology_cache_num_entries++] = ret_value;
        }
    }
    else
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, NULL, "unknown subfiling object type for ID %" PRId64, object_id);

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_free_object
 *
 * Purpose:     Frees the underlying subfiling object for a given subfiling
 *              object ID.
 *
 *              NOTE: Because we want to avoid the potentially large
 *              overhead of determining the application topology on every
 *              file open, we currently assume that all created subfiling
 *              topology objects are cached in the (very simple) topology
 *              cache until application exit. This allows us to quickly
 *              find and assign a cached topology object to a subfiling
 *              context object for a file when opened. Therefore, a
 *              subfiling topology object should (currently) only ever be
 *              freed by this routine if a function fails right after
 *              creating a topology object. Otherwise, the internal
 *              indexing for the topology cache will be invalid and we will
 *              either leak memory or assign invalid topology pointers to
 *              subfiling context objects after that point.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_free_object(int64_t object_id)
{
    int64_t obj_type  = (object_id >> 32) & 0x0FFFF;
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (obj_type == SF_CONTEXT) {
        subfiling_context_t *sf_context;

        if (NULL == (sf_context = H5FD__subfiling_get_object(object_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context for subfiling object ID");

        if (sf_context->file_ref == 0 || --sf_context->file_ref == 0) {
            if (H5FD__subfiling_free_context(sf_context) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling context object");

            for (size_t idx = 0; idx < sf_context_cache_size; idx++) {
                if (sf_context != sf_context_cache[idx])
                    continue;

                if (idx < sf_context_cache_next_index)
                    sf_context_cache_next_index = idx;

                sf_context_cache[idx] = NULL;
            }
        }
    }
    else if (obj_type == SF_TOPOLOGY) {
        sf_topology_t *sf_topology;

        if (NULL == (sf_topology = H5FD__subfiling_get_object(object_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context for subfiling object ID");

        if (H5FD__subfiling_free_topology(sf_topology) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling topology object");

        assert(sf_topology_cache_num_entries > 0);
        assert(sf_topology == sf_topology_cache[sf_topology_cache_num_entries - 1]);
        sf_topology_cache[sf_topology_cache_num_entries - 1] = NULL;
        sf_topology_cache_num_entries--;
    }
    else
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "couldn't free subfiling object - invalid object type");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5FD__subfiling_free_context(subfiling_context_t *sf_context)
{
    int    mpi_finalized;
    int    mpi_code;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(sf_context);

    if (MPI_SUCCESS != (mpi_code = MPI_Finalized(&mpi_finalized))) {
        /* Assume MPI is finalized and try to clean up what we can */
        HMPI_DONE_ERROR(FAIL, "MPI_Finalized failed", mpi_code);
        mpi_finalized = 1;
    }

    if (!mpi_finalized) {
        if (H5_mpi_comm_free(&sf_context->sf_msg_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");
        if (H5_mpi_comm_free(&sf_context->sf_data_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");
        if (H5_mpi_comm_free(&sf_context->sf_eof_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");
        if (H5_mpi_comm_free(&sf_context->sf_node_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");
        if (H5_mpi_comm_free(&sf_context->sf_group_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");
    }

    H5MM_free(sf_context->subfile_prefix);
    H5MM_free(sf_context->config_file_prefix);
    H5MM_free(sf_context->h5_filename);
    H5MM_free(sf_context->sf_fids);

    /*
     * Currently we assume that all created application topology
     * objects are cached until application exit and may be shared
     * among multiple subfiling contexts, so we free them separately
     * from here to avoid issues with stale pointers.
     */
    /* sf_context->topology = NULL; */

    H5MM_free(sf_context);

    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5FD__subfiling_free_topology(sf_topology_t *topology)
{
    int    mpi_finalized;
    int    mpi_code;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(topology);

    if (MPI_SUCCESS != (mpi_code = MPI_Finalized(&mpi_finalized))) {
        /* Assume MPI is finalized, but clean up what we can */
        HMPI_DONE_ERROR(FAIL, "MPI_Finalized failed", mpi_code);
        mpi_finalized = 1;
    }

#ifndef NDEBUG
    {
        bool topology_cached = false;

        /* Make sure this application topology object is in the cache */
        for (size_t i = 0; i < sf_topology_cache_num_entries; i++)
            if (topology == sf_topology_cache[i])
                topology_cached = true;
        assert(topology_cached);
    }
#endif

    if (topology->app_layout) {
        H5MM_free(topology->app_layout->layout);
        H5MM_free(topology->app_layout->node_ranks);
        H5MM_free(topology->app_layout);
    }

    H5MM_free(topology->io_concentrators);

    if (!mpi_finalized)
        if (H5_mpi_comm_free(&topology->app_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");

    H5MM_free(topology);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_open_stub_file
 *
 * Purpose:     Opens the stub file for an HDF5 file created with the subfiling
 *              VFD. This stub file only contains some superblock metadata that
 *              can allow HDF5 applications to determine that the file is an HDF5
 *              file and was created with the subfiling VFD.
 *
 *              This routine is collective across `file_comm`; once the
 *              stub file has been opened, the inode value for the file is
 *              retrieved and broadcasted to all MPI ranks in `file_comm`
 *              for future use.
 *
 *              To avoid unnecessary overhead from a large-scale file open,
 *              this stub file is currently only opened on MPI rank 0. Note
 *              that this assumes that all the relevant metadata will be
 *              written from MPI rank 0. This should be fine for now since
 *              the HDF file signature and subfiling driver info is really
 *              all that's needed, but this should be revisited since the
 *              file metadata can and will come from other MPI ranks as well.
 *
 * Return:      Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_open_stub_file(const char *name, unsigned flags, MPI_Comm file_comm, H5FD_t **file_ptr,
                               uint64_t *file_id)
{
    H5P_genplist_t *plist         = NULL;
    uint64_t        stub_file_id  = UINT64_MAX;
    bool            bcasted_inode = false;
    H5FD_t         *stub_file     = NULL;
    hid_t           fapl_id       = H5I_INVALID_HID;
    int             mpi_rank      = 0;
    int             mpi_size      = 1;
    int             mpi_code;
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid subfiling stub file name");
    if (file_comm == MPI_COMM_NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid MPI communicator");
    if (!file_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL file ID pointer");

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(file_comm, &mpi_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(file_comm, &mpi_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    if (!file_ptr && mpi_rank == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL stub file pointer");

    /* Open stub file on MPI rank 0 only */
    if (mpi_rank == 0) {
        h5_stat_t st;
        MPI_Comm  stub_comm = MPI_COMM_SELF;
        MPI_Info  stub_info = MPI_INFO_NULL;

        if ((fapl_id = H5P_create_id(H5P_CLS_FILE_ACCESS_g, false)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, FAIL, "can't create FAPL for stub file");
        if (NULL == (plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS)))
            HGOTO_ERROR(H5E_VFL, H5E_BADTYPE, FAIL, "not a file access property list");

        /* Use MPI I/O driver for stub file to allow access to vector I/O */
        if (H5P_set(plist, H5F_ACS_MPI_PARAMS_COMM_NAME, &stub_comm) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "can't set MPI communicator");
        if (H5P_set(plist, H5F_ACS_MPI_PARAMS_INFO_NAME, &stub_info) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "can't set MPI info object");
        if (H5P_set_driver(plist, H5FD_MPIO, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "can't set MPI I/O driver on FAPL");

        if (H5FD_open(false, &stub_file, name, flags, fapl_id, HADDR_UNDEF) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "couldn't open HDF5 stub file");

        HDcompile_assert(sizeof(uint64_t) >= sizeof(ino_t));

        /* Retrieve Inode value for stub file */
        memset(&st, 0, sizeof(h5_stat_t));
        if (HDstat(name, &st) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                        "couldn't stat HDF5 stub file, errno = %d, error message = '%s'", errno,
                        strerror(errno));
        stub_file_id = (uint64_t)st.st_ino;
    }

    if (mpi_size > 1)
        if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&stub_file_id, 1, MPI_UINT64_T, 0, file_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
    bcasted_inode = true;

    if (stub_file_id == UINT64_MAX)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get inode value for HDF5 stub file");

    if (file_ptr)
        *file_ptr = stub_file;
    *file_id = stub_file_id;

done:
    if (fapl_id >= 0 && H5I_dec_ref(fapl_id) < 0)
        HDONE_ERROR(H5E_VFL, H5E_CANTDEC, FAIL, "can't close FAPL ID");

    if (ret_value < 0) {
        if (!bcasted_inode && (mpi_size > 1))
            if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&stub_file_id, 1, MPI_UINT64_T, 0, file_comm)))
                HMPI_DONE_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
        if (stub_file)
            if (H5FD_close(stub_file) < 0)
                HDONE_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "couldn't close HDF5 stub file");
    }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_open_subfiles
 *
 * Purpose:     Initializes a subfiling context object for a file with the
 *              given filename and file ID (inode) and opens the associated
 *              subfiles. As part of this process, information about the
 *              application topology will be gathered and stored in the
 *              context object for future use. This includes identifying
 *              which MPI ranks will act as I/O concentrators and "own"
 *              one or more of the opened subfiles. The process of
 *              initializing the subfiling context object also involves
 *              creating MPI communicators that facilitate messaging
 *              between HDF5 clients and the I/O concentrators.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_open_subfiles(const char *base_filename, uint64_t file_id,
                              H5FD_subfiling_params_t *subfiling_config, int file_acc_flags,
                              MPI_Comm file_comm, int64_t *context_id_out)
{
    subfiling_context_t *sf_context = NULL;
    int64_t              context_id = -1;
    int                  mpi_size;
    int                  err_result;
    bool                 recorded_fid = false;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!base_filename)
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "invalid subfiling base filename");
    if (!subfiling_config)
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "invalid subfiling configuration");
    if (!context_id_out)
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "invalid subfiling context ID pointer");

    /* Make sure open file mapping is initialized in case this
     * is the first file open call with the VFD
     */
    if (H5FD__subfiling_init_open_file_map() < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize open file mapping");

    /* Check if this file is already open */
    if (H5FD__subfile_fid_to_context(file_id, &context_id) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't retrieve context ID from open file mapping");

    if (context_id >= 0) {
        /* Retrieve the subfiling object for the cached context ID */
        if (NULL == (sf_context = H5FD__subfiling_get_object(context_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling object from context ID");
    }
    else {
        /* Set up new subfiling context based on configuration information */
        if (H5FD__subfiling_setup_context(base_filename, file_id, subfiling_config, file_acc_flags, file_comm,
                                          &context_id) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize subfiling context");

        /* Retrieve the subfiling object for the newly-created context ID */
        if (NULL == (sf_context = H5FD__subfiling_get_object(context_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling object from context ID");

        /*
         * If this rank is an I/O concentrator, actually open the subfiles
         * belonging to this IOC rank and start the I/O service threads
         */
        if (sf_context->topology->rank_is_ioc)
            if (H5FD__subfiling_ioc_open_files(sf_context->sf_context_id, file_acc_flags) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "IOC couldn't open subfile");
    }

#ifdef H5_SUBFILING_DEBUG
    {
        struct tm *tm = NULL;
        time_t     cur_time;
        int        mpi_rank;

        /* Open debugging logfile */

        if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(file_comm, &mpi_rank)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);

        snprintf(sf_context->sf_logfile_name, PATH_MAX, "%s.log.%d", sf_context->h5_filename, mpi_rank);

        if (NULL == (sf_context->sf_logfile = fopen(sf_context->sf_logfile_name, "a")))
            HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "couldn't open subfiling debug logfile");

        cur_time = time(NULL);
        tm       = localtime(&cur_time);

        H5FD__subfiling_log(context_id, "-- LOGGING BEGIN - %s", asctime(tm));
    }
#endif

    /*
     * Save the HDF5 file ID (e.g., inode) to subfile context mapping.
     * There shouldn't be any issue, but check the status and
     * return if there was a problem.
     */
    if (H5FD__subfiling_record_fid_map_entry(sf_context->h5_file_id, sf_context->sf_context_id, NULL) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't record HDF5 file ID to subfile context mapping");
    recorded_fid = true;

    *context_id_out = context_id;

done:
    /* Form consensus on whether opening subfiles was successful */
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(file_comm, &mpi_size)))
        HMPI_DONE_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);
    err_result = (ret_value < 0);
    if (mpi_size > 1)
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Allreduce(MPI_IN_PLACE, &err_result, 1, MPI_INT, MPI_MAX, file_comm)))
            HMPI_DONE_ERROR(FAIL, "MPI_Allreduce failed", mpi_code);
    if (err_result)
        HDONE_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "one or more IOC ranks couldn't open subfiles");

    if (ret_value < 0) {
        if (recorded_fid && H5FD__subfiling_clear_fid_map_entry(file_id, context_id) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "unable to clear entry from file ID to context mapping");

        if (context_id >= 0 && H5FD__subfiling_free_object(context_id) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling object");

        *context_id_out = -1;
    }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*
-------------------------------------------------------------------------
  Purpose:     Called as part of a file open operation, we initialize a
               subfiling context which includes the application topology
               along with other relevant info such as the MPI objects
               (communicators) for communicating with IO concentrators.
               We also identify which MPI ranks will have IOC threads
               started on them.

               We return a context ID via the 'sf_context' variable.

  Errors:      returns an error if we detect any initialization errors,
               including malloc failures or any resource allocation
               problems.

-------------------------------------------------------------------------
*/
static herr_t
H5FD__subfiling_setup_context(const char *base_filename, uint64_t file_id,
                              H5FD_subfiling_params_t *subfiling_config, int file_acc_flags, MPI_Comm comm,
                              int64_t *context_id_out)
{
    subfiling_context_t *context       = NULL;
    sf_topology_t       *app_topology  = NULL;
    MPI_Comm             node_comm     = MPI_COMM_NULL;
    int64_t              context_id    = -1;
    FILE                *config_file   = NULL;
    char                *file_basename = NULL;
    char                *subfile_dir   = NULL;
    char                *prefix_env    = NULL;
    char                *env_value     = NULL;
    int                  mpi_rank;
    int                  mpi_size;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(context_id_out);

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &mpi_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(comm, &mpi_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    /* Check if a prefix has been set for the configuration file name */
    prefix_env = getenv(H5FD_SUBFILING_CONFIG_FILE_PREFIX);

    /*
     * If there's an existing subfiling configuration file for this file, read the
     * stripe size and number of subfiles from it
     */
    if (0 == (file_acc_flags & O_CREAT)) {
        int64_t config[2] = {0, 0}; /* {stripe size, num subfiles} */

        if (mpi_rank == 0) {
            /* TODO: currently no support for subfile prefix */
            if (H5_dirname(base_filename, &subfile_dir) < 0)
                config[0] = -1;

            if (config[0] >= 0)
                if (H5_basename(base_filename, &file_basename) < 0)
                    config[0] = -1;

            if (config[0] >= 0)
                /*
                 * If a prefix has been specified, try to read the config file
                 * from there, otherwise look for it next to the generated subfiles.
                 */
                if (H5FD__subfiling_open_config_file(file_basename, prefix_env ? prefix_env : subfile_dir,
                                                     file_id, "r", &config_file) < 0)
                    config[0] = -1;

            if (config[0] >= 0) {
                if (!config_file)
                    config[0] = -2; /* No config file; use setting from configuration */
                else {
                    /*
                     * If a subfiling configuration file exists and we aren't truncating
                     * it, read the number of subfiles used at file creation time.
                     */
                    if (H5FD__subfiling_get_config_from_file(config_file, &config[0], &config[1]) < 0)
                        config[0] = -1;
                }
            }
        }

        if (mpi_size > 1)
            if (MPI_SUCCESS != (mpi_code = MPI_Bcast(config, 2, MPI_INT64_T, 0, comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code);

        /*
         * Override the stripe size and stripe count settings in the application's
         * subfiling configuration if we read values from an existing subfiling
         * configuration file
         */
        if (config[0] == -1)
            HGOTO_ERROR(
                H5E_VFL, H5E_CANTOPENFILE, FAIL,
                "lead process couldn't read the number of subfiles from subfiling configuration file");
        else {
            if (config[0] > 0)
                subfiling_config->stripe_size = config[0];
            if (config[1] > 0) {
                H5_CHECK_OVERFLOW(config[1], int64_t, int32_t);
                subfiling_config->stripe_count = (int32_t)config[1];
            }
        }
    }
    else {
        /* Check for a subfiling stripe size setting from the environment */
        env_value = getenv(H5FD_SUBFILING_STRIPE_SIZE);
        if (env_value && (strlen(env_value) > 0)) {
            long long stripe_size = -1;

            errno       = 0;
            stripe_size = strtoll(env_value, NULL, 0);
            if (ERANGE == errno)
                HSYS_GOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                                "invalid stripe size setting for " H5FD_SUBFILING_STRIPE_SIZE);

            if (stripe_size > 0)
                subfiling_config->stripe_size = (int64_t)stripe_size;
        }
    }

#if H5_CHECK_MPI_VERSION(3, 0)
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &mpi_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);

    /* Create an MPI sub-communicator for intra-node communications */
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, mpi_rank, MPI_INFO_NULL, &node_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_split_type failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(node_comm, MPI_ERRORS_RETURN)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_set_errhandler failed", mpi_code);
#else
#error "MPI-3 required for MPI_Comm_split_type"
#endif

    /* Use the file's index to create a new subfiling context ID */
    if ((context_id = H5FD__subfiling_new_object_id(SF_CONTEXT)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't create new subfiling context ID");

    /*
     * Initialize the application topology information, including the computed
     * number and distribution map of the set of I/O concentrators
     */
    if (H5FD__subfiling_init_app_topology(context_id, subfiling_config, comm, node_comm, &app_topology) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize application topology");

    /* Initialize the subfiling context */
    if (H5FD__subfiling_init_context(context_id, prefix_env, base_filename, file_id, subfiling_config,
                                     app_topology, comm, node_comm, &context) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize subfiling context");
    assert(context);
    node_comm = MPI_COMM_NULL; /* New context takes ownership of node_comm */

    *context_id_out = context_id;

done:
    if (config_file && EOF == fclose(config_file))
        HDONE_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "couldn't close subfiling configuration file");

    H5MM_free(file_basename);
    H5MM_free(subfile_dir);

    if (ret_value < 0) {
        if (app_topology)
            if (H5FD__subfiling_free_topology(app_topology) < 0)
                HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling topology");

        if (H5_mpi_comm_free(&node_comm) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free MPI communicator");

        if (context_id >= 0 && context)
            if (H5FD__subfiling_free_object(context_id) < 0)
                HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling object");

        *context_id_out = -1;
    }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_init_app_topology
 *
 * Purpose:     Determine the topology of the application so that MPI ranks
 *              can be assigned as I/O concentrators. The default is to use
 *              1 MPI rank per node as an I/O concentrator, but this can be
 *              changed by the application's subfiling configuration, or by
 *              an environment variable (H5FD_SUBFILING_IOC_PER_NODE).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_init_app_topology(int64_t sf_context_id, H5FD_subfiling_params_t *subfiling_config,
                                  MPI_Comm comm, MPI_Comm node_comm, sf_topology_t **app_topology_out)
{
    H5FD_subfiling_ioc_select_t ioc_selection_type;
    sf_topology_t              *app_topology   = NULL;
    int64_t                     topology_id    = -1;
    char                       *env_value      = NULL;
    char                       *ioc_sel_str    = NULL;
    long                        ioc_select_val = -1;
    long                        iocs_per_node  = 1;
    int                         ioc_count      = 0;
    int                         rank_multiple  = 1;
    int                         comm_rank;
    int                         comm_size;
    int                         mpi_code;
    herr_t                      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(subfiling_config);
    assert(MPI_COMM_NULL != comm);
    assert(MPI_COMM_NULL != node_comm);
    assert(app_topology_out);
    assert(!*app_topology_out);

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &comm_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(comm, &comm_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    ioc_selection_type = subfiling_config->ioc_selection;

    /* Check if an IOC selection type was specified by environment variable */
    if (H5FD__subfiling_get_ioc_selection_criteria_from_env(&ioc_selection_type, &ioc_sel_str) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get IOC selection type from environment");

    /*
     * Check parameters for the specified IOC selection strategy
     * and determine the maximum number of I/O concentrators
     */
    switch (ioc_selection_type) {
        case SELECT_IOC_ONE_PER_NODE: {
            if (comm_size > 1) {
                /* Check for an IOC-per-node value set in the environment */
                env_value = getenv(H5FD_SUBFILING_IOC_PER_NODE);
                if (env_value && (strlen(env_value) > 0)) {
                    errno          = 0;
                    ioc_select_val = strtol(env_value, NULL, 0);
                    if (ERANGE == errno)
                        ioc_select_val = 1;

                    if (ioc_select_val > 0)
                        iocs_per_node = ioc_select_val;
                }
            }

            /* IOC count will be adjusted after number of nodes is determined */
            H5_CHECK_OVERFLOW(iocs_per_node, long, int);
            ioc_count = (int)iocs_per_node;

            break;
        }

        case SELECT_IOC_EVERY_NTH_RANK: {
            /*
             * User specifies a rank multiple value. Selection starts
             * with rank 0 and then the user-specified stride is applied
             * to identify other IOC ranks.
             */
            ioc_select_val = 1;
            if (ioc_sel_str) {
                errno          = 0;
                ioc_select_val = strtol(ioc_sel_str, NULL, 0);
                if (ERANGE == errno || ioc_select_val <= 0) {
                    ioc_select_val     = 1;
                    ioc_selection_type = SELECT_IOC_ONE_PER_NODE;

                    H5_CHECK_OVERFLOW(iocs_per_node, long, int);
                    ioc_count = (int)iocs_per_node;

                    break;
                }
            }

            if (ioc_select_val > comm_size)
                ioc_select_val = comm_size;

            H5_CHECK_OVERFLOW(ioc_select_val, long, int);
            ioc_count = ((comm_size - 1) / (int)ioc_select_val) + 1;

            rank_multiple = (int)ioc_select_val;

            break;
        }

        case SELECT_IOC_TOTAL: {
            /*
             * User specifies a total number of I/O concentrators.
             * Starting with rank 0, a stride of (mpi_size / total)
             * is applied to identify other IOC ranks.
             */
            ioc_select_val = 1;
            if (ioc_sel_str) {
                errno          = 0;
                ioc_select_val = strtol(ioc_sel_str, NULL, 0);
                if (ERANGE == errno || ioc_select_val <= 0) {
                    ioc_select_val     = 1;
                    ioc_selection_type = SELECT_IOC_ONE_PER_NODE;

                    H5_CHECK_OVERFLOW(iocs_per_node, long, int);
                    ioc_count = (int)iocs_per_node;

                    break;
                }
            }

            if (ioc_select_val > comm_size)
                ioc_select_val = comm_size;

            H5_CHECK_OVERFLOW(ioc_select_val, long, int);
            ioc_count = (int)ioc_select_val;

            if (ioc_select_val > 1)
                rank_multiple = (comm_size - 1) / ((int)ioc_select_val - 1);
            else
                rank_multiple = 1;

            break;
        }

        case SELECT_IOC_WITH_CONFIG:
        case ioc_selection_options:
        default:
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid IOC selection strategy");
            break;
    }

    /*
     * TODO: A different IOC selection string from the environment than what was
     *       used originally will cause the IOCs to be assigned differently than
     *       expected. While this generally shouldn't cause issues (other than
     *       for the SELECT_IOC_TOTAL case), this should still be dealt with
     *       eventually.
     */
    /* Check the subfiling topology cache to see if there's a matching object */
    if (H5FD__subfiling_find_cached_topology_info(comm, subfiling_config, iocs_per_node, &app_topology) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't check for cached subfiling topology object");
    assert(!app_topology || (app_topology->selection_type == ioc_selection_type));

    if (NULL == app_topology) {
        /* Generate an ID for the application topology object */
        if ((topology_id = H5FD__subfiling_new_object_id(SF_TOPOLOGY)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get ID for subfiling topology object");

        /* Get a new application topology object from the cache */
        if (NULL == (app_topology = H5FD__subfiling_get_object(topology_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get subfiling topology object");
        app_topology->app_layout         = NULL;
        app_topology->app_comm           = MPI_COMM_NULL;
        app_topology->rank_is_ioc        = false;
        app_topology->ioc_idx            = -1;
        app_topology->n_io_concentrators = ioc_count;
        app_topology->io_concentrators   = NULL;
        app_topology->selection_type     = ioc_selection_type;

        if (H5_mpi_comm_dup(comm, &app_topology->app_comm) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTCOPY, FAIL, "can't duplicate MPI communicator");

        if (H5FD__subfiling_init_app_layout(app_topology, comm, node_comm) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize application layout");
        assert(app_topology->app_layout);
        assert(app_topology->app_layout->layout);
        assert(app_topology->app_layout->node_ranks);
        assert(app_topology->app_layout->node_count > 0);

        /*
         * Now that the application node count has been determined, adjust the
         * number of I/O concentrators for the SELECT_IOC_ONE_PER_NODE case
         */
        if (app_topology->selection_type == SELECT_IOC_ONE_PER_NODE)
            app_topology->n_io_concentrators = (int)iocs_per_node * app_topology->app_layout->node_count;

        /*
         * Make sure the number of I/O concentrators doesn't
         * exceed the specified number of subfiles
         */
        if (subfiling_config->stripe_count != H5FD_SUBFILING_DEFAULT_STRIPE_COUNT)
            if (app_topology->n_io_concentrators > subfiling_config->stripe_count)
                app_topology->n_io_concentrators = subfiling_config->stripe_count;

        /*
         * Determine which ranks are I/O concentrator ranks, based on the
         * given IOC selection strategy and MPI information.
         */
        if (H5FD__subfiling_identify_ioc_ranks(sf_context_id, app_topology, rank_multiple) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL,
                        "couldn't determine which MPI ranks are I/O concentrators");
    }

    *app_topology_out = app_topology;

done:
    if (ret_value < 0)
        if (app_topology && (topology_id >= 0))
            if (H5FD__subfiling_free_object(topology_id) < 0)
                HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free subfiling topology object");

    FUNC_LEAVE_NOAPI(ret_value)
}

/*
-------------------------------------------------------------------------
  Purpose:     Return a character string which represents either the
               default selection method: SELECT_IOC_ONE_PER_NODE; or
               if the user has selected a method via the environment
               variable (H5FD_SUBFILING_IOC_SELECTION_CRITERIA), we
               return that along with any optional qualifier with for
               that method.

  Errors:      None.

  Revision History -- Initial implementation
-------------------------------------------------------------------------
*/
static herr_t
H5FD__subfiling_get_ioc_selection_criteria_from_env(H5FD_subfiling_ioc_select_t *ioc_selection_type,
                                                    char                       **ioc_sel_info_str)
{
    char  *opt_value = NULL;
    char  *env_value;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(ioc_selection_type);
    assert(ioc_sel_info_str);

    *ioc_sel_info_str = NULL;

    env_value = getenv(H5FD_SUBFILING_IOC_SELECTION_CRITERIA);
    if (env_value && (strlen(env_value) > 0)) {
        /*
         * Parse I/O Concentrator selection strategy criteria as either a single
         * value or two colon-separated values of the form
         * 'integer:[integer|string]'. If two values are given, the first value
         * specifies the I/O Concentrator selection strategy to use (given as the
         * integer value corresponding to the H5FD_subfiling_ioc_select_t enum
         * value for that strategy) and the second value specifies the criteria
         * for that strategy.
         *
         * For example, to assign every 64th MPI rank as an I/O Concentrator, the
         * criteria string should have the format '1:64' to specify the "every
         * Nth rank" strategy with a criteria of '64'.
         */
        opt_value = strchr(env_value, ':');
        if (opt_value) {
            long check_value;

            *opt_value++ = '\0';

            errno       = 0;
            check_value = strtol(env_value, NULL, 0);
            if (errno == ERANGE)
                HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                                "couldn't parse value from " H5FD_SUBFILING_IOC_SELECTION_CRITERIA
                                " environment variable");
            if (check_value < 0 || check_value >= ioc_selection_options)
                HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                            "invalid IOC selection type value %ld from " H5FD_SUBFILING_IOC_SELECTION_CRITERIA
                            " environment variable",
                            check_value);

            *ioc_selection_type = (H5FD_subfiling_ioc_select_t)check_value;
            *ioc_sel_info_str   = opt_value;
        }
        else
            *ioc_sel_info_str = env_value;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_find_cached_topology_info
 *
 * Purpose:     Given an MPI communicator and IOC selection strategy,
 *              checks the subfiling topology cache to see if any matching
 *              topology objects have been cached.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_find_cached_topology_info(MPI_Comm comm, H5FD_subfiling_params_t *subf_config,
                                          long iocs_per_node, sf_topology_t **app_topology)
{
    H5FD_subfiling_ioc_select_t ioc_selection_type;
    int32_t                     stripe_count;
    herr_t                      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(subf_config);

    ioc_selection_type = subf_config->ioc_selection;
    stripe_count       = subf_config->stripe_count;

    for (size_t i = 0; i < sf_topology_cache_num_entries; i++) {
        sf_topology_t *cached_topology = sf_topology_cache[i];
        int            result;
        int            mpi_code;

        assert(cached_topology);

        /*
         * If the selection types differ, just reject the cached topology
         * for now rather than checking if the mapping is equivalent
         */
        if (ioc_selection_type != cached_topology->selection_type)
            continue;

        /*
         * If the number of I/O concentrators in the cached topology
         * is greater than the specified target number of subfiles,
         * reject the cached topology
         */
        if (stripe_count != H5FD_SUBFILING_DEFAULT_STRIPE_COUNT)
            if (stripe_count < cached_topology->n_io_concentrators)
                continue;

        if (cached_topology->selection_type == SELECT_IOC_ONE_PER_NODE) {
            assert(iocs_per_node >= 1);
            assert(cached_topology->app_layout->node_count > 0);

            /*
             * If a IOCs-per-node setting was set in the environment and would
             * cause the application topology to differ from the cached topology
             * we found, don't reuse the cached topology
             */
            if (cached_topology->n_io_concentrators !=
                (iocs_per_node * cached_topology->app_layout->node_count))
                continue;
        }

        if (MPI_SUCCESS != (mpi_code = MPI_Comm_compare(comm, cached_topology->app_comm, &result)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_compare failed", mpi_code);

        if (MPI_IDENT == result || MPI_CONGRUENT == result) {
            *app_topology = cached_topology;
            break;
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_init_app_layout
 *
 * Purpose:     Determines the layout of MPI ranks across nodes in order to
 *              figure out the final application topology
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_init_app_layout(sf_topology_t *app_topology, MPI_Comm comm, MPI_Comm node_comm)
{
    app_layout_t *app_layout = NULL;
    int           mpi_code;
    herr_t        ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(app_topology);
    assert(!app_topology->app_layout);
    assert(MPI_COMM_NULL != comm);
    assert(MPI_COMM_NULL != node_comm);

    if (NULL == (app_layout = H5MM_calloc(sizeof(*app_layout))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate application layout structure");

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &app_layout->world_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(comm, &app_layout->world_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(node_comm, &app_layout->node_local_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(node_comm, &app_layout->node_local_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    if (NULL ==
        (app_layout->layout = H5MM_malloc((size_t)app_layout->world_size * sizeof(*app_layout->layout))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate application layout array");

    /* Gather the list of layout_t pairs to all ranks */
    if (H5FD__subfiling_gather_topology_info(app_layout, comm, node_comm) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "can't gather application topology info");

    /* Sort the list according to the node local lead rank values */
    qsort(app_layout->layout, (size_t)app_layout->world_size, sizeof(layout_t),
          H5FD__subfiling_compare_layout_nodelocal);

    /*
     * Count the number of nodes by checking how many
     * entries have a node local rank value of 0
     */
    app_layout->node_count = 0;
    for (size_t i = 0; i < (size_t)app_layout->world_size; i++)
        if (app_layout->layout[i].node_local_rank == 0)
            app_layout->node_count++;

    if (app_layout->node_count <= 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "node count less than or equal to zero");

    if (NULL == (app_layout->node_ranks =
                     H5MM_malloc((size_t)app_layout->node_count * sizeof(*app_layout->node_ranks))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate application layout node rank array");

    /* Record the rank value of the "lead" MPI rank on each node for later use */
    for (size_t i = 0, node_rank_index = 0; i < (size_t)app_layout->world_size; i++)
        if (app_layout->layout[i].node_local_rank == 0)
            app_layout->node_ranks[node_rank_index++] = app_layout->layout[i].rank;

    app_topology->app_layout = app_layout;

done:
    if (ret_value < 0)
        if (app_layout) {
            H5MM_free(app_layout->layout);
            H5MM_free(app_layout->node_ranks);
            H5MM_free(app_layout);
        }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_gather_topology_info
 *
 * Purpose:     Collectively generate a list of layout_t structures
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_gather_topology_info(app_layout_t *app_layout, MPI_Comm comm, MPI_Comm intra_comm)
{
    MPI_Group file_group = MPI_GROUP_NULL;
    MPI_Group node_group = MPI_GROUP_NULL;
    layout_t  my_layout_info;
    layout_t *layout_info_partial = NULL;
    MPI_Comm  aggr_comm           = MPI_COMM_NULL;
    int      *recv_counts         = NULL;
    int      *recv_displs         = NULL;
    int       sf_world_size;
    int       sf_world_rank;
    int       node_local_rank;
    int       node_local_size;
    const int local_lead = 0;
    int       lead_rank;
    int       mpi_code;
    herr_t    ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(app_layout);
    assert(app_layout->layout);
    assert(MPI_COMM_NULL != comm);

    sf_world_rank   = app_layout->world_rank;
    sf_world_size   = app_layout->world_size;
    node_local_rank = app_layout->node_local_rank;
    node_local_size = app_layout->node_local_size;

    my_layout_info.rank            = sf_world_rank;
    my_layout_info.node_local_rank = node_local_rank;
    my_layout_info.node_local_size = node_local_size;

    /*
     * Get the rank value for the "lead" rank on this rank's node so that we can
     * group the layout_t information for all node-local ranks together
     */
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_group(comm, &file_group)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_group failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_group(intra_comm, &node_group)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_group failed", mpi_code);
    if (MPI_SUCCESS !=
        (mpi_code = MPI_Group_translate_ranks(node_group, 1, &local_lead, file_group, &lead_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Group_translate_ranks failed", mpi_code);
    if (MPI_UNDEFINED == lead_rank)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't determine lead rank on node");

    my_layout_info.node_lead_rank = lead_rank;

    if (MPI_SUCCESS != (mpi_code = MPI_Group_free(&node_group)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Group_free failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Group_free(&file_group)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Group_free failed", mpi_code);

    app_layout->layout[sf_world_rank] = my_layout_info;

    if (sf_world_size > 1) {
#ifdef H5_SUBFILING_PREFER_ALLGATHER_TOPOLOGY
        (void)intra_comm;

        if (MPI_SUCCESS !=
            (mpi_code = MPI_Allgather(&my_layout_info, 4, MPI_INT, app_layout->layout, 4, MPI_INT, comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Allgather failed", mpi_code);
#else
        assert(MPI_COMM_NULL != intra_comm);

        /* Split the file communicator into a sub-group of one rank per node */
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_split(comm, node_local_rank, sf_world_rank, &aggr_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_split failed", mpi_code);

        /* Allocate a partial layout info array to aggregate into from node-local ranks */
        if (node_local_rank == 0)
            if (NULL ==
                (layout_info_partial = H5MM_malloc((size_t)node_local_size * sizeof(*layout_info_partial))))
                /* Push error, but participate in gather operation */
                HDONE_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate layout info array");

        /* Gather node-local layout info to single leader rank on each node */
        if (MPI_SUCCESS != (mpi_code = MPI_Gather(&my_layout_info, 4, MPI_INT, layout_info_partial, 4,
                                                  MPI_INT, 0, intra_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Gather failed", mpi_code);

        /* Gather total layout info from/to each leader rank on each node */
        if (node_local_rank == 0) {
            int aggr_comm_size = 0;
            int send_size      = 4 * node_local_size;

            if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(aggr_comm, &aggr_comm_size)))
                HMPI_DONE_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

            if (NULL == (recv_counts = H5MM_malloc((size_t)aggr_comm_size * sizeof(*recv_counts))))
                HDONE_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate receive counts array");
            if (NULL == (recv_displs = H5MM_malloc((size_t)aggr_comm_size * sizeof(*recv_displs))))
                HDONE_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate receive displacements array");

            if (MPI_SUCCESS !=
                (mpi_code = MPI_Allgather(&send_size, 1, MPI_INT, recv_counts, 1, MPI_INT, aggr_comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Allgather failed", mpi_code);

            recv_displs[0] = 0;
            for (int i = 1; i < aggr_comm_size; i++)
                recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];

            if (MPI_SUCCESS !=
                (mpi_code = MPI_Allgatherv(layout_info_partial, send_size, MPI_INT, app_layout->layout,
                                           recv_counts, recv_displs, MPI_INT, aggr_comm)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Allgatherv failed", mpi_code);
        }

        /*
         * Each leader rank on each node distributes the total
         * layout info back to other node-local ranks
         */
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Bcast(app_layout->layout, 4 * sf_world_size, MPI_INT, 0, intra_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
#endif
    }

done:
    H5MM_free(recv_displs);
    H5MM_free(recv_counts);
    H5MM_free(layout_info_partial);

    if (H5_mpi_comm_free(&aggr_comm) < 0)
        HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "can't free MPI communicator");

    if (node_group != MPI_GROUP_NULL)
        if (MPI_SUCCESS != (mpi_code = MPI_Group_free(&node_group)))
            HMPI_DONE_ERROR(FAIL, "MPI_Group_free failed", mpi_code);
    if (file_group != MPI_GROUP_NULL)
        if (MPI_SUCCESS != (mpi_code = MPI_Group_free(&file_group)))
            HMPI_DONE_ERROR(FAIL, "MPI_Group_free failed", mpi_code);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_compare_layout_nodelocal
 *
 * Purpose:     Qsort sorting callback that sorts layout_t structures
 *              according to their node local lead MPI rank values. Ties
 *              are broken according to their regular node local MPI rank
 *              values
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD__subfiling_compare_layout_nodelocal(const void *layout1, const void *layout2)
{
    const layout_t *l1 = (const layout_t *)layout1;
    const layout_t *l2 = (const layout_t *)layout2;

    if (l1->node_lead_rank == l2->node_lead_rank)
        return (l1->node_local_rank > l2->node_local_rank) - (l1->node_local_rank < l2->node_local_rank);
    else
        return (l1->node_lead_rank > l2->node_lead_rank) - (l1->node_lead_rank < l2->node_lead_rank);
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_identify_ioc_ranks
 *
 * Purpose:     We've already identified the number of unique nodes and have a
 *              sorted list of layout_t structures.  Under normal conditions, we
 *              only utilize a single IOC per node. Under that circumstance, we
 *              only need to fill the io_concentrators vector from the node_ranks
 *              array (which contains the index into the layout array of lowest
 *              MPI rank on each node) into the io_concentrators vector; Otherwise,
 *              while determining the number of local ranks per node, we can also
 *              select one or more additional IOCs.
 *
 *              As a side effect, we fill the 'io_concentrators' vector
 *              and set the 'rank_is_ioc' flag to true if our rank is
 *              identified as owning an I/O Concentrator (IOC).
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_identify_ioc_ranks(int64_t sf_context_id, sf_topology_t *app_topology, int rank_stride)
{
    app_layout_t *app_layout       = NULL;
    int          *io_concentrators = NULL;
    int           max_iocs         = 0;
    herr_t        ret_value        = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(app_topology);
    assert(!app_topology->io_concentrators);
    assert(app_topology->n_io_concentrators > 0);
    assert(app_topology->app_layout);
    assert(app_topology->app_layout->layout);
    assert(app_topology->app_layout->node_count > 0);
    assert(app_topology->app_layout->node_count <= app_topology->app_layout->world_size);

#ifndef H5_SUBFILING_DEBUG
    (void)sf_context_id;
#endif

    app_layout = app_topology->app_layout;

    max_iocs = app_topology->n_io_concentrators;
    if (NULL == (app_topology->io_concentrators =
                     H5MM_malloc((size_t)max_iocs * sizeof(*app_topology->io_concentrators))))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate array of I/O concentrator ranks");

    io_concentrators = app_topology->io_concentrators;

    switch (app_topology->selection_type) {
        case SELECT_IOC_ONE_PER_NODE: {
            int total_ioc_count = 0;
            int iocs_per_node   = 1;
            int last_lead_rank;

            if (app_topology->n_io_concentrators > app_layout->node_count)
                iocs_per_node = app_topology->n_io_concentrators / app_layout->node_count;

            /*
             * NOTE: The below code assumes that the app_layout->layout
             * array was sorted according to the node_lead_rank field,
             * such that entries for MPI ranks on the same node will occur
             * together in the array.
             */

            last_lead_rank = app_layout->layout[0].node_lead_rank;
            for (size_t i = 0, layout_idx = 0; i < (size_t)app_layout->node_count; i++) {
                int local_size = app_layout->layout[layout_idx].node_local_size;

                /* Assign first I/O concentrator from this node */
                assert(total_ioc_count < app_topology->n_io_concentrators);
                io_concentrators[total_ioc_count] = app_layout->layout[layout_idx++].rank;

                if (app_layout->world_rank == io_concentrators[total_ioc_count]) {
                    assert(!app_topology->rank_is_ioc);

                    app_topology->ioc_idx     = total_ioc_count;
                    app_topology->rank_is_ioc = true;
                }

                total_ioc_count++;

                /* Assign any additional I/O concentrators from this node */
                for (size_t j = 1; j < (size_t)iocs_per_node; j++) {
                    if (total_ioc_count >= max_iocs)
                        break;
                    if (j >= (size_t)local_size)
                        break;

                    /* Sanity check to make sure this rank is on the same node */
                    assert(app_layout->layout[layout_idx].node_lead_rank ==
                           app_layout->layout[layout_idx - 1].node_lead_rank);

                    assert(total_ioc_count < app_topology->n_io_concentrators);
                    io_concentrators[total_ioc_count] = app_layout->layout[layout_idx++].rank;

                    if (app_layout->world_rank == io_concentrators[total_ioc_count]) {
                        assert(!app_topology->rank_is_ioc);

                        app_topology->ioc_idx     = total_ioc_count;
                        app_topology->rank_is_ioc = true;
                    }

                    total_ioc_count++;
                }

                if (total_ioc_count >= max_iocs)
                    break;

                /* Find the block of layout structures for the next node */
                while ((layout_idx < (size_t)app_layout->world_size) &&
                       (last_lead_rank == app_layout->layout[layout_idx].node_lead_rank))
                    layout_idx++;

                if (layout_idx >= (size_t)app_layout->world_size)
                    break;

                last_lead_rank = app_layout->layout[layout_idx].node_lead_rank;
            }

#ifdef H5_SUBFILING_DEBUG
            if (app_topology->n_io_concentrators != total_ioc_count)
                H5FD__subfiling_log(sf_context_id,
                                    "%s: **WARN** Number of I/O concentrators adjusted from %d to %d",
                                    __func__, app_topology->n_io_concentrators, total_ioc_count);
#endif

            /* Set final number of I/O concentrators after adjustments */
            app_topology->n_io_concentrators = total_ioc_count;

            break;
        }

        case SELECT_IOC_EVERY_NTH_RANK:
        case SELECT_IOC_TOTAL: {
            int num_iocs_assigned = 0;
            int world_size        = app_layout->world_size;

            assert(rank_stride > 0);

            for (int i = 0; num_iocs_assigned < max_iocs; num_iocs_assigned++) {
                int ioc_index = rank_stride * i++;

                if (num_iocs_assigned >= max_iocs)
                    break;
                if (ioc_index >= world_size)
                    break;

                io_concentrators[num_iocs_assigned] = app_layout->layout[ioc_index].rank;

                if (app_layout->world_rank == io_concentrators[num_iocs_assigned]) {
                    app_topology->ioc_idx     = num_iocs_assigned;
                    app_topology->rank_is_ioc = true;
                }
            }

            /* Set final number of I/O concentrators after adjustments */
            app_topology->n_io_concentrators = num_iocs_assigned;

            break;
        }

        case SELECT_IOC_WITH_CONFIG:
        case ioc_selection_options:
        default:
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid IOC selection strategy");
            break;
    }

done:
    if (ret_value < 0)
        if (app_topology)
            H5MM_free(app_topology->io_concentrators);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_init_context
 *
 * Purpose:     Called as part of the HDF5 file + subfiling opening.
 *              This initializes the subfiling context and associates
 *              this context with the specific HDF5 file.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_init_context(int64_t context_id, const char *prefix_env, const char *base_filename,
                             uint64_t file_id, H5FD_subfiling_params_t *subfiling_config,
                             sf_topology_t *app_topology, MPI_Comm file_comm, MPI_Comm node_comm,
                             subfiling_context_t **context)
{
    subfiling_context_t *sf_context = NULL;
    char                *env_value  = NULL;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Create a new subfiling context object with the created context ID */
    if (NULL == (sf_context = H5FD__subfiling_get_object(context_id)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't create new subfiling object");

    /* Set non-zero fields */
    sf_context->h5_file_id      = file_id;
    sf_context->sf_context_id   = context_id;
    sf_context->sf_num_subfiles = subfiling_config->stripe_count;
    sf_context->sf_eof          = HADDR_UNDEF;
    sf_context->sf_stripe_size  = H5FD_SUBFILING_DEFAULT_STRIPE_SIZE;
    sf_context->sf_msg_comm     = MPI_COMM_NULL;
    sf_context->sf_data_comm    = MPI_COMM_NULL;
    sf_context->sf_eof_comm     = MPI_COMM_NULL;
    sf_context->sf_node_comm    = node_comm;
    sf_context->sf_group_comm   = MPI_COMM_NULL;
    sf_context->sf_group_size   = 1;
    sf_context->topology        = app_topology;

    /* Check if a prefix has been set for the configuration file name */
    if (prefix_env && (strlen(prefix_env) > 0))
        if (NULL == (sf_context->config_file_prefix = strdup(prefix_env)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTCOPY, FAIL, "couldn't copy config file prefix string");

    if (NULL == (sf_context->h5_filename = strdup(base_filename)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate space for subfiling filename");

    /* Check for a subfile name prefix setting in the environment */
    env_value = getenv(H5FD_SUBFILING_SUBFILE_PREFIX);
    if (env_value && (strlen(env_value) > 0))
        if (NULL == (sf_context->subfile_prefix = strdup(env_value)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't copy subfile prefix value");

    /* Set IOC stripe size from subfiling configuration */
    if (subfiling_config->stripe_size > 0)
        sf_context->sf_stripe_size = subfiling_config->stripe_size;

    /*
     * If still set to the default, set the number of subfiles according to the
     * default mapping of 1 I/O concentrator -> 1 subfile
     */
    if (sf_context->sf_num_subfiles == H5FD_SUBFILING_DEFAULT_STRIPE_COUNT)
        sf_context->sf_num_subfiles = app_topology->n_io_concentrators;

    /*
     * Set blocksize per stripe value after possibly adjusting for user-specified
     * subfile stripe size and number of subfiles
     */
    sf_context->sf_blocksize_per_stripe = sf_context->sf_stripe_size * sf_context->sf_num_subfiles;

    if (app_topology->rank_is_ioc) {
        int leftover_subfiles;

        /* Adjust base address after stripe size is set, if necessary */
        sf_context->sf_base_addr = (int64_t)(app_topology->ioc_idx * sf_context->sf_stripe_size);

        /*
         * Calculate the number of subfiles this rank owns by round-robining them
         * across the available IOCs and then allocate an array for the subfile IDs
         */
        sf_context->sf_num_fids = sf_context->sf_num_subfiles / app_topology->n_io_concentrators;

        leftover_subfiles = sf_context->sf_num_subfiles % app_topology->n_io_concentrators;
        if (leftover_subfiles && (leftover_subfiles > app_topology->ioc_idx))
            sf_context->sf_num_fids++;

        if (NULL == (sf_context->sf_fids =
                         H5MM_malloc((size_t)sf_context->sf_num_fids * sizeof(*sf_context->sf_fids))))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate subfile IDs array");

        for (int i = 0; i < sf_context->sf_num_fids; i++)
            sf_context->sf_fids[i] = -1;
    }

    /* Set up various MPI sub-communicators for MPI operations to/from IOC ranks */

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_dup(file_comm, &sf_context->sf_msg_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(sf_context->sf_msg_comm, MPI_ERRORS_RETURN)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_set_errhandler failed", mpi_code);

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_dup(file_comm, &sf_context->sf_data_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(sf_context->sf_data_comm, MPI_ERRORS_RETURN)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_set_errhandler failed", mpi_code);

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_dup(file_comm, &sf_context->sf_eof_comm)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_dup failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_errhandler(sf_context->sf_eof_comm, MPI_ERRORS_RETURN)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_set_errhandler failed", mpi_code);

    /* Create an MPI sub-communicator for IOC ranks */
    if (app_topology->n_io_concentrators > 1) {
        int mpi_rank;

        if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(file_comm, &mpi_rank)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_split(file_comm, app_topology->rank_is_ioc, mpi_rank,
                                                      &sf_context->sf_group_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_split failed", mpi_code);

        if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(sf_context->sf_group_comm, &sf_context->sf_group_rank)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
        if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(sf_context->sf_group_comm, &sf_context->sf_group_size)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);
    }

    /* Perform some final validation of subfiling configuration */
    if (sf_context->sf_stripe_size <= 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid subfiling stripe size (%" PRId64 ")",
                    sf_context->sf_stripe_size);
    if (sf_context->sf_num_subfiles <= 0)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid subfiling stripe count (%d)",
                    sf_context->sf_num_subfiles);
    assert(sf_context->sf_num_subfiles >= app_topology->n_io_concentrators);

    *context = sf_context;

done:
    if (ret_value < 0) {
        if (context_id >= 0 && H5FD__subfiling_free_object(context_id) < 0)
            HDONE_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling object");
        *context = NULL;
    }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_init_open_file_map
 *
 * Purpose:     Allocates and initializes an array that keeps a mapping
 *              between a file's inode value (__ino_t st_ino) and the ID
 *              of the context object associated with it.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_init_open_file_map(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!sf_open_file_map) {
        if (NULL ==
            (sf_open_file_map = H5MM_malloc((size_t)DEFAULT_FILE_MAP_ENTRIES * sizeof(*sf_open_file_map))))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate open file mapping");

        sf_file_map_size = DEFAULT_FILE_MAP_ENTRIES;
        for (int i = 0; i < sf_file_map_size; i++) {
            sf_open_file_map[i].file_id       = UINT64_MAX;
            sf_open_file_map[i].sf_context_id = -1;
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_record_fid_map_entry
 *
 * Purpose:     Every opened HDF5 file that uses subfiling  will have a subfiling
 *              context associated with it. It is important that the HDF5 file
 *              index is a constant rather than utilizing a POSIX file handle
 *              since files can be opened multiple times and with each file open,
 *              a new file handle will be assigned.  Note that in such a case, the
 *              actual filesystem id will be retained.
 *
 *              We use the filesystem id (ino_t inode) so that irrespective of
 *              what process opens a common file, the subfiling system will
 *              generate a consistent context for this file across all parallel
 *              ranks.
 *
 *              This function simply records the filesystem handle to
 *              subfiling context mapping.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_record_fid_map_entry(uint64_t file_id, int64_t subfile_context_id, int *next_index)
{
    subfiling_context_t *sf_context = NULL;
    int                  index;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    for (index = 0; index < sf_file_map_size; index++) {
        if (sf_open_file_map[index].file_id == file_id) {
            /* Increment file ref. count for this context */
            if (NULL == (sf_context = H5FD__subfiling_get_object(sf_open_file_map[index].sf_context_id)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context");
            sf_context->file_ref++;
            HGOTO_DONE(SUCCEED);
        }

        if (sf_open_file_map[index].file_id == UINT64_MAX) {
            sf_open_file_map[index].file_id       = file_id;
            sf_open_file_map[index].sf_context_id = subfile_context_id;

            /* First open of this file - set file ref. count to 1 for this context */
            if (NULL == (sf_context = H5FD__subfiling_get_object(subfile_context_id)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context");
            sf_context->file_ref = 1;

            if (next_index)
                *next_index = index;

            HGOTO_DONE(SUCCEED);
        }
    }

    if (index == sf_file_map_size) {
        void *tmp_realloc;

        if (NULL == (tmp_realloc = H5MM_realloc(
                         sf_open_file_map, ((size_t)(sf_file_map_size * 2) * sizeof(*sf_open_file_map)))))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't reallocate open file mapping");

        sf_open_file_map = tmp_realloc;
        sf_file_map_size *= 2;

        for (int i = index; i < sf_file_map_size; i++)
            sf_open_file_map[i].file_id = UINT64_MAX;

        if (next_index)
            *next_index = index;

        sf_open_file_map[index].file_id       = file_id;
        sf_open_file_map[index].sf_context_id = subfile_context_id;

        /* First open of this file - set file ref. count to 1 for this context */
        if (NULL == (sf_context = H5FD__subfiling_get_object(subfile_context_id)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context");
        sf_context->file_ref = 1;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_clear_fid_map_entry
 *
 * Purpose:     Remove the map entry associated with the file->inode.
 *              This is done at file close.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_clear_fid_map_entry(uint64_t file_id, int64_t sf_context_id)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!sf_open_file_map)
        HGOTO_DONE(SUCCEED);

    for (int i = 0; i < sf_file_map_size; i++) {
        if (sf_open_file_map[i].file_id == file_id && sf_open_file_map[i].sf_context_id == sf_context_id) {
            subfiling_context_t *sf_context;

            /* Only clear map entry if this is the last file
             * holding a reference to the context
             */
            if (NULL == (sf_context = H5FD__subfiling_get_object(sf_context_id)))
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling context object");

            if (sf_context->file_ref == 0 || sf_context->file_ref == 1) {
                sf_open_file_map[i].file_id       = UINT64_MAX;
                sf_open_file_map[i].sf_context_id = -1;
            }

            break;
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__subfiling_clear_fid_map_entry() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_get_default_ioc_config
 *
 * Purpose:     This simply fills in
 *              the basics.   This avoids the necessity of having the
 *              user write code to initialize the config structure.
 *
 * Return:      SUCCEED/FAIL
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_get_default_ioc_config(H5FD_ioc_config_t *config)
{
    FUNC_ENTER_PACKAGE_NOERR

    assert(config);

    memset(config, 0, sizeof(*config));

    config->magic            = H5FD_IOC_FAPL_MAGIC;
    config->version          = H5FD_IOC_CURR_FAPL_VERSION;
    config->thread_pool_size = H5FD_IOC_DEFAULT_THREAD_POOL_SIZE;

    FUNC_LEAVE_NOAPI(SUCCEED)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_ioc_open_files
 *
 * Purpose:     This function is called by an I/O concentrator in order to
 *              open the subfiles it is responsible for.
 *
 *              The names of the subfiles to be opened are generated based
 *              on values from either:
 *
 *              - The corresponding subfiling configuration file, if one
 *                exists and the HDF5 file isn't being truncated
 *              - The current subfiling context object for the file, if a
 *                subfiling configuration file doesn't exist or the HDF5
 *                file is being truncated
 *
 *              After the subfiles have been opened, a subfiling
 *              configuration file will be created if this is a file
 *              creation operation. If the truncate flag is specified, the
 *              subfiling configuration file will be re-created in order to
 *              account for any possible changes in the subfiling
 *              configuration.
 *
 *              Note that the HDF5 file opening protocol may attempt to
 *              open a file twice. A first open attempt is made without any
 *              truncate or other flags which would modify the file state
 *              if it already exists. Then, if this tentative open wasn't
 *              sufficient, the file is closed and a second file open using
 *              the user supplied open flags is invoked.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_ioc_open_files(int64_t file_context_id, int file_acc_flags)
{
    subfiling_context_t *sf_context   = NULL;
    mode_t               mode         = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char                *filepath     = NULL;
    char                *subfile_dir  = NULL;
    char                *base         = NULL;
    int                  num_subfiles = 0;
    int                  num_digits   = 0;
    herr_t               ret_value    = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (NULL == (sf_context = H5FD__subfiling_get_object(file_context_id)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "couldn't get subfiling object from context ID");

    assert(sf_context->h5_file_id != UINT64_MAX);
    assert(sf_context->h5_filename);
    assert(sf_context->sf_fids);
    assert(sf_context->sf_num_subfiles > 0);
    assert(sf_context->sf_num_fids > 0);
    assert(sf_context->topology);
    assert(sf_context->topology->ioc_idx >= 0); /* Only IOC ranks should be here */

    /* Get the basename of the full HDF5 filename */
    if (H5_basename(sf_context->h5_filename, &base) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't get HDF5 file basename");

    /*
     * Get the directory prefix where subfiles will be placed.
     * Under normal circumstances, the subfiles are co-located
     * with the HDF5 file, but users may specify a different
     * directory name.
     */
    if (sf_context->subfile_prefix) {
        if (NULL == (subfile_dir = H5MM_strdup(sf_context->subfile_prefix)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't copy subfile prefix");
    }
    else if (H5_dirname(sf_context->h5_filename, &subfile_dir) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't get HDF5 file dirname");

    if (NULL == (filepath = H5MM_malloc(PATH_MAX)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "couldn't allocate space for subfile filename");

    num_subfiles = sf_context->sf_num_subfiles;
    num_digits   = (int)(log10(num_subfiles) + 1);

    /*
     * For each subfile this IOC rank owns, generate the name
     * of the subfile and create/open it
     */
    for (int i = 0; i < sf_context->sf_num_fids; i++) {
        int subfile_idx;

        /* Round-robin subfiles among the available IOCs */
        subfile_idx = (i * sf_context->topology->n_io_concentrators) + sf_context->topology->ioc_idx + 1;

        /*
         * Generate the name of the subfile. The subfile naming should
         * produce files of the following form:
         * If we assume the HDF5 file is named ABC.h5, and 20 subfiles
         * are used, then the subfiles will have names:
         *   ABC.h5.subfile_<file-number>_01_of_20,
         *   ABC.h5.subfile_<file-number>_02_of_20, etc.
         *
         * and the configuration file will be named:
         *   ABC.h5.subfile_<file-number>.config
         */
        snprintf(filepath, PATH_MAX, "%s/" H5FD_SUBFILING_FILENAME_TEMPLATE, subfile_dir, base,
                 sf_context->h5_file_id, num_digits, subfile_idx, num_subfiles);

        if ((sf_context->sf_fids[i] = HDopen(filepath, file_acc_flags, mode)) < 0)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "failed to open subfile");
    }

    if (file_acc_flags & O_CREAT)
        sf_context->sf_eof = 0;

    /*
     * If subfiles were created (rather than simply opened),
     * check if we also need to create a config file.
     */
    if ((file_acc_flags & O_CREAT) && (sf_context->topology->ioc_idx == 0)) {
        char *config_dir = NULL;

        /*
         * If a config file prefix has been specified, place the
         * config file there, otherwise place it next to the
         * generated subfiles.
         */
        if (sf_context->config_file_prefix)
            config_dir = sf_context->config_file_prefix;
        else
            config_dir = subfile_dir;

        if (H5FD__subfiling_create_config_file(sf_context, base, config_dir, subfile_dir,
                                               (file_acc_flags & O_TRUNC)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTCREATE, FAIL, "couldn't create subfiling configuration file");
    }

done:
    if (ret_value < 0)
        if (sf_context)
            for (int i = 0; i < sf_context->sf_num_fids; i++) {
                if (sf_context->sf_fids[i] >= 0 && HDclose(sf_context->sf_fids[i]) < 0)
                    HDONE_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "failed to close subfile");
                sf_context->sf_fids[i] = -1;
            }

    H5MM_free(base);
    H5MM_free(subfile_dir);
    H5MM_free(filepath);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_create_config_file
 *
 * Purpose:     Creates a configuration file that contains
 *              subfiling-related information for a file. This file
 *              includes information such as:
 *
 *              - the stripe size for the file's subfiles
 *              - the number of I/O concentrators used for I/O to the file's subfiles
 *              - the number of subfiles the logical HDF5 file consists of
 *              - the base HDF5 filename
 *              - the optional directory prefix where the file's subfiles are placed
 *              - the names of each of the file's subfiles
 *
 * Return:      Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_create_config_file(subfiling_context_t *sf_context, const char *base_filename,
                                   const char *config_dir, const char *subfile_dir, bool truncate_if_exists)
{
    bool   config_file_exists = false;
    FILE  *config_file        = NULL;
    char  *config_filename    = NULL;
    char  *line_buf           = NULL;
    int    ret                = 0;
    herr_t ret_value          = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(sf_context);
    assert(base_filename);
    assert(config_dir);
    assert(subfile_dir);

    if (sf_context->h5_file_id == UINT64_MAX)
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid HDF5 file ID %" PRIu64, sf_context->h5_file_id);
    if (*base_filename == '\0')
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid base HDF5 filename '%s'", base_filename);
    if (*config_dir == '\0')
        config_dir = ".";
    if (*subfile_dir == '\0')
        subfile_dir = ".";

    if (NULL == (config_filename = H5MM_malloc(PATH_MAX)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                    "couldn't allocate space for subfiling configuration filename");

    snprintf(config_filename, PATH_MAX, "%s/" H5FD_SUBFILING_CONFIG_FILENAME_TEMPLATE, config_dir,
             base_filename, sf_context->h5_file_id);

    /* Determine whether a subfiling configuration file exists */
    errno = 0;
    ret   = HDaccess(config_filename, F_OK);

    config_file_exists = (ret == 0) || ((ret < 0) && (ENOENT != errno));

    if (config_file_exists && ret != 0)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL,
                        "couldn't check existence of subfiling configuration file");

    /*
     * If a config file doesn't exist, create one. If a config file does exist,
     * don't touch it unless the O_TRUNC flag was specified. In this case,
     * truncate the existing config file and create a new one.
     */
    if (!config_file_exists || truncate_if_exists) {
        int n_subfiles = sf_context->sf_num_subfiles;
        int num_digits;

        if (NULL == (config_file = fopen(config_filename, "w+")))
            HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL,
                            "couldn't create/truncate subfiling configuration file");

        if (NULL == (line_buf = H5MM_malloc(PATH_MAX)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                        "couldn't allocate buffer for writing to subfiling configuration file");

        /* Write the subfiling stripe size to the configuration file */
        snprintf(line_buf, PATH_MAX, "stripe_size=%" PRId64 "\n", sf_context->sf_stripe_size);
        if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "failed to write to subfiling configuration file");

        /* Write the number of I/O concentrators to the configuration file */
        snprintf(line_buf, PATH_MAX, "aggregator_count=%d\n", sf_context->topology->n_io_concentrators);
        if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "failed to write to subfiling configuration file");

        /* Write the number of subfiles to the configuration file */
        snprintf(line_buf, PATH_MAX, "subfile_count=%d\n", n_subfiles);
        if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "failed to write to subfiling configuration file");

        /* Write the base HDF5 filename to the configuration file */
        snprintf(line_buf, PATH_MAX, "hdf5_file=%s\n", sf_context->h5_filename);
        if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "failed to write to subfiling configuration file");

        /* Write the optional subfile directory prefix to the configuration file */
        snprintf(line_buf, PATH_MAX, "subfile_dir=%s\n", subfile_dir);
        if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
            HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL, "failed to write to subfiling configuration file");

        /* Write out each subfile name to the configuration file */
        num_digits = (int)(log10(n_subfiles) + 1);
        for (int k = 0; k < n_subfiles; k++) {
            snprintf(line_buf, PATH_MAX, H5FD_SUBFILING_FILENAME_TEMPLATE "\n", base_filename,
                     sf_context->h5_file_id, num_digits, k + 1, n_subfiles);

            if (fwrite(line_buf, strlen(line_buf), 1, config_file) != 1)
                HSYS_GOTO_ERROR(H5E_VFL, H5E_WRITEERROR, FAIL,
                                "failed to write to subfiling configuration file");
        }
    }

done:
    if (config_file)
        if (EOF == fclose(config_file))
            HDONE_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "couldn't close subfiling configuration file");

    H5MM_free(line_buf);
    H5MM_free(config_filename);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_open_config_file
 *
 * Purpose:     Opens the subfiling configuration file for a given HDF5
 *              file and sets `config_file_out`, if a configuration file
 *              exists. Otherwise, `config_file_out` is set to NULL.
 *
 *              It is the caller's responsibility to check `config_file_out`
 *              on success and close an opened file as necessary.
 *
 * Return:      Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__subfiling_open_config_file(const char *base_filename, const char *config_dir, uint64_t file_id,
                                 const char *mode, FILE **config_file_out)
{
    bool   config_file_exists = false;
    FILE  *config_file        = NULL;
    char  *config_filename    = NULL;
    int    ret                = 0;
    herr_t ret_value          = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(base_filename);
    assert(config_dir);
    assert(file_id != UINT64_MAX);
    assert(mode);
    assert(config_file_out);

    *config_file_out = NULL;

    if (*base_filename == '\0')
        HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL, "invalid base HDF5 filename '%s'", base_filename);
    if (*config_dir == '\0')
        config_dir = ".";

    if (NULL == (config_filename = H5MM_malloc(PATH_MAX)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                    "couldn't allocate space for subfiling configuration filename");

    snprintf(config_filename, PATH_MAX, "%s/" H5FD_SUBFILING_CONFIG_FILENAME_TEMPLATE, config_dir,
             base_filename, file_id);

    /* Determine whether a subfiling configuration file exists */
    errno = 0;
    ret   = HDaccess(config_filename, F_OK);

    config_file_exists = (ret == 0) || ((ret < 0) && (ENOENT != errno));

    if (!config_file_exists)
        HGOTO_DONE(SUCCEED);

    if (config_file_exists && ret != 0)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL,
                        "couldn't check existence of subfiling configuration file");

    if (NULL == (config_file = fopen(config_filename, mode)))
        HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL, "couldn't open subfiling configuration file");

    *config_file_out = config_file;

done:
    if (ret_value < 0)
        if (config_file && (EOF == fclose(config_file)))
            HDONE_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "couldn't close subfiling configuration file");

    H5MM_free(config_filename);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_get_config_from_file
 *
 * Purpose:     Reads a Subfiling configuration file to get the stripe size
 *              and number of subfiles used for the logical HDF5 file.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_get_config_from_file(FILE *config_file, int64_t *stripe_size, int64_t *num_subfiles)
{
    int64_t read_stripe_size  = 0;
    int64_t read_num_subfiles = 0;
    char   *config_buf        = NULL;
    char   *substr            = NULL;
    long    config_file_len   = 0;
    herr_t  ret_value         = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(config_file);

    if (HDfseek(config_file, 0, SEEK_END) < 0)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_SEEKERROR, FAIL, "couldn't seek to end of subfiling configuration file");

    if ((config_file_len = HDftell(config_file)) < 0)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get size of subfiling configuration file");

    if (HDfseek(config_file, 0, SEEK_SET) < 0)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_SEEKERROR, FAIL,
                        "couldn't seek to beginning of subfiling configuration file");

    if (NULL == (config_buf = H5MM_malloc((size_t)config_file_len + 1)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL,
                    "couldn't allocate space for reading from subfiling configuration file");

    if (fread(config_buf, (size_t)config_file_len, 1, config_file) != 1)
        HSYS_GOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "couldn't read from subfiling configuration file");

    config_buf[config_file_len] = '\0';

    if (stripe_size) {
        if (NULL == (substr = strstr(config_buf, "stripe_size")))
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                        "malformed subfiling configuration file - no stripe size entry");

        if (EOF == sscanf(substr, "stripe_size=%" PRId64, &read_stripe_size))
            HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                            "couldn't get stripe size from subfiling configuration file");

        if (read_stripe_size <= 0)
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                        "invalid stripe size (%" PRId64 ") read from subfiling configuration file",
                        read_stripe_size);

        *stripe_size = read_stripe_size;
    }

    if (num_subfiles) {
        if (NULL == (substr = strstr(config_buf, "subfile_count")))
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                        "malformed subfiling configuration file - no subfile count entry");

        if (EOF == sscanf(substr, "subfile_count=%" PRId64, &read_num_subfiles))
            HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                            "couldn't get number of subfiles from subfiling configuration file");

        if (read_num_subfiles <= 0)
            HGOTO_ERROR(H5E_VFL, H5E_BADVALUE, FAIL,
                        "invalid number of subfiles (%" PRId64 ") read from subfiling configuration file",
                        read_num_subfiles);

        *num_subfiles = read_num_subfiles;
    }

done:
    H5MM_free(config_buf);

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_resolve_pathname
 *
 * Purpose:     Simple wrapper routine around realpath(3) to fully resolve
 *              a given filepath. Collective across the specified MPI
 *              communicator in order to minimize file system contention
 *              between MPI ranks.
 *
 *              The resolved filepath returned through `resolved_filepath`
 *              must be freed by the caller with free.
 *
 * Return       Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_resolve_pathname(const char *filepath, MPI_Comm comm, char **resolved_filepath)
{
    hsize_t path_len         = HSIZE_UNDEF;
    bool    bcasted_path_len = false;
    bool    bcasted_path     = false;
    char   *resolved_path    = NULL;
    char   *file_basename    = NULL;
    char   *file_dirname     = NULL;
    char   *cwd              = NULL;
    int     mpi_rank;
    int     mpi_size;
    int     mpi_code;
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(filepath);
    assert(resolved_filepath);

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_rank(comm, &mpi_rank)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_rank failed", mpi_code);
    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(comm, &mpi_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    if (mpi_rank == 0) {
        errno = 0;
        if (NULL == (resolved_path = HDrealpath(filepath, NULL))) {
            if (ENOENT == errno) {
                if (H5_dirname(filepath, &file_dirname) < 0)
                    HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't get file dirname");

                /* If filepath is just the filename, set up path using CWD */
                if (!strcmp(file_dirname, ".")) {
                    if (NULL == (resolved_path = H5MM_malloc(PATH_MAX)))
                        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate buffer for filepath");
                    if (H5_basename(filepath, &file_basename) < 0)
                        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't get file basename");
                    if (NULL == (cwd = H5MM_malloc(PATH_MAX)))
                        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate buffer for CWD");

                    if (NULL == HDgetcwd(cwd, PATH_MAX))
                        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                                    "can't get current working directory, errno = %d, error message = '%s'",
                                    errno, strerror(errno));

                    snprintf(resolved_path, PATH_MAX, "%s/%s", cwd, file_basename);
                }
                else
                    /* Otherwise, just use what was given as the pathname */
                    if (NULL == (resolved_path = strdup(filepath)))
                        HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't copy filename");
            }
            else
                HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                            "can't resolve subfile path, errno = %d, error message = '%s'", errno,
                            strerror(errno));
        }

        if (resolved_path)
            H5_CHECKED_ASSIGN(path_len, hsize_t, (strlen(resolved_path) + 1), size_t);
        else
            path_len = HSIZE_UNDEF;
    }

    /* Broadcast the size of the resolved filepath string to other ranks */
    bcasted_path_len = true;
    if (mpi_size > 1)
        if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&path_len, 1, HSIZE_AS_MPI_TYPE, 0, comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code);

    if (path_len == HSIZE_UNDEF)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't resolve filepath");

    if (mpi_rank != 0)
        if (NULL == (resolved_path = H5MM_malloc(path_len)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTALLOC, FAIL, "can't allocate file name buffer");

    /* Broadcast the resolved filepath to other ranks */
    bcasted_path = true;
    if (mpi_size > 1) {
        H5_CHECK_OVERFLOW(path_len, hsize_t, int);
        if (MPI_SUCCESS != (mpi_code = MPI_Bcast(resolved_path, (int)path_len, MPI_CHAR, 0, comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
    }

    *resolved_filepath = resolved_path;

done:
    H5MM_free(cwd);
    H5MM_free(file_basename);
    H5MM_free(file_dirname);

    if (ret_value < 0) {
        if (!bcasted_path_len)
            if (MPI_SUCCESS != (mpi_code = MPI_Bcast(&path_len, 1, HSIZE_AS_MPI_TYPE, 0, comm)))
                HMPI_DONE_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
        if (!bcasted_path && (path_len != HSIZE_UNDEF)) {
            H5_CHECK_OVERFLOW(path_len, hsize_t, int);
            if (MPI_SUCCESS != (mpi_code = MPI_Bcast(resolved_path, (int)path_len, MPI_CHAR, 0, comm)))
                HMPI_DONE_ERROR(FAIL, "MPI_Bcast failed", mpi_code);
        }

        H5MM_free(resolved_path);
    }

    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_close_subfiles
 *
 * Purpose:     When closing an HDF5 file, we need to close any associated
 *              subfiles as well.  This function cycles through all known
 *              IO Concentrators to send a file CLOSE_OP command.
 *
 *              This function is collective across all MPI ranks which
 *              have opened HDF5 file which associated with the provided
 *              sf_context.  Once the request has been issued by all
 *              ranks, the subfile at each IOC will be closed and an
 *              completion ACK will be received.
 *
 *              Once the subfiles are closed, we initiate a teardown of
 *              the IOC and associated thread_pool threads.
 *
 * Return:      Success (0) or Failure (non-zero)
 * Errors:      If MPI operations fail for some reason.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_close_subfiles(int64_t subfiling_context_id, MPI_Comm file_comm)
{
    subfiling_context_t *sf_context  = NULL;
    MPI_Request          barrier_req = MPI_REQUEST_NULL;
    int                  mpi_size;
    int                  mpi_code;
    herr_t               ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (NULL == (sf_context = H5FD__subfiling_get_object(subfiling_context_id)))
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "couldn't get subfiling object from context ID");

    if (MPI_SUCCESS != (mpi_code = MPI_Comm_size(file_comm, &mpi_size)))
        HMPI_GOTO_ERROR(FAIL, "MPI_Comm_size failed", mpi_code);

    /* We make the subfile close operation collective.  Otherwise, there may be a
     * race condition between our closing the subfiles and the user application
     * moving ahead and possibly re-opening a file.
     *
     * If we can, we utilize an async barrier which gives us the opportunity to
     * reduce the CPU load due to MPI spinning while waiting for the barrier to
     * complete.  This is especially important if there is heavy thread
     * utilization due to subfiling activities, i.e. the thread pool might be
     * busy servicing I/O requests from all HDF5 application ranks.
     */
    if (mpi_size > 1) {
#if H5_CHECK_MPI_VERSION(3, 1)
        int barrier_complete = 0;

        if (MPI_SUCCESS != (mpi_code = MPI_Ibarrier(file_comm, &barrier_req)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Ibarrier failed", mpi_code);

        while (!barrier_complete) {
            useconds_t t_delay = 5;
            usleep(t_delay);

            if (MPI_SUCCESS != (mpi_code = MPI_Test(&barrier_req, &barrier_complete, MPI_STATUS_IGNORE)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Test failed", mpi_code);
        }
#else
        if (MPI_SUCCESS != (mpi_code = MPI_Barrier(file_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code);
#endif
    }

    /* The map from file handle to subfiling context can now be cleared */
    if (sf_context->h5_file_id != UINT64_MAX)
        H5FD__subfiling_clear_fid_map_entry(sf_context->h5_file_id, sf_context->sf_context_id);

    if (sf_context->topology->rank_is_ioc)
        if (sf_context->sf_fids)
            for (int i = 0; i < sf_context->sf_num_fids; i++) {
                errno = 0;
                if (sf_context->sf_fids[i] >= 0 && HDclose(sf_context->sf_fids[i]) < 0)
                    HSYS_GOTO_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, "couldn't close subfile");
                sf_context->sf_fids[i] = -1;
            }

    /*
     * Run another barrier to prevent some ranks from running ahead, and opening
     * another file before this file is completely closed down.
     */
    if (mpi_size > 1) {
#if H5_CHECK_MPI_VERSION(3, 1)
        int barrier_complete = 0;

        if (MPI_SUCCESS != (mpi_code = MPI_Ibarrier(file_comm, &barrier_req)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Ibarrier failed", mpi_code);

        while (!barrier_complete) {
            useconds_t t_delay = 5;
            usleep(t_delay);

            if (MPI_SUCCESS != (mpi_code = MPI_Test(&barrier_req, &barrier_complete, MPI_STATUS_IGNORE)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Test failed", mpi_code);
        }
#else
        if (MPI_SUCCESS != (mpi_code = MPI_Barrier(file_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code);
#endif
    }

#ifdef H5_SUBFILING_DEBUG
    if (sf_context->sf_logfile) {
        struct tm *tm = NULL;
        time_t     cur_time;

        cur_time = time(NULL);
        tm       = localtime(&cur_time);

        H5FD__subfiling_log(sf_context->sf_context_id, "\n-- LOGGING FINISH - %s", asctime(tm));

        fclose(sf_context->sf_logfile);
        sf_context->sf_logfile = NULL;
    }
#endif

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_set_config_prop
 *
 * Purpose:     Sets the specified Subfiling VFD configuration as a
 *              property on the given FAPL pointer. The Subfiling VFD uses
 *              this property to pass its configuration down to the IOC VFD
 *              without needing each IOC VFD to include it as part of its
 *              public configuration.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_set_config_prop(H5P_genplist_t *plist, const H5FD_subfiling_params_t *vfd_config)
{
    htri_t prop_exists = FAIL;
    herr_t ret_value   = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!plist)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL FAPL pointer");
    if (!vfd_config)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid subfiling configuration pointer");

    if ((prop_exists = H5P_exist_plist(plist, H5FD_SUBFILING_CONFIG_PROP)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                    "can't check if subfiling configuration property exists in FAPL");

    if (prop_exists) {
        if (H5P_set(plist, H5FD_SUBFILING_CONFIG_PROP, vfd_config) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "can't set subfiling configuration property on FAPL");
    }
    else
        /*
         * Cast away const since H5P_insert doesn't match the signature
         * for "value" as H5P_set
         */
        if (H5P_insert(plist, H5FD_SUBFILING_CONFIG_PROP, sizeof(H5FD_subfiling_params_t),
                       H5FD__subfiling_cast_to_void(vfd_config), NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                       NULL) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, FAIL,
                        "unable to register subfiling configuration property in FAPL");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_get_config_prop
 *
 * Purpose:     Retrieves the Subfiling VFD configuration from the given
 *              FAPL pointer. The Subfiling VFD uses this property to pass
 *              its configuration down to the IOC VFD without needing each
 *              IOC VFD to include it as part of its public configuration.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_get_config_prop(H5P_genplist_t *plist, H5FD_subfiling_params_t *vfd_config)
{
    htri_t prop_exists = FAIL;
    herr_t ret_value   = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!plist)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL FAPL pointer");
    if (!vfd_config)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid subfiling configuration pointer");

    if ((prop_exists = H5P_exist_plist(plist, H5FD_SUBFILING_CONFIG_PROP)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL,
                    "can't check if subfiling configuration property exists in FAPL");

    if (prop_exists) {
        if (H5P_get(plist, H5FD_SUBFILING_CONFIG_PROP, vfd_config) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get subfiling configuration property from FAPL");
    }
    else {
        vfd_config->ioc_selection = SELECT_IOC_ONE_PER_NODE;
        vfd_config->stripe_size   = H5FD_SUBFILING_DEFAULT_STRIPE_SIZE;
        vfd_config->stripe_count  = H5FD_SUBFILING_DEFAULT_STRIPE_COUNT;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_set_file_id_prop
 *
 * Purpose:     Sets the specified file ID (Inode) value as a property on
 *              the given FAPL pointer. The Subfiling VFD uses this
 *              property to pass the HDF5 stub file ID value down to the
 *              IOC VFD.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_set_file_id_prop(H5P_genplist_t *plist, uint64_t file_id)
{
    htri_t prop_exists = FAIL;
    herr_t ret_value   = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!plist)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL FAPL pointer");
    if (file_id == UINT64_MAX)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid file ID value");

    if ((prop_exists = H5P_exist_plist(plist, H5FD_SUBFILING_STUB_FILE_ID)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't check if file ID property exists in FAPL");

    if (prop_exists) {
        if (H5P_set(plist, H5FD_SUBFILING_STUB_FILE_ID, &file_id) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTSET, FAIL, "can't set file ID property on FAPL");
    }
    else if (H5P_insert(plist, H5FD_SUBFILING_STUB_FILE_ID, sizeof(uint64_t), &file_id, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTREGISTER, FAIL, "unable to register file ID property in FAPL");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_get_file_id_prop
 *
 * Purpose:     Retrieves the file ID (Inode) value from the given FAPL
 *              pointer. The Subfiling VFD uses this property to pass the
 *              HDF5 stub file ID value down to the IOC VFD.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_get_file_id_prop(H5P_genplist_t *plist, uint64_t *file_id)
{
    htri_t prop_exists = FAIL;
    herr_t ret_value   = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!plist)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL FAPL pointer");
    if (!file_id)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL file ID pointer");

    if ((prop_exists = H5P_exist_plist(plist, H5FD_SUBFILING_STUB_FILE_ID)) < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't check if file ID property exists in FAPL");

    if (prop_exists) {
        if (H5P_get(plist, H5FD_SUBFILING_STUB_FILE_ID, file_id) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get file ID property from FAPL");
    }
    else
        *file_id = UINT64_MAX;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfile_fid_to_context
 *
 * Purpose:     This is a basic lookup function which returns the subfiling
 *              context ID associated with the specified file ID. If no
 *              such context ID exists, `context_id_out` will be set to a
 *              negative value.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfile_fid_to_context(uint64_t file_id, int64_t *context_id_out)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    assert(context_id_out);

    *context_id_out = -1;

    if (H5FD__subfiling_init_open_file_map() < 0)
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, "couldn't initialize open file mapping");

    for (int i = 0; i < sf_file_map_size; i++)
        if (sf_open_file_map[i].file_id == file_id)
            *context_id_out = sf_open_file_map[i].sf_context_id;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5FD__subfile_fid_to_context() */

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_validate_config_params
 *
 * Purpose:     Checks that the given subfiling configuration parameters
 *              are valid
 *
 * Return:      Non-negative on success/Negative on failure
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_validate_config_params(const H5FD_subfiling_params_t *subf_config)
{
    H5FD_subfiling_ioc_select_t ioc_sel_type;
    herr_t                      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    if (!subf_config)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL subfiling configuration pointer");

    /*
     * Compare against each IOC selection value directly since
     * the enum might be a signed or unsigned type and a comparison
     * against < 0 could generate a warning
     */
    ioc_sel_type = subf_config->ioc_selection;
    if (ioc_sel_type != SELECT_IOC_ONE_PER_NODE && ioc_sel_type != SELECT_IOC_EVERY_NTH_RANK &&
        ioc_sel_type != SELECT_IOC_WITH_CONFIG && ioc_sel_type != SELECT_IOC_TOTAL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid IOC selection method");

    if (subf_config->stripe_size <= 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid stripe size");

    if (subf_config->stripe_count <= 0 && subf_config->stripe_count != H5FD_SUBFILING_DEFAULT_STRIPE_COUNT)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid stripe count");

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

/*-------------------------------------------------------------------------
 * Function:    H5FD__subfiling_terminate
 *
 * Purpose:     A cleanup routine to be called by the Subfiling VFD when
 *              it is terminating. Cleans up internal resources such as the
 *              context and topology caches.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5FD__subfiling_terminate(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Clean up subfiling context and topology caches */
    if (sf_context_cache) {
        for (size_t i = 0; i < sf_context_cache_size; i++)
            if (sf_context_cache[i]) {
                if (H5FD__subfiling_free_context(sf_context_cache[i]) < 0)
                    HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling context object");
                sf_context_cache[i] = NULL;
            }

        sf_context_cache_size       = 0;
        sf_context_cache_next_index = 0;

        H5MM_free(sf_context_cache);
        sf_context_cache = NULL;
    }
    if (sf_topology_cache) {
        for (size_t i = 0; i < sf_topology_cache_num_entries; i++) {
            if (H5FD__subfiling_free_topology(sf_topology_cache[i]) < 0)
                HGOTO_ERROR(H5E_VFL, H5E_CANTFREE, FAIL, "couldn't free subfiling topology object");
            sf_topology_cache[i] = NULL;
        }

        sf_topology_cache_size        = 0;
        sf_topology_cache_num_entries = 0;

        H5MM_free(sf_topology_cache);
        sf_topology_cache = NULL;
    }

    /* Clean up the file ID to context object mapping */
    sf_file_map_size = 0;
    H5MM_free(sf_open_file_map);
    sf_open_file_map = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

#ifdef H5_SUBFILING_DEBUG
void
H5FD__subfiling_log(int64_t sf_context_id, const char *fmt, ...)
{
    subfiling_context_t *sf_context = NULL;
    va_list              log_args;

    FUNC_ENTER_PACKAGE_NOERR

    va_start(log_args, fmt);

    /* Retrieve the subfiling object for the newly-created context ID */
    if (NULL == (sf_context = H5FD__subfiling_get_object(sf_context_id))) {
        printf("%s: couldn't get subfiling object from context ID\n", __func__);
        goto done;
    }

    H5FD_ioc_begin_thread_exclusive();

    if (sf_context->sf_logfile) {
        vfprintf(sf_context->sf_logfile, fmt, log_args);
        fputs("\n", sf_context->sf_logfile);
        fflush(sf_context->sf_logfile);
    }
    else {
        vprintf(fmt, log_args);
        puts("");
        fflush(stdout);
    }

    H5FD_ioc_end_thread_exclusive();

done:
    va_end(log_args);

    FUNC_LEAVE_NOAPI_VOID
}

void
H5FD__subfiling_log_nonewline(int64_t sf_context_id, const char *fmt, ...)
{
    subfiling_context_t *sf_context = NULL;
    va_list              log_args;

    FUNC_ENTER_PACKAGE_NOERR

    va_start(log_args, fmt);

    /* Retrieve the subfiling object for the newly-created context ID */
    if (NULL == (sf_context = H5FD__subfiling_get_object(sf_context_id))) {
        printf("%s: couldn't get subfiling object from context ID\n", __func__);
        goto done;
    }

    H5FD_ioc_begin_thread_exclusive();

    if (sf_context->sf_logfile) {
        vfprintf(sf_context->sf_logfile, fmt, log_args);
        fflush(sf_context->sf_logfile);
    }
    else {
        vprintf(fmt, log_args);
        fflush(stdout);
    }

    H5FD_ioc_end_thread_exclusive();

done:
    va_end(log_args);

    FUNC_LEAVE_NOAPI_VOID
}
#endif
