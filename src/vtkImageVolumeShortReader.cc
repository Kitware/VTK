/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolumeShortReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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

  this->Inc[0] = 1;
  this->Inc[1] = 512;
  this->Inc[2] = 512 * 512;
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

  this->Inc[0] = 1;
  this->Inc[1] = size0;
  this->Inc[2] = size0 * size1;

  this->Modified();
}
void vtkImageVolumeShortReader::SetSize(int *size)
{
  this->SetSize(size[0], size[1], size[2]);
}

//----------------------------------------------------------------------------
// Description:
// This function opens a file for reading.
void vtkImageVolumeShortReader::SetFileRoot(char *fileRoot)
{
  long fileLength;

  strcpy(this->FileRoot, fileRoot);
  
  // Close file from any previous image
  if (this->File)
    {
    this->File->close();
    this->File = NULL;
    }
  
  sprintf(this->FileName, "%s.%d", this->FileRoot, this->First);
  
  // Open the new file
  vtkDebugMacro(<< "SetFileName: opening Short file " << this->FileName);
  this->File = new ifstream(this->FileName, ios::in);
  if (! this->File)
    {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return;
    }
  
  // Get the size of the header from the size of the image
  this->File->seekg(0,ios::end);
  fileLength = this->File->tellg();

  this->HeaderSize = fileLength
    - sizeof(unsigned short int) * this->Inc[2];
  
  vtkDebugMacro(<< "SetFileName: Header " << this->HeaderSize 
                << " bytes, fileLength = " << fileLength << " bytes.");
  
  this->File->close();
  this->File = NULL;
}

//----------------------------------------------------------------------------
// Description:
// This function reads in one slice of the region.
void vtkImageVolumeShortReader::GenerateSlice(vtkImageRegion *region, 
					      int slice)
{
  int *offset;
  int size0, size1, size2;
  int inc0, inc1, inc2;
  float *pf0, *pf1;
  long streamStartPos;
  long streamRowSkip;
  long streamRowRead;
  unsigned char *buf, *pbuf;
  unsigned char swap[2];
  unsigned short *pshort;
  int idx0, idx1;
  
  // get the information needed to find a location in the file
  offset = region->GetOffset();
  region->GetSize(size0, size1, size2);
  region->GetInc(inc0, inc1, inc2);
  streamStartPos = offset[0] * this->Inc[0] 
                 + offset[1] * this->Inc[1];
  streamStartPos *= sizeof(unsigned short int);
  streamStartPos += this->HeaderSize;

  streamRowRead = size0 * sizeof(unsigned short int);
  streamRowSkip = (this->Inc[1] - size0 * this->Inc[0]) 
    * sizeof(unsigned short int);

  // move to the correct location in the file (offset of region)
  this->File->seekg(streamStartPos, ios::beg);
  if (this->File->fail())
    {
    vtkErrorMacro(<< "File operation failed.");
    return;
    }
  
  // create a buffer to hold a row of the region
  buf = new unsigned char[size0 * 2 + 2]; 
  
  // read the data row by row
  pf1 = region->GetPointer(offset);
  // move to the right slice
  pf1 += slice * inc2;
  for (idx1 = 0; idx1 < size1; ++idx1)
    {
    if ( ! this->File->read(buf, streamRowRead))
      {
      vtkErrorMacro(<< "File operation failed. row = " << idx1
		    << ", StartPos = " << streamStartPos
		    << ", RowSkip = " << streamRowSkip
		    << ", RowRead = " << streamRowRead
		    << ", FilePos = " << this->File->tellg());
      return;
      }
    
    // copy the bytes into the float region
    pf0 = pf1;
    pbuf = buf;
    for (idx0 = 0; idx0 < size0; ++idx0)
      {
      // handle byte swapping
      if (this->SwapBytes)
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
      *pshort = *pshort & this->PixelMask;

      // Convert to float
      if (this->Signed)
	*pf0 = (float)(*((short int *)swap));
      else
	*pf0 = (float)(*((unsigned short int *)swap));

      // move to next pixel
      pbuf += 2;
      pf0 += inc0;
      }
    // move to the next row in the file and region
    this->File->seekg(streamRowSkip, ios::cur);
    pf1 += inc1;
    }
  
  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// Description:
// This function is the external write function.  
// It writes the first image set and ignores the rest.
void vtkImageVolumeShortReader::GenerateRegion(int *outOffset, int *outSize)
{
  vtkImageRegion *region;
  int idx, image;
  

  vtkDebugMacro(<< "GenerateRegion: offset = ("
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = ("
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2]
                << ")");
  
  // Get the region to fill from the cache
  if ( ! this->Cache)
    {
    vtkErrorMacro(<< "GenerateRegion: Cache not created yet");
    return;
    }
  region = this->Cache->GetRegion(outOffset, outSize);

  // loop through the images to read
  for (idx = 0; idx < outSize[2]; ++idx)
    {
    image = idx + outOffset[2] + this->First;
    // open the correct file for this slice
    sprintf(this->FileName, "%s.%d", this->FileRoot, image);
    vtkDebugMacro(<< "GenerateRegion: opening file " << this->FileName);
    this->File = new ifstream(this->FileName, ios::in);
    if (! this->File)
      {
      vtkErrorMacro(<< "Could not open file " << this->FileName);
      return;
      }
    // read in the slice
    this->GenerateSlice(region, idx);
    this->File->close();
    }
}


//----------------------------------------------------------------------------
// Description:
// This method returns in "offset" and "size" the boundary of data
// in the image. Request for regions of the image out side of these
// bounds will have unpridictable effects and will give a file read error.
// i.e. no error checking is performed.
void vtkImageVolumeShortReader::GetBoundary(int *offset, int *size)
{
  offset[0] = offset[1] = offset[2] = 0;
  size[0] = this->Size[0];
  size[1] = this->Size[1];
  size[2] = this->Size[2];
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}

  
  
  
  
  
  
  







