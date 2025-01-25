/*-------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 *   _____ _____ _   _  _____
 *  / ____/ ____| \ | |/ ____|
 * | |   | |  __|  \| | (___
 * | |   | | |_ | . ` |\___ \
 * | |___| |__| | |\  |____) |
 *  \_____\_____|_| \_|_____/
 *
 *  PURPOSE:
 *    Provides Parallel Mid-Level Library (MLL) CGNS interfaces and
 *    various supporting APIs.
 *
 *  DOCUMENTATION DESIGN
 *    * Document all new public APIs with Doxygen entries.
 *    * Keep descriptive text line lengths between 50-60 characters optimally,
 *      with a maximum of 90 characters.
 *    * Consider using Doxygen aliases for recurring entries.
 */


/**
 * \defgroup ParallelMisc Parallel Miscellaneous Routines
 * \defgroup ParallelFile Parallel File Operations
 * \defgroup ParallelGridCoordinate Parallel Grid Coordinate Data
 * \defgroup ElementConnectivityData Parallel Element Connectivity Data
 * \defgroup SolutionData Parallel Solution Data
 * \defgroup ArrayData Parallel Array Data
 * \defgroup PointListData Parallel Point List Data
 * \defgroup ParallelParticleCoordinate Parallel Particle Coordinates
 * \defgroup ParallelParticleSolutionData Parallel Particle Solution Data
 **/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pcgnslib.h"
#include "cgns_header.h"
#include "cgns_io.h"
#include "mpi.h"
#include "vtk_hdf5.h"

#define IS_FIXED_SIZE(type) ((type >= CGNS_ENUMV(NODE) && \
                              type <= CGNS_ENUMV(HEXA_27)) || \
                              type == CGNS_ENUMV(PYRA_13) || \
                             (type >= CGNS_ENUMV(BAR_4) && \
                              type <= CGNS_ENUMV(HEXA_125)))

#include "cgio_internal_type.h" /* for cgns_io_ctx_t */
extern cgns_io_ctx_t ctx_cgio; /* located in cgns_io.c */

extern int cgns_filetype;
extern void* cgns_rindindex;

typedef struct cg_rw_t {
  union {
    void *rbuf;             /* Pointer to buffer for read */
    const void *wbuf;       /* Pointer to buffer to write */
  } u;
} cg_rw_t;

typedef struct cg_rw__ptr_t {
  union {
    void **rbuf;             /* Pointer to buffer for read */
    const void **wbuf;       /* Pointer to buffer to write */
  } u;
} cg_rw_ptr_t;

/* flag for parallel reading or parallel writing */
enum cg_par_rw{
  CG_PAR_READ,
  CG_PAR_WRITE
};

/*===== parallel IO functions =============================*/

static int readwrite_data_parallel(hid_t group_id, CGNS_ENUMT(DataType_t) type,
    int ndims, const cgsize_t *rmin, const cgsize_t *rmax, cg_rw_t *data, enum cg_par_rw rw_mode)
{
  int k;
  hid_t data_id, mem_shape_id, data_shape_id;
  hsize_t start[CGIO_MAX_DIMENSIONS], dims[CGIO_MAX_DIMENSIONS];
  herr_t herr, herr1;
  hid_t type_id, plist_id;

  /* convert from CGNS to HDF5 data type */
  switch (type) {
  case CGNS_ENUMV(Character):
    type_id = H5T_NATIVE_CHAR;
    break;
  case CGNS_ENUMV(Integer):
    type_id = H5T_NATIVE_INT32;
    break;
  case CGNS_ENUMV(LongInteger):
    type_id = H5T_NATIVE_INT64;
    break;
  case CGNS_ENUMV(RealSingle):
    type_id = H5T_NATIVE_FLOAT;
    break;
  case CGNS_ENUMV(RealDouble):
    type_id = H5T_NATIVE_DOUBLE;
    break;
  default:
    cgi_error("unhandled data type %d\n", type);
    return CG_ERROR;
  }

  /* Open the data */
  if ((data_id = H5Dopen2(group_id, " data", H5P_DEFAULT)) < 0) {
    cgi_error("H5Dopen2() failed");
    return CG_ERROR;
  }

  /* Set the start position and size for the data write */
  /* fix dimensions due to Fortran indexing and ordering */
  if((rw_mode == CG_PAR_WRITE && data[0].u.wbuf) || (rw_mode == CG_PAR_READ && data[0].u.rbuf)) {
    for (k = 0; k < ndims; k++) {
      start[k] = rmin[ndims-k-1] - 1;
      dims[k] = rmax[ndims-k-1] - start[k];
    }
  }
  else { /* no data to read or write, but must still call H5Screate_simple */
      for (k = 0; k < ndims; k++) {
        dims[k] = 0;
      }
  }

  /* Create a shape for the data in memory */
  mem_shape_id = H5Screate_simple(ndims, dims, NULL);
  if (mem_shape_id < 0) {
    H5Dclose(data_id);
    cgi_error("H5Screate_simple() failed");
    return CG_ERROR;
  }

  /* Create a shape for the data in the file */
  data_shape_id = H5Dget_space(data_id);
  if (data_shape_id < 0) {
    H5Sclose(mem_shape_id);
    H5Dclose(data_id);
    cgi_error("H5Dget_space() failed");
    return CG_ERROR;
  }

  if((rw_mode == CG_PAR_WRITE && data[0].u.wbuf) || (rw_mode == CG_PAR_READ && data[0].u.rbuf)) {
    /* Select a section of the array in the file */
    herr = H5Sselect_hyperslab(data_shape_id, H5S_SELECT_SET, start,
			       NULL, dims, NULL);
    herr1 = 0;
  } else {
    herr = H5Sselect_none(data_shape_id);
    herr1 = H5Sselect_none(mem_shape_id);
  }

  if (herr < 0 || herr1 < 0) {
    H5Sclose(data_shape_id);
    H5Sclose(mem_shape_id);
    H5Dclose(data_id);
    cgi_error("H5Sselect_hyperslab() failed");
    return CG_ERROR;
  }

  /* Set the access property list for data transfer */
  plist_id = H5Pcreate(H5P_DATASET_XFER);
  if (plist_id < 0) {
    H5Sclose(data_shape_id);
    H5Sclose(mem_shape_id);
    H5Dclose(data_id);
    cgi_error("H5Pcreate() failed");
    return CG_ERROR;
  }

  /* Set MPI-IO independent or collective communication */
  herr = H5Pset_dxpl_mpio(plist_id, ctx_cgio.default_pio_mode);
  if (herr < 0) {
    H5Pclose(plist_id);
    H5Sclose(data_shape_id);
    H5Sclose(mem_shape_id);
    H5Dclose(data_id);
    cgi_error("H5Pset_dxpl_mpio() failed");
    return CG_ERROR;
  }

  /* Write the data in parallel I/O */
  if (rw_mode == CG_PAR_READ) {
    herr = H5Dread(data_id, type_id, mem_shape_id,
		   data_shape_id, plist_id, data[0].u.rbuf);
    if (herr < 0)
      cgi_error("H5Dread() failed");

  } else {
    herr = H5Dwrite(data_id, type_id, mem_shape_id,
		    data_shape_id, plist_id, data[0].u.wbuf);
    if (herr < 0)
      cgi_error("H5Dwrite() failed");
  }

  H5Pclose(plist_id);
  H5Sclose(data_shape_id);
  H5Sclose(mem_shape_id);
  H5Dclose(data_id);

  return herr < 0 ? CG_ERROR : CG_OK;
}

/* Note: if dataset[0] == NULL, meaning this processor writes no data, then
 * m_numdim must be valid and m_dimvals[:] = 0 */
static int readwrite_shaped_data_parallel(
    hid_t group_id,
    const cgsize_t *s_start, const cgsize_t *s_end, const cgsize_t *s_stride,
    CGNS_ENUMT(DataType_t) m_type, int m_numdim, const cgsize_t *m_dimvals,
    const cgsize_t *m_start, const cgsize_t *m_end, const cgsize_t *m_stride,
    void **dataset, enum cg_par_rw rw_mode)
{
    hid_t data_id, mem_shape_id, data_shape_id;
    hid_t type_id, plist_id;
    hsize_t dimvals[CGIO_MAX_DIMENSIONS];
    hsize_t start[CGIO_MAX_DIMENSIONS];
    hsize_t stride[CGIO_MAX_DIMENSIONS];
    hsize_t count[CGIO_MAX_DIMENSIONS];
    herr_t herr;
    int n;

     /* convert from CGNS to HDF5 data type */
    switch (m_type) {
    case CGNS_ENUMV(Character):
        type_id = H5T_NATIVE_CHAR;
        break;
    case CGNS_ENUMV(Integer):
        type_id = H5T_NATIVE_INT32;
        break;
    case CGNS_ENUMV(LongInteger):
        type_id = H5T_NATIVE_INT64;
        break;
    case CGNS_ENUMV(RealSingle):
        type_id = H5T_NATIVE_FLOAT;
        break;
    case CGNS_ENUMV(RealDouble):
        type_id = H5T_NATIVE_DOUBLE;
        break;
    default:
        cgi_error("Unhandled data type %d\n", m_type);
        herr = -1;
        goto error_0;
    }

     /* Open the data */
    data_id = H5Dopen2(group_id, " data", H5P_DEFAULT);
    if (data_id < 0) {
        cgi_error("H5Dopen2() failed");
        herr = -1;
        goto error_0;
    }

     /* Get file dataspace extents */
    data_shape_id = H5Dget_space(data_id);
    if (data_shape_id < 0) {
        cgi_error("H5Dget_space() failed");
        herr = -1;
        goto error_1df;
    }
    const int s_numdim = H5Sget_simple_extent_ndims(data_shape_id);

    /* Create file hyperslab (shape for data in the file) */
    if (dataset[0]) {
        /* Reverse unit stride dimension (because of Fortran ordering) */
        for (n = 0; n < s_numdim; n++) {
            start [s_numdim-1-n] = s_start[n] - 1;
            stride[s_numdim-1-n] = s_stride[n];
            count [s_numdim-1-n] = (s_end[n] - s_start[n] + 1) / s_stride[n];
        }
        herr = H5Sselect_hyperslab(data_shape_id, H5S_SELECT_SET,
                                   start, stride, count, NULL);
    } else {
        herr = H5Sselect_none(data_shape_id);
    }
    if (herr < 0) {
        cgi_error("H5Sselect_hyperslab() for file data failed");
        goto error_2ds;
    }

    /* Create memory hyperslab (shape for data in memory) */
    if (dataset[0]) {
        /* Reverse unit stride dimension (because of Fortran ordering) */
        for (n = 0; n < m_numdim; n++) {
            dimvals  [m_numdim-1-n] = m_dimvals[n];
            start    [m_numdim-1-n] = m_start[n] - 1;
            stride   [m_numdim-1-n] = m_stride[n];
            count    [m_numdim-1-n] = (m_end[n] - m_start[n] + 1) / m_stride[n];
        }
        mem_shape_id = H5Screate_simple(m_numdim, dimvals, NULL);
        if (mem_shape_id < 0) {
            cgi_error("H5Screate_simple() for memory space failed");
            herr = -1;
            goto error_2ds;
        }
        herr = H5Sselect_hyperslab(mem_shape_id, H5S_SELECT_SET,
                                   start, stride, count, NULL);
    } else {  /* m_numdim should be valid and m_dimvals[:] should be 0 */
        for (n = 0; n < m_numdim; n++) {
            dimvals[n] = m_dimvals[n];
        }
        mem_shape_id = H5Screate_simple(m_numdim, dimvals, NULL);
        if (mem_shape_id < 0) {
            cgi_error("H5Screate_simple() for null memory space failed");
            herr = -1;
            goto error_2ds;
        }
        herr = H5Sselect_none(mem_shape_id);
    }
    if (herr < 0) {
        cgi_error("H5Sselect_hyperslab() for memory data failed");
        goto error_3ms;
    }

    /* Make sure memory space and file space have same number of points */
    if (H5Sget_select_npoints(mem_shape_id) !=
        H5Sget_select_npoints(data_shape_id)) {
        cgi_error("Unequal points in memory and file space");
        herr = -1;
        goto error_3ms;
    }

    /* Set the access property list for data transfer */
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    if (plist_id < 0) {
        cgi_error("H5Pcreate() failed");
        herr = -1;
        goto error_3ms;
    }

    /* Set MPI-IO independent or collective communication */
    herr = H5Pset_dxpl_mpio(plist_id, ctx_cgio.default_pio_mode);
    if (herr < 0) {
        cgi_error("H5Pset_dxpl_mpio() failed");
        goto error_4pl;
    }

    /* Read/write the data in parallel I/O */
    if (rw_mode == CG_PAR_READ) {
        herr = H5Dread(data_id, type_id, mem_shape_id,
                       data_shape_id, plist_id, dataset[0]);
        if (herr < 0)
            cgi_error("H5Dread() failed");
    } else {
        herr = H5Dwrite(data_id, type_id, mem_shape_id,
                        data_shape_id, plist_id, dataset[0]);
        if (herr < 0)
            cgi_error("H5Dwrite() failed");
    }

error_4pl: H5Pclose(plist_id);
error_3ms: H5Sclose(mem_shape_id);
error_2ds: H5Sclose(data_shape_id);
error_1df: H5Dclose(data_id);
error_0: ;
    return herr < 0 ? CG_ERROR : CG_OK;
}

