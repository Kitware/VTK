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
#include "vtkImageReader.h"
#include "vtkImageCache.h"

//----------------------------------------------------------------------------
vtkImageReader::vtkImageReader()
{
  int idx;
  
  this->File = NULL;
  this->DataScalarType = VTK_SHORT;
  // Output should default to the same scalar type as file data.
  this->SetOutputScalarType(VTK_SHORT);
  
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,
		VTK_IMAGE_Z_AXIS, VTK_IMAGE_COMPONENT_AXIS);

  // Arbitrary default value.  This ivar is not used by this object.
  this->Dimensionality = 4;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->FileIncrements[idx] = 1;
    this->DataExtent[idx*2] = this->DataExtent[idx*2 + 1] = 0;
    this->FileExtent[idx*2] = this->FileExtent[idx*2 + 1] = 0;
    this->DataDimensions[idx] = 1;
    this->DataSpacing[idx] = 1.0;
    this->DataOrigin[idx] = 0.0;
    this->Flips[idx] = 0;
    }

  this->FileName = NULL;
  
  this->HeaderSize = 0;
  this->FileSize = 0;
  this->Initialized = 0;
  this->ManualHeaderSize = 0;

  // Left over from short reader
  this->PixelMask = 0xffff;
  this->SwapBytes = 0;
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
}

void vtkImageReader::SetFileByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkImageReader::SetFileByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkImageReader::SetFileByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
    this->SetFileByteOrderToBigEndian();
  else
    this->SetFileByteOrderToLittleEndian();
}

int vtkImageReader::GetFileByteOrder()
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

char *vtkImageReader::GetFileByteOrderAsString()
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
  
  vtkImageCachedSource::PrintSelf(os,indent);
  
  if (this->FileName)
    {
    os << indent << "FileName: " << this->FileName << "\n";
    }

  os << indent << "DataScalarType: " 
     << vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";

  os << indent << "DataDimensions: (" << this->DataDimensions[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->DataDimensions[idx];
    }
  os << ")\n";

  os << indent << "DataExtent: (" << this->DataExtent[0];
  for (idx = 1; idx < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx)
    {
    os << ", " << this->DataExtent[idx];
    }
  os << ")\n";
  
  os << indent << "DataSpacing: (" << this->DataSpacing[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->DataSpacing[idx];
    }
  os << ")\n";
  
  os << indent << "DataOrigin: (" << this->DataOrigin[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->DataOrigin[idx];
    }
  os << ")\n";
  
  if ( ! this->Initialized)
    {
    os << indent << "Not initialized.\n";
    }
  else
    {
    os << indent << "HeaderSize: " << this->HeaderSize << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkImageReader::SetDataDimensions(int num, int *size)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->DataDimensions[idx] = size[idx];
    // Also set the image extent (do not modify mins)
    this->DataExtent[idx*2+1] = this->DataExtent[idx*2] + size[idx] - 1;
    }

  this->Initialized = 0;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageReader::GetDataDimensions(int num, int *size)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    size[idx] = this->DataDimensions[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageReader::SetDataExtent(int num, int *extent)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->DataExtent[idx*2] = extent[idx*2];
    this->DataExtent[idx*2+1] = extent[idx*2+1];
    // Also set the dimensions
    this->DataDimensions[idx] = extent[idx*2+1] - extent[idx*2] + 1;
    }

  this->Initialized = 0;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageReader::GetDataExtent(int num, int *extent)
{
  int idx;
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->DataExtent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageReader::SetDataSpacing(int num, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->DataSpacing[idx] = ratio[idx];
    }
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageReader::GetDataSpacing(int num, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    ratio[idx] = this->DataSpacing[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageReader::SetDataOrigin(int num, float *origin)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->DataOrigin[idx] = origin[idx];
    }
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageReader::GetDataOrigin(int num, float *origin)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    origin[idx] = this->DataOrigin[idx];
    }
}


//----------------------------------------------------------------------------
void vtkImageReader::SetFlips(int num, int *flips)
{
  int idx;
  
  this->Modified();
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("SetFlips: " << num << " out of range");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Flips[idx] = flips[idx];
    }
}

  
//----------------------------------------------------------------------------
void vtkImageReader::GetFlips(int num, int *flips)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro("GetFlips: " << num << " out of range");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    flips[idx] = this->Flips[idx];
    }
}

  
//----------------------------------------------------------------------------
// Description:
// This method returns the largest region that can be generated.
void vtkImageReader::UpdateImageInformation(vtkImageRegion *region)
{
  int idx;
  float outOrigin[VTK_IMAGE_DIMENSIONS];
  
  region->SetImageExtent(5, this->DataExtent); // One extra for component ?
  // Flips should change aspect ratio, but VTK cannot handle 
  // negative aspect ratios.
  region->SetSpacing(4, this->DataSpacing);
  // Flips an axis changes the origin.
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    if ( this->Flips[idx])
      {
      outOrigin[idx] = this->DataOrigin[idx] + 
	(this->DataSpacing[idx] * this->DataExtent[idx*2+1]);
      }
    else
      {
      outOrigin[idx] = this->DataOrigin[idx];
      }
    }
  region->SetOrigin(4, outOrigin);
}


