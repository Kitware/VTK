/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"

#include "vtkImageReader.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageReader* vtkImageReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageReader");
  if(ret)
    {
    return (vtkImageReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageReader;
}




#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif

//----------------------------------------------------------------------------
vtkImageReader::vtkImageReader()
{
  int idx;
  
  this->FilePrefix = NULL;
  this->FilePattern = new char[strlen("%s.%d") + 1];
  strcpy (this->FilePattern, "%s.%d");
  this->File = NULL;

  this->DataScalarType = VTK_SHORT;
  this->NumberOfScalarComponents = 1;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->DataIncrements[idx] = 1;
    this->DataExtent[idx*2] = this->DataExtent[idx*2 + 1] = 0;
    this->DataVOI[idx*2] = this->DataVOI[idx*2 + 1] = 0;
    this->DataSpacing[idx] = 1.0;
    this->DataOrigin[idx] = 0.0;
    }
  this->DataIncrements[3] = 1;
  
  this->FileName = NULL;
  this->InternalFileName = NULL;
  
  this->HeaderSize = 0;
  this->ManualHeaderSize = 0;
  
  // Left over from short reader
  this->DataMask = 0xffff;
  this->SwapBytes = 0;
  this->Transform = NULL;
  this->FileLowerLeft = 0;
  this->FileDimensionality = 2;
}

//----------------------------------------------------------------------------
vtkImageReader::~vtkImageReader()
{ 
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
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
  
  this->SetTransform(NULL);
}

//----------------------------------------------------------------------------
// This function sets the name of the file. 
void vtkImageReader::ComputeInternalFileName(int slice)
{
  // delete any old filename
  if (this->InternalFileName)
    {
    delete [] this->InternalFileName;
    }

  
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
    }

  // make sure we figure out a filename to open
  if (this->FileName)
    {
    this->InternalFileName = new char [strlen(this->FileName) + 10];
    sprintf(this->InternalFileName,"%s",this->FileName);
    }
  else 
    {
    if (this->FilePrefix)
      {
      this->InternalFileName = new char [strlen(this->FilePrefix) +
                                        strlen(this->FilePattern) + 10];
      sprintf (this->InternalFileName, this->FilePattern, 
               this->FilePrefix, slice);
      }
    else
      {
      this->InternalFileName = new char [strlen(this->FilePattern) + 10];
      sprintf (this->InternalFileName, this->FilePattern, slice);
      }
    }
}


//----------------------------------------------------------------------------
// This function sets the name of the file. 
void vtkImageReader::SetFileName(const char *name)
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
    }
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }
  if (name)
    {
    this->FileName = new char[strlen(name) + 1];
    strcpy(this->FileName, name);
    }
  else
    {
    this->FileName = NULL;
    }

  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageReader::SetFilePrefix(const char *prefix)
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
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }  
  this->FilePrefix = new char[strlen(prefix) + 1];
  strcpy(this->FilePrefix, prefix);
  this->Modified();
}

//----------------------------------------------------------------------------
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%3d" would be the
// pattern of a series: image.001, image.002 ...
void vtkImageReader::SetFilePattern(const char *pattern)
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
    }
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  this->FilePattern = new char[strlen(pattern) + 1];
  strcpy(this->FilePattern, pattern);
  this->Modified();
}

void vtkImageReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkImageReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkImageReader::SetDataByteOrder(int byteOrder)
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

int vtkImageReader::GetDataByteOrder()
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

