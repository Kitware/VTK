/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClippingExtents.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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

#include <math.h>
#include "vtkImageClippingExtents.h"
#include "vtkImplicitFunction.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImageClippingExtents* vtkImageClippingExtents::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageClippingExtents");
  if(ret)
    {
    return (vtkImageClippingExtents*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageClippingExtents;
}

//----------------------------------------------------------------------------
vtkImageClippingExtents::vtkImageClippingExtents()
{
  this->ClippingObject = NULL;

  this->ClippingExtent[0] =  0;
  this->ClippingExtent[1] = -1;
  this->ClippingExtent[2] =  0;
  this->ClippingExtent[3] = -1;
  this->ClippingExtent[4] =  0;
  this->ClippingExtent[5] = -1;

  this->ClippingSpacing[0] = 1;
  this->ClippingSpacing[1] = 1;
  this->ClippingSpacing[2] = 1;

  this->ClippingOrigin[0] = 0;
  this->ClippingOrigin[1] = 0;
  this->ClippingOrigin[2] = 0;

  this->ClippingLists = NULL;
  this->ClippingListLengths = NULL;
}

//----------------------------------------------------------------------------
vtkImageClippingExtents::~vtkImageClippingExtents()
{
  if (this->ClippingObject)
    {
    this->ClippingObject->Delete();
    }

  if (this->ClippingLists)
    {
    int n = ((this->ClippingExtent[3] - this->ClippingExtent[2] + 1) *
             (this->ClippingExtent[5] - this->ClippingExtent[4] + 1));
    for (int i = 0; i < n; i++)
      {
      delete [] this->ClippingLists[i];
      }
    delete [] this->ClippingLists;
    }
  if (this->ClippingListLengths)
    {
    delete [] this->ClippingListLengths;
    }
}

//----------------------------------------------------------------------------
void vtkImageClippingExtents::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImageClippingExtents::BuildExtents(vtkImageData *output)
{
  // check to see whether the output information has changed
  int informationChanged = 0;
  int *extent = output->GetWholeExtent();
  float *spacing = output->GetSpacing();
  float *origin = output->GetOrigin();

  for (int j = 0; j < 3; j++)
    {
    if (extent[2*j] != this->ClippingExtent[2*j] ||
        extent[2*j+1] != this->ClippingExtent[2*j+1] ||
        spacing[j] != this->ClippingSpacing[j] ||
        origin[j] != this->ClippingOrigin[j])
      {
      informationChanged = 1;
      break;
      }
    }

  // check to see whether the clipping object has changed
  int clippingChanged = 0;
  if (this->ClippingObject->IsA("vtkDataObject"))
    {
    ((vtkDataObject *)this->ClippingObject)->Update();
    }

  if (this->GetMTime() > this->BuildTime.GetMTime() ||
      (this->ClippingObject &&
       this->ClippingObject->GetMTime() > this->BuildTime.GetMTime()))
    {
    clippingChanged = 1;
    }

  if (!(clippingChanged || informationChanged))
    {
    return;
    }
  
  // delete the old clipping information
  if (this->ClippingLists)
    {
    int n = ((this->ClippingExtent[3] - this->ClippingExtent[2] + 1) *
             (this->ClippingExtent[5] - this->ClippingExtent[4] + 1));
    for (int i = 0; i < n; i++)
      {
      delete [] this->ClippingLists[i];
      }
    delete [] this->ClippingLists;
    this->ClippingLists = NULL;
    }
  if (this->ClippingListLengths)
    {
    delete [] this->ClippingListLengths;
    this->ClippingListLengths = NULL;
    }

  // copy WholeExtent to ClippingExtent
  output->GetWholeExtent(this->ClippingExtent);
  output->GetSpacing(this->ClippingSpacing);
  output->GetOrigin(this->ClippingOrigin);

  if (!this->ClippingObject)
    {
    return;
    }

  this->PrepareForThreadedBuildExtents();
  // no multithreading yet...
  this->ThreadedBuildExtents(extent, 0);

  this->Modified();
  this->BuildTime.Modified();
}

void vtkImageClippingExtents::PrepareForThreadedBuildExtents()
{
}

void vtkImageClippingExtents::ThreadedBuildExtents(int extent[6],
                                                   int vtkNotUsed(ThreadId))
{
  float *spacing = this->ClippingSpacing;
  float *origin = this->ClippingOrigin;
  vtkObject *clipper = this->ClippingObject;

  if (clipper->IsA("vtkImplicitFunction"))
    {
    // allocate new clipping information
    int n = ((extent[3] - extent[2] + 1) *
             (extent[5] - extent[4] + 1));

    this->ClippingLists = new int *[n];
    this->ClippingListLengths = new int[n];

    // set up the clipping extents from an implicit function by brute force
    // (evaluate the function at each and every voxel)
    vtkImplicitFunction *function = (vtkImplicitFunction *)clipper;

    float point[3];
    int *clippingListLength = this->ClippingListLengths;
    int **clippingList = this->ClippingLists;

    for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
      {
      point[2] = idZ*spacing[2] + origin[2];

      for (int idY = extent[2]; idY <= extent[3]; idY++)
        {
        point[1] = idY*spacing[1] + origin[1];
        int clistlen = 0;
        int clistmaxlen = 2;
        int *clist = new int[clistmaxlen];
        int state = 1; // inside or outside, start outside

        for (int idX = extent[0]; idX <= extent[1]; idX++)
          {
          point[0] = idX*spacing[0] + origin[0];
          int newstate = 1;
          if (function->FunctionValue(point) < 0)
            {
            newstate = -1;
            }
          if (newstate != state)
            {
            if (clistlen >= clistmaxlen)
              { // need to allocate more space
              clistmaxlen *= 2;
              int *newclist = new int[clistmaxlen];
              for (int k = 0; k < clistlen; k++)
                {
                newclist[k] = clist[k];
                }
              delete [] clist;
              clist = newclist;
              }

            clist[clistlen++] = idX;
            }

          state = newstate;
          } // for idX

        *clippingListLength++ = clistlen;
        *clippingList++ = clist;
        } // for idY

      } // for idZ
    }
  else
    {
    vtkErrorMacro("Update: unrecognized clipping object type " << 
                  clipper->GetClassName());
    }
}

//----------------------------------------------------------------------------
// Given the output x extent [rmin,rmax] and the current y, z indices,
// return the sub extents [r1,r2] for which transformation/interpolation
// are to be done.  The variable 'iter' should be initialized to zero
// before the first call.  The return value is zero if there are no
// extents remaining.
int vtkImageClippingExtents::GetNextExtent(int &r1, int &r2,
                                           int rmin, int rmax,
                                           int yIdx, int zIdx, int &iter)
{
  int yExt = this->ClippingExtent[3] - this->ClippingExtent[2] + 1;
  int zExt = this->ClippingExtent[5] - this->ClippingExtent[4] + 1;
  yIdx -= this->ClippingExtent[2];
  zIdx -= this->ClippingExtent[4];

  // initialize r1, r2 to defaults
  r1 = rmax + 1;
  r2 = rmax;

  if (yIdx < 0 || yIdx >= yExt || zIdx < 0 || zIdx >= zExt)
    { // out-of-bounds in y or z, use null extent
    return 0;
    }

  // if no clipping object is set, just use ClippingExtent
  if (this->ClippingLists == NULL)
    {
    if (iter++ == 0)
      {
      r1 = this->ClippingExtent[0];
      r2 = this->ClippingExtent[1];
      if (r1 < rmin)
        {
        r1 = rmin;
        }
      if (r1 > rmax)
        {
        r1 = rmax + 1;
        }
      if (r2 > rmax)
        {
        r2 = rmax;
        }
      return (r2 >= r1);
      }
    else
      {
      return 0;
      }
    }

  // get the ClippingList and ClippingListLength for this yIdx,zIdx
  int incr = zIdx*yExt + yIdx;
  int *clist = this->ClippingLists[incr];
  int clistlen = this->ClippingListLengths[incr];

  if (iter == 0)
    {
    int state = 1; // start outside
    r1 = VTK_INT_MIN;
    for ( ; iter < clistlen; iter++)
      {
      if (clist[iter] >= rmin)
        {
        if (state > 0)
          {
          r1 = clist[iter++];
          }
        break;
        }
      state = -state;
      }
    if (r1 == VTK_INT_MIN)
      {
      r1 = rmin;
      if (state > 0)
	{
	r1 = rmax + 1;
	}
      }
    }
  else
    {
    if (iter >= clistlen)
      {
      return 0;
      }
    r1 = clist[iter++];
    }

  if (r1 > rmax)
    {
    r1 = rmax + 1;
    return 0;
    }

  if (iter >= clistlen)
    {
    return 1;
    }

  r2 = clist[iter++] - 1;

  if (r2 > rmax)
    {
    r2 = rmax;
    }

  return 1;
}

