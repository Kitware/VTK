/* ------------------------------------------------------------------------- *
 * CGNS - CFD General Notation System (http://www.cgns.org)                  *
 * CGNS/MLL - Mid-Level Library header file                                  *
 * Please see cgnsconfig.h file for this local installation configuration    *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *

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

 * ------------------------------------------------------------------------- */

#ifndef PCGNSLIB_H_
#define PCGNSLIB_H_

#include "cgnslib.h"
#include "mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CGP_INDEPENDENT=0,
    CGP_COLLECTIVE=1,
} CGNS_ENUMT( PIOmode_t );

/*===== MPI communicator =====*/

CGNSDLL int cgp_mpi_comm(MPI_Comm mpicomm);

/*===== MPI info =====*/

CGNSDLL int cgp_mpi_info(MPI_Info info);

/*===== parallel IO mode =====*/

CGNSDLL int cgp_pio_mode(CGNS_ENUMT(PIOmode_t) mode);

/*===== File IO Prototypes =====*/

CGNSDLL int cgp_open(const char *filename, int mode, int *fn);
CGNSDLL int cgp_close(int fn);

/*===== Grid IO Prototypes =====*/

CGNSDLL int cgp_coord_write(int fn, int B, int Z,
    CGNS_ENUMT(DataType_t) type, const char *coordname, int *C);
CGNSDLL int cgp_coord_write_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *coord_array);
CGNSDLL int cgp_coord_general_write_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *coords);
CGNSDLL int cgp_coord_read_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, void *coord_array);
CGNSDLL int cgp_coord_general_read_data(int fn, int B, int Z, int C,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *coords);

CGNSDLL int cgp_coord_multi_read_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                      int nsets, void **buf);

CGNSDLL int cgp_coord_multi_write_data(int fn, int B, int Z, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                       int nsets, const void **buf);

/*===== Unstructured Grid Prototypes =====*/

CGNSDLL int cgp_section_write(int fn, int B, int Z,
    const char *sectionname, CGNS_ENUMT(ElementType_t) type,
    cgsize_t start, cgsize_t end, int nbndry, int *S);
CGNSDLL int cgp_elements_write_data(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, const cgsize_t *elements);
CGNSDLL int cgp_elements_read_data(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, cgsize_t *elements);

CGNSDLL int cgp_poly_section_write(int fn, int B, int Z,
    const char *sectionname, CGNS_ENUMT(ElementType_t) type,
    cgsize_t start, cgsize_t end, cgsize_t maxoffset,
    int nbndry, int *S);
CGNSDLL int cgp_poly_elements_write_data(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, const cgsize_t *elements, const cgsize_t *offsets);
CGNSDLL int cgp_poly_elements_read_data_offsets(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, cgsize_t *offsets);
CGNSDLL int cgp_poly_elements_read_data_elements(int fn, int B, int Z, int S,
    cgsize_t start, cgsize_t end, const cgsize_t *offsets, cgsize_t *elements);
CGNSDLL int cgp_parent_data_write(int fn, int B, int Z, int S,
				  cgsize_t start, cgsize_t end,
				  const cgsize_t *parent_data);
CGNSDLL int cgp_parentelements_read_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, cgsize_t *parentelements);
CGNSDLL int cgp_parentelements_write_data(int fn, int B, int Z, int S, cgsize_t start,
    cgsize_t end, cgsize_t *parentelements);

/*===== Solution IO Prototypes =====*/

CGNSDLL int cgp_field_write(int fn, int B, int Z, int S,
    CGNS_ENUMT(DataType_t) type, const char *fieldname, int *F);
CGNSDLL int cgp_field_write_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *data);
CGNSDLL int cgp_field_general_write_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *data);
CGNSDLL int cgp_field_read_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, void *data);
CGNSDLL int cgp_field_general_read_data(int fn, int B, int Z, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *data);


