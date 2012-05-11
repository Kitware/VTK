/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageReader2.h"

#include "vtkByteSwap.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkErrorCode.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <sys/stat.h>

vtkStandardNewMacro(vtkImageReader2);

#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif

//----------------------------------------------------------------------------
vtkImageReader2::vtkImageReader2()
{
  this->FilePrefix = NULL;
  this->FilePattern = new char[strlen("%s.%d") + 1];
  strcpy (this->FilePattern, "%s.%d");
  this->File = NULL;

  this->DataScalarType = VTK_SHORT;
  this->NumberOfScalarComponents = 1;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;

  this->DataSpacing[0] = this->DataSpacing[1] = this->DataSpacing[2] = 1.0;

  this->DataExtent[0] = this->DataExtent[2] = this->DataExtent[4] = 0;
  this->DataExtent[1] = this->DataExtent[3] = this->DataExtent[5] = 0;

  this->DataIncrements[0] = this->DataIncrements[1] =
  this->DataIncrements[2] = this->DataIncrements[3] = 1;

  this->FileNames = NULL;

  this->FileName = NULL;
  this->InternalFileName = NULL;

  this->HeaderSize = 0;
  this->ManualHeaderSize = 0;

  this->FileNameSliceOffset = 0;
  this->FileNameSliceSpacing = 1;

  // Left over from short reader
  this->SwapBytes = 0;
  this->FileLowerLeft = 0;
  this->FileDimensionality = 2;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkImageReader2::~vtkImageReader2()
{
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }

  if (this->FileNames)
    {
    this->FileNames->Delete();
    this->FileNames = NULL;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    this->FilePattern = NULL;
    }
  if (this->InternalFileName)
    {
    delete [] this->InternalFileName;
    this->InternalFileName = NULL;
    }
}

//----------------------------------------------------------------------------
// This function sets the name of the file.
void vtkImageReader2::ComputeInternalFileName(int slice)
{
  // delete any old filename
  if (this->InternalFileName)
    {
    delete [] this->InternalFileName;
    this->InternalFileName = NULL;
    }

  if (!this->FileName && !this->FilePattern && !this->FileNames)
    {
    vtkErrorMacro(<<"Either a FileName, FileNames, or FilePattern"
                  <<" must be specified.");
    return;
    }

  // make sure we figure out a filename to open
  if (this->FileNames)
    {
    const char *filename = this->FileNames->GetValue(slice);
    this->InternalFileName = new char [strlen(filename) + 10];
    sprintf(this->InternalFileName,"%s",filename);
    }
  else if (this->FileName)
    {
    this->InternalFileName = new char [strlen(this->FileName) + 10];
    sprintf(this->InternalFileName,"%s",this->FileName);
    }
  else
    {
    int slicenum =
      slice * this->FileNameSliceSpacing
      + this->FileNameSliceOffset;
    if (this->FilePrefix && this->FilePattern)
      {
      this->InternalFileName = new char [strlen(this->FilePrefix) +
                                        strlen(this->FilePattern) + 10];
      sprintf (this->InternalFileName, this->FilePattern,
               this->FilePrefix, slicenum);
      }
    else if (this->FilePattern)
      {
      this->InternalFileName = new char [strlen(this->FilePattern) + 10];
      int len = static_cast<int>(strlen(this->FilePattern));
      int hasPercentS = 0;
      for(int i =0; i < len-1; ++i)
        {
        if(this->FilePattern[i] == '%' && this->FilePattern[i+1] == 's')
          {
          hasPercentS = 1;
          break;
          }
        }
      if(hasPercentS)
        {
        sprintf (this->InternalFileName, this->FilePattern, "", slicenum);
        }
      else
        {
        sprintf (this->InternalFileName, this->FilePattern, slicenum);
        }
      }
    else
      {
      delete [] this->InternalFileName;
      this->InternalFileName = 0;
      }
    }
}