//----------------------------------------------------------------------------
// Manual initialization.
void vtkImageReader::SetHeaderSize(int size)
{
  this->HeaderSize = size;
  this->Modified();
  this->ManualHeaderSize = 1;
}
  

//----------------------------------------------------------------------------
// Description:
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkImageReader::Initialize()
{
  int idx, inc;
  
  if (this->Initialized)
    {
    return;
    }
  
  if ( ! this->FileName)
    {
    vtkErrorMacro(<< "Initialize: No FileName.");
    return;
    }
  
  // Determine the expected length of the data ...
  switch (this->DataScalarType)
    {
    case VTK_FLOAT:
      this->PixelSize = sizeof(float);
      break;
    case VTK_INT:
      this->PixelSize = sizeof(int);
      break;
    case VTK_SHORT:
      this->PixelSize = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      this->PixelSize = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      this->PixelSize = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro(<< "Initialize: Unknown DataScalarType");
      return;
    }

  // compute the increments (in units of bytes)
  inc = this->PixelSize;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->FileIncrements[idx] = inc;
    inc *= this->DataDimensions[idx];
    }
  
  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }
  
  // Open the new file
  vtkDebugMacro(<< "Initialize: opening file " << this->FileName);
#ifdef _WIN32
  this->File = new ifstream(this->FileName, ios::in | ios::binary);
#else
  this->File = new ifstream(this->FileName, ios::in);
#endif
  if (! this->File || this->File->fail())
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << this->FileName);
    return;
    }
  
  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  this->FileSize = this->File->tellg();
  if ( ! this->ManualHeaderSize)
    {
    this->HeaderSize = this->FileSize - this->FileIncrements[4];
    vtkDebugMacro(<< "Initialize: Header " << this->HeaderSize 
                  << " bytes, fileLength = " << this->FileSize << " bytes.");
    }
  
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Description:
// This function reads in one region of one slice.
// templated to handle different data types.
template <class IT, class OT>
static void vtkImageReaderUpdate2(vtkImageReader *self, vtkImageRegion *region,
				  IT *inPtr, OT *outPtr)
{
  int min0, max0,  min1, max1,  min2, max2,  min3, max3;
  int outInc0, outInc1, outInc2, outInc3;
  OT *outPtr0, *outPtr1, *outPtr2, *outPtr3;
  long streamStart;
  long streamSkip0, streamSkip1, streamSkip2;
  long streamRead, temp;
  int idx0, idx1, idx2, idx3, pixelRead;
  unsigned char *buf;

  region->GetIncrements(outInc0, outInc1, outInc2, outInc3);
  // Get the requested extents.
  region->GetExtent(min0, max0,  min1, max1,  min2, max2,  min3, max3);
  // Convert them into to the extent needed from the file. (because of flips)
  // Flipping does not change the image extent of the output.
  // The second part of flipping the data is that we march through
  // the output region backwards.
  if (self->Flips[0])
    {
    outPtr += (max0 - min0) * outInc0;
    outInc0 = -outInc0;
    temp = -max0 + self->FileExtent[0] + self->FileExtent[1];
    max0 = -min0 + self->FileExtent[0] + self->FileExtent[1];
    min0 = temp;
    }
  if (self->Flips[1])
    {
    outPtr += (max1 - min1) * outInc1;
    outInc1 = -outInc1;
    temp = -max1 + self->FileExtent[2] + self->FileExtent[3];
    max1 = -min1 + self->FileExtent[2] + self->FileExtent[3];
    min1 = temp;
    }
  if (self->Flips[2])
    {
    outPtr += (max2 - min2) * outInc2;
    outInc2 = -outInc2;
    temp = -max2 + self->FileExtent[4] + self->FileExtent[5];
    max2 = -min2 + self->FileExtent[4] + self->FileExtent[5];
    min2 = temp;
    }
  if (self->Flips[3])
    {
    outPtr += (max3 - min3) * outInc3;
    outInc3 = -outInc3;
    temp = -max3 + self->FileExtent[6] + self->FileExtent[7];
    max3 = -min3 + self->FileExtent[6] + self->FileExtent[7];
    min3 = temp;
    }
  
  // convert data extent into constants that can be used to seek through files.
  streamStart = (min0 - self->FileExtent[0]) * self->FileIncrements[0] 
    + (min1 - self->FileExtent[2]) * self->FileIncrements[1]
    + (min2 - self->FileExtent[4]) * self->FileIncrements[3] 
    + (min3 - self->FileExtent[6]) * self->FileIncrements[4];

  streamStart += self->HeaderSize;

  pixelRead = max0 - min0 + 1; // length of a row, num pixels read at a time
  temp = pixelRead * self->FileIncrements[0];  
  streamRead = temp;
  streamSkip0 = self->FileIncrements[1] - temp;
  temp = + (max1-min1+1) * self->FileIncrements[1];
  streamSkip1 = self->FileIncrements[2] - temp;
  temp = + (max2-min2+1) * self->FileIncrements[2];
  streamSkip2 = self->FileIncrements[3] - temp;

  // Reset the extent to be that of the output. (not really needed)
  region->GetExtent(min0, max0,  min1, max1,  min2, max2,  min3, max3);

  // error checking
  if (streamStart < 0 || streamStart > self->FileSize)
    {
    cerr << "streamStart: " << streamStart << ", headerSize: "
	 << self->HeaderSize << "\n";
    cerr << "vtkImageReader GenerateData: bad offset \n";
    return;
    }
    
  // move to the correct location in the file (offset of region)
  self->File->seekg(streamStart, ios::beg);
  if (self->File->fail())
    {
    cerr << "File operation failed.\n";
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[streamRead];
  
  // read the data row by row
  outPtr3 = outPtr;
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    outPtr2 = outPtr3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      outPtr1 = outPtr2;
      for (idx1 = min1; idx1 <= max1; ++idx1)
	{
	outPtr0 = outPtr1;
	
	// read the row.
	if ( ! self->File->read((char *)buf, streamRead))
	  {
	  cerr << "File operation failed. row = " << idx1
	       << ", Read = " << streamRead
	       << ", Start = " << streamStart
	       << ", Skip0 = " << streamSkip0
	       << ", Skip1 = " << streamSkip1
	       << ", Skip2 = " << streamSkip2
	       << ", FilePos = " << self->File->tellg() << "\n";
	  return;
	  }
	
	// handle swapping (legacy)
	if (self->SwapBytes)
	  {
	  self->Swap(buf, pixelRead, self->PixelSize);
	  }
	
	// copy the bytes into the typed region
	inPtr = (IT *)(buf);
	for (idx0 = min0; idx0 <= max0; ++idx0)
	  {

	  // Copy pixel into the output.
	  if (self->PixelMask == 0xffff)
	    {
	    *outPtr0 = (OT)(*inPtr);
	    }
	  else
	    {
	    // left over from short reader (what about other types.
	    *outPtr0 = (OT)((short)(*inPtr) & self->PixelMask);
	    }
	  
	  // move to next pixel
	  inPtr += 1;
	  outPtr0 += outInc0;
	  }
	// move to the next row in the file and region
	self->File->seekg(streamSkip0, ios::cur);
	outPtr1 += outInc1;
	}
      // move to the next image in the file and region
      self->File->seekg(streamSkip1, ios::cur);
      outPtr2 += outInc2;
      }
    // move to the next volume in the file and region
    self->File->seekg(streamSkip2, ios::cur);
    outPtr3 += outInc3;
    }

  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function reads in one region of one slice.
