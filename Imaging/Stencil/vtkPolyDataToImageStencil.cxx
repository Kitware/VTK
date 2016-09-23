/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2008 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
#include "vtkPolyDataToImageStencil.h"
#include "vtkImageStencilData.h"
#include "vtkObjectFactory.h"

#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkSignedCharArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <map>
#include <vector>
#include <utility>
#include <algorithm>

#include <cmath>


vtkStandardNewMacro(vtkPolyDataToImageStencil);

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::vtkPolyDataToImageStencil()
{
  // The default tolerance is 0.5*2^(-16)
  this->Tolerance = 7.62939453125e-06;
}

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::~vtkPolyDataToImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::SetInputData(vtkPolyData *input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataToImageStencil::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return NULL;
  }

  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::PrintSelf(ostream& os,
                                          vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

//----------------------------------------------------------------------------
// A helper class to quickly locate an edge, given the endpoint ids.
// It uses an stl map rather than a table partitioning scheme, since
// we have no idea how many entries there will be when we start.  So
// the performance is approximately log(n).
namespace {

// A Node in a linked list that contains information about one edge
class EdgeLocatorNode
{
public:
  EdgeLocatorNode() :
    ptId(-1), edgeId(-1), next(0) {}

  // Free the list that this node is the head of
  void FreeList() {
    EdgeLocatorNode *ptr = this->next;
    while (ptr)
    {
      EdgeLocatorNode *tmp = ptr;
      ptr = ptr->next;
      tmp->next = 0;
      delete tmp;
    }
  }

  vtkIdType ptId;
  vtkIdType edgeId;
  EdgeLocatorNode *next;
};

// The EdgeLocator class itself, for keeping track of edges
class EdgeLocator
{
private:
  typedef std::map<vtkIdType, EdgeLocatorNode> MapType;
  MapType EdgeMap;

public:
  EdgeLocator() : EdgeMap() {}
  ~EdgeLocator() { this->Initialize(); }

  // Description:
  // Initialize the locator.
  void Initialize();

  // Description:
  // If the edge (i0, i1) is not in the list, then it will be added and
  // given the supplied edgeId, and the return value will be false.  If
  // the edge (i0, i1) is in the list, then edgeId will be set to the
  // stored value and the return value will be true.
  bool InsertUniqueEdge(vtkIdType i0, vtkIdType i1, vtkIdType &edgeId);

  // Description:
  // A helper function for interpolating a new point along an edge.  It
  // stores the index of the interpolated point in "i", and returns true
  // if a new point was added to the locator.  The values i0, i1, v0, v1
  // are the edge endpoints and scalar values, respectively.
  bool InterpolateEdge(
    vtkPoints *inPoints, vtkPoints *outPoints,
    vtkIdType i0, vtkIdType i1, double v0, double v1,
    vtkIdType &i);
};

void EdgeLocator::Initialize()
{
  for (MapType::iterator i = this->EdgeMap.begin();
       i != this->EdgeMap.end();
       ++i)
  {
    i->second.FreeList();
  }
  this->EdgeMap.clear();
}

bool EdgeLocator::InsertUniqueEdge(
  vtkIdType i0, vtkIdType i1, vtkIdType &edgeId)
{
  // Ensure consistent ordering of edge
  if (i1 < i0)
  {
    vtkIdType tmp = i0;
    i0 = i1;
    i1 = tmp;
  }

  EdgeLocatorNode *node = &this->EdgeMap[i0];

  if (node->ptId < 0)
  {
    // Didn't find key, so add a new edge entry
    node->ptId = i1;
    node->edgeId = edgeId;
    return true;
  }

  // Search through the list for i1
  if (node->ptId == i1)
  {
    edgeId = node->edgeId;
    return false;
  }

  int i = 1;
  while (node->next != 0)
  {
    i++;
    node = node->next;

    if (node->ptId == i1)
    {
      edgeId = node->edgeId;
      return false;
    }
  }

  // No entry for i1, so make one and return
  node->next = new EdgeLocatorNode;
  node = node->next;
  node->ptId = i1;
  node->edgeId = edgeId;
  return true;
}

bool EdgeLocator::InterpolateEdge(
  vtkPoints *points, vtkPoints *outPoints,
  vtkIdType i0, vtkIdType i1, double v0, double v1,
  vtkIdType &i)
{
  // This swap guarantees that exactly the same point is computed
  // for both line directions, as long as the endpoints are the same.
  if (v1 > 0)
  {
    vtkIdType tmpi = i0;
    i0 = i1;
    i1 = tmpi;

    double tmp = v0;
    v0 = v1;
    v1 = tmp;
  }

  // Check to see if this point has already been computed
  i = outPoints->GetNumberOfPoints();
  if (!this->InsertUniqueEdge(i0, i1, i))
  {
    return false;
  }

  // Get the edge and interpolate the new point
  double p0[3], p1[3], p[3];
  points->GetPoint(i0, p0);
  points->GetPoint(i1, p1);

  double f = v0/(v0 - v1);
  double s = 1.0 - f;
  double t = 1.0 - s;

  p[0] = s*p0[0] + t*p1[0];
  p[1] = s*p0[1] + t*p1[1];
  p[2] = s*p0[2] + t*p1[2];

  // Add the point, store the new index in the locator
  outPoints->InsertNextPoint(p);

  return true;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// Select contours within slice z
void vtkPolyDataToImageStencil::PolyDataSelector(
  vtkPolyData *input, vtkPolyData *output, double z, double thickness)
{
  vtkPoints *points = input->GetPoints();
  vtkCellArray *lines = input->GetLines();
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetDataType(points->GetDataType());
  newPoints->Allocate(333);
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(1000);

  double minz = z - 0.5*thickness;
  double maxz = z + 0.5*thickness;

  // use a map to avoid adding duplicate points
  std::map<vtkIdType, vtkIdType> pointLocator;

  vtkIdType loc = 0;
  vtkIdType numCells = lines->GetNumberOfCells();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    // check if all points in cell are within the slice
    vtkIdType npts, *ptIds;
    lines->GetCell(loc, npts, ptIds);
    loc += npts + 1;
    vtkIdType i;
    for (i = 0; i < npts; i++)
    {
      double point[3];
      points->GetPoint(ptIds[i], point);
      if (point[2] < minz || point[2] >= maxz)
      {
        break;
      }
    }
    if (i < npts)
    {
      continue;
    }
    newLines->InsertNextCell(npts);
    for (i = 0; i < npts; i++)
    {
      vtkIdType oldId = ptIds[i];
      std::map<vtkIdType, vtkIdType>::iterator iter =
        pointLocator.lower_bound(oldId);
      vtkIdType ptId = 0;
      if (iter == pointLocator.end() || iter->first != oldId)
      {
        double point[3];
        points->GetPoint(oldId, point);
        ptId = newPoints->InsertNextPoint(point);
        pointLocator.insert(iter, std::make_pair(oldId, ptId));
      }
      else
      {
        ptId = iter->second;
      }
      newLines->InsertCellPoint(ptId);
    }
  }

  output->SetPoints(newPoints);
  output->SetLines(newLines);
  newPoints->Delete();
  newLines->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::PolyDataCutter(
  vtkPolyData *input, vtkPolyData *output, double z)
{
  vtkPoints *points = input->GetPoints();
  vtkCellArray *inputPolys = input->GetPolys();
  vtkCellArray *inputStrips = input->GetStrips();
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetDataType(points->GetDataType());
  newPoints->Allocate(333);
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(1000);

  // An edge locator to avoid point duplication while clipping
  EdgeLocator edgeLocator;

  // Go through all cells and clip them.
  vtkIdType numPolys = input->GetNumberOfPolys();
  vtkIdType numStrips = input->GetNumberOfStrips();
  vtkIdType numCells = numPolys + numStrips;

  vtkIdType loc = 0;
  vtkCellArray *cellArray = inputPolys;
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    // switch to strips when polys are done
    if (cellId == numPolys)
    {
      loc = 0;
      cellArray = inputStrips;
    }

    vtkIdType npts, *ptIds;
    cellArray->GetCell(loc, npts, ptIds);
    loc += npts + 1;

    vtkIdType numSubCells = 1;
    if (cellArray == inputStrips)
    {
      numSubCells = npts - 2;
      npts = 3;
    }

    for (vtkIdType subId = 0; subId < numSubCells; subId++)
    {
      vtkIdType i1 = ptIds[npts-1];
      double point[3];
      points->GetPoint(i1, point);
      double v1 = point[2] - z;
      bool c1 = (v1 > 0);
      bool odd = ((subId & 1) != 0);

      // To store the ids of the contour line
      vtkIdType linePts[2];
      linePts[0] = 0;
      linePts[1] = 0;

      for (vtkIdType i = 0; i < npts; i++)
      {
        // Save previous point info
        vtkIdType i0 = i1;
        double v0 = v1;
        bool c0 = c1;

        // Generate new point info
        i1 = ptIds[i];
        points->GetPoint(i1, point);
        v1 = point[2] - z;
        c1 = (v1 > 0);

        // If at least one edge end point wasn't clipped
        if ( (c0 | c1) )
        {
          // If only one end was clipped, interpolate new point
          if ( (c0 ^ c1) )
          {
            edgeLocator.InterpolateEdge(
              points, newPoints, i0, i1, v0, v1, linePts[c0 ^ odd]);
          }
        }
      }

      // Insert the contour line if one was created
      if (linePts[0] != linePts[1])
      {
        newLines->InsertNextCell(2, linePts);
      }

      // Increment to get to the next triangle, if cell is a strip
      ptIds++;
    }
  }

  output->SetPoints(newPoints);
  output->SetLines(newLines);
  newPoints->Delete();
  newLines->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::ThreadedExecute(
  vtkImageStencilData *data,
  int extent[6],
  int threadId)
{
  // Description of algorithm:
  // 1) cut the polydata at each z slice to create polylines
  // 2) find all "loose ends" and connect them to make polygons
  //    (if the input polydata is closed, there will be no loose ends)
  // 3) go through all line segments, and for each integer y value on
  //    a line segment, store the x value at that point in a bucket
  // 4) for each z integer index, find all the stored x values
  //    and use them to create one z slice of the vtkStencilData

  // the spacing and origin of the generated stencil
  double *spacing = data->GetSpacing();
  double *origin = data->GetOrigin();

  // if we have no data then return
  if (!this->GetInput()->GetNumberOfPoints())
  {
    return;
  }

  // Only divide once
  double invspacing[3];
  invspacing[0] = 1.0/spacing[0];
  invspacing[1] = 1.0/spacing[1];
  invspacing[2] = 1.0/spacing[2];

  // get the input data
  vtkPolyData *input = this->GetInput();

  // the output produced by cutting the polydata with the Z plane
  vtkPolyData *slice = vtkPolyData::New();

  // This raster stores all line segments by recording all "x"
  // positions on the surface for each y integer position.
  vtkImageStencilRaster raster(&extent[2]);
  raster.SetTolerance(this->Tolerance);

  // The extent for one slice of the image
  int sliceExtent[6];
  sliceExtent[0] = extent[0]; sliceExtent[1] = extent[1];
  sliceExtent[2] = extent[2]; sliceExtent[3] = extent[3];
  sliceExtent[4] = extent[4]; sliceExtent[5] = extent[4];

  // Loop through the slices
  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
  {
    if (threadId == 0)
    {
      this->UpdateProgress((idxZ - extent[4])*1.0/(extent[5] - extent[4] + 1));
    }

    double z = idxZ*spacing[2] + origin[2];

    slice->PrepareForNewData();
    raster.PrepareForNewData();

    // Step 1: Cut the data into slices
    if (input->GetNumberOfPolys() > 0 || input->GetNumberOfStrips() > 0)
    {
      this->PolyDataCutter(input, slice, z);
    }
    else
    {
      // if no polys, select polylines instead
      this->PolyDataSelector(input, slice, z, spacing[2]);
    }

    if (!slice->GetNumberOfLines())
    {
      continue;
    }

    // convert to structured coords via origin and spacing
    vtkPoints *points = slice->GetPoints();
    vtkIdType numberOfPoints = points->GetNumberOfPoints();

    for (vtkIdType j = 0; j < numberOfPoints; j++)
    {
      double tempPoint[3];
      points->GetPoint(j, tempPoint);
      tempPoint[0] = (tempPoint[0] - origin[0])*invspacing[0];
      tempPoint[1] = (tempPoint[1] - origin[1])*invspacing[1];
      tempPoint[2] = (tempPoint[2] - origin[2])*invspacing[2];
      points->SetPoint(j, tempPoint);
    }

    // Step 2: Find and connect all the loose ends
    std::vector<vtkIdType> pointNeighbors(numberOfPoints);
    std::vector<vtkIdType> pointNeighborCounts(numberOfPoints);
    std::fill(pointNeighborCounts.begin(), pointNeighborCounts.end(), 0);

    // get the connectivity count for each point
    vtkCellArray *lines = slice->GetLines();
    vtkIdType npts = 0;
    vtkIdType *pointIds = 0;
    vtkIdType count = lines->GetNumberOfConnectivityEntries();
    for (vtkIdType loc = 0; loc < count; loc += npts + 1)
    {
      lines->GetCell(loc, npts, pointIds);
      if (npts > 0)
      {
        pointNeighborCounts[pointIds[0]] += 1;
        for (vtkIdType j = 1; j < npts-1; j++)
        {
          pointNeighborCounts[pointIds[j]] += 2;
        }
        pointNeighborCounts[pointIds[npts-1]] += 1;
        if (pointIds[0] != pointIds[npts-1])
        {
          // store the neighbors for end points, because these are
          // potentially loose ends that will have to be dealt with later
          pointNeighbors[pointIds[0]] = pointIds[1];
          pointNeighbors[pointIds[npts-1]] = pointIds[npts-2];
        }
      }
    }

    // use connectivity count to identify loose ends and branch points
    std::vector<vtkIdType> looseEndIds;
    std::vector<vtkIdType> branchIds;

    for (vtkIdType j = 0; j < numberOfPoints; j++)
    {
      if (pointNeighborCounts[j] == 1)
      {
        looseEndIds.push_back(j);
      }
      else if (pointNeighborCounts[j] > 2)
      {
        branchIds.push_back(j);
      }
    }

    // remove any spurs
    for (size_t b = 0; b < branchIds.size(); b++)
    {
      for (size_t i = 0; i < looseEndIds.size(); i++)
      {
        if (pointNeighbors[looseEndIds[i]] == branchIds[b])
        {
          // mark this pointId as removed
          pointNeighborCounts[looseEndIds[i]] = 0;
          looseEndIds.erase(looseEndIds.begin() + i);
          i--;
          if (--pointNeighborCounts[branchIds[b]] <= 2)
          {
            break;
          }
        }
      }
    }

    // join any loose ends
    while (looseEndIds.size() >= 2)
    {
      size_t n = looseEndIds.size();

      // search for the two closest loose ends
      double maxval = -VTK_FLOAT_MAX;
      vtkIdType firstIndex = 0;
      vtkIdType secondIndex = 1;
      bool isCoincident = false;
      bool isOnHull = false;

      for (size_t i = 0; i < n && !isCoincident; i++)
      {
        // first loose end
        vtkIdType firstLooseEndId = looseEndIds[i];
        vtkIdType neighborId = pointNeighbors[firstLooseEndId];

        double firstLooseEnd[3];
        slice->GetPoint(firstLooseEndId, firstLooseEnd);
        double neighbor[3];
        slice->GetPoint(neighborId, neighbor);

        for (size_t j = i+1; j < n; j++)
        {
          vtkIdType secondLooseEndId = looseEndIds[j];
          if (secondLooseEndId != neighborId)
          {
            double currentLooseEnd[3];
            slice->GetPoint(secondLooseEndId, currentLooseEnd);

            // When connecting loose ends, use dot product to favor
            // continuing in same direction as the line already
            // connected to the loose end, but also favour short
            // distances by dividing dotprod by square of distance.
            double v1[2], v2[2];
            v1[0] = firstLooseEnd[0] - neighbor[0];
            v1[1] = firstLooseEnd[1] - neighbor[1];
            v2[0] = currentLooseEnd[0] - firstLooseEnd[0];
            v2[1] = currentLooseEnd[1] - firstLooseEnd[1];
            double dotprod = v1[0]*v2[0] + v1[1]*v2[1];
            double distance2 = v2[0]*v2[0] + v2[1]*v2[1];

            // check if points are coincident
            if (distance2 == 0)
            {
              firstIndex = i;
              secondIndex = j;
              isCoincident = true;
              break;
            }

            // prefer adding segments that lie on hull
            double midpoint[2], normal[2];
            midpoint[0] = 0.5*(currentLooseEnd[0] + firstLooseEnd[0]);
            midpoint[1] = 0.5*(currentLooseEnd[1] + firstLooseEnd[1]);
            normal[0] = currentLooseEnd[1] - firstLooseEnd[1];
            normal[1] = -(currentLooseEnd[0] - firstLooseEnd[0]);
            double sidecheck = 0.0;
            bool checkOnHull = true;
            for (size_t k = 0; k < n; k++)
            {
              if (k != i && k != j)
              {
                double checkEnd[3];
                slice->GetPoint(looseEndIds[k], checkEnd);
                double dotprod2 = ((checkEnd[0] - midpoint[0])*normal[0] +
                                   (checkEnd[1] - midpoint[1])*normal[1]);
                if (dotprod2*sidecheck < 0)
                {
                  checkOnHull = false;
                }
                sidecheck = dotprod2;
              }
            }

            // check if new candidate is better than previous one
            if ((checkOnHull && !isOnHull) ||
                (checkOnHull == isOnHull && dotprod > maxval*distance2))
            {
              firstIndex = i;
              secondIndex = j;
              isOnHull |= checkOnHull;
              maxval = dotprod/distance2;
            }
          }
        }
      }

      // get info about the two loose ends and their neighbors
      vtkIdType firstLooseEndId = looseEndIds[firstIndex];
      vtkIdType neighborId = pointNeighbors[firstLooseEndId];
      double firstLooseEnd[3];
      slice->GetPoint(firstLooseEndId, firstLooseEnd);
      double neighbor[3];
      slice->GetPoint(neighborId, neighbor);

      vtkIdType secondLooseEndId = looseEndIds[secondIndex];
      vtkIdType secondNeighborId = pointNeighbors[secondLooseEndId];
      double secondLooseEnd[3];
      slice->GetPoint(secondLooseEndId, secondLooseEnd);
      double secondNeighbor[3];
      slice->GetPoint(secondNeighborId, secondNeighbor);

      // remove these loose ends from the list
      looseEndIds.erase(looseEndIds.begin() + secondIndex);
      looseEndIds.erase(looseEndIds.begin() + firstIndex);

      if (!isCoincident)
      {
        // create a new line segment by connecting these two points
        lines->InsertNextCell(2);
        lines->InsertCellPoint(firstLooseEndId);
        lines->InsertCellPoint(secondLooseEndId);
      }
    }

    // Step 3: Go through all the line segments for this slice,
    // and for each integer y position on the line segment,
    // drop the corresponding x position into the y raster line.
    count = lines->GetNumberOfConnectivityEntries();
    for (vtkIdType loc = 0; loc < count; loc += npts + 1)
    {
      lines->GetCell(loc, npts, pointIds);
      if (npts > 0)
      {
        vtkIdType pointId0 = pointIds[0];
        double point0[3];
        points->GetPoint(pointId0, point0);
        for (vtkIdType j = 1; j < npts; j++)
        {
          vtkIdType pointId1 = pointIds[j];
          double point1[3];
          points->GetPoint(pointId1, point1);

          // make sure points aren't flagged for removal
          if (pointNeighborCounts[pointId0] > 0 &&
              pointNeighborCounts[pointId1] > 0)
          {
            raster.InsertLine(point0, point1);
          }

          pointId0 = pointId1;
          point0[0] = point1[0];
          point0[1] = point1[1];
          point0[2] = point1[2];
        }
      }
    }

    // Step 4: Use the x values stored in the xy raster to create
    // one z slice of the vtkStencilData
    sliceExtent[4] = idxZ;
    sliceExtent[5] = idxZ;
    raster.FillStencilData(data, sliceExtent);
  }

  slice->Delete();
}

//----------------------------------------------------------------------------
int vtkPolyDataToImageStencil::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int extent[6];
  data->GetExtent(extent);
  // ThreadedExecute is only called from a single thread for
  // now, but it could as easily be called from ThreadedRequestData
  this->ThreadedExecute(data, extent, 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataToImageStencil::FillInputPortInformation(
  int,
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