/*---------------------------------------------------------*/

static int check_parallel(cgns_file *cgfile)
{
    int type;

    if (cgfile == NULL) return CG_ERROR;
    if (cgio_get_file_type(cgfile->cgio, &type) ||
        type != CGIO_FILE_HDF5) {
        cgi_error("file not opened for parallel IO");
        return CG_ERROR;
    }
    return CG_OK;
}

/*================================*/
/*== Begin Function Definitions ==*/
/*================================*/

/**
 * \ingroup ParallelMisc
 *
 * \brief Set the MPI communicator.
 *
 * \param[in] comm The MPI communicator to be used by the CGNS library.
 * \details Sets the MPI communicator for parallel operations by the CGNS library.
 *          The default value is MPI_COMM_WORLD.
 * \return \ier
 */
int cgp_mpi_comm(MPI_Comm comm)
{
    /* check if we are actually running a parallel program */
    /* Flag is true if MPI_Init or MPI_Init_thread has been called and false otherwise. */
    ctx_cgio.pcg_mpi_initialized = 0;
    MPI_Initialized(&ctx_cgio.pcg_mpi_initialized);

    if (ctx_cgio.pcg_mpi_initialized) {
      if( cgio_configure(CG_CONFIG_HDF5_MPI_COMM, &comm) != CG_OK) {
        cgi_error("Invalid CG_CONFIG_HDF5_MPI_COMM configure parameter");
        return CG_ERROR;
      }

      ctx_cgio.pcg_mpi_comm=comm;
      MPI_Comm_rank(ctx_cgio.pcg_mpi_comm, &ctx_cgio.pcg_mpi_comm_rank);
      MPI_Comm_size(ctx_cgio.pcg_mpi_comm, &ctx_cgio.pcg_mpi_comm_size);
    }

    return ctx_cgio.pcg_mpi_initialized ? CG_OK : CG_ERROR;
}
/**
 * \ingroup ParallelMisc
 *
 * \brief Set the MPI info object.
 *
 * \param[in] info The MPI info object to be used by the CGNS library.
 * \return \ier
 * \details Passes the MPI info object for parallel operations to the CGNS library.
 *          Notes for Fortran: the data type for info is an INTEGER.
 */
int cgp_mpi_info(MPI_Info info)
{
    ctx_cgio.pcg_mpi_info = info;

    return CG_OK;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelMisc
 *
 * \brief Set the parallel IO mode.
 *
 * \param[in]  mode Parallel input/output mode.
 * \return \ier
 * \details Sets the mode for parallel data reads and writes. The default value
 *          is \p CGP_COLLECTIVE, which allows any number of processes to access the data.
 *          When set to \p CGP_COLLECTIVE, all processes must access the data.
 */
int cgp_pio_mode(CGNS_ENUMT(PIOmode_t) mode)
{
    if (mode == CGP_INDEPENDENT)
        ctx_cgio.default_pio_mode = H5FD_MPIO_INDEPENDENT;
    else if (mode == CGP_COLLECTIVE)
        ctx_cgio.default_pio_mode = H5FD_MPIO_COLLECTIVE;
    else {
        cgi_error("unknown parallel IO mode");
        return CG_ERROR;
    }

    return CG_OK;
}


/*---------------------------------------------------------*/
/**
 * \ingroup ParallelMisc
 *
 * \brief Exit with error message.
 *
 * \details Is similar to cg_error_exit() in that the process will exit with
 *          an error message. However, it will also print the process rank and
 *          call \p MPI_Abort with an exit code of 1.
 */
void cgp_error_exit(void)
{
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "[process %d]:%s\n", rank, cg_get_error());
    cgio_cleanup();
    MPI_Abort(MPI_COMM_WORLD, 1);
}

/*===== File IO Prototypes ================================*/
/**
 * \ingroup ParallelFile
 *
 * \brief Open a file for parallel IO.
 *
 * \param[in]  filename \FILE_filename
 * \param[in]  mode     \FILE_mode
 * \param[out] fn       \FILE_fn
 * \return \ier
 * \details Similar to cg_open() and calls that routine. The differences
 *          is that cgp_open() explicitly sets an internal CGNS flag to
 *          indicate parallel access.
 */
