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

// don't need all of these
#include "vtkLine.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkSignedCharArray.h"
#include "vtkMergePoints.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkGenericCell.h"
#include "vtkLine.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

#include <vtkstd/vector>
#include <vtkstd/algorithm>


vtkStandardNewMacro(vtkPolyDataToImageStencil);
vtkCxxSetObjectMacro(vtkPolyDataToImageStencil, InformationInput,
                     vtkImageData);

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::vtkPolyDataToImageStencil()
{
  // InformationInput is an input used only for its information.
  // The vtkImageStencilSource produces a structured data
  // set with a specific "Spacing" and "Origin", and the
  // InformationInput is a source of this information.
  this->InformationInput = NULL;

  // If no InformationInput is set, then the Spacing and Origin
  // for the output must be set directly.
  this->OutputOrigin[0] = 0;
  this->OutputOrigin[1] = 0;
  this->OutputOrigin[2] = 0;
  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  // The default output extent is essentially infinite, which allows
  // this filter to produce any requested size.  This would not be a
  // great source to connect to some sort of writer or viewer, it
  // should only be connected to multiple-input filters that take
  // compute their output extent from one of the other inputs.
  this->OutputWholeExtent[0] = 0;
  this->OutputWholeExtent[1] = VTK_LARGE_INTEGER >> 2;
  this->OutputWholeExtent[2] = 0;
  this->OutputWholeExtent[3] = VTK_LARGE_INTEGER >> 2;
  this->OutputWholeExtent[4] = 0;
  this->OutputWholeExtent[5] = VTK_LARGE_INTEGER >> 2;

  this->Tolerance = 1e-3;
}

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::~vtkPolyDataToImageStencil()
{
  this->SetInformationInput(NULL);
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::SetInput(vtkPolyData *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    this->SetInputConnection(0, 0);
    }
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

    os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputWholeExtent: " << this->OutputWholeExtent[0] << " " <<
    this->OutputWholeExtent[1] << " " << this->OutputWholeExtent[2] << " " <<
    this->OutputWholeExtent[3] << " " << this->OutputWholeExtent[4] << " " <<
    this->OutputWholeExtent[5] << "\n";
  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

//----------------------------------------------------------------------------
// This method was taken from vtkCutter and slightly modified
void vtkPolyDataToImageStencil::DataSetCutter(
  vtkDataSet *input, vtkPolyData *output, double z, vtkMergePoints *locator)
{
  vtkIdType cellId, i;
  vtkPoints *cellPts;
  vtkDoubleArray *cellScalars;
  vtkGenericCell *cell;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPoints;
  double s;
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  int numCellPts;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkIdList *cellIds;
  
  cellScalars=vtkDoubleArray::New();

  // Create objects to hold output of contour operation
  estimatedSize = (vtkIdType) pow ((double) numCells, .75);
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);

  // Interpolate data along edge.
  inPD = input->GetPointData();
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);
    
  // locator used to merge potentially duplicate points
  //
  locator->InitPointInsertion (newPoints, input->GetBounds());

  // Compute some information for progress methods
  //
  cell = vtkGenericCell::New();
  
  // Loop over all cells; get scalar values for all cell points
  // and process each cell.
  //
  for (cellId=0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId,cell);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();

    numCellPts = cellPts->GetNumberOfPoints();
    cellScalars->SetNumberOfTuples(numCellPts);
    for (i=0; i < numCellPts; i++)
      {
      // scalar value is distance from the specified z plane
      s = input->GetPoint(cellIds->GetId(i))[2] - z;
      cellScalars->SetTuple(i,&s);
      }

    cell->Contour(0.0, cellScalars, locator, 
                  newVerts, newLines, newPolys, inPD, outPD,
                  inCD, cellId, outCD);
    }

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory. 
  //
  cell->Delete();
  cellScalars->Delete();

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells())
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  locator->Initialize();//release any extra memory
  output->Squeeze();
}

