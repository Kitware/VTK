/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePolyDataClippingExtents.cxx
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
#include "vtkImagePolyDataClippingExtents.h"
#include "vtkPolyData.h"
#include "vtkOBBTree.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImagePolyDataClippingExtents* vtkImagePolyDataClippingExtents::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImagePolyDataClippingExtents");
  if(ret)
    {
    return (vtkImagePolyDataClippingExtents*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImagePolyDataClippingExtents;
}

//----------------------------------------------------------------------------
vtkImagePolyDataClippingExtents::vtkImagePolyDataClippingExtents()
{
  this->OBBTree = NULL;
}

//----------------------------------------------------------------------------
vtkImagePolyDataClippingExtents::~vtkImagePolyDataClippingExtents()
{
  if (this->OBBTree)
    {
    this->OBBTree->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImagePolyDataClippingExtents::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageClippingExtents::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkImagePolyDataClippingExtents::PrepareForThreadedBuildExtents()
{
  vtkObject *clipper = this->ClippingObject;

  // build OBB tree if clipping with polydata
  if (clipper->IsA("vtkPolyData"))
    {
    vtkPolyData *polydata = (vtkPolyData *)clipper;
    if (this->OBBTree == NULL)
      {
      this->OBBTree = vtkOBBTree::New();
      }
    this->OBBTree->SetDataSet(polydata);
    this->OBBTree->SetTolerance(0);
    this->OBBTree->BuildLocator();
    }
  else
    {
    if (this->OBBTree)
      {
      this->OBBTree->Delete();
      this->OBBTree = NULL;
      }
    }
}

//----------------------------------------------------------------------------
static inline
void vtkAddEntryToList(int *&clist, int &clistlen, int &clistmaxlen, int r)
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

  if (clistlen > 0 && r == clist[0])
    { // chop out zero-length extents
    clistlen--;
    }
  else
    {
    clist[clistlen++] = r;
    }
}

//----------------------------------------------------------------------------
static
void vtkTurnPointsIntoList(vtkPoints *points, int *&clist, int &clistlen,
			   int extent[6], float origin[3], float spacing[3],
			   int state, int dim)
{
  int clistmaxlen = 2;
  clistlen = 0;
  clist = new int[clistmaxlen];

  if (state <= 0)
    {
    clist[clistlen++] = extent[2*dim];
    }

  int npoints = points->GetNumberOfPoints();
  for (int idP = 0; idP < npoints; idP++)
    {
    float *point = points->GetPoint(idP);
    int r = (int)ceil((point[dim] - origin[dim])/spacing[dim]);
    vtkAddEntryToList(clist, clistlen, clistmaxlen, r);
    }
}

//----------------------------------------------------------------------------
void vtkImagePolyDataClippingExtents::ThreadedBuildExtents(int extent[6],
							   int ThreadId)
{
  float *spacing = this->ClippingSpacing;
  float *origin = this->ClippingOrigin;
  vtkObject *clipper = this->ClippingObject;

  if (clipper->IsA("vtkPolyData") || clipper->IsA("vtkOBBTree"))
    {
    // allocate new clipping information
    int n = ((extent[3] - extent[2] + 1) *
	     (extent[5] - extent[4] + 1));

    this->ClippingLists = new int *[n];
    this->ClippingListLengths = new int[n];

    vtkOBBTree *tree;

    if (clipper->IsA("vtkOBBTree"))
      {
      tree = (vtkOBBTree *)clipper;
      }
    else
      {
      tree = this->OBBTree;
      }

    vtkPoints *points = vtkPoints::New();

    float p0[3],p1[3];
    int *clippingListLength = this->ClippingListLengths;
    int **clippingList = this->ClippingLists;

    p1[0] = p0[0] = extent[0]*spacing[0] + origin[0];
    p1[0] = p0[1] = extent[2]*spacing[1] + origin[1];
    p0[2] = extent[4]*spacing[2] + origin[2];
    p1[2] = extent[5]*spacing[2] + origin[2];

    int state = tree->InsideOrOutside(p0);
    if (state == 0)
      {
      state = -1;
      }

    int *zlist = 0;
    int zlistlen = 0;
    int zlistidx = 0;
    if (extent[4] != extent[5])
      {
      tree->IntersectWithLine(p0, p1, points, 0);
      vtkTurnPointsIntoList(points, zlist, zlistlen,
			    extent, origin, spacing, 1, 2);
      }

    for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
      {
      if (zlistidx < zlistlen && idZ >= zlist[zlistidx])
	{
	state = -state;
	zlistidx++;
	}

      p1[0] = p0[0] = extent[0]*spacing[0] + origin[0];
      p0[1] = extent[2]*spacing[1] + origin[1];
      p1[1] = extent[3]*spacing[1] + origin[1];
      p1[2] = p0[2] = idZ*spacing[2] + origin[2];

      int *ylist = 0;
      int ylistlen = 0;
      int ylistidx = 0;
      if (extent[2] != extent[3])
	{
	tree->IntersectWithLine(p0, p1, points, 0);
	vtkTurnPointsIntoList(points, ylist, ylistlen,
			      extent, origin, spacing, 1, 1);
	}

      for (int idY = extent[2]; idY <= extent[3]; idY++)
	{
	if (ylistidx < ylistlen && idY >= ylist[ylistidx])
	  {
	  state = -state;
	  ylistidx++;
	  }

	p0[1] = p1[1] = idY*spacing[1] + origin[1];
	p0[2] = p1[2] = idZ*spacing[2] + origin[2];
	p0[0] = extent[0]*spacing[0] + origin[0];
	p1[0] = extent[1]*spacing[0] + origin[0];

	tree->IntersectWithLine(p0, p1, points, 0);
	vtkTurnPointsIntoList(points, *clippingList++, *clippingListLength++,
			      extent, origin, spacing, state, 0);

	} // for idY
      if (ylist)
	{
	delete [] ylist;
	}

      } // for idZ

    if (zlist)
      {
      delete [] zlist;
      }
    points->Delete();
    return;
    }

  this->vtkImageClippingExtents::ThreadedBuildExtents(extent, ThreadId);
}
