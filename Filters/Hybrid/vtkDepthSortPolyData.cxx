/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthSortPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDepthSortPolyData.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProp3D.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDataArray.h"

#include <algorithm>
#include <limits>
#include <cstdlib>

namespace {

template <typename T>
struct greaterf
{
  greaterf(const T *az): z(az) {}
  bool operator()(vtkIdType l, vtkIdType r) const
  { return z[l] > z[r]; }
  const T *z;
};

template <typename T>
struct lessf
{
  lessf(const T *az): z(az) {}
  bool operator()(vtkIdType l, vtkIdType r) const
  { return z[l] < z[r]; }
  const T *z;
};

template <typename T>
T getCellBoundsCenter(vtkIdType *pids, vtkIdType nPids, const T *px)
{
  T mn = nPids ? px[3*pids[0]] : T();
  T mx = mn;
  for (vtkIdType i = 1; i < nPids; ++i)
  {
    T v = px[3*pids[i]];
    mn = v < mn ? v : mn;
    mx = v > mx ? v : mx;
  }
  return (mn + mx)/T(2);
}

template <typename T>
void getCellCenterDepth(vtkPolyData *pds, vtkDataArray *gpts,
    vtkIdType nCells, double *origin, double *direction, T *&depth)
{
  if (nCells < 1)
  {
    return;
  }

  T *ppts = static_cast<T*>(gpts->GetVoidPointer(0));
  T *px = ppts;
  T *py = ppts + 1;
  T *pz = ppts + 2;

  // this call insures that BuildCells gets done if it's
  // needed and we can use the faster GetCellPoints api
  // that doesn't check
  if (pds->NeedToBuildCells())
  {
    pds->BuildCells();
  }

  // compute cell centers
  T *cx = new T[nCells];
  T *cy = new T[nCells];
  T *cz = new T[nCells];
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    // get the cell point ids using the fast api
    vtkIdType *pids = NULL;
    vtkIdType nPids = 0;
    pds->GetCellPoints(cid, nPids, pids);

    // compute the center of the cell bounds
    cx[cid] = getCellBoundsCenter(pids, nPids, px);
    cy[cid] = getCellBoundsCenter(pids, nPids, py);
    cz[cid] = getCellBoundsCenter(pids, nPids, pz);
  }

  // compute the distance to the cell center
  T x0 = static_cast<T>(origin[0]);
  T y0 = static_cast<T>(origin[1]);
  T z0 = static_cast<T>(origin[2]);
  T vx = static_cast<T>(direction[0]);
  T vy = static_cast<T>(direction[1]);
  T vz = static_cast<T>(direction[2]);
  depth = new T[nCells];
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    depth[cid] = (cx[cid] - x0)*vx + (cy[cid] - y0)*vy + (cz[cid]- z0)*vz;
  }

  delete [] cx;
  delete [] cy;
  delete [] cz;
}

template <typename T>
void getCellPoint0Depth(vtkPolyData *pds, vtkDataArray *gpts,
    vtkIdType nCells, double *origin, double *direction, T *&depth)
{
  if (nCells < 1)
  {
    return;
  }

  T *ppts = static_cast<T*>(gpts->GetVoidPointer(0));
  T *px = ppts;
  T *py = ppts + 1;
  T *pz = ppts + 2;

  // this call insures that BuildCells gets done if it's
  // needed and we can use the faster GetCellPoints api
  if (pds->NeedToBuildCells())
  {
    pds->BuildCells();
  }

  T *cx = new T[nCells];
  T *cy = new T[nCells];
  T *cz = new T[nCells];
  vtkIdType *pids = NULL;
  vtkIdType nPids = 0;
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    // get the cell point ids using the fast api
    pds->GetCellPoints(cid, nPids, pids);
    vtkIdType ii = pids[0];
    cx[cid] = px[3*ii];
    cy[cid] = py[3*ii];
    cz[cid] = pz[3*ii];
  }

  // compute the distance to the cell's first point
  T x0 = static_cast<T>(origin[0]);
  T y0 = static_cast<T>(origin[1]);
  T z0 = static_cast<T>(origin[2]);
  T vx = static_cast<T>(direction[0]);
  T vy = static_cast<T>(direction[1]);
  T vz = static_cast<T>(direction[2]);
  depth = new T[nCells];
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    depth[cid] = (cx[cid] - x0)*vx + (cy[cid] - y0)*vy + (cz[cid] - z0)*vz;
  }

  delete [] cx;
  delete [] cy;
  delete [] cz;
}
};

