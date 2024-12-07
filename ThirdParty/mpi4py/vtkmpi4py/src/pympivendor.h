/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

#include <stdio.h>
#include <string.h>

static int PyMPI_Get_vendor(const char **vendor_name,
                            int         *version_major,
                            int         *version_minor,
                            int         *version_micro)
{
  const char *name = "unknown";
  int major=0, minor=0, micro=0;

#if defined(PyMPI_ABI) || defined(CIBUILDWHEEL)

  int ierr, len = 0, cnt;
  char lib[MPI_MAX_LIBRARY_VERSION_STRING] = {0}, *str;

  ierr = MPI_Get_library_version(lib, &len);
  if (ierr != MPI_SUCCESS) return ierr;

  cnt = sscanf(lib, "MPICH Version: %d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "MPICH"; goto done; }

  cnt = sscanf(lib, "Open MPI v%d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "Open MPI"; goto done; }

  cnt = sscanf(lib, "Intel(R) MPI Library %d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "Intel MPI"; goto done; }

  cnt = sscanf(lib, "Microsoft MPI %d.%d", &major, &minor);
  if (cnt > 0) { name = "Microsoft MPI"; goto done; }

  str = strstr(lib, "CRAY MPICH version"); if (!str) str = lib;
  cnt = sscanf(str, "CRAY MPICH version %d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "Cray MPICH"; goto done; }

  cnt = sscanf(lib, "MVAPICH Version: %d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "MVAPICH"; goto done; }

  cnt = sscanf(lib, "MVAPICH2 Version: %d.%d.%d", &major, &minor, &micro);
  if (cnt > 0) { name = "MVAPICH"; goto done; }

 done:

#elif defined(I_MPI_VERSION)

  name = "Intel MPI";
  #if defined(I_MPI_NUMVERSION)
  {int version = I_MPI_NUMVERSION/1000;
  major = version/10000; version -= major*10000;
  minor = version/100;   version -= minor*100;
  micro = version/1;     version -= micro*1; }
  #else
  (void)sscanf(I_MPI_VERSION,"%d.%d Update %d",&major,&minor,&micro);
  #endif

#elif defined(MSMPI_VER)

  name = "Microsoft MPI";
  major = MSMPI_VER >> 8;
  minor = MSMPI_VER & 0xFF;

#elif defined(CRAY_MPICH_VERSION)

  name = "Cray MPICH";
# define str(s) #s
# define xstr(s) str(s)
  (void)sscanf(xstr(CRAY_MPICH_VERSION),"%d.%d.%d",&major,&minor,&micro);
# undef xstr
# undef str

#elif defined(MVAPICH_VERSION) || defined(MVAPICH_NUMVERSION)

  name = "MVAPICH";
  #if defined(MVAPICH_NUMVERSION)
  {int version = MVAPICH_NUMVERSION/1000; if (version<1000) version *= 100;
  major = version/10000; version -= major*10000;
  minor = version/100;   version -= minor*100;
  micro = version/1;     version -= micro*1; }
  #elif defined(MVAPICH_VERSION)
  (void)sscanf(MVAPICH_VERSION,"%d.%d.%d",&major,&minor,&micro);
  #endif

#elif defined(MVAPICH2_VERSION) || defined(MVAPICH2_NUMVERSION)

  name = "MVAPICH";
  #if defined(MVAPICH2_NUMVERSION)
  {int version = MVAPICH2_NUMVERSION/1000; if (version<1000) version *= 100;
  major = version/10000; version -= major*10000;
  minor = version/100;   version -= minor*100;
  micro = version/1;     version -= micro*1; }
  #elif defined(MVAPICH2_VERSION)
  (void)sscanf(MVAPICH2_VERSION,"%d.%d.%d",&major,&minor,&micro);
  #endif

#elif defined(MPICH_NAME) && (MPICH_NAME >= 3)

  name = "MPICH";
  #if defined(MPICH_NUMVERSION)
  {int version = MPICH_NUMVERSION/1000;
  major = version/10000; version -= major*10000;
  minor = version/100;   version -= minor*100;
  micro = version/1;     version -= micro*1; }
  #elif defined(MPICH_VERSION)
  (void)sscanf(MPICH_VERSION,"%d.%d.%d",&major,&minor,&micro);
  #endif

#elif defined(MPICH_NAME) && (MPICH_NAME == 2)

  name = "MPICH2";
  #if defined(MPICH2_NUMVERSION)
  {int version = MPICH2_NUMVERSION/1000;
  major = version/10000; version -= major*10000;
  minor = version/100;   version -= minor*100;
  micro = version/1;     version -= micro*1; }
  #elif defined(MPICH2_VERSION)
  (void)sscanf(MPICH2_VERSION,"%d.%d.%d",&major,&minor,&micro);
  #endif

#elif defined(MPICH_NAME) && (MPICH_NAME == 1)

  name = "MPICH1";
  #if defined(MPICH_VERSION)
  (void)sscanf(MPICH_VERSION,"%d.%d.%d",&major,&minor,&micro);
  #endif

#elif defined(OPEN_MPI)

  name = "Open MPI";
  #if defined(OMPI_MAJOR_VERSION)
  major = OMPI_MAJOR_VERSION;
  #endif
  #if defined(OMPI_MINOR_VERSION)
  minor = OMPI_MINOR_VERSION;
  #endif
  #if defined(OMPI_RELEASE_VERSION)
  micro = OMPI_RELEASE_VERSION;
  #endif

#elif defined(LAM_MPI)

  name = "LAM/MPI";
  #if defined(LAM_MAJOR_VERSION)
  major = LAM_MAJOR_VERSION;
  #endif
  #if defined(LAM_MINOR_VERSION)
  minor = LAM_MINOR_VERSION;
  #endif
  #if defined(LAM_RELEASE_VERSION)
  micro = LAM_RELEASE_VERSION;
  #endif

#endif

  if (vendor_name)   *vendor_name   = name;
  if (version_major) *version_major = major;
  if (version_minor) *version_minor = minor;
  if (version_micro) *version_micro = micro;

  return MPI_SUCCESS;
}

/*
   Local variables:
   c-basic-offset: 2
   indent-tabs-mode: nil
   End:
*/