const char *vtkImageReader::GetDataByteOrderAsString()
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
void vtkImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageSource::PrintSelf(os,indent);

  // this->File, this->Colors need not be printed  
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "FilePrefix: " << 
    (this->FilePrefix ? this->FilePrefix : "(none)") << "\n";
  os << indent << "FilePattern: " << 
    (this->FilePattern ? this->FilePattern : "(none)") << "\n";

  os << indent << "DataScalarType: " 
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "NumberOfScalarComponents: " 
     << this->NumberOfScalarComponents << "\n";
 
  os << indent << "Data Mask: " << this->DataMask << "\n";

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
  
  os << indent << "DataVOI: (" << this->DataVOI[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->DataVOI[idx];
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

  if ( this->Transform )
    {
    os << indent << "Transform: " << this->Transform << "\n";
    }
  else
    {
    os << indent << "Transform: (none)\n";
    }

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
// This method returns the largest data that can be generated.
void vtkImageReader::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  float spacing[3];
  int extent[6];
  float origin[3];
  
  
  // set the extent, if the VOI has not been set then default to
  // the DataExtent
  if (this->DataVOI[0] || this->DataVOI[1] || 
      this->DataVOI[2] || this->DataVOI[3] || 
      this->DataVOI[4] || this->DataVOI[5])
    {
    this->ComputeTransformedExtent(this->DataVOI,extent);
    output->SetWholeExtent(extent);
    }
  else
    {
    this->ComputeTransformedExtent(this->DataExtent,extent);
    output->SetWholeExtent(extent);
    }
    
  // set the spacing
  this->ComputeTransformedSpacing(spacing);
  output->SetSpacing(spacing);

  // set the origin.
  this->ComputeTransformedOrigin(origin);
  output->SetOrigin(origin);

  output->SetScalarType(this->DataScalarType);
  output->SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}


//----------------------------------------------------------------------------
// Manual initialization.
void vtkImageReader::SetHeaderSize(unsigned long size)
{
  if (size != this->HeaderSize)
    {
    this->HeaderSize = size;
    this->Modified();
    }
  this->ManualHeaderSize = 1;
}
  

//----------------------------------------------------------------------------
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkImageReader::ComputeDataIncrements()
{
  int idx;
  unsigned long fileDataLength;
  
  // Determine the expected length of the data ...
  switch (this->DataScalarType)
    {
    case VTK_FLOAT:
      fileDataLength = sizeof(float);
      break;
    case VTK_DOUBLE:
      fileDataLength = sizeof(double);
      break;
    case VTK_INT:
      fileDataLength = sizeof(int);
      break;
    case VTK_SHORT:
      fileDataLength = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      fileDataLength = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      fileDataLength = sizeof(unsigned char);
      break;
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


void vtkImageReader::OpenFile()
{
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
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
#ifdef _WIN32
  this->File = new ifstream(this->InternalFileName, ios::in | ios::binary);
#else
  this->File = new ifstream(this->InternalFileName, ios::in);
#endif
  if (! this->File || this->File->fail())
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << 
    this->InternalFileName);
    return;
    }
}


unsigned long vtkImageReader::GetHeaderSize()
{
  return this->GetHeaderSize(this->DataExtent[4]);
}

unsigned long vtkImageReader::GetHeaderSize(int idx)
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
    this->OpenFile();
    
    // Get the size of the header from the size of the image
    this->File->seekg(0,ios::end);
    
    return (unsigned long)((unsigned long)this->File->tellg() - 
      (unsigned long)this->DataIncrements[this->GetFileDimensionality()]);
    }

  return this->HeaderSize;
}

