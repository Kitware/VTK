typedef struct _cgns_io_ctx_t {
    /* Flag indicating if HDF5 file accesses is PARALLEL or NATIVE */
    char hdf5_access[64];
#if CG_BUILD_PARALLEL
    /* MPI-2 info object */
    MPI_Comm pcg_mpi_comm;
    int pcg_mpi_comm_size;
    int pcg_mpi_comm_rank;
    /* flag indicating if mpi_initialized was called */
    int pcg_mpi_initialized;
    MPI_Info pcg_mpi_info;
    hid_t default_pio_mode;
#endif
} cgns_io_ctx_t;