// templated to handle different data types.
template <class T>
static void vtkImageReaderUpdate1(vtkImageReader *self, 
			   vtkImageRegion *region, T *inPtr)
{
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = region->GetScalarPointer();
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageReaderUpdate2(self, region, inPtr, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageReaderUpdate2(self, region, inPtr, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageReaderUpdate2(self, region, inPtr, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageReaderUpdate2(self, region, inPtr, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageReaderUpdate2(self, region, inPtr, (unsigned char *)(outPtr));
      break;
    default:
      cerr << "Update1: Unknown data type \n";
    }  
}
//----------------------------------------------------------------------------
// Description:
// This function reads a region from a file.  The regions extent/axes
// are assumed to be the same as the file extent/order.
void vtkImageReader::UpdateFromFile(vtkImageRegion *region)
{
  void *ptr = NULL;
  
  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
    {
    case VTK_FLOAT:
      vtkImageReaderUpdate1(this, region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageReaderUpdate1(this, region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageReaderUpdate1(this, region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageReaderUpdate1(this, region, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageReaderUpdate1(this, region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }   
}



//----------------------------------------------------------------------------
// Description:
// Set the data type of pixles in the file.  If the OutputScalarType is not 
// set yet, it is also set to "type" as a default.
void vtkImageReader::SetDataScalarType(int type)
{
  vtkImageCache *cache;
  
  this->Modified();
  this->DataScalarType = type;

  // Set the default output scalar type
  cache = this->GetCache();
  if (cache->GetScalarType() == VTK_VOID)
    {
    cache->SetScalarType(type);
    }
}


//----------------------------------------------------------------------------
// Description:
// Sets the default ScalarType of the cache.
vtkImageSource *vtkImageReader::GetOutput()
{
  this->CheckCache();
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    this->Output->SetScalarType(this->DataScalarType);
    }
  
  return this->Output;
}



//----------------------------------------------------------------------------
// Description:
// Swaps the bytes of a buffer.  
// Assumes the pixel size is divisible by two.
// This should really be in vtkSwapBytes.
void vtkImageReader::Swap(unsigned char *buffer, int numPixels, int pixelSize)
{
  unsigned char temp, *out;
  int idx1, idx2, inc, half;
  
  half = pixelSize / 2;
  inc = pixelSize - 1;
  
  for (idx1 = 0; idx1 < numPixels; ++idx1)
    {
    out = buffer + inc;
    for (idx2 = 0; idx2 < half; ++idx2)
      {
      temp = *out;
      *out = *buffer;
      *buffer = temp;
      ++buffer;
      --out;
      }
    buffer += half;
    }
}

  
    
    
  



  
  
  
  
  
  
  







