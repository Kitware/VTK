/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortWriter.cxx
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
#include "vtkImageShortWriter.h"



//----------------------------------------------------------------------------
vtkImageShortWriter::vtkImageShortWriter()
{
  int idx;
  
  this->Signed = 0;
  this->SwapBytes = 0;  
  this->FilePrefix = NULL;
  this->FilePattern = NULL;
  this->FileName = NULL;
  
  this->SetFilePrefix("");
  this->SetFilePattern("%s.%d");
  
  this->Input = NULL;
  this->WholeImage = 1;
  this->InputMemoryLimit = 5000000;  // A very big image indeed.
  // Split order can not be modified.
  this->SplitOrder[0] = VTK_IMAGE_TIME_AXIS;
  this->SplitOrder[1] = VTK_IMAGE_Z_AXIS;
  this->SplitOrder[2] = VTK_IMAGE_COMPONENT_AXIS;
  this->SplitOrder[3] = VTK_IMAGE_Y_AXIS;
  this->SplitOrder[4] = VTK_IMAGE_X_AXIS;

  // Set default axes. ...
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Axes[idx] = idx;
    this->Extent[idx*2] = this->Extent[idx*2+1] = 0;
    }
}



//----------------------------------------------------------------------------
vtkImageShortWriter::~vtkImageShortWriter()
{
  // get rid of memory allocated for file names
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
void vtkImageShortWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Input: (" << this->Input << ")\n";
  os << indent << "WholeImage: " << this->WholeImage << "\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] 
     << ", " << this->Extent[2] << ", " << this->Extent[3] << ", " 
     << this->Extent[4] << ", " << this->Extent[5] << ")\n";

  os << indent << "InputMemoryLimit: " << this->InputMemoryLimit << "\n";
  os << indent << "SplitOrder: (";
  os << vtkImageAxisNameMacro(this->SplitOrder[0]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[1]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[2]) << ", ";
  os << vtkImageAxisNameMacro(this->SplitOrder[3]) << ")\n";

  os << indent << "FilePrefix: " << this->FilePrefix << "\n";
  os << indent << "FilePattern: " << this->FilePattern << "\n";
  os << indent << "Signed: " << this->Signed << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
}

