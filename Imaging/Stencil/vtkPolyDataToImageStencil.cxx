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
#include "vtkMergePoints.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>


vtkStandardNewMacro(vtkPolyDataToImageStencil);

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::vtkPolyDataToImageStencil()
{
  this->Tolerance = 1e-3;
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
// Select contours within slice z
void vtkPolyDataToImageStencil::PolyDataSelector(
  vtkPolyData *input, vtkPolyData *output, double z, double thickness,
  vtkMergePoints *locator)
{
  vtkPoints *points = input->GetPoints();
  vtkCellArray *lines = input->GetLines();
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(333);
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(1000);

  double minz = z - 0.5*thickness;
  double maxz = z + 0.5*thickness;

  double bounds[6];
  input->GetBounds(bounds);
  bounds[4] = minz;
  bounds[5] = maxz;
  locator->InitPointInsertion(newPoints, bounds);

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
      vtkIdType ptId;
      double point[3];
      points->GetPoint(ptIds[i], point);
      locator->InsertUniquePoint(point, ptId);
      newLines->InsertCellPoint(ptId);
      }
    }

  output->SetPoints(newPoints);
  output->SetLines(newLines);
  newPoints->Delete();
  newLines->Delete();
}

//----------------------------------------------------------------------------
// This method was taken from vtkCutter and slightly modified
void vtkPolyDataToImageStencil::PolyDataCutter(
  vtkPolyData *input, vtkPolyData *output, double z,
  vtkMergePoints *locator)
{
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  vtkDoubleArray *cellScalars = vtkDoubleArray::New();

  // For the new points and lines
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate(333);
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(1000);

  // No verts or polys are expected
  vtkCellArray *newVerts = vtkCellArray::New();
  vtkCellArray *newPolys = vtkCellArray::New();

  // Allocate space for the cell data
  outCD->CopyAllocate(inCD, 1000);

  // locator used to merge potentially duplicate points
  locator->InitPointInsertion(newPoints, input->GetBounds());

  // Compute some information for progress methods
  vtkGenericCell *cell = vtkGenericCell::New();

  // Loop over all cells; get scalar values for all cell points
  // and process each cell.
  vtkIdType numCells = input->GetNumberOfCells();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId, cell);
    vtkPoints *cellPts = cell->GetPoints();
    vtkIdList *cellIds = cell->GetPointIds();

    if (cell->GetCellDimension() == 2)
      {
      vtkIdType numCellPts = cellPts->GetNumberOfPoints();
      cellScalars->SetNumberOfTuples(numCellPts);
      for (vtkIdType i = 0; i < numCellPts; i++)
        {
        // scalar value is distance from the specified z plane
        cellScalars->SetValue(i, input->GetPoint(cellIds->GetId(i))[2]);
        }

      cell->Contour(z, cellScalars, locator,
                    newVerts, newLines, newPolys, NULL, NULL,
                    inCD, cellId, outCD);
      }
    }

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  cell->Delete();
  cellScalars->Delete();

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();
  newVerts->Delete();
  newPolys->Delete();

  //release any extra memory
  locator->Initialize();
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

  double tolerance = this->Tolerance;

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

  // the locator to use with the data
  vtkMergePoints *locator = vtkMergePoints::New();

  // the output produced by cutting the polydata with the Z plane
  vtkPolyData *slice = vtkPolyData::New();

  // This raster stores all line segments by recording all "x"
  // positions on the surface for each y integer position.
  vtkImageStencilRaster raster(&extent[2]);
  raster.SetTolerance(tolerance);

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
      this->PolyDataCutter(input, slice, z, locator);
      }
    else
      {
      // if no polys, select polylines instead
      this->PolyDataSelector(input, slice, z, spacing[2], locator);
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
    vtkCellArray *lines = slice->GetLines();

    vtkIdList *looseEndIdList = vtkIdList::New();
    vtkIdList *looseEndNeighborList = vtkIdList::New();
    vtkSignedCharArray *inflectionPointList = vtkSignedCharArray::New();

    // find all points with just a single adjacent point,
    // also look for inflection points i.e. where the y direction changes
    for (vtkIdType i = 0; i < numberOfPoints; i++)
      {
      double yval = points->GetPoint(i)[1];
      bool bottomPoint = 1;
      bool topPoint = 1;

      int numberOfNeighbors = 0;
      vtkIdType neighborId = 0;

      lines->InitTraversal();
      vtkIdType npts;
      vtkIdType *pointIds;
      while( lines->GetNextCell(npts, pointIds) )
        {
        int isClosed = 0;
        if (pointIds[0] == pointIds[npts-1])
          {
          isClosed = 1;
          npts -= 1;
          }
        for (vtkIdType j = 0; j < npts; j++)
          {
          if ( pointIds[j] == i )
            {
            if (j > 0 || isClosed)
              {
              numberOfNeighbors++;
              neighborId = pointIds[(j+npts-1)%npts];
              double yneighbor = points->GetPoint(neighborId)[1];
              bottomPoint &= (yneighbor >= yval);
              topPoint &= (yneighbor <= yval);
              }
            if (j < npts-1 || isClosed)
              {
              numberOfNeighbors++;
              neighborId = pointIds[(j+1)%npts];
              double yneighbor = points->GetPoint(neighborId)[1];
              bottomPoint &= (yneighbor >= yval);
              topPoint &= (yneighbor <= yval);
              }
            break;
            }
          }
        }
      if (numberOfNeighbors == 1)
        {
        // store the loose end
        looseEndIdList->InsertNextId( i );
        looseEndNeighborList->InsertNextId( neighborId );
        }
      // mark inflection points
      inflectionPointList->InsertNextValue( bottomPoint | topPoint );
      }

    while (looseEndIdList->GetNumberOfIds() >= 2)
      {
      // first loose end point in the list
      vtkIdType firstLooseEndId = looseEndIdList->GetId(0);
      vtkIdType neighborId = looseEndNeighborList->GetId(0);
      double firstLooseEnd[3];
      slice->GetPoint( firstLooseEndId, firstLooseEnd );
      double neighbor[3];
      slice->GetPoint( neighborId, neighbor);

      // second loose end in the list
      vtkIdType secondLooseEndId = looseEndIdList->GetId(1);
      double secondLooseEnd[3];
      slice->GetPoint( secondLooseEndId, secondLooseEnd );

      // search for the loose end closest to the first one
      double maxval = -VTK_LARGE_FLOAT;

      for(vtkIdType j = 1; j < looseEndIdList->GetNumberOfIds(); j++)
        {
        vtkIdType currentLooseEndId = looseEndIdList->GetId( j );
        if (currentLooseEndId != neighborId)
          {
          double currentLooseEnd[3];
          slice->GetPoint( currentLooseEndId, currentLooseEnd );

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

          if (dotprod > maxval*distance2 && distance2 > 0.0)
            {
            maxval = dotprod/distance2;
            secondLooseEndId = currentLooseEndId;
            }
          }
        }

      // create a new line segment by connecting these two points
      looseEndIdList->DeleteId( firstLooseEndId );
      looseEndIdList->DeleteId( secondLooseEndId );
      looseEndNeighborList->DeleteId( firstLooseEndId );
      looseEndNeighborList->DeleteId( secondLooseEndId );

      lines->InsertNextCell( 2 );
      lines->InsertCellPoint( firstLooseEndId );
      lines->InsertCellPoint( secondLooseEndId );

      // check if the new vertices are vertical inflection points
      slice->GetPoint( secondLooseEndId, secondLooseEnd );
      vtkIdType secondNeighborId = looseEndNeighborList->GetId(0);
      double secondNeighbor[3];
      slice->GetPoint( secondNeighborId, secondNeighbor);

      inflectionPointList->SetValue(firstLooseEndId,
        ((firstLooseEnd[1] - neighbor[1])*
        (secondLooseEnd[1] - firstLooseEnd[1]) <= 0));

      inflectionPointList->SetValue(secondLooseEndId,
        ((secondLooseEnd[1] - firstLooseEnd[1])*
         (secondNeighbor[1] - secondLooseEnd[1]) <= 0));
      }

    // Step 3: Go through all the line segments for this slice,
    // and for each integer y position on the line segment,
    // drop the corresponding x position into the y raster line.
    lines->InitTraversal();
    vtkIdType *pts = 0;
    vtkIdType npts = 0;

    while ( lines->GetNextCell(npts, pts) )
      {
      for (vtkIdType j = 1; j < npts; j++)
        {
        double point1[3], point2[3];
        points->GetPoint(pts[j-1], point1);
        points->GetPoint(pts[j], point2);
        // check to see if line contains a lower inflection point
        bool inflection1 = (inflectionPointList->GetValue(pts[j-1]) != 0);
        bool inflection2 = (inflectionPointList->GetValue(pts[j]) != 0);

        raster.InsertLine(point1, point2, inflection1, inflection2);
        }
      }

    looseEndIdList->Delete();
    looseEndNeighborList->Delete();
    inflectionPointList->Delete();

    // Step 4: Use the x values stored in the xy raster to create
    // one z slice of the vtkStencilData
    sliceExtent[4] = idxZ;
    sliceExtent[5] = idxZ;
    raster.FillStencilData(data, sliceExtent);
    }

  slice->Delete();
  locator->Delete();
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
