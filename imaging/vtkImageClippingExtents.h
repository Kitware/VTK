/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClippingExtents.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageClippingExtents - helper class for clipping images
// .SECTION Description
// vtkImageClippingExtents is a helper class for vtkImageToImageFilter
// classes.  Given a clipping object such as a vtkImplicitFunction, it
// will set up a list of clipping extents for each x-row through the
// image data.  The extents for each x-row can be retrieved via the 
// GetNextExtent() method after the extent lists have been built
// with the BuildExtents() method.  For large images, using clipping
// extents is much more memory efficient (and slightly more time-efficient)
// than building a mask.  This class can be subclassed to allow clipping
// with objects other than vtkImplicitFunction.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkImagePolyDataClippingExtents

#ifndef __vtkImageClippingExtents_h
#define __vtkImageClippingExtents_h


#include "vtkObject.h"
#include "vtkImageData.h"

class VTK_EXPORT vtkImageClippingExtents : public vtkObject
{
public:
  static vtkImageClippingExtents *New();
  vtkTypeMacro(vtkImageClippingExtents, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the object that will be used for clipping.  This must
  // be a vtkImplicitFunction.
  vtkSetObjectMacro(ClippingObject, vtkObject);
  vtkGetObjectMacro(ClippingObject, vtkObject);

  // Description:
  // Build new clipping extents if necessary, given the output of an
  // image filter.  This method should be called as the last step of
  // the ExecuteInformation method of the imaging filter, i.e. after
  // the data WholeExtent, Origin, and Spacing of the output data have
  // been set.
  void BuildExtents(vtkImageData *data);
                         
  // Description:
  // Given the output x extent [rmin,rmax] and the current y, z indices,
  // return each sub extent [r1,r2] that lies within within the unclipped
  // region in sequence.  A value of '0' is returned if no more sub extents
  // are available.  The variable 'iter' must be initialized to zero
  // before the first call.  The variable 'iter' is used internally
  // to keep track of which sub-extent should be returned next.
  int GetNextExtent(int &r1, int &r2, int rmin, int rmax,
                    int yIdx, int zIdx, int &iter);

protected:
  vtkImageClippingExtents();
  ~vtkImageClippingExtents();
  vtkImageClippingExtents(const vtkImageClippingExtents&) {};
  void operator=(const vtkImageClippingExtents&) {};

  // Description:
  // This method is called prior to ThreadedBuildExtents, i.e. before
  // the execution threads have been split off.  It is used to do
  // any preparatory work that is necessary before ThreadedBuildExtents().
  virtual void PrepareForThreadedBuildExtents();

  // Description:
  // Override this method to support clipping with different kinds
  // of objects.  Eventually the extent could be split up and handled
  // by multiple threads, but it isn't for now.  But please ensure
  // that all code inside this method is thread-safe.
  virtual void ThreadedBuildExtents(int extent[6], int threadId);

  vtkObject *ClippingObject;

  int ClippingExtent[6];
  float ClippingSpacing[3];
  float ClippingOrigin[3];

  int **ClippingLists;
  int *ClippingListLengths;

  vtkTimeStamp BuildTime;
};

#endif