vtkStandardNewMacro(vtkDepthSortPolyData);

vtkCxxSetObjectMacro(vtkDepthSortPolyData,Camera,vtkCamera);

vtkDepthSortPolyData::vtkDepthSortPolyData() :
  Direction(VTK_DIRECTION_BACK_TO_FRONT),
  DepthSortMode(VTK_SORT_FIRST_POINT),
  Camera(NULL), Prop3D(NULL),
  Transform(vtkTransform::New()),
  SortScalars(0)
{
  std::fill_n(this->Vector, 3, 0.0);
  std::fill_n(this->Origin, 3, 0.0);
}

vtkDepthSortPolyData::~vtkDepthSortPolyData()
{
  this->Transform->Delete();

  if ( this->Camera )
  {
    this->Camera->Delete();
  }
  //Note: vtkProp3D is not deleted to avoid reference count cycle
}

void vtkDepthSortPolyData::SetProp3D(vtkProp3D *prop3d)
{
  if ( this->Prop3D != prop3d )
  {
    // Don't reference count to avoid nasty cycle
    this->Prop3D = prop3d;
    this->Modified();
  }
}

int vtkDepthSortPolyData::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Compute the sort direction
  double direction[3] = {0.0};
  double origin[3] = {0.0};
  if (this->Direction == VTK_DIRECTION_SPECIFIED_VECTOR)
  {
    memcpy(direction, this->Vector, 3*sizeof(double));
    memcpy(origin, this->Origin, 3*sizeof(double));
  }
  else //compute view direction
  {
    if (!this->Camera)
    {
      vtkErrorMacro("Need a camera to sort");
      return 0;
    }
    this->ComputeProjectionVector(direction, origin);
  }

  // create temporary input
  vtkPolyData *tmpInput = vtkPolyData::New();
  tmpInput->CopyStructure(input);

  // here are the number of cells we have to process
  vtkIdType nVerts = input->GetVerts()->GetNumberOfCells();
  vtkIdType nLines = input->GetLines()->GetNumberOfCells();
  vtkIdType nPolys = input->GetPolys()->GetNumberOfCells();
  vtkIdType nStrips = input->GetStrips()->GetNumberOfCells();
  vtkIdType nCells = nVerts + nLines + nPolys + nStrips;

  vtkIdType *order = new vtkIdType [nCells];
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    order[cid] = cid;
  }

  vtkIdTypeArray *newCellIds = NULL;
  if (this->SortScalars)
  {
    newCellIds = vtkIdTypeArray::New();
    newCellIds->SetName("sortedCellIds");
    newCellIds->SetNumberOfTuples(nCells);
    memcpy(newCellIds->GetPointer(0), order, nCells*sizeof(vtkIdType));
  }

  if (nCells)
  {
    if ((this->DepthSortMode == VTK_SORT_FIRST_POINT)
      || (this->DepthSortMode == VTK_SORT_BOUNDS_CENTER))
    {
      vtkDataArray *pts = tmpInput->GetPoints()->GetData();
      switch (pts->GetDataType())
      {
        vtkTemplateMacro(

          // compute the cell's depth
          VTK_TT *depth = NULL;
          if (this->DepthSortMode == VTK_SORT_FIRST_POINT)
          {
            ::getCellPoint0Depth(
              tmpInput, pts, nCells, origin, direction, depth);
          }
          else
          {
            ::getCellCenterDepth(
              tmpInput, pts, nCells, origin, direction, depth);
          }

          // sort cell ids by depth
          if (this->Direction == VTK_DIRECTION_FRONT_TO_BACK)
          {
            ::lessf<VTK_TT> comp(depth);
            std::sort(order, order + nCells, comp);
          }
          else
          {
            ::greaterf<VTK_TT> comp(depth);
            std::sort(order, order + nCells, comp);
          }

          delete [] depth;
          );
      }
    }
    else // VTK_SORT_PARAMETRIC_CENTER
    {
      vtkGenericCell *cell = vtkGenericCell::New();

      double x[3] = {0.0};
      double p[3] = {0.0};

      size_t maxCellSize = input->GetMaxCellSize();
      double *weight = new double [maxCellSize];
      double *depth = new double [nCells];

      for (vtkIdType cid = 0; cid < nCells; ++cid)
      {
        tmpInput->GetCell(cid, cell);
        int subId = cell->GetParametricCenter(p);
        cell->EvaluateLocation(subId, p, x, weight);

        // compute the distance
        depth[cid] = (x[0] - origin[0])*direction[0]
            + (x[1] - origin[1])*direction[1] + (x[2] - origin[2])*direction[2];
      }

      // sort
      if (this->Direction == VTK_DIRECTION_FRONT_TO_BACK)
      {
        ::lessf<double> comp(depth);
        std::sort(order, order + nCells, comp);
      }
      else
      {
        ::greaterf<double> comp(depth);
        std::sort(order, order + nCells, comp);
      }

      delete [] weight;
      delete [] depth;
      cell->Delete();
    }
  }

  // construct the output
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);

  // pass point through
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  // allocate the cells for the output
  vtkIdType *pOutputVerts = NULL;
  if (nVerts)
  {
    vtkCellArray *outputVertCells = vtkCellArray::New();
    outputVertCells->SetNumberOfCells(nVerts);
    output->SetVerts(outputVertCells);
    outputVertCells->Delete();
    vtkIdTypeArray *outputVerts = outputVertCells->GetData();
    outputVerts->SetNumberOfTuples(
          input->GetVerts()->GetNumberOfConnectivityEntries());
    pOutputVerts = outputVerts->GetPointer(0);
  }

  vtkIdType *pOutputLines = NULL;
  if (nLines)
  {
    vtkCellArray *outputLineCells = vtkCellArray::New();
    outputLineCells->SetNumberOfCells(nLines);
    output->SetLines(outputLineCells);
    outputLineCells->Delete();
    vtkIdTypeArray *outputLines = outputLineCells->GetData();
    outputLines->SetNumberOfTuples(
          input->GetLines()->GetNumberOfConnectivityEntries());
    pOutputLines = outputLines->GetPointer(0);
  }

  vtkIdType *pOutputPolys = NULL;
  if (nPolys)
  {
    vtkCellArray *outputPolyCells = vtkCellArray::New();
    outputPolyCells->SetNumberOfCells(nPolys);
    output->SetPolys(outputPolyCells);
    outputPolyCells->Delete();
    vtkIdTypeArray *outputPolys = outputPolyCells->GetData();
    outputPolys->SetNumberOfTuples(
          input->GetPolys()->GetNumberOfConnectivityEntries());
    pOutputPolys = outputPolys->GetPointer(0);
  }

  vtkIdType *pOutputStrips = NULL;
  if (nStrips)
  {
    vtkCellArray *outputStripCells = vtkCellArray::New();
    outputStripCells->SetNumberOfCells(nStrips);
    output->SetStrips(outputStripCells);
    outputStripCells->Delete();
    vtkIdTypeArray *outputStrips = outputStripCells->GetData();
    outputStrips->SetNumberOfTuples(
          input->GetStrips()->GetNumberOfConnectivityEntries());
    pOutputStrips = outputStrips->GetPointer(0);
  }

  for (vtkIdType i = 0; i < nCells; ++i)
  {
    // get the cell points using the fast api
    vtkIdType *pids = NULL;
    vtkIdType cid = order[i];
    unsigned char ctype = tmpInput->GetCell(cid, pids);
    vtkIdType nids = pids[0] + 1;

    // build the cell
    switch (ctype)
    {
      case VTK_VERTEX: case VTK_POLY_VERTEX:
        memcpy(pOutputVerts, pids, nids*sizeof(vtkIdType));
        pOutputVerts += nids;
        break;

      case VTK_LINE: case VTK_POLY_LINE:
        memcpy(pOutputLines, pids, nids*sizeof(vtkIdType));
        pOutputLines += nids;
        break;

      case VTK_TRIANGLE: case VTK_QUAD: case VTK_POLYGON:
        memcpy(pOutputPolys, pids, nids*sizeof(vtkIdType));
        pOutputPolys += nids;
        break;

      case VTK_TRIANGLE_STRIP:
        memcpy(pOutputStrips, pids, nids*sizeof(vtkIdType));
        pOutputStrips += nids;
        break;
    }
    // copy over data
    outCD->CopyData(inCD, cid, i);
  }

  if (this->SortScalars)
  {
    // add the sort indices
    output->GetCellData()->AddArray(newCellIds);
    newCellIds->Delete();

    vtkIdTypeArray *oldCellIds = vtkIdTypeArray::New();
    oldCellIds->SetName("originalCellIds");
    oldCellIds->SetArray(order, nCells, 0, 1);
    output->GetCellData()->AddArray(oldCellIds);
    oldCellIds->Delete();
  }
  else
  {
    delete [] order;
  }

  tmpInput->Delete();

  return 1;
}

