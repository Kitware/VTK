/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolumeShortWriter.cc
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
#include "vtkImageVolumeShortWriter.hh"
#include "vtkImageCache.hh"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageVolumeShortWriter fitler.
vtkImageVolumeShortWriter::vtkImageVolumeShortWriter()
{
  this->Input = NULL;

  this->Signed = 0;
  this->SwapBytes = 0;  
  this->First = 1;
}


//----------------------------------------------------------------------------
// Description:
// Destructor
vtkImageVolumeShortWriter::~vtkImageVolumeShortWriter()
{
  // get rid of old names
  if (this->FileRoot)
    delete [] this->FileRoot;
  if (this->FileRoot)
    delete [] this->FileRoot;
}


//----------------------------------------------------------------------------
// Description:
// This function sets the root name (and path) of the image files.
void vtkImageVolumeShortWriter::SetFileRoot(char *fileRoot)
{
  long rootLength = strlen(fileRoot);
  
  // get rid of old names
  if (this->FileRoot)
    delete [] this->FileRoot;
  if (this->FileRoot)
    delete [] this->FileRoot;

  this->FileRoot = new char [rootLength + 5];
  this->FileName = new char [rootLength + 15];
  
  strcpy(this->FileRoot, fileRoot);
}



//----------------------------------------------------------------------------
// Description:
// This function writes the whole image to file.
void vtkImageVolumeShortWriter::Write()
{
  int offset[3], size[3];
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Write: Input not set.");
    return;
    }
    
  this->Input->GetBoundary(offset, size);
  this->Write(offset, size);
}



//----------------------------------------------------------------------------
// Description:
// This function writes a region of the image to file.
void vtkImageVolumeShortWriter::Write(int *offset, int *size)
{
  int idx;
  int sliceOffset[3], sliceSize[3];
  vtkImageRegion *region;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Write: Input not set.");
    return;
    }
    
  vtkDebugMacro(<< "Write: offset = ("
                << offset[0] << ", " << offset[1] << ", " << offset[2]
                << "), size = ("
                << size[0] << ", " << size[1] << ", " << size[2]
                << ")");

  for (idx = 0; idx < 3; ++idx)
    {
    sliceOffset[idx] = offset[idx];
    sliceSize[idx] = size[idx];
    }
 
  // write the volume slice by slice
  sliceSize[2] = 1;
  for (idx = 0; idx < sliceSize[2]; ++idx)
    {
    sliceOffset[2] = offset[2] + idx;
    region = this->Input->RequestRegion(offset, size);
    if ( ! region)
      vtkErrorMacro(<< "Write: Request for image " << idx << " failed.");
    else
      this->WriteSlice(region);
    }
}


//----------------------------------------------------------------------------
// Description:
// This function writes a slice into a file.
void vtkImageVolumeShortWriter::WriteSlice(vtkImageRegion *region)
{
  ofstream *file;
  int streamRowRead;
  int idx0, idx1;
  int size0, size1, size2;
  int inc0, inc1, inc2;
  int *offset;
  float *ptr0, *ptr1;
  unsigned char *buf, *pbuf, temp;
  
  offset = region->GetOffset();
  sprintf(this->FileName, "%s.%d", this->FileRoot, offset[2] + this->First);
  vtkDebugMacro(<< "WriteSlice: " << this->FileName);
  
  file = new ofstream(this->FileName, ios::out);
  if (! file)
    {
    vtkErrorMacro(<< "Could not open file " << this->FileName);
    return;
    }
  
  region->GetSize(size0, size1, size2);
  region->GetInc(inc0, inc1, inc2);
  streamRowRead = size0 * sizeof(short int);
  buf = new unsigned char [streamRowRead];
  
  // loop through rows in single slice
  ptr1 = region->GetPointer(region->GetOffset());
  for (idx1 = 0; idx1 < size1; ++idx1)
    {
    ptr0 = ptr1;
    pbuf = buf;
    // copy the row to short buffer
    for (idx0 = 0; idx0 < size0; ++idx0)
      {
      if (this->Signed)
	*((short *)(pbuf)) = (short)(*ptr0);
      else
	*((unsigned short *)(pbuf)) = (unsigned short)(*ptr0);
      // handle byte swapping
      if (this->SwapBytes)
	{
	temp = *pbuf;
	*pbuf = pbuf[1];
	pbuf[1] = temp;
	}
      
      ptr0 += inc0;
      ++pbuf;
      }
    
    // write a row
    if ( ! file->write(buf, streamRowRead))
      {
      vtkErrorMacro(<< "WriteSlice: write failed");
      file->close();
      delete file;
      delete [] buf;
      return;
      }
    ptr1 += inc1;
    }
  
  file->close();
  delete file;
  delete [] buf;
}


  
  







