/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolumeShortWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include "vtkImageVolumeShortWriter.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageVolumeShortWriter fitler.
vtkImageVolumeShortWriter::vtkImageVolumeShortWriter()
{
  this->Input = NULL;

  this->Signed = 0;
  this->SwapBytes = 0;  
  this->First = 1;
  this->FileRoot = NULL;
  this->FileName = NULL;
}


//----------------------------------------------------------------------------
// Description:
// Destructor
vtkImageVolumeShortWriter::~vtkImageVolumeShortWriter()
{
  // get rid of old names
  if (this->FileRoot)
    delete [] this->FileRoot;
  if (this->FileName)
    delete [] this->FileName;
}


//----------------------------------------------------------------------------
// Description:
// This function sets the root name (and path) of the image files.
void vtkImageVolumeShortWriter::SetFileRoot(char *fileRoot)
{
  long rootLength = strlen(fileRoot);
  
  vtkDebugMacro(<< "SetFileRoot: root = " << fileRoot);
  
  // get rid of old names
  if (this->FileRoot)
    delete [] this->FileRoot;
  if (this->FileName)
    delete [] this->FileName;

  this->FileRoot = new char [rootLength + 5];
  this->FileName = new char [rootLength + 15];
  
  strcpy(this->FileRoot, fileRoot);
}



//----------------------------------------------------------------------------
// Description:
// This function writes the whole image to file.
void vtkImageVolumeShortWriter::Write()
{
  int extent[6];
  vtkImageRegion region;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Write: Input not set.");
    return;
    }
    
  this->Input->UpdateImageInformation(&region);
  region.GetImageExtent(3, extent);
  this->Write(extent);
}



//----------------------------------------------------------------------------
// Description:
// For tcl.
void vtkImageVolumeShortWriter::Write(int min0, int max0, 
				      int min1, int max1,
				      int min2, int max2)
{
  int extent[6];
  
  extent[0] = min0;
  extent[1] = max0;
  extent[2] = min1;
  extent[3] = max1;
  extent[4] = min2;
  extent[5] = max2;
  
  this->Write(extent);  
}



//----------------------------------------------------------------------------
// Description:
// This function writes a region of the image to file.
// It requests and writes the volume one 2d image at a time.
void vtkImageVolumeShortWriter::Write(int *extent)
{
  int idx;
  int sliceExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  vtkImageRegion region;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Write: Input not set.");
    return;
    }

  // deal with extra dimensions by taking the first
  this->Input->UpdateImageInformation(&region);
  region.GetImageExtent(sliceExtent);
  for (idx = 3; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    sliceExtent[idx*2] = sliceExtent[idx*2+1];
    }

  // Set the sub region requested
  for (idx = 0; idx < 6; ++idx)
    {
    sliceExtent[idx] = extent[idx];
    }
 
  // write the volume slice by slice
  for (idx = extent[4]; idx <= extent[5]; ++idx)
    {
    sliceExtent[4] = sliceExtent[5] = idx;
    region.SetExtent(sliceExtent);
    this->Input->UpdateRegion(&region);
    if ( ! region.AreScalarsAllocated())
      vtkErrorMacro(<< "Write: Request for image " << idx << " failed.");
    else
      this->Write2d(&region);
    region.ReleaseData();
    }
}


//----------------------------------------------------------------------------
// This function writes a slice into a file.
template <class T>
void vtkImageVolumeShortWriterWrite2D(vtkImageVolumeShortWriter *self,
				      vtkImageRegion *region, T *ptr)
{
  ofstream *file;
  int streamRowRead;
  int idx0, idx1;
  int min0, max0, min1, max1;
  int inc0, inc1;
  T *ptr0, *ptr1;
  unsigned char *buf, *pbuf, temp;
  int *extent = region->GetExtent();
  
  sprintf(self->FileName, "%s.%d", self->FileRoot, 
	  extent[4] + self->First);
  if (self->Debug)
    {
    cerr << "Debug: In " __FILE__ << ", line " << __LINE__ << "\n" 
	 << self->GetClassName() << " (" << self << "): ";
    cerr << "WriteSlice: " << self->FileName << "\n\n";
    }

  file = new ofstream(self->FileName, ios::out);
  if (! file)
    {
    cerr << "vtkImageVolumeShortWriterWrite2D: ERROR: "
	 << "Could not open file " << self->FileName;
    return;
    }
  
  region->GetExtent(min0, max0, min1, max1);
  region->GetIncrements(inc0, inc1);
  streamRowRead = (max0-min0+1) * sizeof(short int);
  buf = new unsigned char [streamRowRead];
  
  // loop through rows in single slice
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    pbuf = buf;
    // copy the row to short buffer
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      if (self->Signed)
	*((short *)(pbuf)) = (short)(*ptr0);
      else
	*((unsigned short *)(pbuf)) = (unsigned short)(*ptr0);
      // handle byte swapping
      if (self->SwapBytes)
	{
	temp = *pbuf;
	*pbuf = pbuf[1];
	pbuf[1] = temp;
	}
      
      ptr0 += inc0;
      pbuf += 2;
      }
    
    // write a row
    if ( ! file->write((char *)buf, streamRowRead))
      {
      cerr << "vtkImageVolumeShortWriterWrite2: ERROR: "
	   << "WriteSlice: write failed";
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


  
  

//----------------------------------------------------------------------------
// Description:
// This function writes a slice into a file.
void vtkImageVolumeShortWriter::Write2d(vtkImageRegion *region)
{
  void *ptr = region->GetScalarPointer();
  
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageVolumeShortWriterWrite2D(this, region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageVolumeShortWriterWrite2D(this, region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageVolumeShortWriterWrite2D(this, region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageVolumeShortWriterWrite2D(this, region, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageVolumeShortWriterWrite2D(this, region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "Write2d: Cannot handle data type.");
    }   
}








