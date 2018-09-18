#ifndef DIY_MPI_NO_MPI_HPP
#define DIY_MPI_NO_MPI_HPP

#include <stdexcept> // std::runtime_error


static const int MPI_SUCCESS = 0;
static const int MPI_ANY_SOURCE = -1;
static const int MPI_ANY_TAG = -1;

/* define communicator type and constants */
using MPI_Comm = int;
static const MPI_Comm MPI_COMM_NULL = 0;
static const MPI_Comm MPI_COMM_WORLD = 1;

/* MPI threading modes */
static const int MPI_THREAD_SINGLE      = 0;
static const int MPI_THREAD_FUNNELED    = 1;
static const int MPI_THREAD_SERIALIZED  = 2;
static const int MPI_THREAD_MULTIPLE    = 3;

/* define datatypes */
using MPI_Datatype = size_t;

#define DIY_NO_MPI_DATATYPE(cpp_type, mpi_type) \
  static const MPI_Datatype mpi_type = sizeof(cpp_type);
DIY_NO_MPI_DATATYPE(char,                  MPI_BYTE);
DIY_NO_MPI_DATATYPE(int,                   MPI_INT);
DIY_NO_MPI_DATATYPE(unsigned,              MPI_UNSIGNED);
DIY_NO_MPI_DATATYPE(long,                  MPI_LONG);
DIY_NO_MPI_DATATYPE(unsigned long,         MPI_UNSIGNED_LONG);
DIY_NO_MPI_DATATYPE(long long,             MPI_LONG_LONG_INT);
DIY_NO_MPI_DATATYPE(unsigned long long,    MPI_UNSIGNED_LONG_LONG);
DIY_NO_MPI_DATATYPE(float,                 MPI_FLOAT);
DIY_NO_MPI_DATATYPE(double,                MPI_DOUBLE);
#endif

/* status type */
struct MPI_Status
{
    /* These fields are publicly defined in the MPI specification.
       User applications may freely read from these fields. */
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
};

/* define MPI_Request */
using MPI_Request = int;

#define DIY_UNSUPPORTED_MPI_CALL(name) \
  throw std::runtime_error("`" #name "` not supported when DIY_NO_MPI is defined.");

/* define operations */
using MPI_Op = int;
static const MPI_Op MPI_MAX = 0;
static const MPI_Op MPI_MIN = 0;
static const MPI_Op MPI_SUM = 0;
static const MPI_Op MPI_PROD = 0;
static const MPI_Op MPI_LAND = 0;
static const MPI_Op MPI_LOR = 0;

/* mpi i/o stuff */
using MPI_Offset = size_t;
using MPI_File = int;
static const MPI_File MPI_FILE_NULL = 0;

static const int MPI_MODE_CREATE          =   1;
static const int MPI_MODE_RDONLY          =   2;
static const int MPI_MODE_WRONLY          =   4;
static const int MPI_MODE_RDWR            =   8;
static const int MPI_MODE_DELETE_ON_CLOSE =  16;
static const int MPI_MODE_UNIQUE_OPEN     =  32;
static const int MPI_MODE_EXCL            =  64;
static const int MPI_MODE_APPEND          = 128;
static const int MPI_MODE_SEQUENTIAL      = 256;

/* define window type */
using MPI_Win = int;

/* window fence assertions */
static const int MPI_MODE_NOSTORE       = 1;
static const int MPI_MODE_NOPUT         = 2;
static const int MPI_MODE_NOPRECEDE     = 4;
static const int MPI_MODE_NOSUCCEED     = 8;
static const int MPI_MODE_NOCHECK       = 16;

/* window lock types */
static const int MPI_LOCK_SHARED        = 1;
static const int MPI_LOCK_EXCLUSIVE     = 2;
