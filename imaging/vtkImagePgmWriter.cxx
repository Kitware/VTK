/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePgmWriter.cxx
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
#include "vtkImagePgmWriter.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImagePgmWriter fitler.
vtkImagePgmWriter::vtkImagePgmWriter()
{
  this->Input = NULL;
}


 
//----------------------------------------------------------------------------
// Description:
// This method ignores its input and just writes a region passed as a parameter
// It only writes one slice of the image.
void vtkImagePgmWriter::WriteRegion(vtkImageRegion *region, int slice,
				    char *fileName)
{
  ofstream *fp;
  int size[3];
  int *offset;
  unsigned char bt;
  int idx0, idx1, idx2;
  int inc0, inc1, inc2;
  float *ptr0, *ptr1, *ptr2;


  vtkDebugMacro(<< "WriteRegion: filename = " << fileName 
                << ", region = (" << region << ")"); 

  // Open the file for writing 
  fp = new ofstream(fileName, ios::out);
  if ( ! fp)
    {
    vtkErrorMacro(<< "file " << fileName << " could not be opened.");
    return;
    }

  // get useful information from the region.
  region->GetSize(size);
  offset = region->GetOffset();
  region->GetInc(inc0, inc1, inc2);

  // ignore all but the first slice 
  size[2] = slice;

  // write the header information 
  (*fp) << "P5\n";
  (*fp) << size[0] << " " << size[1] << "\n";
  (*fp) << "255\n";

  // Invalid min max (min > max)
  this->Max = -1;
  this->Min = 1;

  // Write the image 
  ptr2 = region->GetPointer(offset);
  for (idx2 = 0; idx2 < size[2]; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = 0; idx1 < size[1]; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = 0; idx0 < size[0]; ++idx0)
	{

	// Save the maximum and minimum values for debug report.
	if (this->Min > this->Max)
	  {
	  this->Min = this->Max = *ptr0;
	  } 
	else 
	  {
	  if (*ptr0 > this->Max)
	    this->Max = *ptr0;
	  if (*ptr0 < this->Min)
	    this->Min = *ptr0;
	  }
	
	// write the pixel
	bt = (unsigned char)(*ptr0);
	fp->put(bt);

	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }
  

  vtkDebugMacro(<< "WriteRegion: Range of data was (" 
                << this->Min << ", " << this->Max << ")");

  // clean up 
  fp->close();
}


//----------------------------------------------------------------------------
// Description:
// This function is the external write function.  
// It writes the first image set and ignores the rest.
void vtkImagePgmWriter::Write(char *fileName, int *offset, int *size)
{
  ofstream *fp;

  vtkDebugMacro(<< "Write: filename = " << fileName << ", offset = (" 
		<< offset[0] << ", " << offset[1] << ", " << offset[2] 
		<< "), size = (" 
		<< size[0] << ", " << size[1] << ", " << size[2] << ")");

  // make sure there is a Input 
  if ( ! this->Input)
    {
    vtkErrorMacro(<<"No Input.");
    return;
    }

  // Open the file for writing 
  fp = new ofstream(fileName, ios::out);
  if ( ! fp)
    {
    vtkErrorMacro(<< "file " << fileName << " could not be opened.");
    return;
    }

  // ignore all but the first slice 
  size[2] = 1;

  // write the header information 
  (*fp) << "P5\n";
  (*fp) << size[0] << " " << size[1] << "\n";
  (*fp) << "255\n";

  // Invalid min max (min > max)
  this->Max = -1;
  this->Min = 1;
  this->WriteTiled(fp, offset, size);

  vtkDebugMacro(<< "Write: Range of data was (" 
                << this->Min << ", " << this->Max << ")");

  // clean up 
  fp->close();
}


//----------------------------------------------------------------------------
// Description:
// This function writes as large an image as it can get from its input.
void vtkImagePgmWriter::WriteImage(char *fileName, int slice)
{
  int offset[3], size[3];
  
  vtkDebugMacro(<< "WriteImage: filename = " << fileName << ", slice = "
                << slice);

  // make sure there is a Input 
  if ( ! this->Input)
    {
    vtkErrorMacro(<<"WriteImage: No Input.");
    return;
    }

  this->Input->GetBoundary(offset, size);
  if ( slice < offset[2] || slice >= offset[2] + size[2])
    {
    vtkErrorMacro(<< "WriteImage: Slice " << slice << " not in extent ["
                  << offset[2] << ", " << offset[2] + size[2] << "[");
    return;
    }
  offset[2] = slice;
  size[2] = 1;
  
  this->Write(fileName, offset, size);
}


