#include "stdlib.h"
#include "mpi.h"

#if (defined(OMPI_MAJOR_VERSION) && \
     defined(OMPI_MINOR_VERSION) && \
     defined(OMPI_RELEASE_VERSION))
#define OPENMPI_VERSION_NUMBER \
           ((OMPI_MAJOR_VERSION   * 10000) + \
            (OMPI_MINOR_VERSION   * 100)   + \
            (OMPI_RELEASE_VERSION * 1))
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ompregdescr;
extern int POMP_MAX_ID;
extern struct ompregdescr* pomp_rd_table[];

int POMP_MAX_ID = 0;
struct ompregdescr* pomp_rd_table[] = { 0 };

#if defined(OPENMPI_VERSION_NUMBER)
#if ((OPENMPI_VERSION_NUMBER >= 10300) && \
     (OPENMPI_VERSION_NUMBER <  10403))
int MPI_Init_thread(int *argc, char ***argv,
                    int required, int *provided)
{
  if (provided) *provided = MPI_THREAD_SINGLE;
  return MPI_Init(argc, argv);
}
#endif
#endif

#ifdef __cplusplus
}
#endif