CGNSDLL int cgp_field_multi_read_data(int fn, int B, int Z, int S, int *F,
                                       const cgsize_t *rmin, const cgsize_t *rmax, int nsets, void **buf);

CGNSDLL int cgp_field_multi_write_data(int fn, int B, int Z, int S, int *F,
                                       const cgsize_t *rmin, const cgsize_t *rmax, int nsets, const void **buf);

/*===== Particles IO Prototypes =====*/
CGNSDLL int cgp_particle_coord_write(int fn, int B, int P,
    CGNS_ENUMT(DataType_t) type, const char *coordname, int *C);
CGNSDLL int cgp_particle_coord_write_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *coord_array);
CGNSDLL int cgp_particle_coord_general_write_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *coords);
CGNSDLL int cgp_particle_coord_read_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax, void *coord_array);
CGNSDLL int cgp_particle_coord_general_read_data(int fn, int B, int P, int C,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *coords);

CGNSDLL int cgp_particle_coord_multi_read_data(int fn, int B, int P, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                               int nsets, void **buf);

CGNSDLL int cgp_particle_coord_multi_write_data(int fn, int B, int P, int *C, const cgsize_t *rmin, const cgsize_t *rmax,
                                                int nsets, const void **buf);

/*===== Particles Solution IO Prototypes =====*/

CGNSDLL int cgp_particle_field_write(int fn, int B, int P, int S,
    CGNS_ENUMT(DataType_t) type, const char *fieldname, int *F);
CGNSDLL int cgp_particle_field_write_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, const void *data);
CGNSDLL int cgp_particle_field_general_write_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *data);
CGNSDLL int cgp_particle_field_read_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax, void *data);
CGNSDLL int cgp_particle_field_general_read_data(int fn, int B, int P, int S, int F,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *data);


CGNSDLL int cgp_particle_field_multi_read_data(int fn, int B, int P, int S, int *F,
                                       const cgsize_t *rmin, const cgsize_t *rmax, int nsets, void **buf);

CGNSDLL int cgp_particle_field_multi_write_data(int fn, int B, int P, int S, int *F,
                                       const cgsize_t *rmin, const cgsize_t *rmax, int nsets, const void **buf);
/*===== Array IO Prototypes =====*/

CGNSDLL int cgp_array_write(const char *arrayname,
    CGNS_ENUMT(DataType_t) type, int DataDimension,
    const cgsize_t *DimensionVector, int *A);
CGNSDLL int cgp_array_write_data(int A, const cgsize_t *rmin,
    const cgsize_t *rmax, const void *data);
CGNSDLL int cgp_array_general_write_data(int A,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, const void *data);
CGNSDLL int cgp_array_read_data(int A, const cgsize_t *rmin,
    const cgsize_t *rmax, void *data);
CGNSDLL int cgp_array_general_read_data(int A,
    const cgsize_t *rmin, const cgsize_t *rmax,
    CGNS_ENUMT(DataType_t) m_type,
    int m_numdim, const cgsize_t *m_arg_dimvals,
    const cgsize_t *m_rmin, const cgsize_t *m_rmax, void *data);

CGNSDLL int cgp_array_multi_write_data(int fn, int *A, const cgsize_t *rmin,
                                       const cgsize_t *rmax, int nsets, const void **buf);

CGNSDLL int cgp_array_multi_read_data(int fn, int *A, const cgsize_t *rmin,
                                      const cgsize_t *rmax, int nsets, void **buf);


/*===== PointList Prototypes =====*/
CGNSDLL int cgp_ptlist_write_data(int file_number, cgsize_t start,
    cgsize_t end, const cgsize_t *points);
CGNSDLL int cgp_ptlist_read_data(int file_number, cgsize_t start, cgsize_t end, cgsize_t *points);

/*===== exit with error and call MPI_Abort =====*/

CGNSDLL void cgp_error_exit(void);

#ifdef __cplusplus
}
#endif
#endif