int cgp_open(const char *filename, int mode, int *fn)
{
    int ierr, old_type = cgns_filetype;

    /* Initialize communicators if cgp_mpi_comm() was not called by
       client */
    if (ctx_cgio.pcg_mpi_comm == MPI_COMM_NULL) {
      cgp_mpi_comm(MPI_COMM_WORLD);
    }

    /* Flag this as a parallel access */
    strcpy(ctx_cgio.hdf5_access,"PARALLEL");

    ierr = cg_set_file_type(CG_FILE_HDF5);
    if (ierr) return ierr;
    ierr = cg_open(filename, mode, fn);
    cgns_filetype = old_type;

    return ierr;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelFile
 *
 * \brief Close a CGNS file.
 *
 * \param[in]  fn \FILE_fn
 * \return \ier
 * \details Similar to cg_close() and calls that routine.
 */
int cgp_close(int fn)
{
    /* reset parallel access */
    strcpy(ctx_cgio.hdf5_access,"NATIVE");
    return cg_close(fn);
}

/*===== Grid IO Prototypes ================================*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Create a coordinate data node by multiple processes in a parallel fashion.
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  type      \PGRID_datatype
 * \param[in]  coordname \PGRID_coordname
 * \param[out] C         \PGRID_Coordinate
 * \return \ier
 * \details To write the data in parallel, first call cgp_coord_write() to
 *          create an empty data node. This call is identical to cg_coord_write() with
 *          \p coord_array set to NULL (no data written). The actual data is then written
 *          to the node in parallel using either cgp_coord_write_data() or
 *          cgp_coord_general_write_data() where \p range_min and \p range_max specify
 *          the subset of coordinate data to be written by a given process.
 */
int cgp_coord_write(int fn, int B, int Z, CGNS_ENUMT(DataType_t) type,
    const char *coordname, int *C)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_coord_write(fn, B, Z, type, coordname, NULL, C);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Write coordinate data in parallel.
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  C      \C_Coordinate
 * \param[in]  rmin   \PGRID_range_min
 * \param[in]  rmax   \PGRID_range_max
 * \param[in]  coords \PGRID_coord_array
 * \return \ier
 * \details Writes the actual data to the node in parallel, where
 *          \p rmin and \p rmax specify the subset of coordinate data
 *          to be written by a given process. It is the responsibility of
 *          the application to ensure that the data type for the coordinate
 *          data matches that defined in the file; no conversions are done.
 */
int cgp_coord_write_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *coords)
{
    int n;
    cgns_zone *zone;
    cgns_zcoor *zcoor;
    cgsize_t dims[3];
    hid_t hid;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) return CG_ERROR;

    if (C > zcoor->ncoords || C <= 0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }

    for (n = 0; n < zone->index_dim; n++) {
        dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                  zcoor->rind_planes[2*n+1];
      if(coords) {
        if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
          printf("%d %d %d", rmin[n]> rmax[n], rmin[n] <1, rmax[n] >dims[n]);
          cgi_error("Invalid index ranges. cgp_coord_write_data");
          return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(zcoor->coord[C-1].data_type);

    to_HDF_ID(zcoor->coord[C-1].id,hid);

    cg_rw_t Data;
    Data.u.wbuf = coords;
    return readwrite_data_parallel(hid, type,
				   zone->index_dim, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Write shaped array to a subset of grid coordinates in parallel.
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  C             \PGRID_Coordinate
 * \param[in]  rmin          \PGRID_range_min
 * \param[in]  rmax          \PGRID_range_max
 * \param[in]  m_type        \PGRID_mem_datatype
 * \param[in]  m_numdim      \PGRID_mem_rank
 * \param[in]  m_arg_dimvals \PGRID_mem_dimensions
 * \param[in]  m_rmin        \PGRID_mem_range_min
 * \param[in]  m_rmax        \PGRID_mem_range_max
 * \param[out] coords        \PGRID_coord_array
 * \return \ier
 * \details The cgp_coord_general_write_data() perform data conversions if \e datatype
 *          is different from \e mem_datatype. If \e coords == NULL, meaning this processor
 *          writes no data, then only \e fn, \e B, \e Z, and \e C need be set.  In this case,
 *          \e Z and \e C are "representative" and can point to any valid zone.
 */
int cgp_coord_general_write_data(int fn, int B, int Z, int C,
                                 const cgsize_t *rmin, const cgsize_t *rmax,
                                 CGNS_ENUMT(DataType_t) m_type,
                                 int m_numdim, const cgsize_t *m_arg_dimvals,
                                 const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                 const void *coords)
{
    int n, ier;
    hid_t hid;
    cgns_zone *zone;
    cgns_zcoor *zcoor;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor == 0) return CG_ERROR;

    if (C > zcoor->ncoords || C <= 0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }

     /* get file-space rank.  Dimensions already set by previous null
      * call to cgp_coord_write*/
    const int s_numdim = zone->index_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (coords) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = zcoor->coord[C-1].dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_write_full_range; /* unused */
        int m_read_full_range;  /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Write, cgns_rindindex, zcoor->rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_write_full_range, &m_read_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(zcoor->coord[C-1].data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, Z, and C arguments are needed to get hid */
    to_HDF_ID(zcoor->coord[C-1].id, hid);

    void* dataset[1];
    dataset[0] = (void*)coords;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Read coordinate data in parallel.
 *
 * \param[in]  fn     \FILE_fn
 * \param[in]  B      \B_Base
 * \param[in]  Z      \Z_Zone
 * \param[in]  C      \C_Coordinate
 * \param[in]  rmin   \PGRID_range_min
 * \param[in]  rmax   \PGRID_range_max
 * \param[out] coords \PGRID_coord_array
 * \return \ier
 * \details Reads the actual data to the node in parallel, where \p rmin
 *          and \p rmax specify the subset of coordinate data to be read
 *          by a given process. It is the responsibility of the application
 *          to ensure that the data type for the coordinate data matches that
 *          defined in the file; no conversions are done.
 */
int cgp_coord_read_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, void *coords)
{
    int n;
    hid_t hid;
    cgns_zone *zone;
    cgns_zcoor *zcoor;
    cgsize_t dims[3];
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) return CG_ERROR;

    if (C > zcoor->ncoords || C <= 0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }

    for (n = 0; n < zone->index_dim; n++) {
      dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                zcoor->rind_planes[2*n+1];
      if(coords) {
        if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
          cgi_error("Invalid index ranges.");
          return CG_ERROR;
	}
      }
    }
    type = cgi_datatype(zcoor->coord[C-1].data_type);


    to_HDF_ID(zcoor->coord[C-1].id,hid);
    cg_rw_t Data;
    Data.u.rbuf = coords;
    return readwrite_data_parallel(hid, type,
			      zone->index_dim, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Read shaped array to a subset of grid coordinates in parallel.
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  C             \C_Coordinate
 * \param[in]  rmin          \PGRID_range_min
 * \param[in]  rmax          \PGRID_range_max
 * \param[in]  m_type        \PGRID_mem_datatype
 * \param[in]  m_numdim      \PGRID_mem_rank
 * \param[in]  m_arg_dimvals \PGRID_mem_dimensions
 * \param[in]  m_rmin        \PGRID_mem_range_min
 * \param[in]  m_rmax        \PGRID_mem_range_max
 * \param[out] coords        \PGRID_coord_array
 * \return \ier
 * \details The cgp_coord_general_read_data() perform data conversions if
 *          \e datatype is different from \e mem_datatype. If \e coords == NULL,
 *          meaning this processor reads no data, then only \e fn, \e B, \e Z,
 *          and \e C need to be set.  In this case, \e Z and \e C are "representative"
 *          and can point to any valid zone.
 */
int cgp_coord_general_read_data(int fn, int B, int Z, int C,
                                const cgsize_t *rmin, const cgsize_t *rmax,
                                CGNS_ENUMT(DataType_t) m_type,
                                int m_numdim, const cgsize_t *m_arg_dimvals,
                                const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                void *coords)
{
    int n, ier;
    hid_t hid;
    cgns_zone *zone;
    cgns_zcoor *zcoor;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone == 0) return CG_ERROR;

     /* Get memory address for node "GridCoordinates" */
    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor == 0) return CG_ERROR;

    if (C > zcoor->ncoords || C <= 0) {
        cgi_error("coord number %d invalid",C);
        return CG_ERROR;
    }

     /* get file-space dimensions.  Dimensions already set by previous null
      * call to cgp_coord_write*/
    const int s_numdim = zone->index_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (coords) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = zcoor->coord[C-1].dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_read_full_range;  /* unused */
        int m_write_full_range; /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Read, cgns_rindindex, zcoor->rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_read_full_range, &m_write_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(zcoor->coord[C-1].data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, Z, and C arguments are needed to get hid */
    to_HDF_ID(zcoor->coord[C-1].id, hid);

    void* dataset[1];
    dataset[0] = coords;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_READ);
}

/*===== Elements IO Prototypes ============================*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Create a section data node.
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  sectionname \PCONN_ElementSectionName
 * \param[in]  type        \PCONN_type
 * \param[in]  start       \PCONN_start
 * \param[in]  end         \PCONN_end
 * \param[in]  nbndry      \PCONN_nbndry
 * \param[out] S           \CONN_S
 * \return \ier
 * \details cgp_section_write() is used to write element connectivity data by multiple
 *          processes in a parallel fashion. To write the element data in parallel, first
 *          call cgp_section_write() to create an empty data node. This call is identical
 *          to cg_section_write() with \e Elements set to \e NULL (no data written). The
 *          actual element data is then written to the node in parallel using
 *          cgp_elements_write_data() where \e start and \e end specify the range of the
 *          elements to be written by a given process.
 * \note  Routine only works for constant-sized elements, since it is not possible to
 *        compute file offsets for variable-sized elements without knowledge of the entire
 *        element connectivity data.
 * \note  It is the responsibility of the application to ensure that \e cgsize_t in the
 *        application is the same size as defined in the file; no conversions are done.
 */
int cgp_section_write(int fn, int B, int Z, const char *sectionname,
    CGNS_ENUMT(ElementType_t) type, cgsize_t start, cgsize_t end,
    int nbndry, int *S)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;
    if (!IS_FIXED_SIZE(type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    return cg_section_partial_write(fn, B, Z, sectionname, type,
               start, end, nbndry, S);
}
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Create a section data node.
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  sectionname \PCONN_ElementSectionName
 * \param[in]  type        \PCONN_type
 * \param[in]  start       \PCONN_start
 * \param[in]  end         \PCONN_end
 * \param[in]  maxoffset   \PCONN_MaxOffset
 * \param[in]  nbndry      \PCONN_nbndry
 * \param[out] S           \CONN_S
 * \return \ier
 * \details cgp_poly_section_write() is used to write element connectivity data by multiple processes
 *          in a parallel fashion. To write the element data in parallel, first call
 *          cgp_section_write() to create an empty data node. This call is identical to
 *          cg_section_write() with \e Elements set to \e NULL (no data written). The actual
 *          element data is then written to the node in parallel using cgp_elements_write_data()
 *          where \e start and \e end specify the range of the elements to be written by a given process.
 * \note Routine only works for constant sized elements, since it is not possible to compute file
 *       offsets for variable sized elements without knowing the entire element connectivity data.
 * \note It is the responsibility of the application to ensure that \e cgsize_t in the application is
 *       the same size as that defined in the file; no conversions are done.
 */
int cgp_poly_section_write(int fn, int B, int Z, const char *sectionname,
    CGNS_ENUMT(ElementType_t) type, cgsize_t start, cgsize_t end, cgsize_t maxoffset,
    int nbndry, int *S)
{
  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;
  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
    return CG_ERROR;

  if (IS_FIXED_SIZE(type)) {
    cgi_error("element type must not be a fixed size for this parallel IO");
    return CG_ERROR;
  }

  return cg_section_general_write(fn, B, Z, sectionname, type,
      cgi_datatype(CG_SIZE_DATATYPE), start, end, maxoffset, nbndry, S);

}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Write element data in parallel.
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    \PCONN_start
 * \param[in]  end      \PCONN_end
 * \param[in]  elements \PCONN_Elements
 * \return \ier
 * \details cgp_elements_write_data() is used to write element connectivity data by multiple processes
 *          in a parallel fashion. To write the element data in parallel, first call cgp_section_write()
 *          to create an empty data node. This call is identical to cg_section_write() with \e Elements
 *          set to \e NULL (no data written). The actual element data is then written to the node in
 *          parallel using cgp_elements_write_data() where \e start and \e end specify the range of the
 *          elements to be written by a given process.
 * \note Routine only works for constant-sized elements since it is not possible to compute file offsets
 *       for variable sized elements without knowledge of the entire element connectivity data.
 * \note It is the responsibility of the application to ensure that \e cgsize_t in the application is the
 *       same size as that defined in the file; no conversions are done.
 */
int cgp_elements_write_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, const cgsize_t *elements)
{
    int elemsize;
    hid_t hid;
    cgns_section *section;
    cgsize_t rmin, rmax;
    CGNS_ENUMT(DataType_t) type;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0 || section->connect == 0) return CG_ERROR;

    if (elements) {
      if (start > end ||
          start < section->range[0] ||
          end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
      }
    }
    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    if (cg_npe(section->el_type, &elemsize)) return CG_ERROR;
    rmin = (start - section->range[0]) * elemsize + 1;
    rmax = (end - section->range[0] + 1) * elemsize;
    type = cgi_datatype(section->connect->data_type);

    to_HDF_ID(section->connect->id, hid);

    cg_rw_t Data;
    Data.u.wbuf = elements;
    return readwrite_data_parallel(hid, type,
			       1, &rmin, &rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Write element data in parallel.
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    \PCONN_start
 * \param[in]  end      \PCONN_end
 * \param[in]  elements \PCONN_Elements
 * \param[in]  offsets  \PCONN_Offsets
 * \return \ier
 */
int cgp_poly_elements_write_data(int fn, int B, int Z, int S, cgsize_t start,
                            cgsize_t end, const cgsize_t *elements, const cgsize_t *offsets)
{
  // Very experimental function
  // is offset the local or global offset ?
  // Should we had another argument global_offset in case offsets is local ?
  // The serial partial writing get the global offset from the file
  // so it is not necessary to provide it
  hid_t hid, hid_elem;
  cgns_section *section;
  cgsize_t rmin, rmax;
  cgsize_t rmin_elem, rmax_elem;
  CGNS_ENUMT(DataType_t) type, elem_type;
  cg_rw_t Data;
  cg_rw_t DataElem;
  int status;

  /* get file and check mode */
  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
    return CG_ERROR;

  section = cgi_get_section(cg, B, Z, S);
  if (section == 0 || section->connect == 0) return CG_ERROR;

  if (offsets)
  {
    if (start > end ||
        start < section->range[0] ||
        end > section->range[1])
    {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
    }
  }

  if (IS_FIXED_SIZE(section->el_type)) {
    cgi_error("element must not be a fixed size for this parallel IO");
    return CG_ERROR;
  }

  rmin = start - section->range[0] + 1;
  rmax = end - section->range[0] + 2;

  type = cgi_datatype(section->connect_offset->data_type);
  elem_type = cgi_datatype(section->connect->data_type);

  to_HDF_ID(section->connect_offset->id, hid);
  to_HDF_ID(section->connect->id, hid_elem);

  Data.u.wbuf = offsets;
  DataElem.u.wbuf = elements;

  if (offsets){
    rmin_elem = offsets[0] + 1;
    rmax_elem = offsets[end-start+1];
  }
  else
  {
    rmin_elem = 1;
    rmax_elem = 1;
    DataElem.u.wbuf = NULL;
  }

  status = readwrite_data_parallel(hid, type, 1, &rmin, &rmax, &Data, CG_PAR_WRITE);
  if (status != CG_OK)
    return status;
  return readwrite_data_parallel(hid_elem, elem_type, 1, &rmin_elem, &rmax_elem, &DataElem, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Read offsets data in parallel.
 *
 * \param[in]  fn      \FILE_fn
 * \param[in]  B       \B_Base
 * \param[in]  Z       \Z_Zone
 * \param[in]  S       \CONN_S
 * \param[in]  start   \PCONN_start
 * \param[in]  end     \PCONN_end
 * \param[out] offsets \PCONN_Offsets
 * \return \ier
 */
int cgp_poly_elements_read_data_offsets(int fn, int B, int Z, int S, cgsize_t start,
                            cgsize_t end, cgsize_t *offsets)
{
  CGNS_ENUMT(DataType_t) type;
  cgsize_t rmin, rmax;
  cg_rw_t Data;
  hid_t hid;
  cgns_section *section;

  /* get file and check mode */
  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
	  return CG_ERROR;

  section = cgi_get_section(cg, B, Z, S);
  if (section == 0) return CG_ERROR;

  if (IS_FIXED_SIZE(section->el_type)) {
    cgi_error("element must not be a fixed size for this parallel IO");
    return CG_ERROR;
  }

  if (section->connect == 0 || section->connect_offset == 0) return CG_ERROR;

  rmin = start - section->range[0] + 1;
  rmax = end - section->range[0] + 2;

  type = cgi_datatype(section->connect_offset->data_type);

  to_HDF_ID(section->connect_offset->id, hid);

  Data.u.rbuf = (void*)offsets;
  return readwrite_data_parallel(hid, type, 1, &rmin, &rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Read elements data in parallel.
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    \PCONN_start
 * \param[in]  end      \PCONN_end
 * \param[in]  offsets  \PCONN_Offsets
 * \param[out] elements \PCONN_Elements
 * \return \ier
 */
int cgp_poly_elements_read_data_elements(int fn, int B, int Z, int S, cgsize_t start,
					                  cgsize_t end, const cgsize_t *offsets, cgsize_t *elements)
{
  // Very experimental function
  // is offset the local or global offset ?
  // Should we had another argument global_offset in case offsets is local ?
  // The serial partial writing get the global offset from the file
  // so it is not necessary to provide it
  hid_t hid_elem;
  cgns_section *section;
  cgsize_t rmin_elem, rmax_elem;
  CGNS_ENUMT(DataType_t) elem_type;
  cg_rw_t DataElem;
  int status;

  /* get file and check mode */
  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
    return CG_ERROR;

  section = cgi_get_section(cg, B, Z, S);
  if (section == 0) return CG_ERROR;

  if (IS_FIXED_SIZE(section->el_type)) {
    cgi_error("element must not be a fixed size for this parallel IO");
    return CG_ERROR;
  }

  if (section->connect == 0 || section->connect_offset == 0) return CG_ERROR;

  elem_type = cgi_datatype(section->connect->data_type);

  to_HDF_ID(section->connect->id, hid_elem);

  DataElem.u.rbuf = (void*)elements;

  if (offsets){
    rmin_elem = offsets[0] + 1;
    rmax_elem = offsets[end-start+1];
  }
  else
  {
    rmin_elem = 1;
    rmax_elem = 1;
    DataElem.u.rbuf = NULL;
  }

  return readwrite_data_parallel(hid_elem, elem_type, 1, &rmin_elem, &rmax_elem, &DataElem, CG_PAR_READ);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Read element data in parallel.
 *
 * \param[in]  fn       \FILE_fn
 * \param[in]  B        \B_Base
 * \param[in]  Z        \Z_Zone
 * \param[in]  S        \CONN_S
 * \param[in]  start    \PCONN_start
 * \param[in]  end      \PCONN_end
 * \param[out] elements \PCONN_Elements
 * \return \ier
 */
int cgp_elements_read_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, cgsize_t *elements)
{
    int elemsize;
    hid_t hid;
    cgns_section *section;
    cgsize_t rmin, rmax;
    CGNS_ENUMT(DataType_t) type;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0 || section->connect == 0) return CG_ERROR;

    if (elements) { /* A processor may have nothing to read */
      if (start > end ||
          start < section->range[0] ||
          end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
      }
    }
    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    if (cg_npe(section->el_type, &elemsize)) return CG_ERROR;
    rmin = (start - section->range[0]) * elemsize + 1;
    rmax = (end - section->range[0] + 1) * elemsize;
    type = cgi_datatype(sizeof(cgsize_t) == 4 ? "I4" : "I8");

    to_HDF_ID(section->connect->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = elements;
    return readwrite_data_parallel(hid, type,
			      1, &rmin, &rmax, &Data, CG_PAR_READ);
}
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Write parent info for an element section data in parallel.
 *
 * \param[in]  fn          \FILE_fn
 * \param[in]  B           \B_Base
 * \param[in]  Z           \Z_Zone
 * \param[in]  S           \CONN_S
 * \param[in]  start       \PCONN_start
 * \param[in]  end         \PCONN_end
 * \param[in]  parent_data \PCONN_Elements
 * \return \ier
 */
int cgp_parent_data_write(int fn, int B, int Z, int S,
			  cgsize_t start, cgsize_t end,
			  const cgsize_t *parent_data)
{
    cgns_section *section;
    hid_t hid;
    cgsize_t rmin[2], rmax[2];
    CGNS_ENUMT(DataType_t) type;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (cg == 0) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
      return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    /* check input range */
    if (parent_data) {
      if (start > end ||
      start < section->range[0] ||
      end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
      }
    } else {
        start = end = 0;
    }

    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    /* ParentElements ... */
    if (section->parelem) {
        if (cg->mode == CG_MODE_WRITE) {
            cgi_error("ParentElements is already defined under Elements_t '%s'",
                   section->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(section->id, section->parelem->id))
            return CG_ERROR;
        cgi_free_array(section->parelem);
        memset(section->parelem, 0, sizeof(cgns_array));
    } else {
        section->parelem = CGNS_NEW(cgns_array, 1);
    }

    /* Get total size across all processors */
    cgsize_t num = end == 0 ? 0 : end - start + 1;
    num = num < 0 ? 0 : num;
    MPI_Datatype mpi_type = sizeof(cgsize_t) == 32 ? MPI_INT : MPI_LONG_LONG_INT;
    MPI_Allreduce(MPI_IN_PLACE, &num, 1, mpi_type, MPI_SUM, ctx_cgio.pcg_mpi_comm);

    strcpy(section->parelem->data_type, CG_SIZE_DATATYPE);
    section->parelem->data_dim = 2;
    section->parelem->dim_vals[0] = num;
    section->parelem->dim_vals[1] = 2;
    strcpy(section->parelem->name, "ParentElements");

    if (cgi_write_array(section->id, section->parelem)) return CG_ERROR;

    /* ParentElementsPosition ... */
    if (section->parface) {
        if (cg->mode==CG_MODE_WRITE) {
            cgi_error("ParentElementsPosition is already defined under Elements_t '%s'",
                   section->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(section->id, section->parface->id))
            return CG_ERROR;
        cgi_free_array(section->parface);
        memset(section->parface, 0, sizeof(cgns_array));
    } else {
        section->parface = CGNS_NEW(cgns_array, 1);
    }

    strcpy(section->parface->data_type, CG_SIZE_DATATYPE);
    section->parface->data_dim = 2;
    section->parface->dim_vals[0] = num;
    section->parface->dim_vals[1] = 2;
    strcpy(section->parface->name, "ParentElementsPosition");

    if (cgi_write_array(section->id, section->parface)) return CG_ERROR;

    /* ParentElements -- write data */
    rmin[0] = start - section->range[0] + 1;
    rmax[0] = end - section->range[0] + 1;
    rmin[1] = 1;
    rmax[1] = 2;
    type = cgi_datatype(section->parelem->data_type);

    cg_rw_t Data;
    Data.u.wbuf = parent_data;

    to_HDF_ID(section->parelem->id, hid);
    int herr = readwrite_data_parallel(hid, type, 2, rmin, rmax, &Data, CG_PAR_WRITE);
    if (herr != CG_OK)
      return herr;

    /* ParentElementsPosition -- data follows ParentElements data */
    type = cgi_datatype(section->parface->data_type);

    if (parent_data) {
      cgsize_t delta = rmax[0] - rmin[0] + 1;
      Data.u.wbuf = &parent_data[2*delta];
    }
    to_HDF_ID(section->parface->id, hid);
    return readwrite_data_parallel(hid, type, 2, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Read parent elements data in parallel.
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[in]  start          \PCONN_start
 * \param[in]  end            \PCONN_end
 * \param[out] parentelements \PCONN_Elements
 * \return \ier
 */
int cgp_parentelements_read_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, cgsize_t *parentelements)
{
    hid_t hid;
    cgns_section *section;
    cgsize_t rmin[2], rmax[2];
    CGNS_ENUMT(DataType_t) type;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
      return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (parentelements) { /* A processor may have nothing to read */
      if (start > end ||
          start < section->range[0] ||
          end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
      }
    } else {
      start = end = 0;
    }
    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    rmin[0] = start - section->range[0] + 1;
    rmax[0] = end - section->range[0] + 1;
    rmin[1] = 1;
    rmax[1] = 2;
    type = cgi_datatype(section->parelem->data_type);

    to_HDF_ID(section->parelem->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = parentelements;
    return readwrite_data_parallel(hid, type,
			      2, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ElementConnectivityData
 *
 * \brief Write parent elements data in parallel.
 *
 * \param[in]  fn             \FILE_fn
 * \param[in]  B              \B_Base
 * \param[in]  Z              \Z_Zone
 * \param[in]  S              \CONN_S
 * \param[in]  start          \PCONN_start
 * \param[in]  end            \PCONN_end
 * \param[in]  parentelements \PCONN_Elements
 * \return \ier
 */
int cgp_parentelements_write_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, cgsize_t *parentelements)
{
    hid_t hid;
    cgns_section *section;
    cgsize_t rmin[2], rmax[2];
    CGNS_ENUMT(DataType_t) type;

     /* get file and check mode */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
      return CG_ERROR;

    section = cgi_get_section(cg, B, Z, S);
    if (section == 0) return CG_ERROR;

    if (parentelements) { /* A processor may have nothing to read */
      if (start > end ||
          start < section->range[0] ||
          end > section->range[1]) {
        cgi_error("Error in requested element data range.");
        return CG_ERROR;
      }
    } else {
      start = end = 0;
    }
    if (!IS_FIXED_SIZE(section->el_type)) {
        cgi_error("element must be a fixed size for parallel IO");
        return CG_ERROR;
    }

    if (section->parelem) {
        if (cg->mode == CG_MODE_WRITE) {
            cgi_error("ParentElements is already defined under Elements_t '%s'",
                   section->name);
            return CG_ERROR;
        }
        if (cgi_delete_node(section->id, section->parelem->id))
            return CG_ERROR;
        cgi_free_array(section->parelem);
        memset(section->parelem, 0, sizeof(cgns_array));
    } else {
        section->parelem = CGNS_NEW(cgns_array, 1);
    }

    /* Get total size across all processors */
    cgsize_t num = end == 0 ? 0 : end - start + 1;
    num = num < 0 ? 0 : num;
    MPI_Datatype mpi_type = sizeof(cgsize_t) == 32 ? MPI_INT : MPI_LONG_LONG_INT;
    MPI_Allreduce(MPI_IN_PLACE, &num, 1, mpi_type, MPI_SUM, ctx_cgio.pcg_mpi_comm);

    strcpy(section->parelem->data_type, CG_SIZE_DATATYPE);
    section->parelem->data_dim = 2;
    section->parelem->dim_vals[0] = num;
    section->parelem->dim_vals[1] = 2;
    strcpy(section->parelem->name, "ParentElements");

    if (cgi_write_array(section->id, section->parelem)) return CG_ERROR;

    rmin[0] = start - section->range[0] + 1;
    rmax[0] = end - section->range[0] + 1;
    rmin[1] = 1;
    rmax[1] = 2;
    type = cgi_datatype(section->parelem->data_type);

    to_HDF_ID(section->parelem->id, hid);
    cg_rw_t Data;
    Data.u.wbuf = parentelements;
    return readwrite_data_parallel(hid, type,
			      2, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*===== Solution IO Prototypes ============================*/
/**
 * \ingroup SolutionData
 *
 * \brief Create a solution field data node in parallel.
 *
 * \param[in]  fn        \FILE_fn
 * \param[in]  B         \B_Base
 * \param[in]  Z         \Z_Zone
 * \param[in]  S         \PSOL_S
 * \param[in]  DataType  \PSOL_datatype
 * \param[in]  fieldname \PSOL_fieldname
 * \param[in]  F         \PSOL_F
 * \return \ier
 */
int cgp_field_write(int fn, int B, int Z, int S,
    CGNS_ENUMT(DataType_t) DataType, const char *fieldname, int *F)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_field_write(fn, B, Z, S, DataType, fieldname, NULL, F);
}

/*---------------------------------------------------------*/
/**
 * \ingroup SolutionData
 *
 * \brief Write field data in parallel.
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  S    \PSOL_S
 * \param[in]  F    \PSOL_F
 * \param[in]  rmin \PSOL_range_min
 * \param[in]  rmax \PSOL_range_max
 * \param[in]  data \PSOL_solution_array
 * \return \ier
 */
int cgp_field_write_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *data)
{
    int n;
    hid_t hid;
    cgns_array *field = NULL;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    field = cgi_get_field(cg, B, Z, S, F);
    if (field==0) return CG_ERROR;

     /* verify that range requested does not exceed range stored */
    if (data) {
      for (n = 0; n < field->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > field->dim_vals[n] ||
            rmin[n] < 1) {
          cgi_error("Invalid range of data requested");
          return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(field->data_type);

    to_HDF_ID(field->id,hid);

    cg_rw_t Data;
    Data.u.wbuf = data;
    return readwrite_data_parallel(hid, type,
			       field->data_dim, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/

/**
 * \ingroup SolutionData
 *
 * \brief Write shaped array to a subset of flow solution field in parallel.
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  S             \PSOL_S
 * \param[in]  F             \PSOL_F
 * \param[in]  rmin          \PSOL_range_min
 * \param[in]  rmax          \PSOL_range_max
 * \param[in]  m_type        \PSOL_mem_datatype
 * \param[in]  m_numdim      \PSOL_mem_rank
 * \param[in]  m_arg_dimvals \PSOL_mem_dimensions
 * \param[in]  m_rmin        \PSOL_mem_range_min
 * \param[in]  m_rmax        \PSOL_mem_range_max
 * \param[in]  data          \PSOL_solution_array
 * \return \ier
 * \details If \e data == NULL, meaning this processor reads no data, then
 *  only \e fn,\e  B, \e Z, \e S, and \e F need be set.  In this case, \e Z, \e S, and \e F are
 *  "representative" and can point to any valid zone.
 */
int cgp_field_general_write_data(int fn, int B, int Z, int S, int F,
                                 const cgsize_t *rmin, const cgsize_t *rmax,
                                 CGNS_ENUMT(DataType_t) m_type,
                                 int m_numdim, const cgsize_t *m_arg_dimvals,
                                 const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                 const void *data)
{
    int n, ier;
    hid_t hid;
    cgns_sol *sol;
    cgns_array *field = NULL;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* get memory address for field */
    field = cgi_get_field(cg, B, Z, S, F);
    if (field == 0) return CG_ERROR;

     /* get file-space rank.  Dimensions already set by previous null
        call to cgp_field_write */
    const int s_numdim = field->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = field->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_write_full_range; /* unused */
        int m_read_full_range;  /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Write, cgns_rindindex, sol->rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_write_full_range, &m_read_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(field->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, Z, F, and S arguments are needed to get hid */
    to_HDF_ID(field->id, hid);

    void* dataset[1];
    dataset[0] = (void*)data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup SolutionData
 *
 * \brief Read field data in parallel.
 *
 * \param[in]  fn   \FILE_fn
 * \param[in]  B    \B_Base
 * \param[in]  Z    \Z_Zone
 * \param[in]  S    \PSOL_S
 * \param[in]  F    \PSOL_F
 * \param[in]  rmin \PSOL_range_min
 * \param[in]  rmax \PSOL_range_max
 * \param[in]  data \PSOL_solution_array
 * \return \ier
 */
int cgp_field_read_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, void *data)
{
    int n;
    hid_t hid;
    cgns_array *field = NULL;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    field = cgi_get_field(cg, B, Z, S, F);
    if (field==0) return CG_ERROR;

     /* verify that range requested does not exceed range stored */
    if (data) {
      for (n = 0; n < field->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > field->dim_vals[n] ||
            rmin[n] < 1) {
	  cgi_error("Invalid range of data requested");
	  return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(field->data_type);

    to_HDF_ID(field->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = data;
    return readwrite_data_parallel(hid, type,
			      field->data_dim, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/

/**
 * \ingroup SolutionData
 *
 * \brief Read subset of flow solution field to a shaped array in parallel.
 *
 * \param[in]  fn            \FILE_fn
 * \param[in]  B             \B_Base
 * \param[in]  Z             \Z_Zone
 * \param[in]  S             \PSOL_S
 * \param[in]  F             \PSOL_F
 * \param[in]  rmin          \PSOL_range_min
 * \param[in]  rmax          \PSOL_range_max
 * \param[in]  m_type        \PSOL_mem_datatype
 * \param[in]  m_numdim      \PSOL_mem_rank
 * \param[in]  m_arg_dimvals \PSOL_mem_dimensions
 * \param[in]  m_rmin        \PSOL_mem_range_min
 * \param[in]  m_rmax        \PSOL_mem_range_max
 * \param[out] data          \PSOL_solution_array
 * \return \ier
 *
 * \details If \e data == NULL, meaning this processor reads no data, then
 *          only \e fn, \e B, \e Z, \e S, and \e F need be set.  In this case, \e Z, \e S, and \e F are
 *          "representative" and can point to any valid zone.
 */
int cgp_field_general_read_data(int fn, int B, int Z, int S, int F,
                                const cgsize_t *rmin, const cgsize_t *rmax,
                                CGNS_ENUMT(DataType_t) m_type,
                                int m_numdim, const cgsize_t *m_arg_dimvals,
                                const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                void *data)
{
    int n, ier;
    hid_t hid;
    cgns_sol *sol;
    cgns_array *field = NULL;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_sol(cg, B, Z, S);
    if (sol == 0) return CG_ERROR;

     /* get memory address for field */
    field = cgi_get_field(cg, B, Z, S, F);
    if (field == 0) return CG_ERROR;

     /* get file-space rank */
    const int s_numdim = field->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = field->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_read_full_range;  /* unused */
        int m_write_full_range; /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Read, cgns_rindindex, sol->rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_read_full_range, &m_write_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(field->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, Z, F, and S arguments are needed to get hid */
    to_HDF_ID(field->id, hid);

    void* dataset[1];
    dataset[0] = data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_READ);
}

/*===== Particle IO Prototypes ================================*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Create a coordinate data node by multiple processes in a parallel fashion.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] datatype \PGRID_datatype
 * \param[in] coordname \PGRID_coordname
 * \param[out] C \PGRID_Coordinate
 * \return \ier
 * \details To write the data in parallel, first call \e cgp_coord_write to create an
 *          empty data node. This call is identical to \e cg_particle_coord_write with
 *          \p coord_array set to NULL (no data written). The actual data is then written
 *          to the node in parallel using either \e cgp_particle_coord_write_data or
 *          \e cgp_particle_coord_general_write_data where \p range_min and \p range_max
 *          specify the subset of coordinate data to be written by a given process.
 */

int cgp_particle_coord_write(int fn, int B, int P, CGNS_ENUMT(DataType_t) datatype,
    const char *coordname, int *C)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_particle_coord_write(fn, B, P, datatype, coordname, NULL, C);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Write particle coordinate data in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] C \C_Coordinate
 * \param[in] rmin \PGRID_range_min
 * \param[in] rmax \PGRID_range_max
 * \param[in] coords \PGRID_coord_array
 * \return \ier
 *
 * \details Writes the actual data to the node in parallel, where \p rmin and \p rmax specify the subset
 *          of coordinate data to be written by a given process. It is the
 *          responsibility of the application to ensure that the data type for the coordinate data
 *          matches that as defined in the file; no conversions are done.
 */

int cgp_particle_coord_write_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *coords)
{
    cgns_pzone *pzone;
    cgns_pcoor *pcoor;
    cgsize_t dims[1];
    hid_t hid;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) return CG_ERROR;

    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor==0) return CG_ERROR;

    if (C > pcoor->ncoords || C <= 0) {
        cgi_error("Particle coord number %d invalid",C);
        return CG_ERROR;
    }

    dims[0] = pzone->nparticles;

       if(coords) {
          if (rmin[0] > rmax[0] || rmin[0] < 1 || rmax[0] > dims[0]) {
             printf("%d %d %d", rmin[0]> rmax[0], rmin[0] <1, rmax[0] >dims[0]);
             cgi_error("Invalid index ranges. cgp_coord_write_data");
             return CG_ERROR;
          }
       }

    type = cgi_datatype(pcoor->coord[C-1].data_type);

    to_HDF_ID(pcoor->coord[C-1].id,hid);

    cg_rw_t Data;
    Data.u.wbuf = coords;
    return readwrite_data_parallel(hid, type, 1, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Write shaped array to a subset of grid coordinates in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] C \PGRID_Coordinate
 * \param[in] rmin \PGRID_range_min
 * \param[in] rmax \PGRID_range_max
 * \param[in] m_type \PGRID_mem_datatype
 * \param[in] m_numdim \PGRID_mem_rank
 * \param[in] m_arg_dimvals \PGRID_mem_dimensions
 * \param[in] m_rmin \PGRID_mem_range_min
 * \param[in] m_rmax \PGRID_mem_range_max
 * \param[out] coords \PGRID_coord_array
 * \return \ier
 *
 * \details The \e cgp_particle_coord_general_write_data perform data conversions
 *          if \e datatype is different from \e mem_datatype. If \e coords == NULL, meaning
 *          this processor writes no data, then only \e fn, \e B, \e P, and \e C need be set.
 *          In this case, \e P and \e C are "representative" and can point to any valid zone.
 */

int cgp_particle_coord_general_write_data(int fn, int B, int P, int C,
                                 const cgsize_t *rmin, const cgsize_t *rmax,
                                 CGNS_ENUMT(DataType_t) m_type,
                                 int m_numdim, const cgsize_t *m_arg_dimvals,
                                 const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                 const void *coords)
{
    int n, ier;
    hid_t hid;
    cgns_pzone *pzone;
    cgns_pcoor *pcoor;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone == 0) return CG_ERROR;

     /* Get memory address for node "ParticleCoordinates" */
    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor == 0) return CG_ERROR;

    if (C > pcoor->ncoords || C <= 0) {
        cgi_error("particle coord number %d invalid",C);
        return CG_ERROR;
    }

     /* get file-space rank.  Dimensions already set by previous null
      * call to cgp_coord_write*/
    const int s_numdim = 1;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (coords) {
       cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
       s_dimvals[0] = pcoor->coord[C-1].dim_vals[0];
       m_dimvals[0] = m_arg_dimvals[0];
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_write_full_range; /* unused */
        int m_read_full_range;  /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Write, cgns_rindindex, NULL,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_write_full_range, &m_read_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(pcoor->coord[C-1].data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, P, and C arguments are needed to get hid */
    to_HDF_ID(pcoor->coord[C-1].id, hid);

    void* dataset[1];
    dataset[0] = (void*)coords;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Read coordinate data in parallel.
 *
 * \param[in]  fn \FILE_fn
 * \param[in]  B \B_Base
 * \param[in]  P \P_ParticleZone
 * \param[in]  C \C_Coordinate
 * \param[in]  rmin \PGRID_range_min
 * \param[in]  rmax \PGRID_range_max
 * \param[out] coords \PGRID_coord_array
 * \return \ier
 *
 * \details Reads the actual data to the node in parallel, where \p rmin and \p rmax specify the subset
 *          of coordinate data to be read by a given process. It is the
 *          responsibility of the application to ensure that the data type for the coordinate data
 *          matches that as defined in the file; no conversions are done.
 */

int cgp_particle_coord_read_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, void *coords)
{
    hid_t hid;
    cgns_pzone *pzone;
    cgns_pcoor *pcoor;
    cgsize_t dims;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) return CG_ERROR;

    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor==0) return CG_ERROR;

    if (C > pcoor->ncoords || C <= 0) {
        cgi_error("particle coord number %d invalid",C);
        return CG_ERROR;
    }

    dims = pzone->nparticles;

    if(coords) {
       if (rmin[0] > rmax[0] || rmin[0] < 1 || rmax[0] > dims) {
          cgi_error("Invalid index ranges.");
          return CG_ERROR;
       }
    }
    type = cgi_datatype(pcoor->coord[C-1].data_type);

    to_HDF_ID(pcoor->coord[C-1].id,hid);
    cg_rw_t Data;
    Data.u.rbuf = coords;
    return readwrite_data_parallel(hid, type, 1, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Read shaped array to a subset of grid coordinates in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] C \C_Coordinate
 * \param[in] rmin \PGRID_range_min
 * \param[in] rmax \PGRID_range_max
 * \param[in] m_type \PGRID_mem_datatype
 * \param[in] m_numdim \PGRID_mem_rank
 * \param[in] m_arg_dimvals \PGRID_mem_dimensions
 * \param[in] m_rmin \PGRID_mem_range_min
 * \param[in] m_rmax \PGRID_mem_range_max
 * \param[out] coords \PGRID_coord_array
 * \return \ier
 *
 * \details The \e cgp_particle_coord_general_read_data perform data conversions if \e datatype
 *          is different from \e mem_datatype. If \e coords == NULL, meaning
 *          this processor reads no data, then only \e fn, \e B, \e P, and \e C need be set.
 *          In this case, \e P and \e C are "representative" and can point to any valid zone.
 */

int cgp_particle_coord_general_read_data(int fn, int B, int P, int C,
                                         const cgsize_t *rmin, const cgsize_t *rmax,
                                         CGNS_ENUMT(DataType_t) m_type,
                                         int m_numdim, const cgsize_t *m_arg_dimvals,
                                         const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                         void *coords)
{
    int ier;
    hid_t hid;
    cgns_pzone *pzone;
    cgns_pcoor *pcoor;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

    pzone = cgi_get_particle(cg, B, P);
    if (pzone == 0) return CG_ERROR;

     /* Get memory address for node "ParticleCoordinates" */
    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor == 0) return CG_ERROR;

    if (C > pcoor->ncoords || C <= 0) {
        cgi_error("particle coord number %d invalid",C);
        return CG_ERROR;
    }

     /* get file-space dimensions.  Dimensions already set by previous null
      * call to cgp_particle_coord_write*/
    const int s_numdim = 1;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (coords) {
       cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
       s_dimvals[0] = pcoor->coord[C-1].dim_vals[0];
       m_dimvals[0] = m_arg_dimvals[0];
       /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
       int s_read_full_range;  /* unused */
       int m_write_full_range; /* unused */
       cgsize_t numpt;         /* unused */
       ier = cgi_array_general_verify_range(
                CGI_Read, cgns_rindindex, NULL,
                s_numdim, s_dimvals, rmin, rmax,
                m_numdim, m_dimvals, m_rmin, m_rmax,
                s_rmin, s_rmax, stride,
                &s_read_full_range, &m_write_full_range, &numpt);
       if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(pcoor->coord[C-1].data_type);
        m_numdim = s_numdim;
        m_dimvals[0] = 0;
    }

     /* fn, B, P, and C arguments are needed to get hid */
    to_HDF_ID(pcoor->coord[C-1].id, hid);

    void* dataset[1];
    dataset[0] = coords;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_READ);
}

/*===== Particle Solution IO Prototypes ============================*/
/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Create a particle solution field data node in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] S \PSOL_S
 * \param[in] datatype \PSOL_datatype
 * \param[in] fieldname \PSOL_fieldname
 * \param[in] F \PSOL_F
 * \return \ier
 */
int cgp_particle_field_write(int fn, int B, int P, int S,
    CGNS_ENUMT(DataType_t) datatype, const char *fieldname, int *F)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_particle_field_write(fn, B, P, S, datatype, fieldname, NULL, F);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Write field data in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] S \PSOL_S
 * \param[in] F \PSOL_F
 * \param[in] rmin \PSOL_range_min
 * \param[in] rmax \PSOL_range_max
 * \param[in] data \PSOL_solution_array
 * \return \ier
 */
int cgp_particle_field_write_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *data)
{
    int n;
    hid_t hid;
    cgns_array *field = NULL;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    field = cgi_get_particle_field(cg, B, P, S, F);
    if (field==0) return CG_ERROR;

     /* verify that range requested does not exceed range stored */
    if (data) {
      for (n = 0; n < field->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > field->dim_vals[n] ||
            rmin[n] < 1) {
          cgi_error("Invalid range of data requested");
          return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(field->data_type);

    to_HDF_ID(field->id,hid);

    cg_rw_t Data;
    Data.u.wbuf = data;
    return readwrite_data_parallel(hid, type,
                field->data_dim, rmin, rmax, &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/

/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Write shaped array to a subset of particle solution field in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] S \PSOL_S
 * \param[in] F \PSOL_F
 * \param[in] rmin \PSOL_range_min
 * \param[in] rmax \PSOL_range_max
 * \param[in] m_type \PSOL_mem_datatype
 * \param[in] m_numdim \PSOL_mem_rank
 * \param[in] m_arg_dimvals \PSOL_mem_dimensions
 * \param[in] m_rmin \PSOL_mem_range_min
 * \param[in] m_rmax \PSOL_mem_range_max
 * \param[in] data \PSOL_solution_array
 * \return \ier
 *
 * \details If \e data == NULL, meaning this processor reads no data, then
 *          only \e fn,\e  B, \e P, \e S, and \e F need be set.  In this case, \e P, \e S, and \e F are
 *          "representative" and can point to any valid zone.
 */
int cgp_particle_field_general_write_data(int fn, int B, int P, int S, int F,
                                          const cgsize_t *rmin, const cgsize_t *rmax,
                                          CGNS_ENUMT(DataType_t) m_type,
                                          int m_numdim, const cgsize_t *m_arg_dimvals,
                                          const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                          const void *data)
{
    int n, ier;
    hid_t hid;
    cgns_psol *sol;
    cgns_array *field = NULL;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE)) return CG_ERROR;

     /* get memory address for solution */
    sol = cgi_get_particle_sol(cg, B, P, S);
    if (sol == 0) return CG_ERROR;

     /* get memory address for field */
    field = cgi_get_particle_field(cg, B, P, S, F);
    if (field == 0) return CG_ERROR;

     /* get file-space rank.  Dimensions already set by previous null
        call to cgp_field_write */
    const int s_numdim = field->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = field->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_write_full_range; /* unused */
        int m_read_full_range;  /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Write, cgns_rindindex, NULL,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_write_full_range, &m_read_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(field->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, P, F, and S arguments are needed to get hid */
    to_HDF_ID(field->id, hid);

    void* dataset[1];
    dataset[0] = (void*)data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Read particle field data in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] S \PSOL_S
 * \param[in] F \PSOL_F
 * \param[in] rmin \PSOL_range_min
 * \param[in] rmax \PSOL_range_max
 * \param[in] data \PSOL_solution_array
 * \return \ier
 */
int cgp_particle_field_read_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, void *data)
{
    int n;
    hid_t hid;
    cgns_array *field = NULL;
    CGNS_ENUMT(DataType_t) type;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
        return CG_ERROR;

    field = cgi_get_particle_field(cg, B, P, S, F);
    if (field==0) return CG_ERROR;

     /* verify that range requested does not exceed range stored */
    if (data) {
      for (n = 0; n < field->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > field->dim_vals[n] ||
            rmin[n] < 1) {
     cgi_error("Invalid range of data requested");
     return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(field->data_type);

    to_HDF_ID(field->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = data;
    return readwrite_data_parallel(hid, type,
               field->data_dim, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/

/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Read subset of particle solution field to a shaped array in parallel.
 *
 * \param[in] fn \FILE_fn
 * \param[in] B \B_Base
 * \param[in] P \P_ParticleZone
 * \param[in] S \PSOL_S
 * \param[in] F \PSOL_F
 * \param[in] rmin \PSOL_range_min
 * \param[in] rmax \PSOL_range_max
 * \param[in] m_type \PSOL_mem_datatype
 * \param[in] m_numdim \PSOL_mem_rank
 * \param[in] m_arg_dimvals \PSOL_mem_dimensions
 * \param[in] m_rmin \PSOL_mem_range_min
 * \param[in] m_rmax \PSOL_mem_range_max
 * \param[out] data \PSOL_solution_array
 * \return \ier
 *
 * \details If \e data == NULL, meaning this processor reads no data, then
 *          only \e fn, \e B, \e P, \e S, and \e F need be set.  In this case, \e P, \e S, and \e F are
 *          "representative" and can point to any valid zone.
 */
int cgp_particle_field_general_read_data(int fn, int B, int P, int S, int F,
                                         const cgsize_t *rmin, const cgsize_t *rmax,
                                         CGNS_ENUMT(DataType_t) m_type,
                                         int m_numdim, const cgsize_t *m_arg_dimvals,
                                         const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                         void *data)
{
    int n, ier;
    hid_t hid;
    cgns_psol *sol;
    cgns_array *field = NULL;

     /* get memory addresses */
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ)) return CG_ERROR;

     /* get memory address for particle solution */
    sol = cgi_get_particle_sol(cg, B, P, S);
    if (sol == 0) return CG_ERROR;

     /* get memory address for particle field */
    field = cgi_get_particle_field(cg, B, P, S, F);
    if (field == 0) return CG_ERROR;

     /* get file-space rank */
    const int s_numdim = field->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = field->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }
         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_read_full_range;  /* unused */
        int m_write_full_range; /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Read, cgns_rindindex, NULL,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_read_full_range, &m_write_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(field->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* fn, B, P, F, and S arguments are needed to get hid */
    to_HDF_ID(field->id, hid);

    void* dataset[1];
    dataset[0] = data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_READ);
}

/*===== Array IO Prototypes ===============================*/
/**
 * \ingroup ArrayData
 *
 * \brief Create an array data node.
 *
 * \param[in]  ArrayName       \PARR_arrayname
 * \param[in]  DataType        \PARR_datatype
 * \param[in]  DataDimension   \PARR_rank
 * \param[in]  DimensionVector \PARR_dimensions
 * \param[in]  A               \PARR_A
 * \return \ier
 */
int cgp_array_write(const char *ArrayName, CGNS_ENUMT(DataType_t) DataType,
    int DataDimension, const cgsize_t *DimensionVector, int *A)
{
    int ierr, na, n;
    cgns_array *array;

   if (posit == NULL) {
        cgi_error("No current position set by cg_goto");
        return CG_ERROR;
    }
    if (check_parallel(cg)) return CG_ERROR;

    ierr = cg_array_write(ArrayName, DataType, DataDimension,
                          DimensionVector, NULL);
    if (ierr) return ierr;
    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, 1, "dummy", &have_dup, &ierr);
    if (array == NULL) return ierr;
    ierr = cg_narrays(&na);
    if (ierr) return ierr;
    for (n = 0; n < na; n++) {
        if (0 == strcmp(ArrayName, array->name)) {
            *A = n + 1;
            return CG_OK;
        }
        array++;
    }
    *A = 0;
    cgi_error("array %s not found", ArrayName);
    return CG_ERROR;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ArrayData
 *
 * \brief Write array data in parallel.
 *
 * \param[in]  A    \PARR_A
 * \param[in]  rmin \PARR_range_min
 * \param[in]  rmax \PARR_range_max
 * \param[in]  data \PARR_data
 * \return \ier
 */
int cgp_array_write_data(int A, const cgsize_t *rmin,
    const cgsize_t *rmax, const void *data)
{
    int n, ierr = 0;
    hid_t hid;
    cgns_array *array;
    CGNS_ENUMT(DataType_t) type;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ierr);
    if (array == NULL) return ierr;

    if (data) {
      for (n = 0; n < array->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > array->dim_vals[n] ||
            rmin[n] < 1) {
	  cgi_error("Invalid range of data requested");
	  return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(array->data_type);

    to_HDF_ID(array->id, hid);

    cg_rw_t Data;
    Data.u.wbuf = data;
    return readwrite_data_parallel(hid, type,
			       array->data_dim, rmin, rmax,  &Data, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/

/**
 * \ingroup ArrayData
 *
 * \brief Write shaped array to a subset of data array in parallel.
 *
 * \param[in]  A             \PARR_A
 * \param[in]  rmin          \PARR_range_min
 * \param[in]  rmax          \PARR_range_max
 * \param[in]  m_type        \PARR_mem_datatype
 * \param[in]  m_numdim      \PARR_mem_rank
 * \param[in]  m_arg_dimvals \PARR_mem_dimensions
 * \param[in]  m_rmin        \PARR_mem_range_min
 * \param[in]  m_rmax        \PARR_mem_range_max
 * \param[out] data          \PARR_data
 * \return \ier
 *
 * \details If \e data == NULL, meaning this processor reads no data, then
 *          only \e A need be set.  In this case, \e A is "representative" and can point to
 *          any valid array being written by another processor
 *
 */
int cgp_array_general_write_data(int A,
                                 const cgsize_t *rmin, const cgsize_t *rmax,
                                 CGNS_ENUMT(DataType_t) m_type,
                                 int m_numdim, const cgsize_t *m_arg_dimvals,
                                 const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                 const void *data)
{
    int n, ier;
    hid_t hid;
    cgns_array *array;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array == 0) return ier;

     /* get file-space rank.  Dimensions already set by previous null
        call to cgp_array_write */
    const int s_numdim = array->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = array->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }

         /* do we have rind planes? */
        int *rind_planes = cgi_rind_address(CG_MODE_READ, &ier);
        if (ier != CG_OK) rind_planes = NULL;

         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_write_full_range; /* unused */
        int m_read_full_range;  /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Write, cgns_rindindex, rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_write_full_range, &m_read_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(array->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* A argument is needed to get hid */
    to_HDF_ID(array->id, hid);

    void* dataset[1];
    dataset[0] = (void*)data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/
/**
 * \ingroup ArrayData
 *
 * \brief Read array data in parallel.
 *
 * \param[in]  A    \PARR_A
 * \param[in]  rmin \PARR_range_min
 * \param[in]  rmax \PARR_range_max
 * \param[in]  data \PARR_data
 * \return \ier
 */
int cgp_array_read_data(int A, const cgsize_t *rmin,
    const cgsize_t *rmax, void *data)
{
    int n, ierr = 0;
    hid_t hid;
    cgns_array *array;
    CGNS_ENUMT(DataType_t) type;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ierr);
    if (array == NULL) return ierr;

    if (data) {
      for (n = 0; n < array->data_dim; n++) {
        if (rmin[n] > rmax[n] ||
            rmax[n] > array->dim_vals[n] ||
            rmin[n] < 1) {
	  cgi_error("Invalid range of data requested");
	  return CG_ERROR;
        }
      }
    }
    type = cgi_datatype(array->data_type);

    to_HDF_ID(array->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = data;
    return readwrite_data_parallel(hid, type,
				   array->data_dim, rmin, rmax, &Data, CG_PAR_READ);
}

/*---------------------------------------------------------*/

/**
 * \ingroup ArrayData
 *
 * \brief Read subset of data array to a shaped array in parallel.
 *
 * \param[in]  A             \PARR_A
 * \param[in]  rmin          \PARR_range_min
 * \param[in]  rmax          \PARR_range_max
 * \param[in]  m_type        \PARR_mem_datatype
 * \param[in]  m_numdim      \PARR_mem_rank
 * \param[in]  m_arg_dimvals \PARR_mem_dimensions
 * \param[in]  m_rmin        \PARR_mem_range_min
 * \param[in]  m_rmax        \PARR_mem_range_max
 * \param[out] data          \PARR_data
 * \return \ier
 *
 * \details If \e data == NULL, meaning this processor reads no data, then
 *          only \e A need be set.  In this case, \e A is "representative" and can point to
 *          any valid array being written by another processor.
 */
int cgp_array_general_read_data(int A,
                                const cgsize_t *rmin, const cgsize_t *rmax,
                                CGNS_ENUMT(DataType_t) m_type,
                                int m_numdim, const cgsize_t *m_arg_dimvals,
                                const cgsize_t *m_rmin, const cgsize_t *m_rmax,
                                void *data)
{
    int n, ier;
    hid_t hid;
    cgns_array *array;

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A, "dummy", &have_dup, &ier);
    if (array == 0) return ier;

     /* get file-space rank.  Dimensions already set by previous null
        call to cgp_array_write */
    const int s_numdim = array->data_dim;

     /* we may modify m_arg_dimvals but do not want to change user assignments
        so m_arg_dimvals will be copied */
    cgsize_t m_dimvals[CGIO_MAX_DIMENSIONS];
    cgsize_t s_rmin[CGIO_MAX_DIMENSIONS], s_rmax[CGIO_MAX_DIMENSIONS];
    cgsize_t stride[CGIO_MAX_DIMENSIONS];
    if (data) {
        cgsize_t s_dimvals[CGIO_MAX_DIMENSIONS];
        for (n=0; n<s_numdim; n++) {
            s_dimvals[n] = array->dim_vals[n];
        }
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = m_arg_dimvals[n];
        }

         /* do we have rind planes? */
        int *rind_planes = cgi_rind_address(CG_MODE_READ, &ier);
        if (ier != CG_OK) rind_planes = NULL;

         /* verify the ranges provided and set s_rmin and s_rmax giving internal
            file-space ranges */
        int s_read_full_range;  /* unused */
        int m_write_full_range; /* unused */
        cgsize_t numpt;         /* unused */
        ier = cgi_array_general_verify_range(
            CGI_Read, cgns_rindindex, rind_planes,
            s_numdim, s_dimvals, rmin, rmax,
            m_numdim, m_dimvals, m_rmin, m_rmax,
            s_rmin, s_rmax, stride,
            &s_read_full_range, &m_write_full_range, &numpt);
        if (ier != CG_OK) return ier;
    }
    else {
         /* Note: all data unused except m_type, m_numdim and m_dimvals. */
         /* If null data, set memory rank to same as file and memory dimensions
            to 0 */
        m_type = cgi_datatype(array->data_type);
        m_numdim = s_numdim;
        for (n=0; n<m_numdim; n++) {
            m_dimvals[n] = 0;
        }
    }

     /* A argument is needed to get hid */
    to_HDF_ID(array->id, hid);

    void* dataset[1];
    dataset[0] = data;

    return readwrite_shaped_data_parallel(
        hid,
        s_rmin, s_rmax, stride,
        m_type, m_numdim, m_dimvals, m_rmin, m_rmax, stride,
        dataset, CG_PAR_READ);
}

/********************************
  Multidataset APIs
*********************************/

static int readwrite_multi_data_parallel(size_t count, hid_t *dset_id, hid_t *mem_type_id,
                                         hid_t *mem_space_id, hid_t *file_space_id,
                                         cg_rw_ptr_t *data, int ndims, const cgsize_t *rmin,
                                         const cgsize_t *rmax, enum cg_par_rw rw_mode)
{
  /*
   *  Needs to handle a NULL dataset. MSB
   */
    int k, n;
    hsize_t *start, *dims;
    herr_t herr;
    hid_t plist_id;

    start = malloc(count*sizeof(hsize_t));
    dims = malloc(count*sizeof(hsize_t));

    /* convert from CGNS to HDF5 data type */
    for (n = 0; n < count; n++) {
      switch ((CGNS_ENUMT(DataType_t))mem_type_id[n]) {
      case CGNS_ENUMV(Character):
        mem_type_id[n] = H5T_NATIVE_CHAR;
	break;
      case CGNS_ENUMV(Integer):
        mem_type_id[n] = H5T_NATIVE_INT32;
	break;
      case CGNS_ENUMV(LongInteger):
        mem_type_id[n] = H5T_NATIVE_INT64;
	break;
      case CGNS_ENUMV(RealSingle):
        mem_type_id[n] = H5T_NATIVE_FLOAT;
	break;
      case CGNS_ENUMV(RealDouble):
        mem_type_id[n] = H5T_NATIVE_DOUBLE;
	break;
      default:
        cgi_error("unhandled data type %d\n", mem_type_id[n]);
	free(start);
	free(dims);
	return CG_ERROR;
      }
    }

    /* Set the start position and size for the data write */
    /* fix dimensions due to Fortran indexing and ordering */
    for (k = 0; k < ndims; k++) {
        start[k] = rmin[ndims-k-1] - 1;
        dims[k] = rmax[ndims-k-1] - start[k];
    }

    for (k = 0; k < count; k++) {
	/* Create a shape for the data in memory */
        mem_space_id[k] = H5Screate_simple(ndims, dims, NULL);
        if (mem_space_id[k] < 0) {
	  cgi_error("H5Screate_simple() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Open the data */
        if ((dset_id[k] = H5Dopen2(dset_id[k], " data", H5P_DEFAULT)) < 0) {
          H5Sclose(mem_space_id[k]); /** needs loop **/
	  cgi_error("H5Dopen2() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Create a shape for the data in the file */
        file_space_id[k] = H5Dget_space(dset_id[k]);
        if (file_space_id[k] < 0) {
          H5Sclose(mem_space_id[k]);
          H5Dclose(dset_id[k]);
	  cgi_error("H5Dget_space() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Select a section of the array in the file */
        herr = H5Sselect_hyperslab(file_space_id[k], H5S_SELECT_SET, start,
				   NULL, dims, NULL);
	if (herr < 0) {
          H5Sclose(mem_space_id[k]);
          H5Dclose(dset_id[k]);
	  cgi_error("H5Sselect_hyperslab() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}
    }

    /* Set the access property list for data transfer */
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    if (plist_id < 0) {
        cgi_error("H5Pcreate() failed");
	free(start);
	free(dims);
        return CG_ERROR;
    }

    /* Set MPI-IO independent or collective communication */
    herr = H5Pset_dxpl_mpio(plist_id, ctx_cgio.default_pio_mode);
    if (herr < 0) {
        H5Pclose(plist_id);
        cgi_error("H5Pset_dxpl_mpio() failed");
	free(start);
	free(dims);
        return CG_ERROR;
    }

    /* If HDF5 does not support multi-dataset APIs, then resort to doing them one-by-one */
#if HDF5_HAVE_MULTI_DATASETS
    /* Read or Write the data in parallel */
    if (rw_mode == CG_PAR_READ) {
      herr = H5Dread_multi(count, dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, data[0].u.rbuf);
      if (herr < 0) {
        cgi_error("H5Dread_multi() failed");
      }
    } else {
      herr = H5Dwrite_multi(count, dset_id, mem_type_id, mem_space_id, file_space_id, plist_id, data[0].u.wbuf);
      if (herr < 0) {
        cgi_error("H5Dwrite_multi() failed");
      }
    }
#else
    for (k = 0; k < count; k++) {
      if (rw_mode == CG_PAR_READ) {
        herr = H5Dread(dset_id[k], mem_type_id[k], mem_space_id[k], file_space_id[k], plist_id, data[0].u.rbuf[k]);
        if (herr < 0) {
          cgi_error("H5Dread_multi() -- pseudo -- failed");
        }
      } else {
        herr = H5Dwrite(dset_id[k], mem_type_id[k], mem_space_id[k], file_space_id[k], plist_id, data[0].u.wbuf[k]);
        if (herr < 0) {
          cgi_error("H5Dwrite_multi() -- pseudo --  failed");
        }
      }
    }
#endif

    H5Pclose(plist_id);
    free(start);
    free(dims);
    return herr < 0 ? CG_ERROR : CG_OK;
}

/*------------------- multi-dataset functions --------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Read multiple sets of coordinate data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[in]  C     \C_Coordinate_multi
 * \param[in]  rmin  \PGRID_range_min
 * \param[in]  rmax  \PGRID_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[out] buf   \PGRID_coord_array
 * \return \ier
 *
 * \details  Reads the actual  coordinate data to the node in parallel, where
 *           \c rmin and \c rmax specify the subset of coordinate data
 *           to be read by a given process. The application is responsible
 *           for ensuring that the coordinate data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dread_multi` to read \c nsets
 *           of coordinate data, whose identifiers are listed in the
 *           \c C array, from the CGNS file into multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the reads into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 *
 */
int cgp_coord_multi_read_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                              int nsets, void *buf[])
{
    int n;
    hid_t hid;
    cgns_zone *zone = NULL;
    cgns_zcoor *zcoor = NULL;
    cgsize_t dims[3];

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
      goto error;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) goto error;

    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) goto error;

    for (n = 0;  n < nsets; n++) {
      if (C[n] > zcoor->ncoords || C[n] <= 0) {
        cgi_error("coord number %d invalid",C[n]);
        goto error;
      }
    }

    for (n = 0; n < zone->index_dim; n++) {
      dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
        zcoor->rind_planes[2*n+1];
      if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
        cgi_error("Invalid index ranges.");
        goto error;
      }
    }

    for (n = 0; n < nsets; n++) {
      mem_type_id[n] = cgi_datatype(zcoor->coord[C[n]-1].data_type);
      to_HDF_ID(zcoor->coord[C[n]-1].id, hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.rbuf = buf;
    status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
                                         zone->index_dim, rmin, rmax, CG_PAR_READ);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
    free(file_space_id);

  return CG_ERROR;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelGridCoordinate
 *
 * \brief Writes multiple sets of coordinate data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[in]  C     \C_Coordinate_multi
 * \param[in]  rmin  \PGRID_range_min
 * \param[in]  rmax  \PGRID_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[in]  buf   \PGRID_coord_array
 * \return \ier
 *
 * \details  Writes the actual coordinate data to the node in parallel, where
 *           \c rmin and \c rmax specify the subset of coordinate data
 *           to be written by a given process. The application is responsible
 *           for ensuring that the coordinate data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dwrite_multi` to write \c nsets
 *           of coordinate data, whose identifiers are listed in the
 *           \c C array, to the CGNS file from the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 *
 */
int cgp_coord_multi_write_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                               int nsets, const void *buf[])
{
    int n;
    hid_t hid;
    cgns_zone *zone = NULL;
    cgns_zcoor *zcoor = NULL;
    cgsize_t dims[3];

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) goto error;

    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) goto error;

    for (n = 0;  n < nsets; n++) {
      if (C[n] > zcoor->ncoords || C[n] <= 0) {
        cgi_error("coord number %d invalid",C[n]);
        goto error;
      }
    }

    for (n = 0; n < zone->index_dim; n++) {
        dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                  zcoor->rind_planes[2*n+1];
        if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
            cgi_error("Invalid index ranges.");
            goto error;
        }
    }

    for (n = 0; n < nsets; n++) {
      mem_type_id[n] = cgi_datatype(zcoor->coord[C[n]-1].data_type);
      to_HDF_ID(zcoor->coord[C[n]-1].id, hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.wbuf = buf;
    status =  readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
                                            zone->index_dim, rmin, rmax, CG_PAR_WRITE);

    return status;

 error:
    if(dset_id)
      free(dset_id);
    if(mem_type_id)
      free(mem_type_id);
    if(mem_space_id)
      free(mem_space_id);
    if(file_space_id)
      free(file_space_id);

    return CG_ERROR;

}

/*---------------------------------------------------------*/
/**
 * \ingroup SolutionData
 *
 * \brief Writes multiple sets of field data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[in]  S     \PSOL_S
 * \param[in]  F     \PSOL_F_multi
 * \param[in]  rmin  \PSOL_range_min
 * \param[in]  rmax  \PSOL_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[in]  buf   \PARR_data_multi_write
 * \return \ier
 *
 * \details  Writes the actual field data from the node in parallel, where
 *           \c rmin and \c rmax specify the subset of field data
 *           to be written by a given process. The application is responsible
 *           for ensuring that the field data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dwrite_multi` to write \c nsets
 *           of field data, whose identifiers are listed in the
 *           \c F array, to the CGNS file from the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_field_multi_write_data(int fn, int B, int Z, int S, int *F,
                               const cgsize_t *rmin, const cgsize_t *rmax, int nsets, const void *buf[])

{
    int n, m;
    hid_t hid;
    cgns_array *field = NULL;

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    for (n = 0; n < nsets; n++) {
      field = cgi_get_field(cg, B, Z, S, F[n]);
      if (field==0) goto error;

      /* verify that range requested does not exceed range stored */
      for (m = 0; m < field->data_dim; m++) {
        if (rmin[m] > rmax[m] ||
            rmax[m] > field->dim_vals[m] ||
            rmin[m] < 1) {
	  cgi_error("Invalid range of data requested");
	  goto error;
        }
      }

      mem_type_id[n] = cgi_datatype(field->data_type);
      to_HDF_ID(field->id,hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.wbuf = buf;
    status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
					   field->data_dim, rmin, rmax, CG_PAR_WRITE);

    free(dset_id);
    free(mem_type_id);
    free(mem_space_id);
    free(file_space_id);

    return status;

 error:
    if(dset_id)
      free(dset_id);
    if(mem_type_id)
      free(mem_type_id);
    if(mem_space_id)
      free(mem_space_id);
    if(file_space_id)
      free(file_space_id);

    return CG_ERROR;
}

/*---------------------------------------------------------*/
/**
 * \ingroup SolutionData
 *
 * \brief Reads multiple sets of field data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  Z     \Z_Zone
 * \param[in]  S     \PSOL_S
 * \param[in]  F     \PSOL_F_multi
 * \param[in]  rmin  \PSOL_range_min
 * \param[in]  rmax  \PSOL_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[out] buf   \PARR_data_multi_read
 * \return \ier
 *
 * \details  Reads the actual field data from the node in parallel, where
 *           \c rmin and \c rmax specify the subset of field data
 *           to be read by a given process. The application is responsible
 *           for ensuring that the field data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dread_multi` to read \c nsets
 *           of field data, whose identifiers are listed in the
 *           \c F array, from the CGNS file into the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the reads into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_field_multi_read_data(int fn, int B, int Z, int S, int *F,
    const cgsize_t *rmin, const cgsize_t *rmax, int nsets, void *buf[])
{
  int n, m;
  hid_t hid;
  cgns_array *field = NULL;

  hid_t *dset_id;
  hid_t *mem_type_id;
  hid_t *mem_space_id;
  hid_t *file_space_id;

  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
    return CG_ERROR;

  dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

  for (n = 0; n < nsets; n++) {

    field = cgi_get_field(cg, B, Z, S, F[n]);
    if (field==0) goto error;

    /* verify that range requested does not exceed range stored */
    for (m = 0; m < field->data_dim; m++) {
      if (rmin[m] > rmax[m] ||
	  rmax[m] > field->dim_vals[m] ||
	  rmin[m] < 1) {
	cgi_error("Invalid range of data requested");
	goto error;
      }
    }

    mem_type_id[n] = cgi_datatype(field->data_type);
    to_HDF_ID(field->id,hid);
    dset_id[n] = hid;
  }

  cg_rw_ptr_t Data;
  Data.u.rbuf = buf;
  status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
					 field->data_dim, rmin, rmax, CG_PAR_READ);

  free(dset_id);
  free(mem_type_id);
  free(mem_space_id);
  free(file_space_id);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
      free(file_space_id);

  return CG_ERROR;
}

/*---------------------------------------------------------*/
 /**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Read multiple sets of coordinate data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  P     \P_ParticleZone
 * \param[in]  C     \C_Coordinate_multi
 * \param[in]  rmin  \PGRID_range_min
 * \param[in]  rmax  \PGRID_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[out] buf   \PGRID_coord_array
 * \return \ier
 *
 * \details  Reads the actual particle coordinate data to the node in parallel, where
 *           \c rmin and \c rmax specify the subset of coordinate data
 *           to be read by a given process. The application is responsible
 *           for ensuring that the coordinate data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dread_multi` to read \c nsets
 *           of coordinate data, whose identifiers are listed in the
 *           \c C array, from the CGNS file into multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the reads into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 *
 */
int cgp_particle_coord_multi_read_data(int fn, int B, int P, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                       int nsets, void *buf[])
{
    int n;
    hid_t hid;
    cgns_pzone *pzone = NULL;
    cgns_pcoor *pcoor = NULL;
    cgsize_t dims;

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
      goto error;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) goto error;

    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor==0) goto error;

    for (n = 0;  n < nsets; n++) {
      if (C[n] > pcoor->ncoords || C[n] <= 0) {
        cgi_error("particle coord number %d invalid",C[n]);
        goto error;
      }
    }

    dims = pzone->nparticles;

    if (rmin[0] > rmax[0] || rmin[0] < 1 || rmax[0] > dims) {
       cgi_error("Invalid index ranges.");
       goto error;
    }


    for (n = 0; n < nsets; n++) {
      mem_type_id[n] = cgi_datatype(pcoor->coord[C[n]-1].data_type);
      to_HDF_ID(pcoor->coord[C[n]-1].id, hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.rbuf = buf;
    status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
                                           1, rmin, rmax, CG_PAR_READ);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
    free(file_space_id);

  return CG_ERROR;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleCoordinate
 *
 * \brief Writes multiple sets of coordinate data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  P     \P_ParticleZone
 * \param[in]  C     \C_Coordinate_multi
 * \param[in]  rmin  \PGRID_range_min
 * \param[in]  rmax  \PGRID_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[in]  buf   \PGRID_coord_array
 * \return \ier
 *
 * \details  Writes the actual particles coordinate data to the node in parallel, where
 *           \c rmin and \c rmax specify the subset of coordinate data
 *           to be written by a given process. The application is responsible
 *           for ensuring that the coordinate data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dwrite_multi` to write \c nsets
 *           of coordinate data, whose identifiers are listed in the
 *           \c C array, to the CGNS file from the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 *
 */
int cgp_particle_coord_multi_write_data(int fn, int B, int P, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                        int nsets, const void *buf[])
{
    int n;
    hid_t hid;
    cgns_pzone *pzone = NULL;
    cgns_pcoor *pcoor = NULL;
    cgsize_t dims;

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    pzone = cgi_get_particle(cg, B, P);
    if (pzone==0) goto error;

    pcoor = cgi_get_particle_pcoorPC(cg, B, P);
    if (pcoor==0) goto error;

    for (n = 0;  n < nsets; n++) {
      if (C[n] > pcoor->ncoords || C[n] <= 0) {
        cgi_error("particle coord number %d invalid",C[n]);
        goto error;
      }
    }

    dims = pzone->nparticles;
    if (rmin[0] > rmax[0] || rmin[0] < 1 || rmax[0] > dims) {
       cgi_error("Invalid index ranges.");
       goto error;
    }

    for (n = 0; n < nsets; n++) {
      mem_type_id[n] = cgi_datatype(pcoor->coord[C[n]-1].data_type);
      to_HDF_ID(pcoor->coord[C[n]-1].id, hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.wbuf = buf;
    status =  readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
                                            1, rmin, rmax, CG_PAR_WRITE);

    return status;

 error:
    if(dset_id)
      free(dset_id);
    if(mem_type_id)
      free(mem_type_id);
    if(mem_space_id)
      free(mem_space_id);
    if(file_space_id)
      free(file_space_id);

    return CG_ERROR;

}

/*---------------------------------------------------------*/
/**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Writes multiple sets of particle solution field data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  P     \P_ParticleZone
 * \param[in]  S     \PSOL_S
 * \param[in]  F     \PSOL_F_multi
 * \param[in]  rmin  \PSOL_range_min
 * \param[in]  rmax  \PSOL_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[in]  buf   \PARR_data_multi_write
 * \return \ier
 *
 * \details  Writes the actual field data from the node in parallel, where
 *           \c rmin and \c rmax specify the subset of field data
 *           to be written by a given process. The application is responsible
 *           for ensuring that the field data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dwrite_multi` to write \c nsets
 *           of field data, whose identifiers are listed in the
 *           \c F array, to the CGNS file from the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_particle_field_multi_write_data(int fn, int B, int P, int S, int *F,
                                        const cgsize_t *rmin, const cgsize_t *rmax,
                                        int nsets, const void *buf[])

{
    int n, m;
    hid_t hid;
    cgns_array *field = NULL;

    hid_t *dset_id = NULL;
    hid_t *mem_type_id = NULL;
    hid_t *mem_space_id = NULL;
    hid_t *file_space_id = NULL;

    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
    file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

    for (n = 0; n < nsets; n++) {
      field = cgi_get_particle_field(cg, B, P, S, F[n]);
      if (field==0) goto error;

      /* verify that range requested does not exceed range stored */
      for (m = 0; m < field->data_dim; m++) {
        if (rmin[m] > rmax[m] ||
            rmax[m] > field->dim_vals[m] ||
            rmin[m] < 1) {
     cgi_error("Invalid range of data requested");
     goto error;
        }
      }

      mem_type_id[n] = cgi_datatype(field->data_type);
      to_HDF_ID(field->id,hid);
      dset_id[n] = hid;
    }

    cg_rw_ptr_t Data;
    Data.u.wbuf = buf;
    status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
					   field->data_dim, rmin, rmax, CG_PAR_WRITE);

    free(dset_id);
    free(mem_type_id);
    free(mem_space_id);
    free(file_space_id);

    return status;

 error:
    if(dset_id)
      free(dset_id);
    if(mem_type_id)
      free(mem_type_id);
    if(mem_space_id)
      free(mem_space_id);
    if(file_space_id)
      free(file_space_id);

    return CG_ERROR;
}

/*---------------------------------------------------------*/
 /**
 * \ingroup ParallelParticleSolutionData
 *
 * \brief Reads multiple sets of particle solution field data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  B     \B_Base
 * \param[in]  P     \P_ParticleZone
 * \param[in]  S     \PSOL_S
 * \param[in]  F     \PSOL_F_multi
 * \param[in]  rmin  \PSOL_range_min
 * \param[in]  rmax  \PSOL_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[out] buf   \PARR_data_multi_read
 * \return \ier
 *
 * \details  Reads the actual field data from the node in parallel, where
 *           \c rmin and \c rmax specify the subset of field data
 *           to be read by a given process. The application is responsible
 *           for ensuring that the field data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dread_multi` to read \c nsets
 *           of field data, whose identifiers are listed in the
 *           \c F array, from the CGNS file into the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the reads into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_particle_field_multi_read_data(int fn, int B, int P, int S, int *F,
    const cgsize_t *rmin, const cgsize_t *rmax, int nsets, void *buf[])
{
  int n, m;
  hid_t hid;
  cgns_array *field = NULL;

  hid_t *dset_id;
  hid_t *mem_type_id;
  hid_t *mem_space_id;
  hid_t *file_space_id;

  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
    return CG_ERROR;

  dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

  for (n = 0; n < nsets; n++) {

    field = cgi_get_particle_field(cg, B, P, S, F[n]);
    if (field==0) goto error;

    /* verify that range requested does not exceed range stored */
    for (m = 0; m < field->data_dim; m++) {
      if (rmin[m] > rmax[m] ||
     rmax[m] > field->dim_vals[m] ||
     rmin[m] < 1) {
   cgi_error("Invalid range of data requested");
   goto error;
      }
    }

    mem_type_id[n] = cgi_datatype(field->data_type);
    to_HDF_ID(field->id,hid);
    dset_id[n] = hid;
  }

  cg_rw_ptr_t Data;
  Data.u.rbuf = buf;
  status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
					 field->data_dim, rmin, rmax, CG_PAR_READ);

  free(dset_id);
  free(mem_type_id);
  free(mem_space_id);
  free(file_space_id);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
      free(file_space_id);

  return CG_ERROR;
}

/*---------------------------------------------------------*/
/**
 * \ingroup ArrayData
 *
 * \brief Writes multiple sets of array data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  A     \PARR_A_multi
 * \param[in]  rmin  \PARR_range_min
 * \param[in]  rmax  \PARR_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[in]  buf   \PARR_data_multi_write
 * \return \ier
 *
 * \details  Writes the actual array data to the node in parallel, where
 *           \c rmin and \c rmax specify the subset of array data
 *           to be written by a given process. The application is responsible
 *           for ensuring that the array data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dwrite_multi` to write \c nsets
 *           of array data, whose identifiers are listed in the
 *           \c F array, to the CGNS file from the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_array_multi_write_data(int fn, int *A, const cgsize_t *rmin,
                               const cgsize_t *rmax, int nsets, const void *buf[])
{
  int n, m, ierr = 0;
  hid_t hid;
  cgns_array *array = NULL;

  hid_t *dset_id;
  hid_t *mem_type_id;
  hid_t *mem_space_id;
  hid_t *file_space_id;

  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

  for (n = 0; n < nsets; n++) {

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A[n], "dummy", &have_dup, &ierr);
    if (array == NULL) goto error;

    for (m = 0; m < array->data_dim; m++) {
      if (rmin[m] > rmax[m] ||
	  rmax[m] > array->dim_vals[m] ||
	  rmin[m] < 1) {
	cgi_error("Invalid range of data requested");
	goto error;
      }
    }

    mem_type_id[n] = cgi_datatype(array->data_type);
    to_HDF_ID(array->id, hid);
    dset_id[n] = hid;
  }

  cg_rw_ptr_t Data;
  Data.u.wbuf = buf;
  status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
               array->data_dim, rmin, rmax, CG_PAR_WRITE);

  free(dset_id);
  free(mem_type_id);
  free(mem_space_id);
  free(file_space_id);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
    free(file_space_id);

  return CG_ERROR;
}

/*---------------------------------------------------------*/

/**
 * \ingroup ArrayData
 *
 * \brief Reads multiple sets of array data in parallel.
 *
 * \param[in]  fn    \FILE_fn
 * \param[in]  A     \PARR_A_multi
 * \param[in]  rmin  \PARR_range_min
 * \param[in]  rmax  \PARR_range_max
 * \param[in]  nsets \PARR_nsets_multi
 * \param[out] buf   \PARR_data_multi_read
 * \return \ier
 *
 * \details  Reads the actual array data from the node in parallel, where
 *           \c rmin and \c rmax specify the subset of array data
 *           to be read by a given process. The application is responsible
 *           for ensuring that the array data type matches what is
 *           defined in the file; no conversions are made.
 *
 *           Uses HDF5's multidataset API `H5Dread_multi` to read \c nsets
 *           of array data, whose identifiers are listed in the
 *           \c F array, from the CGNS file into the multiple application memory
 *           buffers listed in the \c buf array. All array parameters have
 *           length \c nsets. The HDF5 library will combine the writes into
 *           larger I/O requests, which usually results in better parallel
 *           I/O performance.
 */
int cgp_array_multi_read_data(int fn, int *A, const cgsize_t *rmin,
                              const cgsize_t *rmax, int nsets, void *buf[])
{
  int n, m, ierr = 0;
  hid_t hid;
  cgns_array *array = NULL;

  hid_t *dset_id;
  hid_t *mem_type_id;
  hid_t *mem_space_id;
  hid_t *file_space_id;

  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;


  dset_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_type_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  mem_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));
  file_space_id = (hid_t *)malloc(nsets*sizeof(hid_t));

  for (n = 0; n < nsets; n++) {

    int have_dup = 0;
    array = cgi_array_address(CG_MODE_READ, 0, A[n], "dummy", &have_dup, &ierr);
    if (array == NULL) goto error;

    for (m = 0; m < array->data_dim; m++) {
      if (rmin[m] > rmax[m] ||
	  rmax[m] > array->dim_vals[m] ||
	  rmin[m] < 1) {
	cgi_error("Invalid range of data requested");
	goto error;
      }
    }

    mem_type_id[n] = cgi_datatype(array->data_type);
    to_HDF_ID(array->id, hid);
    dset_id[n] = hid;
  }

  cg_rw_ptr_t Data;
  Data.u.rbuf = buf;
  status = readwrite_multi_data_parallel(nsets, dset_id, mem_type_id, mem_space_id, file_space_id, &Data,
               array->data_dim, rmin, rmax, CG_PAR_READ);

  free(dset_id);
  free(mem_type_id);
  free(mem_space_id);
  free(file_space_id);

  return status;

 error:
  if(dset_id)
    free(dset_id);
  if(mem_type_id)
    free(mem_type_id);
  if(mem_space_id)
    free(mem_space_id);
  if(file_space_id)
    free(file_space_id);

  return CG_ERROR;
}

/*===== PointList Functions =============================*/

/**
 * \ingroup PointListData
 *
 * \brief Write index array to PointList in parallel.
 *
 * \param[in]  file_number \FILE_fn
 * \param[in]  rmin        Lower range index in file
 * \param[in]  rmax        Upper range index in file
 * \param[in]  points      Array of points
 * \return \ier
 *
 * \details Functions in <a href="./c_api.html#accessing-a-node">Accessing a Node</a>
 *          must be used to point to a PointSet for writing.
 *
 */
int cgp_ptlist_write_data(int file_number, cgsize_t rmin,
    cgsize_t rmax, const cgsize_t *points)
{
  hid_t hid;
  cgns_ptset *ptset;
  cgsize_t range_min[2], range_max[2];
  CGNS_ENUMT(DataType_t) type;

    /* get memory address of file */
  cg = cgi_get_file(file_number);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
      return CG_ERROR;

  if (posit == 0) {
    cgi_error("No current position set by cg_goto\n");
    return CG_ERROR;
  }
  else if (strcmp(posit->label, "IndexArray_t") == 0) {
    ptset = (cgns_ptset *) posit->posit;
  } else {
    cgi_error("Goto not pointing to IndexArray_t, but %s\n", posit->label);
    return CG_ERROR;
  }

  if (points) {
    if (rmin > rmax ||
        rmin < 1 ||
        rmax > ptset->npts) {
      cgi_error("Error in requested point set range.");
      return CG_ERROR;
    }
  }

  range_min[0] = 1;
  range_max[0] = 1;
  range_min[1] = rmin;
  range_max[1] = rmax;
  type = cgi_datatype(ptset->data_type);

  to_HDF_ID(ptset->id, hid);

  cg_rw_t Data;
  Data.u.wbuf = points;
  return readwrite_data_parallel(hid, type,
            2, range_min, range_max, &Data, CG_PAR_WRITE);
}

/**
 * \ingroup PointListData
 *
 * \brief Read index array to PointList in parallel.
 *
 * \param[in]  file_number \FILE_fn
 * \param[in]  rmin        Lower range index in file
 * \param[in]  rmax        Upper range index in file
 * \param[in]  points      Array of points
 * \return \ier
 *
 * \details Functions in <a href="./c_api.html#accessing-a-node">Accessing a Node</a>
 *          must be used to point to a PointSet for reading.
 */
int cgp_ptlist_read_data(int file_number, cgsize_t rmin, cgsize_t rmax, cgsize_t *points)
{
  hid_t hid;
  cgns_ptset *ptset;
  cgsize_t range_min[2], range_max[2];
  CGNS_ENUMT(DataType_t) type;

    /* get memory address of file */
  cg = cgi_get_file(file_number);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
      return CG_ERROR;

  if (posit == 0) {
    cgi_error("No current position set by cg_goto\n");
    return CG_ERROR;
  }
  else if (strcmp(posit->label, "IndexArray_t") == 0) {
    ptset = (cgns_ptset *) posit->posit;
  } else {
    cgi_error("Goto not pointing to IndexArray_t, but %s\n", posit->label);
    return CG_ERROR;
  }

  if (points) {
    if (rmin > rmax ||
        rmin < 1 ||
        rmax > ptset->npts) {
      cgi_error("Error in requested point set range.");
      return CG_ERROR;
    }
  }

  range_min[0] = 1;
  range_max[0] = 1;
  range_min[1] = rmin;
  range_max[1] = rmax;
  type = cgi_datatype(ptset->data_type);

  to_HDF_ID(ptset->id, hid);

  cg_rw_t Data;
  Data.u.rbuf = points;
  return readwrite_data_parallel(hid, type,
            2, range_min, range_max, &Data, CG_PAR_READ);
}
/*---------------------------------------------------------*/
