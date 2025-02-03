#ifdef DIY_MPI_AS_LIB
#include "io.hpp"
#endif

#include "status.hpp"

#ifdef DIY_MPI_AS_LIB
const int diy::mpi::io::file::rdonly          = MPI_MODE_RDONLY;
const int diy::mpi::io::file::rdwr            = MPI_MODE_RDWR;
const int diy::mpi::io::file::wronly          = MPI_MODE_WRONLY;
const int diy::mpi::io::file::create          = MPI_MODE_CREATE;
const int diy::mpi::io::file::exclusive       = MPI_MODE_EXCL;
const int diy::mpi::io::file::delete_on_close = MPI_MODE_DELETE_ON_CLOSE;
const int diy::mpi::io::file::unique_open     = MPI_MODE_UNIQUE_OPEN;
const int diy::mpi::io::file::sequential      = MPI_MODE_SEQUENTIAL;
const int diy::mpi::io::file::append          = MPI_MODE_APPEND;
#endif

diy::mpi::io::file::
file(const communicator& comm__, const std::string& filename, int mode)
: comm_(comm__)
{
#if DIY_HAS_MPI
  int ret = MPI_File_open(diy::mpi::mpi_cast(comm__.handle()), const_cast<char*>(filename.c_str()), mode, MPI_INFO_NULL, &diy::mpi::mpi_cast(fh));
  if (ret)
      throw std::runtime_error("DIY cannot open file: " + filename);
#else
  (void)comm__; (void)filename; (void)mode;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_open);
#endif
}

void
diy::mpi::io::file::
close()
{
#if DIY_HAS_MPI
  if (diy::mpi::mpi_cast(fh) != MPI_FILE_NULL)
    MPI_File_close(&diy::mpi::mpi_cast(fh));
#endif
}

diy::mpi::io::offset
diy::mpi::io::file::
size() const
{
#if DIY_HAS_MPI
  MPI_Offset sz;
  MPI_File_get_size(diy::mpi::mpi_cast(fh), &sz);
  return static_cast<offset>(sz);
#else
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_get_size);
#endif
}

void
diy::mpi::io::file::
resize(diy::mpi::io::offset size_)
{
#if DIY_HAS_MPI
  MPI_File_set_size(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(size_));
#else
  (void)size_;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_set_size);
#endif
}

void
diy::mpi::io::file::
read_at(offset o, char* buffer, size_t size_)
{
#if DIY_HAS_MPI
  status s;
  MPI_File_read_at(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(o), buffer, static_cast<int>(size_), MPI_BYTE, &diy::mpi::mpi_cast(s.handle));
#else
  (void)o; (void)buffer; (void)size_;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_read_at);
#endif
}

void
diy::mpi::io::file::
read_at_all(offset o, char* buffer, size_t size_)
{
#if DIY_HAS_MPI
  status s;
  MPI_File_read_at_all(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(o), buffer, static_cast<int>(size_), MPI_BYTE, &diy::mpi::mpi_cast(s.handle));
#else
  (void)o; (void)buffer; (void)size_;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_read_at_all);
#endif
}

void
diy::mpi::io::file::
write_at(offset o, const char* buffer, size_t size_)
{
#if DIY_HAS_MPI
  status s;
  MPI_File_write_at(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(o), (void *)buffer, static_cast<int>(size_), MPI_BYTE, &diy::mpi::mpi_cast(s.handle));
#else
  (void)o; (void)buffer; (void)size_;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_write_at);
#endif
}

void
diy::mpi::io::file::
write_at_all(offset o, const char* buffer, size_t size_)
{
#if DIY_HAS_MPI
  status s;
  MPI_File_write_at_all(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(o), (void *)buffer, static_cast<int>(size_), MPI_BYTE, &diy::mpi::mpi_cast(s.handle));
#else
  (void)o; (void)buffer; (void)size_;
  DIY_UNSUPPORTED_MPI_CALL(MPI_File_write_at_all);
#endif
}