//----------------------------------------------------------------------------
// Description:
// This function sets the prefix of the file name. "image" would be the
// name of a series: image.1, image.2 ...
void vtkImageShortWriter::SetFilePrefix(char *prefix)
{
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
void vtkImageShortWriter::SetFilePattern(char *pattern)
{
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


//----------------------------------------------------------------------------
void vtkImageShortWriter::GetSplitOrder(int num, int *axes)
{
  int idx;

  if (num > 5)
    {
    vtkWarningMacro(<< "GetSplitOrder: Only returning "
      << 5 << " of requested " << num << " axes");
    num = 5;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->SplitOrder[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageShortWriter::SetAxes(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetAxes: " << num << "is to many axes.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  for (idx = 0; idx < num; ++idx)
    {
    this->Axes[idx] = axes[idx];
    }
  
}
//----------------------------------------------------------------------------
void vtkImageShortWriter::GetAxes(int num, int *axes)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetAxes: Requesting too many axes");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    axes[idx] = this->Axes[idx];
    }
  
}

  

//----------------------------------------------------------------------------
void vtkImageShortWriter::SetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "SetExtent: " << num << "is to large.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  this->Modified();
  this->WholeImageOff();
  for (idx = 0; idx < num*2; ++idx)
    {
    this->Extent[idx] = extent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkImageShortWriter::GetExtent(int num, int *extent)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkWarningMacro(<< "GetExtent: Requesting too large");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num*2; ++idx)
    {
    extent[idx] = this->Extent[idx];
    }
  
}

  

//----------------------------------------------------------------------------
// Description:
// Writes a file from its input.  The size of the region is determined by
// WholeImage flag and Extent.
void vtkImageShortWriter::Write()
{
  vtkImageRegion *region = new vtkImageRegion;
  int *extent;
  
  // Error checking
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
    }
  if ( ! this->FilePrefix || ! this->FilePattern)
    {
    vtkErrorMacro(<<"Write:Please specify a file prefix and pattern");
    return;
    }
  
  // Make sure the file name is allocated
  if ( ! this->FileName)
    {
    this->FileName = new char[strlen(this->FilePrefix) +
			     strlen(this->FilePattern) + 50];
    }  
  
  // Set the coordinate system of the region
  region->SetAxes(VTK_IMAGE_DIMENSIONS, this->Axes);
  
  // Fill in image information.
  this->Input->UpdateImageInformation(region);

  // Determine the extent of the region we are writing
  if (this->WholeImage)
    {
    extent = region->GetImageExtent();
    region->SetExtent(VTK_IMAGE_DIMENSIONS, extent);    
    }
  else
    {
    region->SetExtent(VTK_IMAGE_DIMENSIONS, this->Extent);
    }

  this->WriteRegion(region);
  region->Delete();
}


//----------------------------------------------------------------------------
// Description:
// For streaming.  Writes a specific region.  The region parameter only 
// comunicates the extent.
void vtkImageShortWriter::WriteRegion(vtkImageRegion *region)
{
  long memory;
  
  // Compute the amount of memory used by the region.
  memory = region->GetVolume();
  switch (this->Input->GetScalarType())
    {
    case VTK_FLOAT:
      memory *= sizeof(float);
      break;
    case VTK_INT:
      memory *= sizeof(int);
      break;
    case VTK_SHORT:
      memory *= sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      memory *= sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      memory *= sizeof(unsigned char);
      break;
    default:
      vtkWarningMacro(<< "WriteRegion: Unknown type");
    }
  // convert to KBytes
  memory /= 1000;
  
  // Handle streaming by splitting the request.
  if ( memory > this->InputMemoryLimit)
    {
    int splitAxisIdx, splitAxis;
    int min, max, mid;
    // We need to split the region.
    // Pick an axis to split
    splitAxisIdx = 0;
    splitAxis = this->SplitOrder[splitAxisIdx];
    region->GetAxisExtent(splitAxis, min, max);
    while ( (min == max) && splitAxisIdx < 3)
      {
      ++splitAxisIdx;
      splitAxis = this->SplitOrder[splitAxisIdx];
      region->GetAxisExtent(splitAxis, min, max);
      }
    // Special case if we need to split an image
    if (min == max)
      {
      vtkWarningMacro(<< "WriteRegion: Cannot split an image (yet). memory = "
        << memory << ", limit = " << this->InputMemoryLimit << ", "
        << vtkImageAxisNameMacro(splitAxis) << ": " << min << "->" << max);

      // Request the data anyway
      this->Input->UpdateRegion(region);
      this->WriteRegionData(region);
      return;
      }
    // Set the first half to save
    mid = (min + max) / 2;
    vtkDebugMacro(<< "WriteRegion: Splitting " 
        << vtkImageAxisNameMacro(splitAxis) << ": " << min << "->" << mid
        << ", " << mid+1 << "->" << max);
    region->SetAxisExtent(splitAxis, min, mid);
    this->WriteRegion(region);
    // Set the second half to save
    region->SetAxisExtent(splitAxis, mid+1, max);
    this->WriteRegion(region);
    // Restore the original extent
    region->SetAxisExtent(splitAxis, min, max);
    return;
    }
  
  // Get the actual data
  this->Input->UpdateRegion(region);
  this->WriteRegionData(region);
}



//----------------------------------------------------------------------------
// Description:
// Writes a region (filled with data) to files.
void vtkImageShortWriter::WriteRegionData(vtkImageRegion *region)
{
  int extent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int extent2D[VTK_IMAGE_EXTENT_DIMENSIONS];
  int min2, max2, min3, max3;
  int idx2, idx3;
  
  // Make sure we actually have data.
  if ( ! region->AreScalarsAllocated())
    {
    vtkErrorMacro(<< "Could not get region from input.");
    return;
    }
  
  region->GetExtent(VTK_IMAGE_DIMENSIONS, extent);
  region->GetExtent(VTK_IMAGE_DIMENSIONS, extent2D);
  min2 = extent[4];  max2 = extent[5];
  min3 = extent[6];  max3 = extent[7];
  
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      // Set the extent to a single image
      extent2D[4] = extent2D[5] = idx2;
      extent2D[6] = extent2D[6] = idx3;
      region->SetExtent(VTK_IMAGE_DIMENSIONS, extent2D);
      this->WriteRegion2D(region);
      }
    }
  
  // Restore original extent
  region->SetExtent(VTK_IMAGE_DIMENSIONS, extent);
}
  

//----------------------------------------------------------------------------
// This function writes a slice into a file.
template <class T>
void vtkImageShortWriterWrite2D(vtkImageShortWriter *self,
				vtkImageRegion *region, T *ptr)
{
  ofstream *file;
  int streamRowRead;
  int idx0, idx1;
  int min0, max0, min1, max1;
  int inc0, inc1;
  T *ptr0, *ptr1;
  unsigned char *buf, *pbuf, temp;
  

  file = new ofstream(self->FileName, ios::out);
  if (! file)
    {
    cerr << "vtkImageShortWriterWrite2D: ERROR: "
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
      cerr << "vtkImageShortWriterWrite2: ERROR: "
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
void vtkImageShortWriter::WriteRegion2D(vtkImageRegion *region)
{
  void *ptr = region->GetScalarPointer();
  int *extent = region->GetExtent();
  int *imageExtent = region->GetImageExtent();
  int fileNumber;
  
  fileNumber = extent[4] * (imageExtent[7] - imageExtent[6] + 1) + extent[6]+1;
  sprintf(this->FileName, this->FilePattern, this->FilePrefix, fileNumber);
  vtkDebugMacro(<<"WriteRegion2D: " << this->FileName);
  
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageShortWriterWrite2D(this, region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageShortWriterWrite2D(this, region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageShortWriterWrite2D(this, region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageShortWriterWrite2D(this, region, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
    vtkImageShortWriterWrite2D(this, region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro(<< "WriteRegion2d: Cannot handle data type.");
    }   
}





  