//----------------------------------------------------------------------------
inline int vtkPolyDataToImageStencilFloor(double x)
{
#if defined i386 || defined _M_IX86 || defined _M_X64
  // This code assumes IEEE 754 64-bit double.
  // It uses a denormalizer to round the double at the
  // 2^(-16) position, or around 1e-5, and then extracts
  // the integer portion.  So, essentially, it is a floor()
  // operation that is accurate to within 1e-5.
  // We use it because it is many, many times
  // faster than the floor() function.
  union { vtkTypeFloat64 d; vtkTypeUInt32 i[2]; } dual;
  dual.d = x + 103079215104.0;  // (2**(52-16))*1.5
  return static_cast<int>((dual.i[1]<<16)|(dual.i[0]>>16));
#else
  // This doesn't assume IEEE 754 and is a good, fast
  // floor() approximation on most architectures
  x += 2147483648.0;
  vtkTypeUInt32 i = static_cast<unsigned int>(x);
  return static_cast<int>(i - 2147483648U);
#endif
}

//----------------------------------------------------------------------------
static void vtkFloatingEndPointScanConvertLine2D(
  double pt1[2], double pt2[2], int inflection1, int inflection2,
  double tolerance, int z, int extent[6],
  vtkstd::vector< vtkstd::vector<double> >& zyBucket)
{
  double x1 = pt1[0];
  double x2 = pt2[0];
  double y1 = pt1[1];
  double y2 = pt2[1];

  // swap end points if necessary
  if (y1 > y2)
    {
    x1 = pt2[0];
    x2 = pt1[0];
    y1 = pt2[1];
    y2 = pt1[1];
    }

  // find min and max of x values
  double xmin = x1;
  double xmax = x2;
  if (x1 > x2)
    {
    xmin = x2;
    xmax = x1;
    }

  // check for parallel to the x-axis
  if (y1 == y2)
    {
    return;
    }
  
  // Integer y values for start and end of line
  int Ay, By;

  double ymin = y1;
  double ymax = y2;
  if (inflection1 < 0 || inflection2 < 0)
    {
    // if this is a lower inflection point, include a tolerance
    ymin -= tolerance;
    }
  if (inflection1 > 0 || inflection2 > 0)
    {
    // likewise, if upper inflection, add tolerance at top
    ymax += tolerance;
    }

  // The "+1" is important, it means that if we have two polygons
  // that share a line, and make a stencil from each, the produced
  // regions are guaranteed not to overlap if tolerance=0.
  Ay = vtkPolyDataToImageStencilFloor(ymin) + 1;
  By = vtkPolyDataToImageStencilFloor(ymax);

  // Precompute values for a Bresenham-like line algorithm
  double grad = (x2 - x1)/(y2 - y1);
  double delta = (Ay - y1)*grad;

  // Take z coordinate and extents into account for bucket index
  int idx0 = (z - extent[4])*(extent[3] - extent[2] + 1) - extent[2];

  // Go along y and place each x in the proper (y,z) bucket.
  for( int y = Ay; y <= By; y++ )
    {
    delta += grad;
    double x = x1 + delta;
    // clamp x (because of tolerance, it might not be in range)
    if (x < xmin)
      {
      x = xmin;
      }
    else if (x > xmax)
      {
      x = xmax;
      }
    zyBucket[idx0 + y].push_back( x );
    }
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
  // 4) for each (y,z) integer index, find all the stored x values
  //    and use them to create the vtkStencilData
    
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

  // Determine data dimensions
  int dims[3];
  dims[0] = (extent[1] - extent[0]) + 1;
  dims[1] = (extent[3] - extent[2]) + 1;
  dims[2] = (extent[5] - extent[4]) + 1;

  // This vector stores all line segments by recording all "x"
  // positions on the surface for each (y,z) integer position.
  vtkstd::vector< vtkstd::vector<double> > zyBucket(dims[1]*dims[2]);

  // Loop through the slices
  int idxY, idxZ;
  for (idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    if (threadId == 0)
      {
      this->UpdateProgress((idxZ - extent[4])*1.0/(extent[5] - extent[4] + 1));
      }

    double z = idxZ*spacing[2] + origin[2];

    slice->PrepareForNewData();

    // Step 1: Cut the data into slices
    this->DataSetCutter(input, slice, z, locator);
    
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
    // also look for lower inflection points
    for (vtkIdType i = 0; i < numberOfPoints; i++)
      {
      double yval = points->GetPoint(i)[1];
      int bottomPoint = 1;
      int topPoint = 1;

      int numberOfNeighbors = 0;
      vtkIdType neighborId = 0;
      
      lines->InitTraversal();
      vtkIdType npts;
      vtkIdType *pointIds;
      while( lines->GetNextCell(npts, pointIds) )
        {
        for (vtkIdType j = 0; j < npts; j++)
          {
          if ( pointIds[j] == i )
            {
            if (j > 0)
              {
              numberOfNeighbors++;
              neighborId = pointIds[j-1];
              double yneighbor = points->GetPoint(neighborId)[1];
              if (yneighbor < yval)
                {
                bottomPoint = 0;
                }
              else if (yneighbor > yval)
                {
                topPoint = 0;
                }
              }
            if (j < npts-1)
              {
              numberOfNeighbors++;
              neighborId = pointIds[j+1];
              double yneighbor = points->GetPoint(neighborId)[1];
              if (yneighbor < yval)
                {
                bottomPoint = 0;
                }
              else if (yneighbor > yval)
                {
                topPoint = 0;
                }
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
      // mark lower inflection points
      int inflection = 0;
      if (bottomPoint)
        {
        inflection = -1;
        }
      else if (topPoint)
        {
        inflection = 1;
        }
      inflectionPointList->InsertNextValue( inflection );
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
      }

    // Step 3: Go through all the line segments for this slice,
    // and for each integer y position on the line segment,
    // drop the corresponding x position into the (y,z) bucket.
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
        int inflection1 = inflectionPointList->GetValue(pts[j-1]);
        int inflection2 = inflectionPointList->GetValue(pts[j]);
      
        vtkFloatingEndPointScanConvertLine2D(point1, point2,
                                             inflection1, inflection2,
                                             tolerance, idxZ,
                                             extent, zyBucket);
        }
      }

    looseEndIdList->Delete();
    looseEndNeighborList->Delete();
    inflectionPointList->Delete();
    }

  // Step 4: The final part of the algorithm is to fill in the
  // stencil, i.e. rasterization of the polyhedron.
  int r1, r2, lastr2;
  for (idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (idxY = extent[2]; idxY <= extent[3]; idxY++)
      {
      vtkstd::vector<double>& xList =
        zyBucket[(idxZ - extent[4])*dims[1] + (idxY - extent[2])];

      if (xList.empty())
        {
        continue;
        }
      
      // handle pairs
      lastr2 = extent[0] - 1;
      if (xList.size() % 2 == 0)
        {
        vtkstd::sort(xList.begin(), xList.end());

        vtkstd::vector<double>::iterator xIter = xList.begin();
        while (xIter != xList.end())
          {
          double x1 = *xIter++;
          double x2 = *xIter++;
          // Use floor()+1 instead of ceil() to ensure that, if
          // tolerance is zero and there are neighboring extents,
          // there will be no overlap.
          r1 = vtkPolyDataToImageStencilFloor(x1 - tolerance) + 1;
          // Take floor() of second value in pair
          r2 = vtkPolyDataToImageStencilFloor(x2 + tolerance);

          // extents are not allowed to overlap
          if (r1 <= lastr2)
            {
            r1 = lastr2 + 1;
            // eliminate empty extents
            if (r1 > r2)
              {
              continue;
              }
            }

          data->InsertNextExtent(r1, r2, idxY, idxZ);

          lastr2 = r2;
          }
        }
      }
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
int vtkPolyDataToImageStencil::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  for (int i = 0; i < 3; i++)
    {
    wholeExtent[2*i] = this->OutputWholeExtent[2*i];
    wholeExtent[2*i+1] = this->OutputWholeExtent[2*i+1];
    spacing[i] = this->OutputSpacing[i];
    origin[i] = this->OutputOrigin[i];
    }

  // If InformationInput is set, then get the spacing,
  // origin, and whole extent from it.
  if (this->InformationInput)
    {
    this->InformationInput->UpdateInformation();
    this->InformationInput->GetWholeExtent(wholeExtent);
    this->InformationInput->GetSpacing(spacing);
    this->InformationInput->GetOrigin(origin);
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

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
