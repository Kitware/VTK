/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataToImageStencil.h"
#include "vtkPolyData.h"
#include "vtkOBBTree.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkPolyDataToImageStencil, "1.3");
vtkStandardNewMacro(vtkPolyDataToImageStencil);

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::vtkPolyDataToImageStencil()
{
  this->OBBTree = NULL;
  this->Tolerance = 1e-3;
}

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::~vtkPolyDataToImageStencil()
{
  if (this->OBBTree)
    {
    this->OBBTree->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::SetInput(vtkPolyData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataToImageStencil::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::ExecuteData(vtkDataObject *out)
{
  // need to build the OBB tree
  vtkPolyData *polydata = this->GetInput();
  if (this->OBBTree == NULL)
    {
    this->OBBTree = vtkOBBTree::New();
    }
  this->OBBTree->SetDataSet(polydata);
  this->OBBTree->SetTolerance(this->Tolerance);
  this->OBBTree->BuildLocator();

  // do superclass stuff
  this->vtkImageStencilSource::ExecuteData(out);
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

  if (clistlen > 0 && r <= clist[clistlen-1])
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
                           int dim)
{
  int clistmaxlen = 2;
  clistlen = 0;
  clist = new int[clistmaxlen];

  int npoints = points->GetNumberOfPoints();
  for (int idP = 0; idP < npoints; idP++)
    {
    float *point = points->GetPoint(idP);
    int r = (int)ceil((point[dim] - origin[dim])/spacing[dim]);
    if (r < extent[2*dim])
      {
      r = extent[2*dim];
      }
    if (r > extent[2*dim+1])
      {
      break;
      }
    vtkAddEntryToList(clist, clistlen, clistmaxlen, r);
    }
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::ThreadedExecute(vtkImageStencilData *data,
                                                int extent[6],
                                                int vtkNotUsed(id))
{
  float *spacing = data->GetSpacing();
  float *origin = data->GetOrigin();

  vtkOBBTree *tree = this->OBBTree;
  vtkPoints *points = vtkPoints::New();

  float p0[3],p1[3];

  p1[0] = p0[0] = extent[0]*spacing[0] + origin[0];
  p1[0] = p0[1] = extent[2]*spacing[1] + origin[1];
  p0[2] = extent[4]*spacing[2] + origin[2];
  p1[2] = extent[5]*spacing[2] + origin[2];

  int zstate = tree->InsideOrOutside(p0);
  if (zstate == 0)
    {
    zstate = -1;
    }
  int *zlist = 0;
  int zlistlen = 0;
  int zlistidx = 0;
  if (extent[4] != extent[5])
    {
    tree->IntersectWithLine(p0, p1, points, 0);
    vtkTurnPointsIntoList(points, zlist, zlistlen,
                          extent, origin, spacing, 2);
    }

  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    if (zlistidx < zlistlen && idZ >= zlist[zlistidx])
      {
      zstate = -zstate;
      zlistidx++;
      }

    p1[0] = p0[0] = extent[0]*spacing[0] + origin[0];
    p0[1] = extent[2]*spacing[1] + origin[1];
    p1[1] = extent[3]*spacing[1] + origin[1];
    p1[2] = p0[2] = idZ*spacing[2] + origin[2];

    int ystate = zstate;
    int *ylist = 0;
    int ylistlen = 0;
    int ylistidx = 0;
    if (extent[2] != extent[3])
      {
      tree->IntersectWithLine(p0, p1, points, 0);
      vtkTurnPointsIntoList(points, ylist, ylistlen,
                            extent, origin, spacing, 1);
      }

    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      if (ylistidx < ylistlen && idY >= ylist[ylistidx])
        {
        ystate = -ystate;
        ylistidx++;
        }

      p0[1] = p1[1] = idY*spacing[1] + origin[1];
      p0[2] = p1[2] = idZ*spacing[2] + origin[2];
      p0[0] = extent[0]*spacing[0] + origin[0];
      p1[0] = extent[1]*spacing[0] + origin[0];

      int xstate = ystate;
      int *xlist = 0;
      int xlistlen = 0;
      int xlistidx = 0;
      tree->IntersectWithLine(p0, p1, points, 0);
      vtkTurnPointsIntoList(points, xlist, xlistlen,
                            extent, origin, spacing, 0);

      // now turn 'xlist' into sub-extents:
      int r1 = extent[0];
      int r2 = extent[1];
      for (xlistidx = 0; xlistidx < xlistlen; xlistidx++)
        {
        xstate = -xstate;
        if (xstate < 0)
          { // sub extent starts
          r1 = xlist[xlistidx];
          }
        else
          { // sub extent ends
          r2 = xlist[xlistidx] - 1;
          data->InsertNextExtent(r1, r2, idY, idZ);
          }
        }
      if (xstate < 0)
        { // if inside at end, cap off the sub extent
        data->InsertNextExtent(r1, extent[1], idY, idZ);
        }      

      if (xlist)
        {
        delete [] xlist;
        }

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
}
