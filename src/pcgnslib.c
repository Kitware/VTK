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

/* MPI-2 info object */
extern MPI_Info pcg_mpi_info;
extern MPI_Comm pcg_mpi_comm;
extern int pcg_mpi_comm_size;
extern int pcg_mpi_comm_rank;
/* Flag indicating if HDF5 file accesses is PARALLEL or NATIVE */
extern char hdf5_access[64];
/* flag indicating if mpi_initialized was called */
extern int pcg_mpi_initialized;

hid_t default_pio_mode = H5FD_MPIO_COLLECTIVE;

extern int cgns_filetype;
extern void* cgns_rindindex;

typedef struct cg_rw_t {
  union {
    void *rbuf;             /* Pointer to buffer for read */
    const void *wbuf;       /* Pointer to buffer to write */
  } u;
} cg_rw_t;

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
  herr = H5Pset_dxpl_mpio(plist_id, default_pio_mode);
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
    herr = H5Pset_dxpl_mpio(plist_id, default_pio_mode);
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

int cgp_mpi_comm(MPI_Comm comm)
{
    /* check if we are actually running a parallel program */
    /* Flag is true if MPI_Init or MPI_Init_thread has been called and false otherwise. */
    pcg_mpi_initialized = 0;
    MPI_Initialized(&pcg_mpi_initialized);

    if (pcg_mpi_initialized) {
      if( cgio_configure(CG_CONFIG_HDF5_MPI_COMM, &comm) != -1) {
	return CG_ERROR;
      }

      pcg_mpi_comm=comm;
      MPI_Comm_rank(pcg_mpi_comm, &pcg_mpi_comm_rank);
      MPI_Comm_size(pcg_mpi_comm, &pcg_mpi_comm_size);
    }

    return pcg_mpi_initialized ? CG_OK : CG_ERROR;
}

int cgp_mpi_info(MPI_Info info)
{
    pcg_mpi_info = info;

    return CG_OK;
}

/*---------------------------------------------------------*/

int cgp_pio_mode(CGNS_ENUMT(PIOmode_t) mode)
{
    if (mode == CGP_INDEPENDENT)
        default_pio_mode = H5FD_MPIO_INDEPENDENT;
    else if (mode == CGP_COLLECTIVE)
        default_pio_mode = H5FD_MPIO_COLLECTIVE;
    else {
        cgi_error("unknown parallel IO mode");
        return CG_ERROR;
    }

    return CG_OK;
}


/*---------------------------------------------------------*/

void cgp_error_exit(void)
{
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "[process %d]:%s\n", rank, cg_get_error());
    cgio_cleanup();
    MPI_Abort(MPI_COMM_WORLD, 1);
}

/*===== File IO Prototypes ================================*/

int cgp_open(const char *filename, int mode, int *fn)
{
    int ierr, old_type = cgns_filetype;

    /* Initialize communicators if cgp_mpi_comm() was not called by
       client */
    if (pcg_mpi_comm == MPI_COMM_NULL) {
      cgp_mpi_comm(MPI_COMM_WORLD);
    }

    /* Flag this as a parallel access */
    strcpy(hdf5_access,"PARALLEL");	

    ierr = cg_set_file_type(CG_FILE_HDF5);
    if (ierr) return ierr;
    ierr = cg_open(filename, mode, fn);
    cgns_filetype = old_type;

    /* reset parallel access */
    strcpy(hdf5_access,"NATIVE");

    return ierr;
}

/*---------------------------------------------------------*/

int cgp_close(int fn)
{
    return cg_close(fn);
}

/*===== Grid IO Prototypes ================================*/

int cgp_coord_write(int fn, int B, int Z, CGNS_ENUMT(DataType_t) type,
    const char *coordname, int *C)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_coord_write(fn, B, Z, type, coordname, NULL, C);
}

/*---------------------------------------------------------*/

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
            cgi_error("Invalid index ranges.");
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

/* Note: if data == NULL, meaning this processor reads no data, then
   only fn, B, Z, and C need be set.  In this case, Z and C are "representative"
   and can point to any valid zone */
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

