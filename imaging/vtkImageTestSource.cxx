/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTestSource.cxx
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
#include "vtkImageTestSource.h"
#include "vtkImageCache.h"

//----------------------------------------------------------------------------
// Description:
// This method generates the data for a given tile.
// As an example, a (128x128x128) pyramid is generated.
void vtkImageTestSource::GenerateRegion(int *outOffset, int *outSize)
{
  vtkImageRegion *outRegion;
  int idx0, idx1, idx2;
  int p0, p1, p2;
  int size0, size1, size2;
  int inc0, inc1, inc2;
  int offset0, offset1, offset2;
  float *ptr0, *ptr1, *ptr2;
  float val;

  vtkDebugMacro(<< "GenerateRegion: offset = ("
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = ("
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2]
                << ")");

  // Get the tile to fill from the cache
  if ( ! this->Output)
    {
    vtkErrorMacro(<< "GenerateRegion: Cache not created yet");
    return;
    }
  outRegion = this->Output->GetRegion(outOffset, outSize);
  
  // Get information to march through data
  ptr2 = outRegion->GetPointer(outRegion->GetOffset());
  outRegion->GetInc(inc0, inc1, inc2);
  outRegion->GetSize(size0, size1, size2);
  outRegion->GetOffset(offset0, offset1, offset2);
  
  // generate data
  for (idx2 = 0, p2 = offset2; idx2 < size2; ++idx2, ++p2){
    ptr1 = ptr2;
    for (idx1 = 0, p1 = offset1; idx1 < size1; ++idx1, ++p1){
      ptr0 = ptr1;
      for (idx0 = 0, p0 = offset0; idx0 < size0; ++idx0, ++p0){
	// compute the value at this voxel
	val = 128.0;
	if (p2 > p1)
	  val = 50.0;
	if (p2 > (128 - p1))
	  val = 50.0;
	if (p2 > p0)
	  val = 50.0;
	if (p2 > (128 - p0))
	  val = 50.0;
	if (p2 < 0)
	  val = 50.0;
	// save the value in the data array
	*ptr0 = val;

	ptr0 += inc0;
      }
      ptr1 += inc1;
    }
    ptr2 += inc2;
  }
}















