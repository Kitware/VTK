#include "dims.h"
#include <assert.h>

#define DIMENSION 3

static int dims_init=0;
static int dims_dims[DIMENSION] = {0,0,0};

int MY_Dims_init_3D(int nnodes, int ndim, int *dims) {
  int i, tmp_nnodes=1;

  assert(ndim == DIMENSION);

  tmp_nnodes = 1;
  for(i=0; i<ndim; i++) {
    dims_dims[i] = dims[i];
    tmp_nnodes *= dims[i];
  }

  assert(tmp_nnodes == nnodes);

  dims_init = 1;

  return 0;
}

int MY_Dims_create_3D(int nnodes, int ndim, int *dims) {
  int i, ret=0;

  assert(ndim == DIMENSION);
  assert(dims[0] == 0);
  assert(dims[1] == 0);
  assert(dims[2] == 0);

  if(dims_init == 0)
    ret = MPI_Dims_create(nnodes, ndim, dims);
  else {
    for(i=0; i<ndim; i++)
      dims[i] = dims_dims[i];
  }

  return ret;
}
