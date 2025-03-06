#ifndef PyMPI_COMPAT_MPICH_H
#define PyMPI_COMPAT_MPICH_H
#if defined(MPICH_NUMVERSION)

/* -------------------------------------------------------------------------- */

/* https://github.com/pmodels/mpich/pull/5467 */

#undef  MPI_MAX_PORT_NAME
#define MPI_MAX_PORT_NAME 1024

static int PyMPI_MPICH_port_info(MPI_Info info, MPI_Info *port_info)
{
  int ierr;
# define pympi_str_(s) #s
# define pympi_str(s) pympi_str_(s)
  const char *key = "port_name_size";
  const char *val = pympi_str(MPI_MAX_PORT_NAME);
# undef pympi_str_
# undef pympi_str
  if (info == MPI_INFO_NULL) {
    ierr = MPI_Info_create(port_info); if (ierr) return ierr;
  } else {
    ierr = MPI_Info_dup(info, port_info); if (ierr) return ierr;
  }
  ierr = MPI_Info_set(*port_info, key, val);
  if (ierr) (void) MPI_Info_free(port_info);
  return ierr;
}

static int PyMPI_MPICH_MPI_Open_port(MPI_Info info, char *port_name)
{
  int ierr;
  ierr = PyMPI_MPICH_port_info(info, &info); if (ierr) return ierr;
  ierr = MPI_Open_port(info, port_name);
  (void) MPI_Info_free(&info);
  return ierr;
}
#undef  MPI_Open_port
#define MPI_Open_port PyMPI_MPICH_MPI_Open_port

static int PyMPI_MPICH_MPI_Lookup_name(const char *service_name,
                                       MPI_Info   info,
                                       char       *port_name)
{
  int ierr;
  ierr = PyMPI_MPICH_port_info(info, &info); if (ierr) return ierr;
  ierr = MPI_Lookup_name(service_name, info, port_name);
  (void) MPI_Info_free(&info);
  return ierr;
}
#undef  MPI_Lookup_name
#define MPI_Lookup_name PyMPI_MPICH_MPI_Lookup_name

/* -------------------------------------------------------------------------- */

/* https://github.com/pmodels/mpich/issues/6981 */

#if MPI_VERSION == 4 && MPI_SUBVERSION <= 1

#if (MPICH_NUMVERSION < 40300300) || defined(CIBUILDWHEEL)
static int PyMPI_MPICH_MPI_Info_free(MPI_Info *info)
{
  if (info && *info == MPI_INFO_ENV) {
    (void) MPI_Comm_call_errhandler(MPI_COMM_SELF, MPI_ERR_INFO);
    return MPI_ERR_INFO;
  }
  return MPI_Info_free(info);
}
#undef  MPI_Info_free
#define MPI_Info_free PyMPI_MPICH_MPI_Info_free
#endif

#endif

/* -------------------------------------------------------------------------- */

/* https://github.com/pmodels/mpich/issues/5413 */
/* https://github.com/pmodels/mpich/pull/6146   */

#if MPI_VERSION == 4 && MPI_SUBVERSION == 0

#if (MPICH_NUMVERSION < 40003300) || defined(CIBUILDWHEEL)
static int PyMPI_MPICH_MPI_Status_set_elements_c(MPI_Status *status,
                                                 MPI_Datatype datatype,
                                                 MPI_Count elements)
{
  return MPI_Status_set_elements_x(status, datatype, elements);
}
#undef  MPI_Status_set_elements_c
#define MPI_Status_set_elements_c PyMPI_MPICH_MPI_Status_set_elements_c
#endif

#if defined(CIBUILDWHEEL) && defined(__linux__)
#undef MPI_Status_set_elements_c
extern int MPI_Status_set_elements_c(MPI_Status *, MPI_Datatype, MPI_Count)
__attribute__((weak, alias("PyMPI_MPICH_MPI_Status_set_elements_c")));
#endif

#endif

/* -------------------------------------------------------------------------- */

/* https://github.com/pmodels/mpich/issues/6351 */
/* https://github.com/pmodels/mpich/pull/6354   */

#if MPI_VERSION == 4 && MPI_SUBVERSION == 0

#if (MPICH_NUMVERSION < 40100300) || defined(CIBUILDWHEEL)
static int PyMPI_MPICH_MPI_Reduce_c(const void *sendbuf, void *recvbuf,
                                    MPI_Count count, MPI_Datatype datatype,
                                    MPI_Op op, int root, MPI_Comm comm)
{
  const char dummy[1] = {0};
  if (!sendbuf && (root == MPI_ROOT || root == MPI_PROC_NULL)) sendbuf = dummy;
  return MPI_Reduce_c(sendbuf, recvbuf, count, datatype, op, root, comm);
}
#undef  MPI_Reduce_c
#define MPI_Reduce_c PyMPI_MPICH_MPI_Reduce_c
#endif

#endif

/* -------------------------------------------------------------------------- */

#if defined(CIBUILDWHEEL)

#define PyMPI_MPICH_CALL_WEAK_SYMBOL(function, ...) \
  if (function) return function(__VA_ARGS__); \
  return PyMPI_UNAVAILABLE(#function, __VA_ARGS__); \

#undef MPI_Type_create_f90_integer
#pragma weak MPI_Type_create_f90_integer
static int PyMPI_MPICH_MPI_Type_create_f90_integer(int r, MPI_Datatype *t)
{ PyMPI_MPICH_CALL_WEAK_SYMBOL(MPI_Type_create_f90_integer, r, t); }
#define MPI_Type_create_f90_integer PyMPI_MPICH_MPI_Type_create_f90_integer

#undef MPI_Type_create_f90_real
#pragma weak MPI_Type_create_f90_real
static int PyMPI_MPICH_MPI_Type_create_f90_real(int p, int r, MPI_Datatype *t)
{ PyMPI_MPICH_CALL_WEAK_SYMBOL(MPI_Type_create_f90_real, p, r, t); }
#define MPI_Type_create_f90_real PyMPI_MPICH_MPI_Type_create_f90_real

#undef MPI_Type_create_f90_complex
#pragma weak MPI_Type_create_f90_complex
static int PyMPI_MPICH_MPI_Type_create_f90_complex(int p, int r, MPI_Datatype *t)
{ PyMPI_MPICH_CALL_WEAK_SYMBOL(MPI_Type_create_f90_complex, p, r, t); }
#define MPI_Type_create_f90_complex PyMPI_MPICH_MPI_Type_create_f90_complex

#undef MPI_Status_c2f
#pragma weak MPI_Status_c2f
static int PyMPI_MPICH_MPI_Status_c2f(const MPI_Status *cs, MPI_Fint *fs)
{ PyMPI_MPICH_CALL_WEAK_SYMBOL(MPI_Status_c2f, cs, fs); }
#define MPI_Status_c2f PyMPI_MPICH_MPI_Status_c2f

#undef MPI_Status_f2c
#pragma weak MPI_Status_f2c
static int PyMPI_MPICH_MPI_Status_f2c(const MPI_Fint *fs, MPI_Status *cs)
{ PyMPI_MPICH_CALL_WEAK_SYMBOL(MPI_Status_f2c, fs, cs); }
#define MPI_Status_f2c PyMPI_MPICH_MPI_Status_f2c

#endif

/* -------------------------------------------------------------------------- */

#endif /* !MPICH_NUMVERSION      */
#endif /* !PyMPI_COMPAT_MPICH_H */
