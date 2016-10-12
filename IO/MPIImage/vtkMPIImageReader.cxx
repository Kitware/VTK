// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPIImageReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkMPIImageReader.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"

// Include the MPI headers and then determine if MPIIO is available.
#include "vtkMPI.h"

#ifdef MPI_VERSION
#  if (MPI_VERSION >= 2)
#    define VTK_USE_MPI_IO 1
#  endif
#endif
#if !defined(VTK_USE_MPI_IO) && defined(ROMIO_VERSION)
#  define VTK_USE_MPI_IO 1
#endif
#if !defined(VTK_USE_MPI_IO) && defined(MPI_SEEK_SET)
#  define VTK_USE_MPI_IO 1
#endif

// If VTK_USE_MPI_IO is set, that means we will read the data ourself using
// MPIIO.  Otherwise, just delegate everything to the superclass.

#ifdef VTK_USE_MPI_IO

// We only need these includes if we are actually loading the data.
#include "vtkByteSwap.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkType.h"
#include <algorithm>

// This macro can be wrapped around MPI function calls to easily report errors.
// Reporting errors is more important with file I/O because, unlike network I/O,
// they usually don't terminate the program.
#define MPICall(funcall) \
  { \
  int __my_result = funcall; \
  if (__my_result != MPI_SUCCESS) \
  { \
    char errormsg[MPI_MAX_ERROR_STRING]; \
    int dummy; \
    MPI_Error_string(__my_result, errormsg, &dummy); \
    vtkErrorMacro(<< "Received error when calling" << endl \
                  << #funcall << endl << endl \
                  << errormsg); \
  } \
  }

#endif // VTK_USE_MPI_IO

//=============================================================================
vtkStandardNewMacro(vtkMPIImageReader);

vtkCxxSetObjectMacro(vtkMPIImageReader, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIImageReader, GroupedController,
                     vtkMultiProcessController);

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
template<class T>
inline void vtkMPIImageReaderMaskBits(T *data, vtkIdType length,
                                      vtkTypeUInt64 _mask)
{
  T mask = (T)_mask;

  // If the mask is the identity, just return.
  if ((_mask == (vtkTypeUInt64)~0UL) || (mask == (T)~0) || (_mask == 0)) return;

  for (vtkIdType i = 0; i < length; i++)
  {
    data[i] &= mask;
  }
}

// Override float and double because masking bits for them makes no sense.
template<>
void vtkMPIImageReaderMaskBits(float *, vtkIdType, vtkTypeUInt64)
{
  return;
}
template<>
void vtkMPIImageReaderMaskBits(double *, vtkIdType, vtkTypeUInt64)
{
  return;
}
#endif //VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
namespace {
  template<class T>
  inline T MY_ABS(T x) { return (x < 0) ? -x : x; }

  template<class T>
  inline T MY_MIN(T x, T y) { return (x < y) ? x : y; }
};
#endif //VTK_USE_MPI_IO

//=============================================================================
vtkMPIImageReader::vtkMPIImageReader()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->GroupedController = NULL;
}

vtkMPIImageReader::~vtkMPIImageReader()
{
  this->SetController(NULL);
  this->SetGroupedController(NULL);
}

void vtkMPIImageReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
int vtkMPIImageReader::GetDataScalarTypeSize()
{
  switch (this->GetDataScalarType())
  {
    vtkTemplateMacro(return sizeof(VTK_TT));
    default:
      vtkErrorMacro("Unknown data type.");
      return 0;
  }
}

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
void vtkMPIImageReader::PartitionController(const int extent[6])
{
  // Number of points in the z direction of the whole data.
  int numZ = this->DataExtent[5] - this->DataExtent[4] + 1;

  if ((this->GetFileDimensionality() == 3) || (numZ == 1))
  {
    // Everyone reads from the same single file.  No need to partion controller.
    this->SetGroupedController(this->Controller);
    return;
  }

  // The following algorithm will have overflow problems if there are more
  // than 2^15 files.  I doubt anyone will ever be crazy enough to set up a
  // large 3D image with that many slice files, but just in case...
  if (numZ >= 32768)
  {
    vtkErrorMacro("I do not support more than 32768 files.");
    return;
  }

  // Hash the Z extent.  This is guaranteed to be unique for any pair of
  // extents (within the constraint given above).
  int extentHash = (  extent[4]+this->DataExtent[4]
                    + (extent[5]+this->DataExtent[4])*numZ );

  vtkMultiProcessController *subController
    = this->Controller->PartitionController(extentHash, 0);
  this->SetGroupedController(subController);
  subController->Delete();
}
#else // VTK_USE_MPI_IO
void vtkMPIImageReader::PartitionController(const int *)
{
  vtkErrorMacro(<< "vtkMPIImageReader::PartitionController() called when MPIIO "
                << "not available.");
}
#endif // VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
// Technically we should be returning a 64 bit number, but I doubt any header
// will be bigger than the value stored in an unsigned int.  Thus, we just
// follow the convention of the superclass.
#ifdef VTK_USE_MPI_IO
unsigned long vtkMPIImageReader::GetHeaderSize(vtkMPIOpaqueFileHandle &file)
{
  if (this->ManualHeaderSize)
  {
    return this->HeaderSize;
  }
  else
  {
    this->ComputeDataIncrements();

    MPI_Offset size;
    MPICall(MPI_File_get_size(file.Handle, &size));
    return static_cast<unsigned long>
      (size - this->DataIncrements[this->GetFileDimensionality()]);
  }
}
#else // VTK_USE_MPI_IO
unsigned long vtkMPIImageReader::GetHeaderSize(vtkMPIOpaqueFileHandle &)
{
  vtkErrorMacro(<< "vtkMPIImageReader::GetHeaderSize() called when MPIIO "
                << "not available.");
  return 0;
}
#endif // VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
void vtkMPIImageReader::SetupFileView(vtkMPIOpaqueFileHandle &file,
                                      const int extent[6])
{
  int arrayOfSizes[3];
  int arrayOfSubSizes[3];
  int arrayOfStarts[3];

  for (int i = 0; i < this->GetFileDimensionality(); i++)
  {
    arrayOfSizes[i] = this->DataExtent[i*2+1] - this->DataExtent[i*2] + 1;
    arrayOfSubSizes[i] = extent[i*2+1] - extent[i*2] + 1;
    arrayOfStarts[i] = extent[i*2];
  }
  // Adjust for base size of data type and tuple size.
  int baseSize = this->GetDataScalarTypeSize() * this->NumberOfScalarComponents;
  arrayOfSizes[0] *= baseSize;
  arrayOfSubSizes[0] *= baseSize;
  arrayOfStarts[0] *= baseSize;

  // Create a view in MPIIO.
  MPI_Datatype view;
  MPICall(MPI_Type_create_subarray(this->GetFileDimensionality(),
                                   arrayOfSizes, arrayOfSubSizes, arrayOfStarts,
                                   MPI_ORDER_FORTRAN, MPI_BYTE, &view));
  MPICall(MPI_Type_commit(&view));
  MPICall(MPI_File_set_view(file.Handle, this->GetHeaderSize(file), MPI_BYTE,
                            view, const_cast<char *>("native"), MPI_INFO_NULL));
  MPICall(MPI_Type_free(&view));
}
#else // VTK_USE_MPI_IO
void vtkMPIImageReader::SetupFileView(vtkMPIOpaqueFileHandle &, const int[6])
{
  vtkErrorMacro(<< "vtkMPIImageReader::SetupFileView() called when MPIIO "
                << "not available.");
}
#endif // VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
void vtkMPIImageReader::ReadSlice(int slice, const int extent[6], void *buffer)
{
  this->ComputeInternalFileName(slice);

  vtkMPICommunicator *mpiComm = vtkMPICommunicator::SafeDownCast(
                                    this->GroupedController->GetCommunicator());

  // Open the file for this slice.
  vtkMPIOpaqueFileHandle file;
  int result;
  result = MPI_File_open(*mpiComm->GetMPIComm()->GetHandle(),
                         this->InternalFileName, MPI_MODE_RDONLY,
                         MPI_INFO_NULL, &file.Handle);
  if (!(result == MPI_SUCCESS))
  {
    vtkErrorMacro("Could not open file: " << this->InternalFileName);
    return;
  }

  // Set up the file view based on the extents.
  this->SetupFileView(file, extent);

  // Figure out how many bytes to read.
  vtkIdType length = this->GetDataScalarTypeSize();
  length *= this->NumberOfScalarComponents;
  length *= extent[1]-extent[0]+1;
  length *= extent[3]-extent[2]+1;
  if (this->GetFileDimensionality() == 3) length *= extent[5]-extent[4]+1;

  vtkIdType pos = 0;
  while (length > pos)
  {
    MPI_Status stat;
    // we know this will fit in an int because it can't exceed VTK_INT_MAX.
   const int remaining = static_cast<int>(std::min(length - pos,
                                  static_cast<vtkIdType>(VTK_INT_MAX)));
   MPICall(MPI_File_read(file.Handle, (static_cast<char*>(buffer)) + pos, remaining,
                         MPI_BYTE, &stat));
   int rd = 0;
   MPICall(MPI_Get_elements(&stat, MPI_BYTE, &rd));
   if (MPI_UNDEFINED == rd)
   {
      vtkErrorMacro("Error obtaining number of values read in " << remaining <<
      "-byte read.");
   }
      pos += static_cast<vtkIdType>(rd);
  }

  MPICall(MPI_File_close(&file.Handle));
}
#else // VTK_USE_MPI_IO
void vtkMPIImageReader::ReadSlice(int, const int [6], void *)
{
  vtkErrorMacro(<< "vtkMPIImageReader::ReadSlice() called with MPIIO "
                << "not available.");
}
#endif // VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI_IO
// This method could be made a lot more efficient.
void vtkMPIImageReader::TransformData(vtkImageData *data)
{
  if (!this->Transform) return;

  vtkDataArray *fileData = data->GetPointData()->GetScalars();
  vtkDataArray *dataData = fileData->NewInstance();
  dataData->SetName(fileData->GetName());
  dataData->SetNumberOfComponents(fileData->GetNumberOfComponents());
  dataData->SetNumberOfTuples(fileData->GetNumberOfTuples());

  int dataExtent[6];
  data->GetExtent(dataExtent);

  int fileExtent[6];
  this->ComputeInverseTransformedExtent(dataExtent, fileExtent);

  vtkIdType dataMinExtent[3];
  vtkIdType fileMinExtent[3];
  vtkIdType dataExtentSize[3];
  vtkIdType fileExtentSize[3];
  for (int i = 0; i < 3; i++)
  {
    dataMinExtent[i] = MY_MIN(dataExtent[2*i], dataExtent[2*i+1]);
    fileMinExtent[i] = MY_MIN(fileExtent[2*i], fileExtent[2*i+1]);
    dataExtentSize[i] = MY_ABS(dataExtent[2*i+1] - dataExtent[2*i]) + 1;
    fileExtentSize[i] = MY_ABS(fileExtent[2*i+1] - fileExtent[2*i]) + 1;
  }

  for (vtkIdType file_k = 0; file_k < fileExtentSize[2]; file_k++)
  {
    for (vtkIdType file_j = 0; file_j < fileExtentSize[1]; file_j++)
    {
      for (vtkIdType file_i = 0; file_i < fileExtentSize[0]; file_i++)
      {
        double fileXYZ[3];
        fileXYZ[0] = file_i + fileMinExtent[0];
        fileXYZ[1] = file_j + fileMinExtent[1];
        fileXYZ[2] = file_k + fileMinExtent[2];
        double dataXYZ[3];
        this->Transform->TransformPoint(fileXYZ, dataXYZ);
        vtkIdType data_i = static_cast<vtkIdType>(dataXYZ[0])-dataMinExtent[0];
        vtkIdType data_j = static_cast<vtkIdType>(dataXYZ[1])-dataMinExtent[1];
        vtkIdType data_k = static_cast<vtkIdType>(dataXYZ[2])-dataMinExtent[2];

        vtkIdType fileTuple
          = ((file_k*fileExtentSize[1] + file_j)*fileExtentSize[0]) + file_i;
        vtkIdType dataTuple
          = ((data_k*dataExtentSize[1] + data_j)*dataExtentSize[0]) + data_i;

        dataData->SetTuple(dataTuple, fileTuple, fileData);
      }
    }
  }

  data->GetPointData()->SetScalars(dataData);
  dataData->Delete();
}
#else // VTK_USE_MPI_IO
void vtkMPIImageReader::TransformData(vtkImageData *)
{
  vtkErrorMacro(<< "vtkMPIImageReader::TransformData() called with MPIIO "
                << "not available.");
}
#endif // VTK_USE_MPI_IO

