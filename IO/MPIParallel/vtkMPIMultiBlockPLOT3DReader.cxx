/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIMultiBlockPLOT3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIMultiBlockPLOT3DReader.h"

#include "vtkByteSwap.h"
#include "vtkDataArrayDispatcher.h"
#include "vtkErrorCode.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkMultiBlockPLOT3DReaderInternals.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredData.h"

#include <exception>
#include <cassert>

#define DEFINE_MPI_TYPE(ctype, mpitype) \
  template <> struct mpi_type<ctype> { static MPI_Datatype type() { return mpitype; }  };

namespace
{
  template <class T> struct mpi_type {};
  DEFINE_MPI_TYPE(char, MPI_CHAR);
  DEFINE_MPI_TYPE(signed char, MPI_SIGNED_CHAR);
  DEFINE_MPI_TYPE(unsigned char, MPI_UNSIGNED_CHAR);
  DEFINE_MPI_TYPE(short, MPI_SHORT);
  DEFINE_MPI_TYPE(unsigned short, MPI_UNSIGNED_SHORT);
  DEFINE_MPI_TYPE(int, MPI_INT);
  DEFINE_MPI_TYPE(unsigned int, MPI_UNSIGNED);
  DEFINE_MPI_TYPE(long, MPI_LONG);
  DEFINE_MPI_TYPE(unsigned long, MPI_UNSIGNED_LONG);
  DEFINE_MPI_TYPE(float, MPI_FLOAT);
  DEFINE_MPI_TYPE(double, MPI_DOUBLE);
#if defined(VTK_TYPE_USE_LONG_LONG)
  DEFINE_MPI_TYPE(long long, MPI_LONG_LONG);
  DEFINE_MPI_TYPE(unsigned long long, MPI_UNSIGNED_LONG_LONG);
#endif
#if defined(VTK_TYPE_USE___INT64)
# if VTK_SIZEOF_LONG == 8
  DEFINE_MPI_TYPE(__int64, MPI_LONG);
  DEFINE_MPI_TYPE(unsigned __int64, MPI_UNSIGNED_LONG);
# elif defined(MPI_LONG_LONG)
  DEFINE_MPI_TYPE(__int64, MPI_LONG_LONG);
  DEFINE_MPI_TYPE(__int64, MPI_UNSIGNED_LONG_LONG);
# endif
#endif

  class MPIPlot3DException : public std::exception
  {
  };

  class SetupFileViewFunctor
  {
    vtkMPICommunicatorOpaqueComm& Communicator;
    vtkMPIOpaqueFileHandle& Handle;
    int Extent[6];
    int WExtent[6];
    vtkTypeUInt64 Offset;
    int ByteOrder;

  public:
    SetupFileViewFunctor(
      vtkMPICommunicatorOpaqueComm& comm,
      vtkMPIOpaqueFileHandle& handle,
        const int extent[6], const int wextent[6], vtkTypeUInt64 offset, int byteOrder)
      : Communicator(comm), Handle(handle), Offset(offset), ByteOrder(byteOrder)
    {
    std::copy(extent, extent+6, this->Extent);
    std::copy(wextent, wextent+6, this->WExtent);
    }