//----------------------------------------------------------------------------
// Description:
// This function writes as large an image as it can get from its input.
void vtkImagePgmWriter::WriteImage(char *fileName)
{
  int offset[3], size[3];
  
  vtkDebugMacro(<< "WriteImage: filename = " << fileName);

  // make sure there is a Input 
  if ( ! this->Input)
    {
    vtkErrorMacro(<<"WriteImage: No Input.");
    return;
    }

  this->Input->GetBoundary(offset, size);
  // Choose the first slice to write
  size[2] = 1;
  
  this->Write(fileName, offset, size);
}



//----------------------------------------------------------------------------
// Description:
// This function writes a piece of the image, and divides the task if
// the piece can not fit into one region.
void vtkImagePgmWriter::WriteTiled(ofstream *fp, int *offset, int *size)
{
  vtkImageRegion *region;

  // get the whole image 
  region = this->Input->RequestRegion(offset, size);

  // check to see if image is too big 
  if ( ! region)
    {
    int offset1[3], offset2[3]; 
    int size1[3], size2[3];

    vtkDebugMacro(<< "WriteTiled: Region too large, must split up.");
    // divide the request into two pieces 
    offset1[0] = offset[0];  offset1[1] = offset[1];  offset1[2] = offset[2];
    offset2[0] = offset[0];  offset2[1] = offset[1];  offset2[2] = offset[2];
    size1[0] = size[0];      size1[1] = size[1];      size1[2] = size[2];  
    size2[0] = size[0];      size2[1] = size[1];      size2[2] = size[2];  
    if (size[2] > 1)
      {
      size1[2] /= 2;
      size2[2] = size[2] - size1[2];
      offset2[2] = offset[2] + size1[2];
      } 
    else if (size[1] > 1) 
      {
      size1[1] /= 2;
      size2[1] = size[1] - size1[1];
      offset2[1] = offset[1] + size1[1];
      } 
    else if (size[0] > 1) 
      {
      size1[0] /= 2;
      size2[0] = size[0] - size1[0];
      offset2[0] = offset[0] + size1[0];
      }
    else
      {
      vtkErrorMacro(<< "Cannot split any more. (request is only one pixel)");
      return;
      }
    // save the pieces 
    this->WriteTiled(fp, offset1, size1);
    this->WriteTiled(fp, offset2, size2);
    return;
    } 
  else 
    {
    unsigned char bt;
    int idx0, idx1, idx2;
    int inc0, inc1, inc2;
    float *ptr0, *ptr1, *ptr2;

    // Get information to march through data 
    region->GetInc(inc0, inc1, inc2);  
    ptr2 = region->GetPointer(offset);
    
    // Write the region to file
    for (idx2 = 0; idx2 < size[2]; ++idx2)
      {
      ptr1 = ptr2;
      for (idx1 = 0; idx1 < size[1]; ++idx1)
	{
	ptr0 = ptr1;
	for (idx0 = 0; idx0 < size[0]; ++idx0)
	  {
	  if (this->Min > this->Max)
	    {
	    this->Min = this->Max = *ptr0;
	    } 
	  else 
	    {
	    if (*ptr0 > this->Max)
	      this->Max = *ptr0;
	    if (*ptr0 < this->Min)
	      this->Min = *ptr0;
	    }
	  bt = (unsigned char)(*ptr0);
	  fp->put(bt);
	  ptr0 += inc0;
	  }
	ptr1 += inc1;
	}
      ptr2 += inc2;
      }
    
    // clean up 
    region->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImagePgmWriter::Write(char *fileName, 
			  int offset0, int offset1, int offset2, 
			  int size0, int size1, int size2)
{
  int offset[3], size[3];

  offset[0] = offset0;
  offset[1] = offset1;
  offset[2] = offset2;

  size[0] = size0;
  size[1] = size1;
  size[2] = size2;

  this->Write(fileName, offset, size);
}