//-----------------------------------------------------------------------------
void vtkMPIImageReader::ExecuteDataWithInformation(vtkDataObject *output,
                                                   vtkInformation *outInfo)
{
#ifdef VTK_USE_MPI_IO
  vtkMPIController *MPIController
    = vtkMPIController::SafeDownCast(this->Controller);
  if (!MPIController)
  {
    this->Superclass::ExecuteDataWithInformation(output, outInfo);
    return;
  }

  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  if (!this->FileName && !this->FilePattern && !this->FileNames)
  {
    vtkErrorMacro("Either a valid FileName, FileNames, or FilePattern"
                  " must be specified.");
    return;
  }

  // VTK stores images in traditional "right handed" coordinates.  That is, the
  // origin is in the lower left corner.  Many images, especially those with RGB
  // colors, have the origin in the upper right corner.  In this case, we have
  // to flip the y axis.
  vtkTransform *saveTransform = this->Transform;
  if (!this->FileLowerLeft)
  {
    vtkTransform *newTransform = vtkTransform::New();
    if (this->Transform)
    {
      newTransform->Concatenate(this->Transform);
    }
    else
    {
      newTransform->Identity();
    }
    newTransform->Scale(1.0, -1.0, 1.0);
    this->Transform = newTransform;
  }

  // Get information on data partion requested.
  int inExtent[6];
  vtkIdType inIncrements[3];
  data->GetExtent(inExtent);
  data->GetIncrements(inIncrements);
  vtkDataArray *outputDataArray = data->GetPointData()->GetScalars();
  vtkIdType numValues = (  outputDataArray->GetNumberOfComponents()
                         * outputDataArray->GetNumberOfTuples() );

  outputDataArray->SetName(this->ScalarArrayName);

  vtkDebugMacro("Reading extent: "
                << inExtent[0] << ", " << inExtent[1] << ", "
                << inExtent[2] << ", " << inExtent[3] << ", "
                << inExtent[4] << ", " << inExtent[5]);

  // Respect the Transform.
  int outExtent[6];
  vtkIdType outIncrements[3];
  this->ComputeInverseTransformedExtent(inExtent, outExtent);

  // The superclass' ComputeInverseTransformedIncrements does not give us
  // increments we can use.  It just reorders the inIncrements, (offsets in the
  // target data structure).  This does not give us valid offsets for the file.
  // Instead, we just recompute them.
  //this->ComputeInverseTransformedIncrements(inIncrements, outIncrements);
  outIncrements[0] = inIncrements[0];
  outIncrements[1] = outIncrements[0]*(MY_ABS(outExtent[1]-outExtent[0])+1);
  outIncrements[2] = outIncrements[1]*(MY_ABS(outExtent[3]-outExtent[2])+1);

  this->ComputeDataIncrements();

  // Get information on data type.
  int typeSize = this->GetDataScalarTypeSize();

  // Group processes based on which files they read.
  this->PartitionController(outExtent);

  // Get the pointer to the data buffer.  Don't worry.  We support all the
  // data types.  I am just casting it to a char (byte) so that I can do
  // byte arithmetic on the data.
  char *dataBuffer = reinterpret_cast<char *>(data->GetScalarPointer());

  if (this->GetFileDimensionality() == 3)
  {
    // Everything is in one big file.  Read it all in one shot.
    this->ReadSlice(0, outExtent, dataBuffer);
  }
  else // this->GetFileDimensionality() == 2
  {
    // Read everything slice-by-slice.
    char *ptr = dataBuffer;
    for (int slice = outExtent[4]; slice <= outExtent[5]; slice++)
    {
      this->UpdateProgress(  (0.9*(slice-outExtent[4]))
                           / (outExtent[5]-outExtent[4]+1));
      this->ReadSlice(slice, outExtent, ptr);
      ptr += typeSize*outIncrements[2];
    }
  }

  this->UpdateProgress(0.9);

  // Swap bytes as necessary.
  if (this->GetSwapBytes() && typeSize > 1)
  {
    vtkByteSwap::SwapVoidRange(dataBuffer, numValues, typeSize);
  }

  // Mask bits as necessary.
  switch (this->GetDataScalarType())
  {
    vtkTemplateMacro(vtkMPIImageReaderMaskBits((VTK_TT *)dataBuffer, numValues,
                                               this->DataMask));
  }

  // Perform permutation transformation of data if necessary.
  this->TransformData(data);

  if (!this->FileLowerLeft)
  {
    this->Transform->Delete();
    this->Transform = saveTransform;
  }

  // Done with this for now.
  this->SetGroupedController(NULL);
#else // VTK_USE_MPI_IO
  this->Superclass::ExecuteDataWithInformation(output, outInfo);
#endif // VTK_USE_MPI_IO
}