void
diy::mpi::io::file::
read_bov(const DiscreteBounds& bounds, int ndims, const int dims[], char* buffer, size_t offset, const datatype& dt, bool collective, int chunk)
{
#if DIY_HAS_MPI
  int total = 1;
  std::vector<int> subsizes;
  for (unsigned i = 0; i < static_cast<unsigned>(ndims); ++i)
  {
    subsizes.push_back(bounds.max[i] - bounds.min[i] + 1);
    total *= subsizes.back();
  }

  MPI_Datatype T_type;
  if (chunk == 1)
  {
    T_type = diy::mpi::mpi_cast(dt.handle);
  }
  else
  {
    // create an MPI struct of size chunk to read the data in those chunks
    // (this allows to work around MPI-IO weirdness where crucial quantities
    // are ints, which are too narrow of a type)
    int             array_of_blocklengths[]  = { chunk };
    MPI_Aint        array_of_displacements[] = { 0 };
    MPI_Datatype    array_of_types[]         = { diy::mpi::mpi_cast(dt.handle) };
    MPI_Type_create_struct(1, array_of_blocklengths, array_of_displacements, array_of_types, &T_type);
    MPI_Type_commit(&T_type);
  }

  MPI_Datatype fileblk;
  MPI_Type_create_subarray(ndims, dims, subsizes.data(), (int*) &bounds.min[0], MPI_ORDER_C, T_type, &fileblk);
  MPI_Type_commit(&fileblk);

  MPI_File_set_view(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(offset), T_type, fileblk, (char*)"native", MPI_INFO_NULL);

  mpi::status s;
  if (!collective)
    MPI_File_read(diy::mpi::mpi_cast(fh), buffer, total, T_type, &mpi_cast(s.handle));
  else
    MPI_File_read_all(diy::mpi::mpi_cast(fh), buffer, total, T_type, &mpi_cast(s.handle));

  if (chunk != 1)
    MPI_Type_free(&T_type);
  MPI_Type_free(&fileblk);
#else
  (void) bounds; (void) ndims; (void) dims, (void) buffer; (void) offset, (void) dt, (void) collective; (void) chunk;
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::io::file::read_bov);
#endif
}

void
diy::mpi::io::file::
write_bov(const DiscreteBounds& bounds, const DiscreteBounds& core, int ndims, const int dims[], const char* buffer, size_t offset, const datatype& dt, bool collective, int chunk)
{
#if DIY_HAS_MPI
  std::vector<int> subsizes;
  std::vector<int> buffer_shape, buffer_start;
  for (unsigned i = 0; i < static_cast<unsigned>(ndims); ++i)
  {
    buffer_shape.push_back(bounds.max[i] - bounds.min[i] + 1);
    buffer_start.push_back(core.min[i] - bounds.min[i]);
    subsizes.push_back(core.max[i] - core.min[i] + 1);
  }

  MPI_Datatype T_type;
  if (chunk == 1)
  {
    T_type = diy::mpi::mpi_cast(dt.handle);
  }
  else
  {
    // assume T is a binary block and create an MPI struct of appropriate size
    int             array_of_blocklengths[]  = { chunk };
    MPI_Aint        array_of_displacements[] = { 0 };
    MPI_Datatype    array_of_types[]         = { diy::mpi::mpi_cast(dt.handle) };
    MPI_Type_create_struct(1, array_of_blocklengths, array_of_displacements, array_of_types, &T_type);
    MPI_Type_commit(&T_type);
  }

  MPI_Datatype fileblk, subbuffer;
  MPI_Type_create_subarray(ndims, dims, subsizes.data(), (int*) &core.min[0], MPI_ORDER_C, T_type, &fileblk);
  MPI_Type_create_subarray(ndims, buffer_shape.data(), subsizes.data(), buffer_start.data(), MPI_ORDER_C, T_type, &subbuffer);
  MPI_Type_commit(&fileblk);
  MPI_Type_commit(&subbuffer);

  MPI_File_set_view(diy::mpi::mpi_cast(fh), static_cast<MPI_Offset>(offset), T_type, fileblk, (char*)"native", MPI_INFO_NULL);

  mpi::status s;
  if (!collective)
    MPI_File_write(diy::mpi::mpi_cast(fh), (void*)buffer, 1, subbuffer, &mpi_cast(s.handle));
  else
    MPI_File_write_all(diy::mpi::mpi_cast(fh), (void*)buffer, 1, subbuffer, &mpi_cast(s.handle));

  if (chunk != 1)
    MPI_Type_free(&T_type);
  MPI_Type_free(&fileblk);
  MPI_Type_free(&subbuffer);
#else
  (void) bounds; (void) core, (void) ndims; (void) dims, (void) buffer; (void) offset, (void) dt, (void) collective; (void) chunk;
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::io::file::write_bov);
#endif
}