void vtkDepthSortPolyData::ComputeProjectionVector(double direction[3],
                                                   double origin[3])
{
  double *focalPoint = this->Camera->GetFocalPoint();
  double *position = this->Camera->GetPosition();

  // If a camera is present, use it
  if (!this->Prop3D)
  {
    memcpy(origin, position, 3*sizeof(double));
    for(int i = 0; i < 3; ++i)
    {
      direction[i] = focalPoint[i] - position[i];
    }
  }
  else  //Otherwise, use Prop3D
  {
    this->Transform->SetMatrix(this->Prop3D->GetMatrix());
    this->Transform->Push();
    this->Transform->Inverse();

    double focalPt[4];
    memcpy(focalPt, focalPoint, 3*sizeof(double));
    focalPt[3] = 1.0;
    this->Transform->TransformPoint(focalPt, focalPt);

    double pos[4];
    memcpy(pos, position, 3*sizeof(double));
    pos[3] = 1.0;
    this->Transform->TransformPoint(pos, pos);

    memcpy(origin, pos, 3*sizeof(double));

    for (int i = 0; i < 3; ++i)
    {
      direction[i] = focalPt[i] - pos[i];
    }

    this->Transform->Pop();
  }
}

vtkMTimeType vtkDepthSortPolyData::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();

  if ( this->Direction != VTK_DIRECTION_SPECIFIED_VECTOR )
  {
    vtkMTimeType time;
    if ( this->Camera != NULL )
    {
      time = this->Camera->GetMTime();
      mTime = ( time > mTime ? time : mTime );
    }

    if ( this->Prop3D != NULL )
    {
      time = this->Prop3D->GetMTime();
      mTime = ( time > mTime ? time : mTime );
    }
  }

  return mTime;
}

void vtkDepthSortPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Camera )
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }

  if ( this->Prop3D )
  {
    os << indent << "Prop3D:\n";
    this->Prop3D->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Prop3D: (none)\n";
  }

  os << indent << "Direction: ";
  if ( this->Direction == VTK_DIRECTION_BACK_TO_FRONT )
  {
    os << "Back To Front" << endl;
  }
  else if ( this->Direction == VTK_DIRECTION_FRONT_TO_BACK )
  {
    os << "Front To Back";
  }
  else
  {
    os << "Specified Direction: ";
    os << "(" << this->Vector[0] << ", " << this->Vector[1] << ", "
       << this->Vector[2] << ")\n";
    os << "Specified Origin: ";
    os << "(" << this->Origin[0] << ", " << this->Origin[1] << ", "
       << this->Origin[2] << ")\n";
  }

  os << indent << "Depth Sort Mode: ";
  if ( this->DepthSortMode == VTK_SORT_FIRST_POINT )
  {
    os << "First Point" << endl;
  }
  else if ( this->DepthSortMode == VTK_SORT_BOUNDS_CENTER )
  {
    os << "Bounding Box Center" << endl;
  }
  else
  {
    os << "Parameteric Center" << endl;
  }

  os << indent << "Sort Scalars: " << (this->SortScalars ? "On\n" : "Off\n");
}
