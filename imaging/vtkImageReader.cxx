/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkByteSwap.h"
#include "vtkImageCache.h"
#include "vtkImageReader.h"

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
}

//----------------------------------------------------------------------------
// Description:
// This function sets the name of the file. 
void vtkImageReader::ComputeInternalFileName(int slice)
{
  // delete any old filename
  if (this->InternalFileName)
    {
    delete [] this->InternalFileName;
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
// Description:
// This function sets the name of the file. 
void vtkImageReader::SetFileName(char *name)
{
  if ( this->FileName && name && (!strcmp(this->FileName,name))) return; 
  if (!name && !this->FileName) return;
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }  
  this->FileName = new char[strlen(name) + 1];
  strcpy(this->FileName, name);
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageReader::SetFilePrefix(char *prefix)
{
  if ( this->FilePrefix && prefix && (!strcmp(this->FilePrefix,prefix))) return; 
  if (!prefix && !this->FilePrefix) return;
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
// Description:
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%3d" would be the
// pattern of a series: image.001, image.002 ...
void vtkImageReader::SetFilePattern(char *pattern)
{
  if ( this->FilePattern && pattern && 
       (!strcmp(this->FilePattern,pattern))) return; 
  if (!pattern && !this->FilePattern) return;
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
    this->SetDataByteOrderToBigEndian();
  else
    this->SetDataByteOrderToLittleEndian();
}

int vtkImageReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  else
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
#else
  if ( this->SwapBytes )
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  else
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
#endif
}

char *vtkImageReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
    return "LittleEndian";
  else
    return "BigEndian";
#else
  if ( this->SwapBytes )
    return "BigEndian";
  else
    return "LittleEndian";
#endif
}


//----------------------------------------------------------------------------
void vtkImageReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageSource::PrintSelf(os,indent);
  
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
// Description:
// This method returns the largest data that can be generated.
void vtkImageReader::UpdateImageInformation()
{
  float spacing[3];
  int extent[6];
  float origin[3];
  
  // Make sure we have an output.
  this->CheckCache();
    
  // set the extent, if the VOI has not been set then default to
  // the DataExtent
  if (this->DataVOI[0] || this->DataVOI[1] || 
      this->DataVOI[2] || this->DataVOI[3] || 
      this->DataVOI[4] || this->DataVOI[5])
    {
    this->ComputeTransformedExtent(this->DataVOI,extent);
    this->Output->SetWholeExtent(extent);
    }
  else
    {
    this->ComputeTransformedExtent(this->DataExtent,extent);
    this->Output->SetWholeExtent(extent);
    }
    
  // set the spacing
  this->ComputeTransformedSpacing(spacing);
  this->Output->SetSpacing(spacing);

  // set the origin.
  this->ComputeTransformedOrigin(origin);
  this->Output->SetOrigin(origin);

  this->GetOutput()->SetScalarType(this->DataScalarType);
  this->GetOutput()->
    SetNumberOfScalarComponents(this->NumberOfScalarComponents);
}


//----------------------------------------------------------------------------
// Manual initialization.
void vtkImageReader::SetHeaderSize(int size)
{
  if (size != this->HeaderSize)
    {
    this->HeaderSize = size;
    this->Modified();
    }
  this->ManualHeaderSize = 1;
}
  

//----------------------------------------------------------------------------
// Description:
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkImageReader::ComputeDataIncrements()
{
  int idx;
  int fileDataLength;
  
  // Determine the expected length of the data ...
  switch (this->DataScalarType)
    {
    case VTK_FLOAT:
      fileDataLength = sizeof(float);
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


int vtkImageReader::GetHeaderSize()
{
  return this->GetHeaderSize(this->DataExtent[4]);
}

int vtkImageReader::GetHeaderSize(int idx)
{
  if ( ! this->ManualHeaderSize)
    {
    this->ComputeDataIncrements();

    // make sure we figure out a filename to open
    this->ComputeInternalFileName(idx);
    this->OpenFile();
    
    // Get the size of the header from the size of the image
    this->File->seekg(0,ios::end);
    
    return this->File->tellg() - 
      this->DataIncrements[this->GetFileDimensionality()];
    }
  
  return this->HeaderSize;
}

void vtkImageReader::OpenAndSeekFile(int dataExtent[6], int idx)
{
  long streamStart;

  this->ComputeInternalFileName(idx);
  if (this->GetFileDimensionality() == 3)
    {
    this->UpdateProgress(0.0);
    } 
  else 
    {
    this->UpdateProgress ((float) (idx - dataExtent[4])
                        / (float) (dataExtent[5] - dataExtent[4] + 1));
    }
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
  if (streamStart < 0)
    {
    vtkWarningMacro("streamStart: " << streamStart << " bad offset");
    return;
    }
  this->File->seekg(streamStart, ios::beg);
  if (this->File->fail())
    {
    vtkWarningMacro("File operation failed.");
      return;
    }
	
}

//----------------------------------------------------------------------------
// Description:
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
  
  // Get the requested extents.
  data->GetExtent(inExtent);
  // Convert them into to the extent needed from the file. 
  self->ComputeInverseTransformedExtent(inExtent,dataExtent);
  
  // get and transform the increments
  data->GetIncrements(inIncr);
  self->ComputeInverseTransformedIncrements(inIncr,outIncr);

  // compute outPtr2 
  outPtr2 = outPtr;
  if (outIncr[0] < 0) 
    outPtr2 = outPtr2 - outIncr[0]*(dataExtent[1] - dataExtent[0]);
  if (outIncr[1] < 0) 
    outPtr2 = outPtr2 - outIncr[1]*(dataExtent[3] - dataExtent[2]);
  if (outIncr[2] < 0) 
    outPtr2 = outPtr2 - outIncr[2]*(dataExtent[5] - dataExtent[4]);

  // length of a row, num pixels read at a time
  pixelRead = dataExtent[1] - dataExtent[0] + 1; 
  streamRead = pixelRead * self->DataIncrements[0];  
  streamSkip0 = self->DataIncrements[1] - streamRead;
  streamSkip1 = self->DataIncrements[2] - 
    (dataExtent[3] - dataExtent[2] + 1)* self->DataIncrements[1];
  pixelSkip = data->GetNumberOfScalarComponents();
    
  // read from the bottom up
  if (!self->GetFileLowerLeft()) 
    {
    streamSkip0 = -streamRead - self->DataIncrements[1];
    streamSkip1 = self->DataIncrements[2] + 
      (dataExtent[3] - dataExtent[2] + 1)* self->DataIncrements[1];
    }
  
    
  // create a buffer to hold a row of the data
  buf = new unsigned char[streamRead];
  
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
    for (idx1 = dataExtent[2]; idx1 <= dataExtent[3]; ++idx1)
      {
      outPtr0 = outPtr1;
      
      // read the row.
      if ( ! self->File->read((char *)buf, streamRead))
	{
	vtkGenericWarningMacro("File operation failed. row = " << idx1
			       << ", Read = " << streamRead
			       << ", Skip0 = " << streamSkip0
			       << ", Skip1 = " << streamSkip1
			       << ", FilePos = " << self->File->tellg());
	return;
	}
      
      // handle swapping
      if (self->SwapBytes)
	{
	vtkByteSwap::SwapVoidRange(buf, pixelRead, sizeof(IT));
	}
      
      // copy the bytes into the typed data
      inPtr = (IT *)(buf);
      for (idx0 = dataExtent[0]; idx0 <= dataExtent[1]; ++idx0)
	{
	// Copy pixel into the output.
	if (self->DataMask == 0xffff)
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
	    outPtr0[comp] = (OT)((short)(inPtr[comp]) & self->DataMask);
	    }
	  }
	// move to next pixel
	inPtr += pixelSkip;
	outPtr0 += outIncr[0];
	}
      // move to the next row in the file and data
      self->File->seekg(self->File->tellg() + streamSkip0, ios::beg);
      outPtr1 += outIncr[1];
      }
    // move to the next image in the file and data
    self->File->seekg(self->File->tellg() + streamSkip1, ios::beg);
    outPtr2 += outIncr[2];
    }

  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
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
    case VTK_FLOAT:
      vtkImageReaderUpdate2(self, data, inPtr, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageReaderUpdate2(self, data, inPtr, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageReaderUpdate2(self, data, inPtr, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageReaderUpdate2(self, data, inPtr, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageReaderUpdate2(self, data, inPtr, (unsigned char *)(outPtr));
      break;
    default:
      cerr << "Update1: Unknown data type \n";
    }  
}
//----------------------------------------------------------------------------
// Description:
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageReader::Execute(vtkImageData *data)
{
  void *ptr = NULL;
  
  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
    {
    case VTK_FLOAT:
      vtkImageReaderUpdate1(this, data, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageReaderUpdate1(this, data, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageReaderUpdate1(this, data, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageReaderUpdate1(this, data, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageReaderUpdate1(this, data, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }   
}



//----------------------------------------------------------------------------
// Description:
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


//----------------------------------------------------------------------------
// Description:
// Returns the cache.
vtkImageCache *vtkImageReader::GetOutput()
{
  this->CheckCache();
  return this->Output;
}


void vtkImageReader::ComputeTransformedSpacing (float Spacing[3])
{
  if (!this->Transform)
    {
    memcpy (Spacing, this->DataSpacing, 3 * sizeof (float));
    }
  else
    {
    float transformedSpacing[4];
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (float));
    // this is zero to prevent translations !!!
    transformedSpacing[3] = 0.0;
    this->Transform->MultiplyPoint (transformedSpacing, transformedSpacing);

    for (int i = 0; i < 3; i++) Spacing[i] = fabs(transformedSpacing[i]);
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
    float transformedOrigin[4];
    float transformedSpacing[4];
    int transformedExtent[6];
    
    memcpy (transformedSpacing, this->DataSpacing, 3 * sizeof (float));
    // this is zero to prevent translations !!!
    transformedSpacing[3] = 0.0;
    this->Transform->MultiplyPoint (transformedSpacing, transformedSpacing);

    memcpy (transformedOrigin, this->DataOrigin, 3 * sizeof (float));
    transformedOrigin[3] = 1.0;
    this->Transform->MultiplyPoint (transformedOrigin, transformedOrigin);

    this->ComputeTransformedExtent(this->DataExtent,transformedExtent);
    
    for (int i = 0; i < 3; i++) 
      {
      if (transformedSpacing[i] < 0)
	{
	origin[i] = transformedOrigin[i] + transformedSpacing[i]*
	  (transformedExtent[i*2+1] -  transformedExtent[i*2]);
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
  float transformedExtent[4];
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
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
    dataExtent[0] = (int) transformedExtent[0];
    dataExtent[2] = (int) transformedExtent[1];
    dataExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = this->DataExtent[1];
    transformedExtent[1] = this->DataExtent[3];
    transformedExtent[2] = this->DataExtent[5];
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
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
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
    outExtent[0] = (int) transformedExtent[0];
    outExtent[2] = (int) transformedExtent[1];
    outExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = inExtent[1];
    transformedExtent[1] = inExtent[3];
    transformedExtent[2] = inExtent[5];
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
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
  float transformedExtent[4];
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
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
    dataExtent[0] = (int) transformedExtent[0];
    dataExtent[2] = (int) transformedExtent[1];
    dataExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = this->DataExtent[1];
    transformedExtent[1] = this->DataExtent[3];
    transformedExtent[2] = this->DataExtent[5];
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
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
    transformedExtent[3] = 1.0;
    // since transform better be orthonormal we can just transpose
    // it will be the same as the inverse
    this->Transform->Transpose();
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
    outExtent[0] = (int) transformedExtent[0];
    outExtent[2] = (int) transformedExtent[1];
    outExtent[4] = (int) transformedExtent[2];
    
    transformedExtent[0] = inExtent[1];
    transformedExtent[1] = inExtent[3];
    transformedExtent[2] = inExtent[5];
    transformedExtent[3] = 1.0;
    this->Transform->MultiplyPoint (transformedExtent, transformedExtent);
    this->Transform->Transpose();
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
  float transformedIncr[4];
  
  if (!this->Transform)
    {
    memcpy (outIncr, inIncr, 3 * sizeof (int));
    }
  else
    {
    transformedIncr[0] = inIncr[0];
    transformedIncr[1] = inIncr[1];
    transformedIncr[2] = inIncr[2];
    // set to zero to prevent translations !!!
    transformedIncr[3] = 0.0;
    this->Transform->MultiplyPoint (transformedIncr, transformedIncr);
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
  float transformedIncr[4];
  
  if (!this->Transform)
    {
    memcpy (outIncr, inIncr, 3 * sizeof (int));
    }
  else
    {
    transformedIncr[0] = inIncr[0];
    transformedIncr[1] = inIncr[1];
    transformedIncr[2] = inIncr[2];
    // set to zero to prevent translations !!!
    transformedIncr[3] = 0.0;
    // since transform better be orthonormal we can just transpose
    // it will be the same as the inverse
    this->Transform->Transpose();
    this->Transform->MultiplyPoint (transformedIncr, transformedIncr);
    this->Transform->Transpose();
    outIncr[0] = (int) transformedIncr[0];
    outIncr[1] = (int) transformedIncr[1];
    outIncr[2] = (int) transformedIncr[2];
    vtkDebugMacro(<< "Inverse Transformed Incr are:" 
    << outIncr[0] << ", " << outIncr[1] << ", " << outIncr[2]);
    }
}