//----------------------------------------------------------------------------
// This function sets the name of the file.
void vtkImageReader2::SetFileName(const char *name)
{
  if ( this->FileName && name && (!strcmp(this->FileName,name)))
    {
    return;
    }
  if (!name && !this->FileName)
    {
    return;
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  if (name)
    {
    this->FileName = new char[strlen(name) + 1];
    strcpy(this->FileName, name);

    if (this->FilePrefix)
      {
      delete [] this->FilePrefix;
      this->FilePrefix = NULL;
      }
    if (this->FileNames)
      {
      this->FileNames->Delete();
      this->FileNames = NULL;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets an array containing file names
void vtkImageReader2::SetFileNames(vtkStringArray *filenames)
{
  if (filenames == this->FileNames)
    {
    return;
    }
  if (this->FileNames)
    {
    this->FileNames->Delete();
    this->FileNames = 0;
    }
  if (filenames)
    {
    this->FileNames = filenames;
    this->FileNames->Register(this);
    if (this->FileNames->GetNumberOfValues() > 0)
      {
      this->DataExtent[4] = 0;
      this->DataExtent[5] = this->FileNames->GetNumberOfValues() - 1;
      }
    if (this->FilePrefix)
      {
      delete [] this->FilePrefix;
      this->FilePrefix = NULL;
      }
    if (this->FileName)
      {
      delete [] this->FileName;
      this->FileName = NULL;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageReader2::SetFilePrefix(const char *prefix)
{
  if ( this->FilePrefix && prefix && (!strcmp(this->FilePrefix,prefix)))
    {
    return;
    }
  if (!prefix && !this->FilePrefix)
    {
    return;
    }
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }
  if (prefix)
    {
    this->FilePrefix = new char[strlen(prefix) + 1];
    strcpy(this->FilePrefix, prefix);

    if (this->FileName)
      {
      delete [] this->FileName;
      this->FileName = NULL;
      }
    if (this->FileNames)
      {
      this->FileNames->Delete();
      this->FileNames = NULL;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%03d" would be the
// pattern of a series: image.001, image.002 ...
void vtkImageReader2::SetFilePattern(const char *pattern)
{
  if ( this->FilePattern && pattern &&
       (!strcmp(this->FilePattern,pattern)))
    {
    return;
    }
  if (!pattern && !this->FilePattern)
    {
    return;
    }
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    this->FilePattern = NULL;
    }
  if (pattern)
    {
    this->FilePattern = new char[strlen(pattern) + 1];
    strcpy(this->FilePattern, pattern);

    if (this->FileName)
      {
      delete [] this->FileName;
      this->FileName = NULL;
      }
    if (this->FileNames)
      {
      this->FileNames->Delete();
      this->FileNames = NULL;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageReader2::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkImageReader2::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

//----------------------------------------------------------------------------
void vtkImageReader2::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
    {
    this->SetDataByteOrderToBigEndian();
    }
  else
    {
    this->SetDataByteOrderToLittleEndian();
    }
}

//----------------------------------------------------------------------------
int vtkImageReader2::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
#else
  if ( this->SwapBytes )
    {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
    }
  else
    {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
    }
#endif
}

//----------------------------------------------------------------------------
const char *vtkImageReader2::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
#else
  if ( this->SwapBytes )
    {
    return "BigEndian";
    }
  else
    {
    return "LittleEndian";
    }
#endif
}


//----------------------------------------------------------------------------
void vtkImageReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;

  this->Superclass::PrintSelf(os,indent);

  // this->File, this->Colors need not be printed
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "FileNames: " << this->FileNames << "\n";
  os << indent << "FilePrefix: " <<
    (this->FilePrefix ? this->FilePrefix : "(none)") << "\n";
  os << indent << "FilePattern: " <<
    (this->FilePattern ? this->FilePattern : "(none)") << "\n";

  os << indent << "FileNameSliceOffset: "
     << this->FileNameSliceOffset << "\n";
  os << indent << "FileNameSliceSpacing: "
     << this->FileNameSliceSpacing << "\n";

  os << indent << "DataScalarType: "
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "NumberOfScalarComponents: "
     << this->NumberOfScalarComponents << "\n";

  os << indent << "File Dimensionality: " << this->FileDimensionality << "\n";

  os << indent << "File Lower Left: " <<
    (this->FileLowerLeft ? "On\n" : "Off\n");

  os << indent << "Swap Bytes: " << (this->SwapBytes ? "On\n" : "Off\n");

  os << indent << "DataIncrements: (" << this->DataIncrements[0];
  for (idx = 1; idx < 2; ++idx)
    {
    os << ", " << this->DataIncrements[idx];
    }
  os << ")\n";

  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DataExtent[idx];
    }
  os << ")\n";

  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataSpacing[idx];
    }
  os << ")\n";

  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->DataOrigin[idx];
    }
  os << ")\n";

  os << indent << "HeaderSize: " << this->HeaderSize << "\n";

  if ( this->InternalFileName )
    {
    os << indent << "Internal File Name: " << this->InternalFileName << "\n";
    }
  else
    {
    os << indent << "Internal File Name: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkImageReader2::ExecuteInformation()
{
  // this is empty, the idea is that converted filters should implement
  // RequestInformation. But to help out old filters we will call
  // ExecuteInformation and hope that the subclasses correctly set the ivars
  // and not the output.
}

//----------------------------------------------------------------------------
// This method returns the largest data that can be generated.
int vtkImageReader2::RequestInformation (
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * outputVector)
{
  this->SetErrorCode( vtkErrorCode::NoError );
  // call for backwards compatibility
  this->ExecuteInformation();
  // Check for any error set by downstream filter (IO in most case)
  if ( this->GetErrorCode() )
    {
    return 0;
    }

  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // if a list of file names is supplied, set slice extent
  if (this->FileNames && this->FileNames->GetNumberOfValues() > 0)
    {
    this->DataExtent[4] = 0;
    this->DataExtent[5] = this->FileNames->GetNumberOfValues()-1;
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->DataExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(),  this->DataOrigin, 3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->DataScalarType,
    this->NumberOfScalarComponents);
  return 1;
}

//----------------------------------------------------------------------------
// Manual initialization.
void vtkImageReader2::SetHeaderSize(unsigned long size)
{
  if (size != this->HeaderSize)
    {
    this->HeaderSize = size;
    this->Modified();
    }
  this->ManualHeaderSize = 1;
}


//----------------------------------------------------------------------------
template <class T>
unsigned long vtkImageReader2GetSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkImageReader2::ComputeDataIncrements()
{
  int idx;
  unsigned long fileDataLength;

  // Determine the expected length of the data ...
  switch (this->DataScalarType)
    {
    vtkTemplateMacro(
      fileDataLength = vtkImageReader2GetSize(static_cast<VTK_TT*>(0))
      );
    default:
      vtkErrorMacro(<< "Unknown DataScalarType");
      return;
    }

  fileDataLength *= this->NumberOfScalarComponents;

  // compute the fileDataLength (in units of bytes)
  for (idx = 0; idx < 3; ++idx)
    {
    this->DataIncrements[idx] = fileDataLength;
    fileDataLength = fileDataLength *
      (this->DataExtent[idx*2+1] - this->DataExtent[idx*2] + 1);
    }
  this->DataIncrements[3] = fileDataLength;
}


//----------------------------------------------------------------------------
int vtkImageReader2::OpenFile()
{
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName, FileNames, or FilePattern"
                  << " must be specified.");
    return 0;
    }

  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }

  // Open the new file
  vtkDebugMacro(<< "Initialize: opening file " << this->InternalFileName);
  struct stat fs;
  if ( !stat( this->InternalFileName, &fs) )
    {
#ifdef _WIN32
    this->File = new ifstream(this->InternalFileName, ios::in | ios::binary);
#else
    this->File = new ifstream(this->InternalFileName, ios::in);
#endif
    }
  if (! this->File || this->File->fail())
    {
    vtkErrorMacro(<< "Initialize: Could not open file "
                  << this->InternalFileName);
    return 0;
    }
  return 1;
}


//----------------------------------------------------------------------------
unsigned long vtkImageReader2::GetHeaderSize()
{
  unsigned long firstIdx;

  if (this->FileNames)
    {
    // if FileNames is used, indexing always starts at zero
    firstIdx = 0;
    }
  else
    {
    // FilePrefix uses the DataExtent to figure out the first slice index
    firstIdx = this->DataExtent[4];
    }

  return this->GetHeaderSize(firstIdx);
}

//----------------------------------------------------------------------------
unsigned long vtkImageReader2::GetHeaderSize(unsigned long idx)
{
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return 0;
    }
  if ( ! this->ManualHeaderSize)
    {
    this->ComputeDataIncrements();

    // make sure we figure out a filename to open
    this->ComputeInternalFileName(idx);

    struct stat statbuf;
    if (!stat(this->InternalFileName, &statbuf))
      {
      return (int)(statbuf.st_size -
                   (long)this->DataIncrements[this->GetFileDimensionality()]);
      }
    }

  return this->HeaderSize;
}

//----------------------------------------------------------------------------
void vtkImageReader2::SeekFile(int i, int j, int k)
{
  unsigned long streamStart;

  // convert data extent into constants that can be used to seek.
  streamStart =
    (i - this->DataExtent[0]) * this->DataIncrements[0];

  if (this->FileLowerLeft)
    {
    streamStart = streamStart +
      (j - this->DataExtent[2]) * this->DataIncrements[1];
    }
  else
    {
    streamStart = streamStart +
      (this->DataExtent[3] - this->DataExtent[2] - j) *
      this->DataIncrements[1];
    }

  // handle three and four dimensional files
  if (this->GetFileDimensionality() >= 3)
    {
    streamStart = streamStart +
      (k - this->DataExtent[4]) * this->DataIncrements[2];
    }

  streamStart += this->GetHeaderSize(k);

  // error checking
  if (!this->File)
    {
    vtkWarningMacro(<<"File must be specified.");
    return;
    }

  this->File->seekg((long)streamStart, ios::beg);
  if (this->File->fail())
    {
    vtkWarningMacro("File operation failed.");
    return;
    }
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkImageReader2Update(vtkImageReader2 *self, vtkImageData *data, OT *outPtr)
{
  vtkIdType outIncr[3];
  OT *outPtr1, *outPtr2;
  long streamRead;
  int idx1, idx2, nComponents;
  int outExtent[6];
  unsigned long count = 0;
  unsigned long target;

  // Get the requested extents and increments
  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);
  nComponents = data->GetNumberOfScalarComponents();

  // length of a row, num pixels read at a time
  int pixelRead = outExtent[1] - outExtent[0] + 1;
  streamRead = (long)(pixelRead*nComponents*sizeof(OT));

  // create a buffer to hold a row of the data
  target = (unsigned long)((outExtent[5]-outExtent[4]+1)*
                           (outExtent[3]-outExtent[2]+1)/50.0);
  target++;

  // read the data row by row
  if (self->GetFileDimensionality() == 3)
    {
    self->ComputeInternalFileName(0);
    if ( !self->OpenFile() )
      {
      return;
      }
    }
  outPtr2 = outPtr;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
    {
    if (self->GetFileDimensionality() == 2)
      {
      self->ComputeInternalFileName(idx2);
      if ( !self->OpenFile() )
        {
        return;
        }
      }
    outPtr1 = outPtr2;
    for (idx1 = outExtent[2];
         !self->AbortExecute && idx1 <= outExtent[3]; ++idx1)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;

      // seek to the correct row
      self->SeekFile(outExtent[0],idx1,idx2);
      // read the row.
      if ( !self->GetFile()->read((char *)outPtr1, streamRead))
        {
        vtkGenericWarningMacro("File operation failed. row = " << idx1
                               << ", Read = " << streamRead
                               << ", FilePos = " << static_cast<vtkIdType>(self->GetFile()->tellg()));
        return;
        }
      // handle swapping
      if (self->GetSwapBytes() && sizeof(OT) > 1)
        {
        vtkByteSwap::SwapVoidRange(outPtr1, pixelRead*nComponents, sizeof(OT));
        }
      outPtr1 += outIncr[1];
      }
    // move to the next image in the file and data
    outPtr2 += outIncr[2];
    }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageReader2::ExecuteDataWithInformation(vtkDataObject *output,
                                                 vtkInformation *outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  void *ptr;

  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro("Either a valid FileName or FilePattern must be specified.");
    return;
    }

  data->GetPointData()->GetScalars()->SetName("ImageFile");

#ifndef NDEBUG
  int *ext = data->GetExtent();
#endif

  vtkDebugMacro("Reading extent: " << ext[0] << ", " << ext[1] << ", "
        << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]);

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  ptr = data->GetScalarPointer();
  switch (this->GetDataScalarType())
    {
    vtkTemplateMacro(vtkImageReader2Update(this, data, (VTK_TT *)(ptr)));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }
}


//----------------------------------------------------------------------------
// Set the data type of pixels in the file.
// If you want the output scalar type to have a different value, set it
// after this method is called.
void vtkImageReader2::SetDataScalarType(int type)
{
  if (type == this->DataScalarType)
    {
    return;
    }

  this->Modified();
  this->DataScalarType = type;
  // Set the default output scalar type
  vtkImageData::SetScalarType(this->DataScalarType,
                              this->GetOutputInformation(0));
}
