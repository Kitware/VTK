#if   defined(MSMPI_VER)
#include "compat/msmpi.h"
#elif defined(MPICH3)
#include "compat/mpich3.h"
#elif defined(MPICH2)
#include "compat/mpich2.h"
#elif defined(OPEN_MPI)
#include "compat/openmpi.h"
#elif defined(PLATFORM_MPI)
#include "compat/pcmpi.h"
#elif defined(HP_MPI)
#include "compat/hpmpi.h"
#elif defined(MPICH1)
#include "compat/mpich1.h"
#elif defined(LAM_MPI)
#include "compat/lammpi.h"
#endif
