/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolumeShortReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageVolumeShortReader.hh"
#include "vtkImageCache.hh"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageVolumeShortReader fitler.
vtkImageVolumeShortReader::vtkImageVolumeShortReader()
{
  this->File = NULL;

  this->Signed = 0;
  this->SwapBytes = 0;  
  this->First = 1;
  
  this->Size[0] = 512;
  this->Size[1] = 512;
  this->Size[2] = 1;

  this->Increments[0] = 1;
  this->Increments[1] = 512;
  this->Increments[2] = 512 * 512;

  this->SetAspectRatio(1.0, 1.0, 1.0);
  
  this->FileRoot[0] = '\0';
  this->HeaderSize = 0;
  this->PixelMask = 65535;
}


//----------------------------------------------------------------------------
void vtkImageVolumeShortReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
  
  os << indent << "FileRoot: " << this->FileRoot << "\n";
  os << indent << "HeaderSize: " << this->HeaderSize << "\n";
  os << indent << "Signed: " << this->Signed << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
  os << indent << "Size: (" << this->Size[0] << ", " 
     << this->Size[1] << ", " << this->Size[2] << ")\n";
  os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", " 
     << this->AspectRatio[1] << ", " << this->AspectRatio[2] << ")\n";
}


//----------------------------------------------------------------------------
// Description:
// This sets the dimensions of the image in the file
void vtkImageVolumeShortReader::SetSize(int size0, int size1, int size2)
{
  vtkDebugMacro(<< "SetSize: (" 
                << size0 << ", " << size1 << ", " << size2 << ")");

  this->Size[0] = size0;
  this->Size[1] = size1;
  this->Size[2] = size2;

  this->Increments[0] = 1;
  this->Increments[1] = size0;
  this->Increments[2] = size0 * size1;

  this->Modified();
}
void vtkImageVolumeShortReader::SetSize(int *size)
{
  this->SetSize(size[0], size[1], size[2]);
}


//----------------------------------------------------------------------------
// Description:
// This method returns the largest region that can be generated.
void vtkImageVolumeShortReader::UpdateImageInformation(vtkImageRegion *region)
{
  region->SetImageBounds3d(0, this->Size[0]-1, 
			   0, this->Size[1]-1, 
			   0, this->Size[2]-1);
  region->SetAspectRatio3d(this->AspectRatio);
}



//----------------------------------------------------------------------------
// Description:
// This function opens a file for reading.
void vtkImageVolumeShortReader::SetFileRoot(char *fileRoot)
{

  strcpy(this->FileRoot, fileRoot);
  
  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    delete this->File;
    this->File = NULL;
    }
  
  sprintf(this->FileName, "%s.%d", this->FileRoot, this->First);
  
  // Open the new file
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
  
  vtkDebugMacro(<< "SetFileName: Header " << this->HeaderSize 
                << " bytes, fileLength = " << this->FileSize << " bytes.");
  
  this->File->close();
  delete this->File;
  this->File = NULL;
}

//----------------------------------------------------------------------------
// Description:
// This function reads in one slice of the region.
// templated to handle different data types.
template <class T>
void vtkImageVolumeShortReaderGenerateData2d(vtkImageVolumeShortReader *self,
					     vtkImageRegion *region, T *ptr)
{
  int min0, max0,  min1, max1;
  int inc0, inc1;
  T *pf0, *pf1;
  long streamStartPos;
  long streamRowSkip;
  long streamRowRead;
  unsigned char *buf, *pbuf;
  unsigned char swap[2];
  unsigned short *pshort;
  int idx0, idx1;

  // get the information needed to find a location in the file
  region->GetBounds2d(min0, max0,  min1, max1);
  region->GetIncrements2d(inc0, inc1);
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
    cerr << "vtkImageVolumeShortReader GenerateData2d: bad offset";
    return;
    }
    
  // move to the correct location in the file (offset of region)
  self->File->seekg(streamStartPos, ios::beg);
  if (self->File->fail())
    {
    cerr << "File operation failed.";
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[streamRowRead]; 
  
  // read the data row by row
  pf1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    if ( ! self->File->read(buf, streamRowRead))
      {
      cerr << "File operation failed. row = " << idx1
	   << ", StartPos = " << streamStartPos
	   << ", RowSkip = " << streamRowSkip
	   << ", RowRead = " << streamRowRead
	   << ", FilePos = " << self->File->tellg();
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

      // move to next pixel
      pbuf += 2;
      pf0 += inc0;
      }
    // move to the next row in the file and region
    self->File->seekg(streamRowSkip, ios::cur);
    pf1 += inc1;
    }
  
  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function reads an image.
void vtkImageVolumeShortReader::UpdateRegion2d(vtkImageRegion *region)
{
  void *ptr;
  int image = region->GetDefaultCoordinate2();

  
  // Get the region to fill from the cache
  if ( ! this->Output)
    {
    vtkErrorMacro(<< "UpdateRegion: Cache not created yet");
    return;
    }
  this->Output->AllocateRegion(region);

  // open the correct file for this slice
  sprintf(this->FileName, "%s.%d", this->FileRoot, image);
  vtkDebugMacro(<< "UpdateRegion2d: opening file " << this->FileName);
  this->File = new ifstream(this->FileName, ios::in);
  if (! this->File)
    {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return;
    }

  // read in the slice
  ptr = region->GetVoidPointer2d();
  switch (region->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImageVolumeShortReaderGenerateData2d(this, region, (float *)(ptr));
      break;
    case VTK_IMAGE_INT:
      vtkImageVolumeShortReaderGenerateData2d(this, region, (int *)(ptr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImageVolumeShortReaderGenerateData2d(this, region, (short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImageVolumeShortReaderGenerateData2d(this, region, 
					  (unsigned short *)(ptr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImageVolumeShortReaderGenerateData2d(this, region, 
					  (unsigned char *)(ptr));
      break;
    }   
  
  this->File->close();
}




//----------------------------------------------------------------------------
// Description:
// Sets the default DataType of the cache.
vtkImageSource *vtkImageVolumeShortReader::GetOutput()
{
  this->CheckCache();
  if (this->Output->GetDataType() == VTK_IMAGE_VOID)
    {
    if (this->Signed)
      {
      this->Output->SetDataType(VTK_IMAGE_SHORT);
      }
    else
      {
      this->Output->SetDataType(VTK_IMAGE_UNSIGNED_SHORT);
      }
    }
  
  return this->Output;
}



  
  
  
  
  
  
  