void vtkImageReader::OpenAndSeekFile(int dataExtent[6], int idx)
{
  unsigned long streamStart;

  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
    }
  this->ComputeInternalFileName(idx);
  this->OpenFile();

  // convert data extent into constants that can be used to seek.
  streamStart = 
    (dataExtent[0] - this->DataExtent[0]) * this->DataIncrements[0];
  
  if (this->FileLowerLeft)
    {
    streamStart = streamStart + 
      (dataExtent[2] - this->DataExtent[2]) * this->DataIncrements[1];
    }
  else
    {
    streamStart = streamStart + 
      (this->DataExtent[3] - this->DataExtent[2] - dataExtent[2]) * 
      this->DataIncrements[1];
    }
  
  // handle three and four dimensional files
  if (this->GetFileDimensionality() >= 3)
    {
    streamStart = streamStart + 
      (dataExtent[4] - this->DataExtent[4]) * this->DataIncrements[2];
    }
  
  streamStart += this->GetHeaderSize(idx);
  
  // error checking
  this->File->seekg((long)streamStart, ios::beg);
  if (this->File->fail())
    {
    cerr << "File operation failed: " << streamStart << ", ext: "
		      << dataExtent[0] << ", " << dataExtent[1] << ", "
		      << dataExtent[2] << ", " << dataExtent[3] << ", "
	   << dataExtent[4] << ", " << dataExtent[5] << endl;
    cerr << "Header size: " << this->GetHeaderSize(idx) << ", file ext: "
		      << this->DataExtent[0] << ", " << this->DataExtent[1] << ", "
		      << this->DataExtent[2] << ", " << this->DataExtent[3] << ", "
	 << this->DataExtent[4] << ", " << this->DataExtent[5] << endl;
    return;
    }
	
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class IT, class OT>
static void vtkImageReaderUpdate2(vtkImageReader *self, vtkImageData *data,
				  IT *inPtr, OT *outPtr)
{
  int inIncr[3], outIncr[3];
  OT *outPtr0, *outPtr1, *outPtr2;
  long streamSkip0, streamSkip1;
  long streamRead;
  int idx0, idx1, idx2, pixelRead;
  unsigned char *buf;
  int inExtent[6];
  int dataExtent[6];
  int comp, pixelSkip;
  long filePos, correction = 0;
  unsigned long count = 0;
  unsigned short DataMask;
  unsigned long target;
  
  // Get the requested extents.
  data->GetExtent(inExtent);
  // Convert them into to the extent needed from the file. 
  self->ComputeInverseTransformedExtent(inExtent,dataExtent);
  
  // get and transform the increments
  data->GetIncrements(inIncr);
  self->ComputeInverseTransformedIncrements(inIncr,outIncr);

  DataMask = self->GetDataMask();

  // compute outPtr2 
  outPtr2 = outPtr;
  if (outIncr[0] < 0) 
    {
    outPtr2 = outPtr2 - outIncr[0]*(dataExtent[1] - dataExtent[0]);
    }
  if (outIncr[1] < 0) 
    {
    outPtr2 = outPtr2 - outIncr[1]*(dataExtent[3] - dataExtent[2]);
    }
  if (outIncr[2] < 0) 
    {
    outPtr2 = outPtr2 - outIncr[2]*(dataExtent[5] - dataExtent[4]);
    }

  // length of a row, num pixels read at a time
  pixelRead = dataExtent[1] - dataExtent[0] + 1; 
  streamRead = (long)(pixelRead * self->GetDataIncrements()[0]);  
  streamSkip0 = (long)(self->GetDataIncrements()[1] - streamRead);
  streamSkip1 = (long)(self->GetDataIncrements()[2] - 
    (dataExtent[3] - dataExtent[2] + 1)* self->GetDataIncrements()[1]);
  pixelSkip = data->GetNumberOfScalarComponents();
    
  // read from the bottom up
  if (!self->GetFileLowerLeft()) 
    {
    streamSkip0 = (long)(-streamRead - self->GetDataIncrements()[1]);
    streamSkip1 = (long)(self->GetDataIncrements()[2] + 
      (dataExtent[3] - dataExtent[2] + 1)* self->GetDataIncrements()[1]);
    }
  
    
  // create a buffer to hold a row of the data
  buf = new unsigned char[streamRead];
  
  target = (unsigned long)((dataExtent[5]-dataExtent[4]+1)*
			   (dataExtent[3]-dataExtent[2]+1)/50.0);
  target++;

  // read the data row by row
  if (self->GetFileDimensionality() == 3)
    {
    self->OpenAndSeekFile(dataExtent,0);
    }
  for (idx2 = dataExtent[4]; idx2 <= dataExtent[5]; ++idx2)
    {
    if (self->GetFileDimensionality() == 2)
      {
      self->OpenAndSeekFile(dataExtent,idx2);
      }
    outPtr1 = outPtr2;
    for (idx1 = dataExtent[2]; 
	 !self->AbortExecute && idx1 <= dataExtent[3]; ++idx1)
      {
      if (!(count%target))
	{
	self->UpdateProgress(count/(50.0*target));
	}
      count++;
      outPtr0 = outPtr1;
  
      // read the row.
      if ( ! self->GetFile()->read((char *)buf, streamRead))
	{
	vtkGenericWarningMacro("File operation failed. row = " << idx1
			       << ", Read = " << streamRead
			       << ", Skip0 = " << streamSkip0
			       << ", Skip1 = " << streamSkip1
			       << ", FilePos = " << self->GetFile()->tellg());
	return;
	}

      // handle swapping
      if (self->GetSwapBytes())
	{
	// pixelSkip is the number of components in data
	vtkByteSwap::SwapVoidRange(buf, pixelRead*pixelSkip, sizeof(IT));
	}
      
      // copy the bytes into the typed data
      inPtr = (IT *)(buf);
      for (idx0 = dataExtent[0]; idx0 <= dataExtent[1]; ++idx0)
	{
	// Copy pixel into the output.
	if (DataMask == 0xffff)
	  {
	  for (comp = 0; comp < pixelSkip; comp++)
	    {
	    outPtr0[comp] = (OT)(inPtr[comp]);
	    }
	  }
	else
	  {
	  // left over from short reader (what about other types.
	  for (comp = 0; comp < pixelSkip; comp++)
	    {
	    outPtr0[comp] = (OT)((short)(inPtr[comp]) & DataMask);
	    }
	  }
	// move to next pixel
	inPtr += pixelSkip;
	outPtr0 += outIncr[0];
	}
      // move to the next row in the file and data
      filePos = self->GetFile()->tellg();
      // watch for case where we might rewind too much
      // if that happens, store the value in correction and apply later
      if (filePos + streamSkip0 >= 0)
	{
	self->GetFile()->seekg(self->GetFile()->tellg() + streamSkip0, ios::beg);
	correction = 0;
	}
      else
	{
	correction = streamSkip0;
	}
      outPtr1 += outIncr[1];
      }
    // move to the next image in the file and data
    self->GetFile()->seekg(self->GetFile()->tellg() + streamSkip1 + correction, 
		      ios::beg);
    outPtr2 += outIncr[2];
    }

  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// This function reads in one data of one slice.
// templated to handle different data types.
template <class T>
static void vtkImageReaderUpdate1(vtkImageReader *self, 
				  vtkImageData *data, T *inPtr)
{
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
    {
    vtkTemplateMacro4(vtkImageReaderUpdate2, self, data, inPtr, 
                      (VTK_TT *)(outPtr));
    default:
      vtkGenericWarningMacro("Update1: Unknown data type\n");
    }  
}
//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  
  void *ptr = NULL;
  int *ext;
  
  if (!this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Either a FileName or FilePattern must be specified.");
    return;
    }

  ext = data->GetExtent();

  vtkDebugMacro("Reading extent: " << ext[0] << ", " << ext[1] << ", " 
	<< ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]);
  
  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
    {
    vtkTemplateMacro3(vtkImageReaderUpdate1, this, data, (VTK_TT *)(ptr));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }   
}



