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
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
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
  DEFINE_MPI_TYPE(long long, MPI_LONG_LONG);
  DEFINE_MPI_TYPE(unsigned long long, MPI_UNSIGNED_LONG_LONG);

  class MPIPlot3DException : public std::exception
  {
  };

  template <class DataType>
  class vtkMPIPLOT3DArrayReader
  {
  public:
    vtkMPIPLOT3DArrayReader() : ByteOrder(
      vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
    {
    }

    vtkIdType ReadScalar(void* vfp,
      vtkTypeUInt64 offset,
      vtkIdType preskip,
      vtkIdType n,
      vtkIdType vtkNotUsed(postskip),
      DataType* scalar,
      const vtkMultiBlockPLOT3DReaderRecord& record = vtkMultiBlockPLOT3DReaderRecord())
    {
      vtkMPIOpaqueFileHandle* fp = reinterpret_cast<vtkMPIOpaqueFileHandle*>(vfp);
      assert(fp);

      // skip preskip if we're setting over subrecord separators.
      offset += record.GetLengthWithSeparators(offset, preskip*sizeof(DataType));

      // Let's see if we encounter markers while reading the data from current
      // position.
      std::vector<std::pair<vtkTypeUInt64, vtkTypeUInt64> > chunks =
        record.GetChunksToRead(offset, sizeof(DataType) * n);

      const int dummy_INT_MAX = 2e9; /// XXX: arbitrary limit that seems
                                     /// to work when reading large files.
      vtkIdType bytesread = 0;
      for (size_t cc=0; cc < chunks.size(); cc++)
      {
        vtkTypeUInt64 start = chunks[cc].first;
        vtkTypeUInt64 length = chunks[cc].second;
        while (length > 0)
        {
          int segment = length > static_cast<vtkTypeUInt64>(dummy_INT_MAX)?
            (length - dummy_INT_MAX) : static_cast<int>(length);

          MPI_Status status;
          if (MPI_File_read_at(fp->Handle, start,
            reinterpret_cast<char*>(scalar) + bytesread,
            segment, MPI_UNSIGNED_CHAR, &status) != MPI_SUCCESS)
          {
            return 0; // let's assume nothing was read.
          }
          start += segment;
          length -= segment;
          bytesread += segment;
        }
      }

      if (this->ByteOrder == vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
      {
        if (sizeof(DataType) == 4)
        {
          vtkByteSwap::Swap4LERange(scalar, n);
        }
        else
        {
          vtkByteSwap::Swap8LERange(scalar, n);
        }
      }
      else
      {
        if (sizeof(DataType) == 4)
        {
          vtkByteSwap::Swap4BERange(scalar, n);
        }
        else
        {
          vtkByteSwap::Swap8BERange(scalar, n);
        }
      }
      return bytesread / sizeof(DataType);
    }

    vtkIdType ReadVector(void* vfp,
      vtkTypeUInt64 offset,
      int extent[6], int wextent[6],
      int numDims, DataType* vector,
      const vtkMultiBlockPLOT3DReaderRecord &record)
    {
      vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);
      vtkIdType totalN = vtkStructuredData::GetNumberOfPoints(wextent);

      // Setting to 0 in case numDims == 0. We still need to
      // populate an array with 3 components but the code below
      // does not read the 3rd component (it doesn't exist
      // in the file)
      memset(vector, 0, n*3*sizeof(DataType));

      vtkIdType retVal = 0;
      DataType* buffer = new DataType[n];
      for (int component = 0; component < numDims; component++)
      {
        vtkIdType preskip, postskip;
        vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
        vtkIdType valread = this->ReadScalar(vfp, offset, preskip, n, postskip, buffer, record);
        if (valread != n)
        {
          return 0; // failed.
        }
        retVal += valread;
        for (vtkIdType i=0; i<n; i++)
        {
          vector[3*i+component] = buffer[i];
        }
        offset += record.GetLengthWithSeparators(offset, totalN * sizeof(DataType));
      }
      delete[] buffer;
      return retVal;
    }
    int ByteOrder;
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
  vtkDataArray* scalar, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)

{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadIntScalar(vfp, extent, wextent, scalar, offset, record);
  }

  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);
  vtkMPIPLOT3DArrayReader<int> arrayReader;
  arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
  vtkIdType preskip, postskip;
  vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
  vtkIntArray* intArray = static_cast<vtkIntArray*>(scalar);
  return arrayReader.ReadScalar(
    vfp, offset, preskip, n, postskip, intArray->GetPointer(0), record) == n;
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::ReadScalar(
  void* vfp,
  int extent[6], int wextent[6],
  vtkDataArray* scalar, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)
{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadScalar(vfp, extent, wextent, scalar, offset, record);
  }

  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);
  if (this->Internal->Settings.Precision == 4)
  {
    vtkMPIPLOT3DArrayReader<float> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    vtkIdType preskip, postskip;
    vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
    return arrayReader.ReadScalar(
      vfp, offset, preskip, n, postskip, floatArray->GetPointer(0),
      record) == n;
  }
  else
  {
    vtkMPIPLOT3DArrayReader<double> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    vtkIdType preskip, postskip;
    vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
    return arrayReader.ReadScalar(
      vfp, offset, preskip, n, postskip, doubleArray->GetPointer(0),
      record) == n;
  }
}

//----------------------------------------------------------------------------
int vtkMPIMultiBlockPLOT3DReader::ReadVector(
  void* vfp,
  int extent[6], int wextent[6],
  int numDims, vtkDataArray* vector, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)
{
  if (!this->CanUseMPIIO())
  {
    return this->Superclass::ReadVector(vfp, extent, wextent, numDims, vector, offset, record);
  }

  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);
  vtkIdType nValues = n*numDims;
  if (this->Internal->Settings.Precision == 4)
  {
    vtkMPIPLOT3DArrayReader<float> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(vector);
    return arrayReader.ReadVector(
      vfp, offset, extent, wextent, numDims, floatArray->GetPointer(0), record) == nValues;
  }
  else
  {
    vtkMPIPLOT3DArrayReader<double> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(vector);
    return arrayReader.ReadVector(
      vfp, offset, extent, wextent, numDims, doubleArray->GetPointer(0), record) == nValues;
  }
}

//----------------------------------------------------------------------------
void vtkMPIMultiBlockPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseMPIIO: " << this->UseMPIIO << endl;
}