    template <class T>
    void operator()(vtkDataArrayDispatcherPointer<T> array) const
    {
    int array_of_sizes[3];
    int array_of_subsizes[3];
    int array_of_starts[3];
    for (int cc=0; cc < 3; ++cc)
      {
      array_of_sizes[cc]    = this->WExtent[cc*2+1] - this->WExtent[cc*2] + 1;
      array_of_subsizes[cc] = this->Extent[cc*2+1] - this->Extent[cc*2] + 1;
      array_of_starts[cc]   = this->Extent[cc*2];
      }

    // Define the file type for this process.
    MPI_Datatype filetype;
    if (MPI_Type_create_subarray(3,
        array_of_sizes,
        array_of_subsizes,
        array_of_starts,
        MPI_ORDER_FORTRAN,
        mpi_type<T>::type(),
        &filetype) != MPI_SUCCESS)
      {
      throw MPIPlot3DException();
      }
    MPI_Type_commit(&filetype);

    // For each component.
    const vtkIdType valuesToRead = vtkStructuredData::GetNumberOfPoints(
      const_cast<int*>(this->Extent));
    const vtkIdType valuesTotal = vtkStructuredData::GetNumberOfPoints(
      const_cast<int*>(this->WExtent));

    const int _INT_MAX = 2e9 / sizeof(T); /// XXX: arbitrary limit that seems
                                          /// to work when reading large files.

    int numIterations = static_cast<int>(valuesToRead/_INT_MAX) + 1;
    int maxNumIterations;
    if (MPI_Allreduce(&numIterations, &maxNumIterations, 1, mpi_type<int>::type(),
        MPI_MAX, *this->Communicator.GetHandle()) != MPI_SUCCESS)
      {
      throw MPIPlot3DException();
      }

    vtkTypeUInt64 byteOffset = this->Offset;
    // simply use the VTK array for scalars.
    T *buffer = array.NumberOfComponents == 1? array.RawPointer : new T[valuesToRead];
    try
      {
      for (int comp=0; comp < array.NumberOfComponents; ++comp)
        {
        // Define the view.
        if (MPI_File_set_view(this->Handle.Handle, byteOffset, mpi_type<T>::type(),
            filetype, const_cast<char*>("native"), MPI_INFO_NULL) != MPI_SUCCESS)
          {
          throw MPIPlot3DException();
          }

        // Read contents from the file in 2GB chunks.
        vtkIdType valuesRead = 0;
        for (int iteration = 0; iteration < maxNumIterations; ++iteration)
          {
          vtkIdType valuesToReadInIteration =
            std::min((valuesToRead - valuesRead), static_cast<vtkIdType>(_INT_MAX));
          if (MPI_File_read_all(this->Handle.Handle, buffer + valuesRead,
              static_cast<int>(valuesToReadInIteration),
              mpi_type<T>::type(), MPI_STATUS_IGNORE) != MPI_SUCCESS)
            {
            throw MPIPlot3DException();
            }
          valuesRead += valuesToReadInIteration;
          }
        if (buffer != array.RawPointer)
          {
          for (vtkIdType i=0; i < valuesToRead; ++i)
            {
            array.RawPointer[array.NumberOfComponents*i + comp] = buffer[i];
            }
          }
        byteOffset += valuesTotal * sizeof(T);
        }
      }
    catch (MPIPlot3DException)
      {
      if (buffer != array.RawPointer)
        {
        delete [] buffer;
        }
      MPI_Type_free(&filetype);
      throw MPIPlot3DException();
      }
    if (buffer != array.RawPointer)
      {
      delete [] buffer;
      }
    MPI_Type_free(&filetype);
    // finally, handle byte order.
    if (this->ByteOrder == vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::SwapLERange(array.RawPointer, array.NumberOfComponents*array.NumberOfTuples);
      }
    else
      {
      vtkByteSwap::SwapBERange(array.RawPointer, array.NumberOfComponents*array.NumberOfTuples);
      }
    }
  };
}

vtkStandardNewMacro(vtkMPIMultiBlockPLOT3DReader);
//----------------------------------------------------------------------------
vtkMPIMultiBlockPLOT3DReader::vtkMPIMultiBlockPLOT3DReader()
{
  this->UseMPIIO = true;
}

//----------------------------------------------------------------------------
vtkMPIMultiBlockPLOT3DReader::~vtkMPIMultiBlockPLOT3DReader()
{
}