//----------------------------------------------------------------------------
// Set the data type of pixles in the file.  
// As a convienience, the OutputScalarType is set to the same value.
// If you want the output scalar type to have a different value, set it
// after this method is called.
void vtkImageReader::SetDataScalarType(int type)
{
  if (type == this->DataScalarType)
    {
    return;
    }
  
  this->Modified();
  this->DataScalarType = type;
  // Set the default output scalar type
  this->GetOutput()->SetScalarType(this->DataScalarType);
}


void vtkImageReader::ComputeTransformedSpacing (float Spacing[3])
{
  if (!this->Transform)
    {
    memcpy (Spacing, this->DataSpacing, 3 * sizeof (float));
    }
  else
    {
    float transformedSpacing[3];
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (float));
    this->Transform->TransformVector(transformedSpacing, transformedSpacing);

    for (int i = 0; i < 3; i++)
      {
      Spacing[i] = fabs(transformedSpacing[i]);
      }
    vtkDebugMacro("Transformed Spacing " << Spacing[0] << ", " << Spacing[1] << ", " << Spacing[2]);
    }
}

// if the spacing is negative we need to tranlate the origin
// basically O' = O + spacing*(dim-1) for any axis that would
// have a negative spaing
void vtkImageReader::ComputeTransformedOrigin (float origin[3])
{
  if (!this->Transform)
    {
    memcpy (origin, this->DataOrigin, 3 * sizeof (float));
    }
  else
    {
    float transformedOrigin[3];
    float transformedSpacing[3];
    int transformedExtent[6];
    
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (float));
    this->Transform->TransformVector(transformedSpacing, transformedSpacing);

    memcpy (transformedOrigin, this->DataOrigin, 3 * sizeof (float));
    this->Transform->TransformPoint(transformedOrigin, transformedOrigin);

    this->ComputeTransformedExtent(this->DataExtent,transformedExtent);
    
    for (int i = 0; i < 3; i++) 
      {
      if (transformedSpacing[i] < 0)
	{
	origin[i] = transformedOrigin[i] + transformedSpacing[i]*
	  (transformedExtent[i*2+1] -  transformedExtent[i*2] + 1);
	}
      else
	{
	origin[i] = transformedOrigin[i];
	}
      }
    vtkDebugMacro("Transformed Origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
    }
}