/* Note: if data == NULL, meaning this processor reads no data, then
   only fn, B, Z, and C need be set.  In this case, Z and C are "representative"
   and can point to any valid zone */
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

/*---------------------------------------------------------*/

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
    type = cgi_datatype(section->connect->data_type);

    to_HDF_ID(section->connect->id, hid);
    cg_rw_t Data;
    Data.u.rbuf = elements;
    return readwrite_data_parallel(hid, type,
			      1, &rmin, &rmax, &Data, CG_PAR_READ);
}

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
    MPI_Allreduce(MPI_IN_PLACE, &num, 1, mpi_type, MPI_SUM, pcg_mpi_comm);

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

/*===== Solution IO Prototypes ============================*/

int cgp_field_write(int fn, int B, int Z, int S,
    CGNS_ENUMT(DataType_t) DataType, const char *fieldname, int *F)
{
    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    return cg_field_write(fn, B, Z, S, DataType, fieldname, NULL, F);
}

/*---------------------------------------------------------*/

int cgp_field_write_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *data)
{
    int n;
    hid_t hid;
    cgns_array *field;
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

/* Note: if data == NULL, meaning this processor reads no data, then
   only fn, B, Z, S, and F need be set.  In this case, Z, S, and F are
   "representative" and can point to any valid zone */
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
    cgns_array *field;

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

int cgp_field_read_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, void *data)
{
    int n;
    hid_t hid;
    cgns_array *field;
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

/* Note: if data == NULL, meaning this processor reads no data, then
   only fn, B, Z, S, and F need be set.  In this case, Z, S, and F are
   "representative" and can point to any valid zone */
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
    cgns_array *field;

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

/*===== Array IO Prototypes ===============================*/

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

/* Note: if data == NULL, meaning this processor reads no data, then
   only A need be set.  In this case, A is "representative" and can point to
   any valid array being written by another processor */
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

/* Note: if data == NULL, meaning this processor reads no data, then
   only A need be set.  In this case, A is "representative" and can point to
   any valid array being written by another processor */
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


#if HDF5_HAVE_MULTI_DATASETS

static int readwrite_multi_data_parallel(size_t count, H5D_rw_multi_t *multi_info,
					 int ndims, const cgsize_t *rmin, const cgsize_t *rmax, enum cg_par_rw rw_mode)
{
  /*
   *  Needs to handle a NULL dataset. MSB
   */
    int k, n;
    hid_t data_id, mem_shape_id, data_shape_id, hid;
    hsize_t *start, *dims;
    herr_t herr;
    hid_t plist_id;

    start = malloc(count*sizeof(hsize_t));
    dims = malloc(count*sizeof(hsize_t));

    /* convert from CGNS to HDF5 data type */
    for (n = 0; n < count; n++) {
      switch ((CGNS_ENUMT(DataType_t))multi_info[n].mem_type_id) {
      case CGNS_ENUMV(Character):
	multi_info[n].mem_type_id = H5T_NATIVE_CHAR;
	break;
      case CGNS_ENUMV(Integer):
	multi_info[n].mem_type_id = H5T_NATIVE_INT32;
	break;
      case CGNS_ENUMV(LongInteger):
	multi_info[n].mem_type_id = H5T_NATIVE_INT64;
	break;
      case CGNS_ENUMV(RealSingle):
	multi_info[n].mem_type_id = H5T_NATIVE_FLOAT;
	break;
      case CGNS_ENUMV(RealDouble):
	multi_info[n].mem_type_id = H5T_NATIVE_DOUBLE;
	break;
      default:
	cgi_error("unhandled data type %d\n", multi_info[n].mem_type_id);
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
	multi_info[k].mem_space_id = H5Screate_simple(ndims, dims, NULL);
	if (multi_info[k].mem_space_id < 0) {
	  cgi_error("H5Screate_simple() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Open the data */
	if ((multi_info[k].dset_id = H5Dopen2(multi_info[k].dset_id, " data", H5P_DEFAULT)) < 0) {
	  H5Sclose(multi_info[k].mem_space_id); /** needs loop **/
	  cgi_error("H5Dopen2() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Create a shape for the data in the file */
	multi_info[k].dset_space_id = H5Dget_space(multi_info[k].dset_id);
	if (multi_info[k].dset_space_id < 0) {
	  H5Sclose(multi_info[k].mem_space_id);
	  H5Dclose(multi_info[k].dset_id);
	  cgi_error("H5Dget_space() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}

	/* Select a section of the array in the file */
	herr = H5Sselect_hyperslab(multi_info[k].dset_space_id, H5S_SELECT_SET, start,
				   NULL, dims, NULL);
	if (herr < 0) {
	  H5Sclose(data_shape_id);
	  H5Sclose(mem_shape_id);
	  H5Dclose(data_id);
	  cgi_error("H5Sselect_hyperslab() failed");
	  free(start);
	  free(dims);
	  return CG_ERROR;
	}
    }

    /* Set the access property list for data transfer */
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    if (plist_id < 0) {
        H5Sclose(data_shape_id);
        H5Sclose(mem_shape_id);
        H5Dclose(data_id);
        cgi_error("H5Pcreate() failed");
	free(start);
	free(dims);
        return CG_ERROR;
    }

    /* Set MPI-IO independent or collective communication */
    herr = H5Pset_dxpl_mpio(plist_id, default_pio_mode);
    if (herr < 0) {
        H5Pclose(plist_id);
        H5Sclose(data_shape_id);
        H5Sclose(mem_shape_id);
        H5Dclose(data_id);
        cgi_error("H5Pset_dxpl_mpio() failed");
	free(start);
	free(dims);
        return CG_ERROR;
    }

    /* Read or Write the data in parallel */
    if (rw_mode == CG_PAR_READ) {
      herr = H5Dread_multi(plist_id, count, multi_info);
      if (herr < 0) {
        cgi_error("H5Dread_multi() failed");
      }
    } else {
      herr = H5Dwrite_multi(plist_id, count, multi_info);
      if (herr < 0) {
        cgi_error("H5Dwrite_multi() failed");
      }
    }
    H5Pclose(plist_id);
    H5Sclose(data_shape_id);
    H5Sclose(mem_shape_id);
    H5Dclose(data_id);
    free(start);
    free(dims);
    return herr < 0 ? CG_ERROR : CG_OK;
}

/*------------------- multi-dataset functions --------------------------------------*/

int cgp_coord_multi_read_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
			       void *coordsX,  void *coordsY,  void *coordsZ)
{
  int n;
  hid_t hid;
  cgns_zone *zone;
  cgns_zcoor *zcoor;
  cgsize_t dims[3];
  cgsize_t index_dim;
  CGNS_ENUMT(DataType_t) type[3];
  H5D_rw_multi_t multi_info[3];

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
    return CG_ERROR;

  zone = cgi_get_zone(cg, B, Z);
  if (zone==0) return CG_ERROR;

  zcoor = cgi_get_zcoorGC(cg, B, Z);
  if (zcoor==0) return CG_ERROR;

  for (n = 0;  n < 3; n++) {
    if (C[n] > zcoor->ncoords || C[n] <= 0) {
      cgi_error("coord number %d invalid",C[n]);
      return CG_ERROR;
    }
  }

  for (n = 0; n < zone->index_dim; n++) {
    dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
      zcoor->rind_planes[2*n+1];
    if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
      cgi_error("Invalid index ranges.");
      return CG_ERROR;
    }
  }

  for (n = 0; n < 3; n++) {
    multi_info[n].mem_type_id = cgi_datatype(zcoor->coord[C[n]-1].data_type);
    to_HDF_ID(zcoor->coord[C[n]-1].id, hid);
    multi_info[n].dset_id = hid;
  }

  multi_info[0].u.rbuf = coordsX;
  multi_info[1].u.rbuf = coordsY;
  multi_info[2].u.rbuf = coordsZ;

  return readwrite_multi_data_parallel(3, multi_info,
					 zone->index_dim, rmin, rmax, CG_PAR_READ);
}

/*---------------------------------------------------------*/

int cgp_coord_multi_write_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
			       const void *coordsX, const void *coordsY, const void *coordsZ)
{
    int n;
    cgns_zone *zone;
    cgns_zcoor *zcoor;
    cgsize_t dims[3];
    cgsize_t index_dim;
    CGNS_ENUMT(DataType_t) type[3];
    H5D_rw_multi_t multi_info[3];
    hid_t hid;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    zone = cgi_get_zone(cg, B, Z);
    if (zone==0) return CG_ERROR;

    zcoor = cgi_get_zcoorGC(cg, B, Z);
    if (zcoor==0) return CG_ERROR;

    for (n = 0;  n < 3; n++) {
      if (C[n] > zcoor->ncoords || C[n] <= 0) {
        cgi_error("coord number %d invalid",C[n]);
        return CG_ERROR;
      }
    }

    for (n = 0; n < zone->index_dim; n++) {
        dims[n] = zone->nijk[n] + zcoor->rind_planes[2*n] +
                                  zcoor->rind_planes[2*n+1];
        if (rmin[n] > rmax[n] || rmin[n] < 1 || rmax[n] > dims[n]) {
            cgi_error("Invalid index ranges.");
            return CG_ERROR;
        }
    }

    for (n = 0; n < 3; n++) {
      multi_info[n].mem_type_id = cgi_datatype(zcoor->coord[C[n]-1].data_type);
      to_HDF_ID(zcoor->coord[C[n]-1].id, hid);
      multi_info[n].dset_id = hid;
    }

    multi_info[0].u.wbuf = coordsX;
    multi_info[1].u.wbuf = coordsY;
    multi_info[2].u.wbuf = coordsZ;

    return readwrite_multi_data_parallel(3, multi_info,
					 zone->index_dim, rmin, rmax, CG_PAR_WRITE);
}

/*---------------------------------------------------------*/

int vcgp_field_multi_write_data(int fn, int B, int Z, int S, int *F,
			       const cgsize_t *rmin, const cgsize_t *rmax, int nsets, va_list ap)

{
    int n, m;
    hid_t hid;
    cgns_array *field;
    CGNS_ENUMT(DataType_t) type;
    H5D_rw_multi_t *multi_info;
    int status;

    cg = cgi_get_file(fn);
    if (check_parallel(cg)) return CG_ERROR;

    if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_WRITE))
        return CG_ERROR;

    multi_info = (H5D_rw_multi_t *)malloc(nsets*sizeof(H5D_rw_multi_t));

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

      multi_info[n].u.wbuf = va_arg(ap, const void *);

      multi_info[n].mem_type_id = cgi_datatype(field->data_type);
      to_HDF_ID(field->id,hid);
      multi_info[n].dset_id = hid;
    }

    status = readwrite_multi_data_parallel(nsets, multi_info,
					   field->data_dim, rmin, rmax, CG_PAR_WRITE);

    free(multi_info);

    return status;

 error:
    if(multi_info)
      free(multi_info);

    return CG_ERROR;
}

int cgp_field_multi_write_data(int fn, int B, int Z, int S, int *F,
				const cgsize_t *rmin, const cgsize_t *rmax, int nsets, ...)
{
  va_list ap;
  int status;
  va_start(ap, nsets);
  status = vcgp_field_multi_write_data(fn, B, Z, S, F, rmin, rmax, nsets, ap);
  va_end(ap);
  return status;

}


/*---------------------------------------------------------*/

int vcgp_field_multi_read_data(int fn, int B, int Z, int S, int *F,
    const cgsize_t *rmin, const cgsize_t *rmax, int nsets, va_list ap)
{
  int n, m;
  hid_t hid;
  cgns_array *field;
  CGNS_ENUMT(DataType_t) type;
  H5D_rw_multi_t *multi_info;
  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  if (cgi_check_mode(cg->filename, cg->mode, CG_MODE_READ))
    return CG_ERROR;

  multi_info = (H5D_rw_multi_t *)malloc(nsets*sizeof(H5D_rw_multi_t));

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
    multi_info[n].u.rbuf = va_arg(ap, void *);

    multi_info[n].mem_type_id = cgi_datatype(field->data_type);
    to_HDF_ID(field->id,hid);
    multi_info[n].dset_id = hid;
  }

  status = readwrite_multi_data_parallel(nsets, multi_info,
					 field->data_dim, rmin, rmax, CG_PAR_READ);
  free(multi_info);

  return status;

 error:
  if(multi_info)
    free(multi_info);

  return CG_ERROR;
}

int cgp_field_multi_read_data(int fn, int B, int Z, int S, int *F,
    const cgsize_t *rmin, const cgsize_t *rmax, int nsets, ...)
{
  va_list ap;
  int status;
  va_start(ap, nsets);
  status = vcgp_field_multi_read_data(fn, B, Z, S, F, rmin, rmax, nsets, ap);
  va_end(ap);
  return status;

}

/*---------------------------------------------------------*/

int vcgp_array_multi_write_data(int fn, int *A, const cgsize_t *rmin,
			       const cgsize_t *rmax, int nsets, va_list ap)
{
  int n, m, ierr = 0;
  hid_t hid;
  cgns_array *array;
  CGNS_ENUMT(DataType_t) type;
  H5D_rw_multi_t *multi_info;
  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  multi_info = (H5D_rw_multi_t *)malloc(nsets*sizeof(H5D_rw_multi_t));

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

    multi_info[n].u.wbuf = va_arg(ap, const void *);

    multi_info[n].mem_type_id = cgi_datatype(array->data_type);
    to_HDF_ID(array->id, hid);
    multi_info[n].dset_id = hid;
  }

  status = readwrite_multi_data_parallel(nsets, multi_info,
               array->data_dim, rmin, rmax, CG_PAR_WRITE);

  free(multi_info);

  return status;

 error:
    if(multi_info)
      free(multi_info);

    return CG_ERROR;
}

