/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortReader.cxx
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
#include "vtkImageShortReader.h"
#include "vtkImageCache.h"

//----------------------------------------------------------------------------
vtkImageShortReader::vtkImageShortReader()
{
  int idx;
 
  vtkErrorMacro(<< "vtkImageShortReader is being phased out. "
  << "Use vtkImageSeriesReader instead");
  
  this->File = NULL;
  this->PixelMask = 0xffff;
  this->Signed = 0;
  this->SwapBytes = 0;  
  this->First = 1;
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,
		VTK_IMAGE_Z_AXIS, VTK_IMAGE_COMPONENT_AXIS);

  // since the update method will only read images.
  this->Dimensionality = 2;
  
  this->PixelMax = -9e99;
  this->PixelMin = +9e99;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Dimensions[idx] = 1;
    this->Increments[idx] = 1;
    this->Spacing[idx] = 1.0;
    this->Origin[idx] = 0.0;
    }

  this->FilePrefix = NULL;
  this->FilePattern = NULL;
  this->FileName = NULL;
  
  this->SetFilePrefix("");
  this->SetFilePattern("%s.%d");

  this->HeaderSize = 0;
  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkImageShortReader::~vtkImageShortReader()
{ 
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
  if (this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkImageShortReader::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkObject::PrintSelf(os,indent);

  os << indent << "FilePrefix: " << this->FilePrefix << "\n";
  os << indent << "FilePattern: " << this->FilePattern << "\n";
  os << indent << "Signed: " << this->Signed << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
  os << indent << "Dimensions: (" << this->Dimensions[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Dimensions[idx];
    }
  os << ")\n";

  os << indent << "Spacing: (" << this->Spacing[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Spacing[idx];
    }
  os << ")\n";
  
  os << indent << "Origin: (" << this->Origin[0];
  for (idx = 1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    os << ", " << this->Origin[idx];
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
void vtkImageShortReader::SetDimensions(int num, int *size)
{
  int idx, inc = 1;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Dimensions[idx] = size[idx];
    this->Increments[idx] = inc;
    inc *= size[idx];
    }

  this->Initialized = 0;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageShortReader::GetDimensions(int num, int *size)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    size[idx] = this->Dimensions[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageShortReader::SetSpacing(int num, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Spacing[idx] = ratio[idx];
    }
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageShortReader::GetSpacing(int num, float *ratio)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    ratio[idx] = this->Spacing[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImageShortReader::SetOrigin(int num, float *origin)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Origin[idx] = origin[idx];
    }
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkImageShortReader::GetOrigin(int num, float *origin)
{
  int idx;
  
  for (idx = 0; idx < num; ++idx)
    {
    origin[idx] = this->Origin[idx];
    }
}


//----------------------------------------------------------------------------
// Description:
// This method returns the largest region that can be generated.
void vtkImageShortReader::UpdateImageInformation(vtkImageRegion *region)
{
  region->SetImageExtent(0, this->Dimensions[0]-1, 
			 0, this->Dimensions[1]-1, 
			 0, this->Dimensions[2]-1,
			 0, this->Dimensions[3]-1);
  region->SetSpacing(4, this->Spacing);
  region->SetOrigin(4, this->Origin);
}



//----------------------------------------------------------------------------
// Description:
// This function opens the first file to determine the header size.
void vtkImageShortReader::Initialize()
{
  if (this->Initialized)
    {
    return;
    }
  
  if ( ! this->FilePrefix || ! this->FilePattern)
    {
    vtkErrorMacro(<< "Initialize: Null string.");
    return;
    }
  
  // Allocate a large enough string for the file name.
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->FileName = new char[strlen(this->FilePrefix) +
			    strlen(this->FilePattern) + 50];
  
  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }
  
  // Open the new file
  sprintf(this->FileName, this->FilePattern, this->FilePrefix, this->First);
  vtkDebugMacro(<< "SetFileName: opening Short file " << this->FileName);
  this->File = new ifstream(this->FileName, ios::in);
  if (! this->File || this->File->fail())
    {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return;
    }
  
  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  this->FileSize = this->File->tellg();
  this->HeaderSize = this->FileSize
    - sizeof(unsigned short int) * this->Increments[2];
  
  vtkDebugMacro(<< "Initialize: Header " << this->HeaderSize 
                << " bytes, fileLength = " << this->FileSize << " bytes.");
  
  this->File->close();
  delete this->File;
  this->File = NULL;
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Description:
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageShortReader::SetFilePrefix(char *prefix)
{
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    }
  this->FilePrefix = new char[strlen(prefix) + 1];
  strcpy(this->FilePrefix, prefix);
  this->Initialized = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This function sets the pattern of the file name which turn a prefix
// into a file name. "%s.%3d" would be the
// pattern of a series: image.001, image.002 ...
void vtkImageShortReader::SetFilePattern(char *pattern)
{
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    }
  this->FilePattern = new char[strlen(pattern) + 1];
  strcpy(this->FilePattern, pattern);
  this->Initialized = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This function reads a whole image.
// This is a special class that should speed reads.
// It is not finished yet, but is a good idea.
template <class T>
static void vtkImageShortReaderGenerateImage(vtkImageShortReader *self,
				      vtkImageRegion *region, T *ptr)
{
  int min0, max0,  min1, max1;
  int inc0, inc1;
  T *pf0, *pf1;
  T pixelMin, pixelMax;
  long headerSize;
  long imageSize;
  unsigned char *buf, *pbuf;
  unsigned char swap[2];
  unsigned short *pshort;
  int idx0, idx1;

  // Hack try to set initial values for min and max
  pixelMax = 0;
  pixelMin = (T)(65000);
  if (255 > pixelMin)
    pixelMin = 255;
  if (-pixelMin < 1)
    pixelMax = -pixelMin;
  
  // get the information needed to find a location in the file
  region->GetExtent(min0, max0,  min1, max1);
  region->GetIncrements(inc0, inc1);
  imageSize = (max0-min0+1)*(max1-min1+1)*2;  // the number of bytes in image
  headerSize = self->GetHeaderSize();
  
  // skip over header
  self->File->seekg(headerSize, ios::beg);
  if (self->File->fail())
    {
    cerr << "File operation failed.\n";
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[imageSize]; 
  
  // read the image all at once
  if ( ! self->File->read((char *)buf, imageSize))
    {
    cerr << "File operation failed.\n";
    return;
    }
  /*    
  // copy the bytes into the typed region
    // handle byte swapping
    if (self->SwapBytes)
      {
      *swap = pbuf[1];
      swap[1] = *pbuf;
      }
    else
      {
      *swap = *pbuf;
      swap[1] = pbuf[1];
      }
    
    // mask the data
    pshort = (unsigned short *)(swap);
    *pshort = *pshort & self->PixelMask;
    
    // Convert to data type T
    if (self->Signed)
      *pf0 = (T)(*((short int *)swap));
    else
      *pf0 = (T)(*((unsigned short int *)swap));
    
    // Keep track of min and max
    if (*pf0 < pixelMin)
      {
      pixelMin = *pf0;
      }
    if (*pf0 > pixelMax)
      {
      pixelMax = *pf0;
      }
      */
  
  // Save global pixel min and max.
  /*
  if (self->PixelMin > (double)(pixelMin))
    {
    self->PixelMin = (double)(pixelMin);
    }
  if (self->PixelMax < (double)(pixelMax))
    {
    self->PixelMax = (double)(pixelMax);
    }
    */
  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function reads in one region of one slice.
// templated to handle different data types.
template <class T>
static void vtkImageShortReaderGenerateRegion(vtkImageShortReader *self,
				       vtkImageRegion *region, T *ptr)
{
  int min0, max0,  min1, max1;
  int inc0, inc1;
  T *pf0, *pf1;
  T pixelMin, pixelMax;
  long streamStartPos;
  long streamRowSkip;
  long streamRowRead;
  unsigned char *buf, *pbuf;
  unsigned char swap[2];
  unsigned short *pshort;
  int idx0, idx1;

  // Hack try to set initial values for min and max
  pixelMax = 0;
  pixelMin = (T)(65000);
  if (255 > pixelMin)
    pixelMin = 255;
  if (-pixelMin < 1)
    pixelMax = -pixelMin;
  
  // get the information needed to find a location in the file
  region->GetExtent(min0, max0,  min1, max1);
  region->GetIncrements(inc0, inc1);
  streamStartPos = min0 * self->Increments[0] 
                 + min1 * self->Increments[1];
  streamStartPos *= sizeof(unsigned short int);
  streamStartPos += self->HeaderSize;

  streamRowRead = (max0-min0+1) * sizeof(unsigned short int);
  streamRowSkip = (self->Increments[1] * sizeof(unsigned short int))
    - streamRowRead; 

  // error checking
  if (streamStartPos < 0 || streamStartPos > self->FileSize)
    {
    cerr << "vtkImageShortReader GenerateData: bad offset";
    return;
    }
    
  // move to the correct location in the file (offset of region)
  self->File->seekg(streamStartPos, ios::beg);
  if (self->File->fail())
    {
    cerr << "File operation failed.\n";
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[streamRowRead]; 
  
  // read the data row by row
  pf1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    if ( ! self->File->read((char *)buf, streamRowRead))
      {
      cerr << "File operation failed. row = " << idx1
	   << ", StartPos = " << streamStartPos
	   << ", RowSkip = " << streamRowSkip
	   << ", RowRead = " << streamRowRead
	   << ", FilePos = " << self->File->tellg() << "\n";
      return;
      }
    
    // copy the bytes into the typed region
    pf0 = pf1;
    pbuf = buf;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      // handle byte swapping
      if (self->SwapBytes)
	{
	*swap = pbuf[1];
	swap[1] = *pbuf;
	}
      else
	{
	*swap = *pbuf;
	swap[1] = pbuf[1];
	}
    
      // mask the data
      pshort = (unsigned short *)(swap);
      *pshort = *pshort & self->PixelMask;

      // Convert to data type T
      if (self->Signed)
	*pf0 = (T)(*((short int *)swap));
      else
	*pf0 = (T)(*((unsigned short int *)swap));

      // Keep track of min and max
      if (*pf0 < pixelMin)
	{
	pixelMin = *pf0;
	}
      if (*pf0 > pixelMax)
	{
	pixelMax = *pf0;
	}
      
      // move to next pixel
      pbuf += 2;
      pf0 += inc0;
      }
    // move to the next row in the file and region
    self->File->seekg(streamRowSkip, ios::cur);
    pf1 += inc1;
    }
  
  // Save global pixel min and max.
  if (self->PixelMin > (double)(pixelMin))
    {
    self->PixelMin = (double)(pixelMin);
    }
  if (self->PixelMax < (double)(pixelMax))
    {
    self->PixelMax = (double)(pixelMax);
    }
  
  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function reads an image.
void vtkImageShortReader::UpdatePointData(vtkImageRegion *region)
{
  void *ptr;
  int *extent = region->GetExtent();
  int fileNumber, idx;
  
  
  // Compute the index of the file for this image.
  // Note the order: axis2 is outside loop (slowest to change).
  fileNumber = extent[4];
  for (idx = 3; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    fileNumber *= this->Dimensions[idx];
    fileNumber += extent[idx*2];
    }
  fileNumber += this->First;
  
  //  make sure we have the header size
  if ( ! this->Initialized)
    {
    this->Initialize();
    }
  
  // Get the region to fill from the cache
  if ( ! this->Output)
    {
    vtkErrorMacro(<< "UpdateRegion: Cache not created yet");
    return;
    }

  // open the correct file for this slice
  sprintf(this->FileName, this->FilePattern, this->FilePrefix, fileNumber);
  vtkDebugMacro(<< "UpdateRegion: opening file " << this->FileName);
  this->File = new ifstream(this->FileName, ios::in);
  if (! this->File)
    {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return;
    }

  // read in the slice
  ptr = region->GetScalarPointer();
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageShortReaderGenerateRegion(this, region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageShortReaderGenerateRegion(this, region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageShortReaderGenerateRegion(this, region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageShortReaderGenerateRegion(this, region, 
					  (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageShortReaderGenerateRegion(this, region, 
					  (unsigned char *)(ptr));
      break;
    }   
  
  vtkDebugMacro(<< "Min = " << this->PixelMin << ", max = " << this->PixelMax);
  
  this->File->close();
}




//----------------------------------------------------------------------------
// Description:
// Sets the default ScalarType of the cache.
vtkImageSource *vtkImageShortReader::GetOutput()
{
  this->CheckCache();
  if (this->Output->GetScalarType() == VTK_VOID)
    {
    if (this->Signed)
      {
      this->Output->SetScalarType(VTK_SHORT);
      }
    else
      {
      this->Output->SetScalarType(VTK_UNSIGNED_SHORT);
      }
    }
  
  return this->Output;
}



  
  
  
  
  
  
  