void vtkImageReader::ComputeTransformedExtent(int inExtent[6],
					      int outExtent[6])
{
  float transformedExtent[3];
  int temp;
  int idx;
  int dataExtent[6];
  
  if (!this->Transform)
    {
    memcpy (outExtent, inExtent, 6 * sizeof (int));
    memcpy (dataExtent, this->DataExtent, 6 * sizeof(int));
    }
  else
    {
    // need to know how far to translate to start at 000
    // first transform the data extent
    transformedExtent[0] = this->DataExtent[0];
    transformedExtent[1] = this->DataExtent[2];
    transformedExtent[2] = this->DataExtent[4];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    dataExtent[0] = (int) transformedExtent[0];
    dataExtent[2] = (int) transformedExtent[1];
    dataExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = this->DataExtent[1];
    transformedExtent[1] = this->DataExtent[3];
    transformedExtent[2] = this->DataExtent[5];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    dataExtent[1] = (int) transformedExtent[0];
    dataExtent[3] = (int) transformedExtent[1];
    dataExtent[5] = (int) transformedExtent[2];

    for (idx = 0; idx < 6; idx += 2)
      {
      if (dataExtent[idx] > dataExtent[idx+1]) 
	{
	temp = dataExtent[idx];
	dataExtent[idx] = dataExtent[idx+1];
	dataExtent[idx+1] = temp;
	}
      }

    // now transform the inExtent
    transformedExtent[0] = inExtent[0];
    transformedExtent[1] = inExtent[2];
    transformedExtent[2] = inExtent[4];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    outExtent[0] = (int) transformedExtent[0];
    outExtent[2] = (int) transformedExtent[1];
    outExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = inExtent[1];
    transformedExtent[1] = inExtent[3];
    transformedExtent[2] = inExtent[5];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    outExtent[1] = (int) transformedExtent[0];
    outExtent[3] = (int) transformedExtent[1];
    outExtent[5] = (int) transformedExtent[2];
    }

  for (idx = 0; idx < 6; idx += 2)
    {
    if (outExtent[idx] > outExtent[idx+1]) 
      {
      temp = outExtent[idx];
      outExtent[idx] = outExtent[idx+1];
      outExtent[idx+1] = temp;
      }
    // do the slide to 000 origin by subtracting the minimum extent
    outExtent[idx] -= dataExtent[idx];
    outExtent[idx+1] -= dataExtent[idx];
    }
  
  vtkDebugMacro(<< "Transformed extent are:" 
  << outExtent[0] << ", " << outExtent[1] << ", "
  << outExtent[2] << ", " << outExtent[3] << ", "
  << outExtent[4] << ", " << outExtent[5]);
}