int cgp_array_multi_write_data(int fn, int *A, const cgsize_t *rmin,
			       const cgsize_t *rmax, int nsets, ...)
{
  va_list ap;
  int status;
  va_start(ap, nsets);
  status = vcgp_array_multi_write_data(fn, A, rmin, rmax, nsets, ap);
  va_end(ap);
  return status;

}

/*---------------------------------------------------------*/

int vcgp_array_multi_read_data(int fn, int *A, const cgsize_t *rmin,
			      const cgsize_t *rmax, int nsets, va_list ap)
{
  int n, m, ierr = 0;
  hid_t hid;
  cgns_array *array;
  CGNS_ENUMT(DataType_t) type;
  H5D_rw_multi_t *multi_info;
  int status;

  cg = cgi_get_file(fn);
  if (check_parallel(cg)) return CG_ERROR;

  multi_info = (H5D_rw_multi_t *)malloc(nsets*sizeof(H5D_rw_multi_t));

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
    multi_info[n].u.rbuf = va_arg(ap, void *);

    multi_info[n].mem_type_id = cgi_datatype(array->data_type);
    to_HDF_ID(array->id, hid);
    multi_info[n].dset_id = hid;
  }
  status = readwrite_multi_data_parallel(nsets, multi_info,
               array->data_dim, rmin, rmax, CG_PAR_READ);

  free(multi_info);

  return status;

 error:
  if(multi_info)
    free(multi_info);

  return CG_ERROR;
}

int cgp_array_multi_read_data(int fn, int *A, const cgsize_t *rmin,
			      const cgsize_t *rmax, int nsets, ...)
{
  va_list ap;
  int status;
  va_start(ap, nsets);
  status = vcgp_array_multi_read_data(fn, A, rmin, rmax, nsets, ap);
  va_end(ap);
  return status;

}

#endif
