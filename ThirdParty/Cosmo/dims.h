#ifndef DIMS_H
#define DIMS_H

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

int MY_Dims_init_3D(int nnodes, int ndim, int *dims);
int MY_Dims_create_3D(int nnodes, int ndim, int *dims);

#ifdef __cplusplus
}
#endif

#endif