void vtkImageReader::ComputeInverseTransformedExtent(int inExtent[6],
						     int outExtent[6])
{
  float transformedExtent[3];
  int temp;
  int idx;
  
  if (!this->Transform)
    {
    memcpy (outExtent, inExtent, 6 * sizeof (int));
    for (idx = 0; idx < 6; idx += 2)
      {
      // do the slide to 000 origin by subtracting the minimum extent
      outExtent[idx] += this->DataExtent[idx];
      outExtent[idx+1] += this->DataExtent[idx];
      }
    }
  else
    {
    // need to know how far to translate to start at 000
    int dataExtent[6];
    // first transform the data extent
    transformedExtent[0] = this->DataExtent[0];
    transformedExtent[1] = this->DataExtent[2];
    transformedExtent[2] = this->DataExtent[4];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    dataExtent[0] = (int) transformedExtent[0];
    dataExtent[2] = (int) transformedExtent[1];
    dataExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = this->DataExtent[1];
    transformedExtent[1] = this->DataExtent[3];
    transformedExtent[2] = this->DataExtent[5];
    this->Transform->TransformPoint(transformedExtent, transformedExtent);
    dataExtent[1] = (int) transformedExtent[0];
    dataExtent[3] = (int) transformedExtent[1];
    dataExtent[5] = (int) transformedExtent[2];

    for (idx = 0; idx < 6; idx += 2)
      {
      if (dataExtent[idx] > dataExtent[idx+1]) 
	{
	temp = dataExtent[idx];
	dataExtent[idx] = dataExtent[idx+1];
	dataExtent[idx+1] = temp;
	}
      }

    for (idx = 0; idx < 6; idx += 2)
      {
      // do the slide to 000 origin by subtracting the minimum extent
      inExtent[idx] += dataExtent[idx];
      inExtent[idx+1] += dataExtent[idx];
      }

    transformedExtent[0] = inExtent[0];
    transformedExtent[1] = inExtent[2];
    transformedExtent[2] = inExtent[4];
    this->Transform->GetLinearInverse()->TransformPoint(transformedExtent, 
							transformedExtent);
    outExtent[0] = (int) transformedExtent[0];
    outExtent[2] = (int) transformedExtent[1];
    outExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = inExtent[1];
    transformedExtent[1] = inExtent[3];
    transformedExtent[2] = inExtent[5];
    this->Transform->GetLinearInverse()->TransformPoint(transformedExtent,
							transformedExtent);
    outExtent[1] = (int) transformedExtent[0];
    outExtent[3] = (int) transformedExtent[1];
    outExtent[5] = (int) transformedExtent[2];

    for (idx = 0; idx < 6; idx += 2)
      {
      if (outExtent[idx] > outExtent[idx+1]) 
	{
	temp = outExtent[idx];
	outExtent[idx] = outExtent[idx+1];
	outExtent[idx+1] = temp;
	}
      }
    }
    
    vtkDebugMacro(<< "Inverse Transformed extent are:" 
    << outExtent[0] << ", " << outExtent[1] << ", "
    << outExtent[2] << ", " << outExtent[3] << ", "
    << outExtent[4] << ", " << outExtent[5]);
}

void vtkImageReader::ComputeTransformedIncrements(int inIncr[3],
						  int outIncr[3])
{
  float transformedIncr[3];
  
  if (!this->Transform)
    {
    memcpy (outIncr, inIncr, 3 * sizeof (int));
    }
  else
    {
    transformedIncr[0] = inIncr[0];
    transformedIncr[1] = inIncr[1];
    transformedIncr[2] = inIncr[2];
    this->Transform->TransformVector(transformedIncr, transformedIncr);
    outIncr[0] = (int) transformedIncr[0];
    outIncr[1] = (int) transformedIncr[1];
    outIncr[2] = (int) transformedIncr[2];
    vtkDebugMacro(<< "Transformed Incr are:" 
    << outIncr[0] << ", " << outIncr[1] << ", " << outIncr[2]);
    }
}


void vtkImageReader::ComputeInverseTransformedIncrements(int inIncr[3],
							 int outIncr[3])
{
  float transformedIncr[3];
  
  if (!this->Transform)
    {
    memcpy (outIncr, inIncr, 3 * sizeof (int));
    }
  else
    {
    transformedIncr[0] = inIncr[0];
    transformedIncr[1] = inIncr[1];
    transformedIncr[2] = inIncr[2];
    this->Transform->GetLinearInverse()->TransformVector(transformedIncr,
							 transformedIncr);
    outIncr[0] = (int) transformedIncr[0];
    outIncr[1] = (int) transformedIncr[1];
    outIncr[2] = (int) transformedIncr[2];
    vtkDebugMacro(<< "Inverse Transformed Incr are:" 
    << outIncr[0] << ", " << outIncr[1] << ", " << outIncr[2]);
    }
}