//----------------------------------------------------------------------------
bool vtkMPIMultiBlockPLOT3DReader::CanUseMPIIO()
{
  return (this->UseMPIIO && this->BinaryFile &&
    this->Internal->Settings.NumberOfDimensions == 3 &&
    vtkMPIController::SafeDownCast(this->Controller) != NULL);
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::OpenFileForDataRead(void*& vfp, const char* fname)
{
  if (!this->CanUseMPIIO())
    {
    return this->Superclass::OpenFileForDataRead(vfp, fname);
    }

  vtkMPICommunicator* mpiComm = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  assert(mpiComm);

  vtkMPIOpaqueFileHandle* handle = new vtkMPIOpaqueFileHandle();
  try
    {
    if (MPI_File_open(*mpiComm->GetMPIComm()->GetHandle(),
                      const_cast<char*>(fname), MPI_MODE_RDONLY,
                      MPI_INFO_NULL, &handle->Handle) != MPI_SUCCESS)
      {
      this->SetErrorCode(vtkErrorCode::FileNotFoundError);
      vtkErrorMacro("File: " << fname << " not found.");
      throw MPIPlot3DException();
      }
    }
  catch (MPIPlot3DException)
    {
    delete handle;
    vfp = NULL;
    return VTK_ERROR;
    }
  vfp = handle;
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkMPIMultiBlockPLOT3DReader::CloseFile(void* vfp)
{
  if (!this->CanUseMPIIO())
    {
    this->Superclass::CloseFile(vfp);
    return;
    }

  vtkMPIOpaqueFileHandle* handle = reinterpret_cast<vtkMPIOpaqueFileHandle*>(vfp);
  assert(handle);
  if (MPI_File_close(&handle->Handle) != MPI_SUCCESS)
    {
    vtkErrorMacro("Failed to close file!");
    }
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::ReadIntScalar(
  void* vfp, int extent[6], int wextent[6],
  vtkDataArray* scalar, vtkTypeUInt64 offset)
{
  if (!this->CanUseMPIIO())
    {
    return this->Superclass::ReadIntScalar(vfp, extent, wextent, scalar, offset);
    }

  return this->ReadScalar(vfp, extent, wextent, scalar, offset);
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::ReadScalar(
    void* vfp,
    int extent[6], int wextent[6],
    vtkDataArray* scalar, vtkTypeUInt64 offset)
{
  if (!this->CanUseMPIIO())
    {
    return this->Superclass::ReadScalar(vfp, extent, wextent, scalar, offset);
    }

  vtkMPIOpaqueFileHandle* handle = reinterpret_cast<vtkMPIOpaqueFileHandle*>(vfp);
  assert(handle);

  vtkMPICommunicatorOpaqueComm* comm =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator())->GetMPIComm();
  assert(comm);

  SetupFileViewFunctor work(*comm, *handle, extent, wextent, offset, this->Internal->Settings.ByteOrder);
  vtkDataArrayDispatcher<SetupFileViewFunctor> dispatcher(work);
  try
    {
    dispatcher.Go(scalar);
    }
  catch (MPIPlot3DException)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::ReadVector(
  void* vfp,
  int extent[6], int wextent[6],
  int numDims, vtkDataArray* vector, vtkTypeUInt64 offset)
{
  if (!this->CanUseMPIIO())
    {
    return this->Superclass::ReadVector(vfp, extent, wextent, numDims, vector, offset);
    }

  vtkMPIOpaqueFileHandle* handle = reinterpret_cast<vtkMPIOpaqueFileHandle*>(vfp);
  assert(handle);

  vtkMPICommunicatorOpaqueComm* comm =
    vtkMPICommunicator::SafeDownCast(this->Controller->GetCommunicator())->GetMPIComm();
  assert(comm);

  SetupFileViewFunctor work(*comm, *handle, extent, wextent, offset, this->Internal->Settings.ByteOrder);
  vtkDataArrayDispatcher<SetupFileViewFunctor> dispatcher(work);
  try
    {
    dispatcher.Go(vector);
    }
  catch (MPIPlot3DException)
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkMPIMultiBlockPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseMPIIO: " << this->UseMPIIO << endl;
}
