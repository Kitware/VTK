/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipClosedSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClipClosedSurface.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkPlaneCollection.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkGenericCell.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkLine.h"
#include "vtkMatrix4x4.h"

#include <vtkstd/vector>
#include <vtkstd/algorithm>

vtkCxxRevisionMacro(vtkClipClosedSurface, "1.8");
vtkStandardNewMacro(vtkClipClosedSurface);

vtkCxxSetObjectMacro(vtkClipClosedSurface,ClippingPlanes,vtkPlaneCollection);

//----------------------------------------------------------------------------
vtkClipClosedSurface::vtkClipClosedSurface()
{
  this->ClippingPlanes = 0;
  this->GenerateColorScalars = 0;
  this->GenerateOutline = 0;
  this->GenerateFaces = 1;
  this->ActivePlaneId = -1;

  this->BaseColor[0] = 1.0;
  this->BaseColor[1] = 0.0;
  this->BaseColor[2] = 0.0;

  this->ClipColor[0] = 1.0;
  this->ClipColor[1] = 0.5;
  this->ClipColor[2] = 0.0;

  this->ActivePlaneColor[0] = 1.0;
  this->ActivePlaneColor[1] = 1.0;
  this->ActivePlaneColor[2] = 0.0;

  // A whole bunch of objects needed during execution
  this->Locator = 0;
  this->CellClipScalars = 0;
  this->IdList = 0;
  this->CellArray = 0;
  this->Polygon = 0;
  this->Cell = 0;
}

//----------------------------------------------------------------------------
vtkClipClosedSurface::~vtkClipClosedSurface()
{
  if (this->ClippingPlanes) { this->ClippingPlanes->Delete(); } 

  if (this->Locator) { this->Locator->Delete(); }
  if (this->CellClipScalars) { this->CellClipScalars->Delete(); }
  if (this->IdList) { this->IdList->Delete(); }
  if (this->CellArray) { this->CellArray->Delete(); }
  if (this->Polygon) { this->Polygon->Delete(); }
  if (this->Cell) { this->Cell->Delete(); }
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ClippingPlanes: ";
  if (this->ClippingPlanes)
    {
    os << this->ClippingPlanes << "\n";
    }
  else
    {
    os << "(none)\n";
    } 

  os << indent << "GenerateOutline: "
     << (this->GenerateOutline ? "On\n" : "Off\n" );

  os << indent << "GenerateFaces: "
     << (this->GenerateFaces ? "On\n" : "Off\n" );

  os << indent << "GenerateColorScalars: "
     << (this->GenerateColorScalars ? "On\n" : "Off\n" );

  os << indent << "BaseColor: " << this->BaseColor[0] << ", "
     << this->BaseColor[1] << ", " << this->BaseColor[2] << "\n";

  os << indent << "ClipColor: " << this->ClipColor[0] << ", "
     << this->ClipColor[1] << ", " << this->ClipColor[2] << "\n";

  os << indent << "ActivePlaneId: " << this->ActivePlaneId << "\n";

  os << indent << "ActivePlaneColor: " << this->ActivePlaneColor[0] << ", "
     << this->ActivePlaneColor[1] << ", " << this->ActivePlaneColor[2] << "\n";
}

//----------------------------------------------------------------------------
int vtkClipClosedSurface::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  unsigned long* mtime)
{
  unsigned long mTime = this->GetMTime();

  vtkPlaneCollection *planes = this->ClippingPlanes;
  vtkPlane *plane = 0;

  if (planes)
    {
    unsigned long planesMTime = planes->GetMTime();
    if (planesMTime > mTime)
      {
      mTime = planesMTime;
      }

    vtkCollectionSimpleIterator iter;
    planes->InitTraversal(iter);
    while ( (plane = planes->GetNextPlane(iter)) )
      {
      unsigned long planeMTime = plane->GetMTime();
      if (planeMTime > mTime)
        {
        mTime = planeMTime;
        }
      }
    }

  *mtime = mTime;

  return 1;
}

//----------------------------------------------------------------------------
int vtkClipClosedSurface::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the input points
  vtkPoints *inputPoints = input->GetPoints();
  vtkIdType numPts = 0;
  if (inputPoints)
    {
    numPts = inputPoints->GetNumberOfPoints();
    }

  // Force points to double precision.
  vtkPoints *points = vtkPoints::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPts);
  for (vtkIdType ptId = 0; ptId < numPts; ptId++)
    {
    double point[3];
    inputPoints->GetPoint(ptId, point);
    points->SetPoint(ptId, point);
    } 

  // The cell scalars
  vtkUnsignedCharArray *lineScalars = 0;
  vtkUnsignedCharArray *polyScalars = 0;
  vtkUnsignedCharArray *inputScalars = 0;

  // For input scalars: the offsets to the various cell types
  vtkIdType firstLineScalar = 0;
  vtkIdType firstPolyScalar = 0;
  vtkIdType firstStripScalar = 0;

  // Make the colors to be used on the data.
  unsigned char colors[3][3];
  this->CreateColorValues(this->BaseColor, this->ClipColor,
                          this->ActivePlaneColor, colors);

  // This is set if we have to work with scalars.  The input scalars
  // will be copied if they are unsigned char with 3 components, otherwise
  // new scalars will be generated.
  if (this->GenerateColorScalars)
    {
    // Make the scalars
    lineScalars = vtkUnsignedCharArray::New();
    lineScalars->SetNumberOfComponents(3);

    vtkDataArray *tryInputScalars = input->GetCellData()->GetScalars();
    // Get input scalars if they are RGB color scalars
    if (tryInputScalars && tryInputScalars->IsA("vtkUnsignedCharArray") &&
        tryInputScalars->GetNumberOfComponents() == 3)
      {
      inputScalars = static_cast<vtkUnsignedCharArray *>(
        input->GetCellData()->GetScalars());

      vtkIdType numVerts = 0;
      vtkIdType numLines = 0;
      vtkIdType numPolys = 0;
      vtkCellArray *tmpCellArray = 0;
      if ( (tmpCellArray = input->GetVerts()) )
        {
        numVerts = tmpCellArray->GetNumberOfCells();
        }
      if ( (tmpCellArray = input->GetLines()) )
        {
        numLines = tmpCellArray->GetNumberOfCells();
        }
      if ( (tmpCellArray = input->GetPolys()) )
        {
        numPolys = tmpCellArray->GetNumberOfCells();
        }
      firstLineScalar = numVerts;
      firstPolyScalar = numVerts + numLines;
      firstStripScalar = numVerts + numLines + numPolys;
      }
    }

  // Break the input lines into segments, generate scalars for lines
  vtkCellArray *lines = 0;
  lines = vtkCellArray::New();
  if (input->GetLines() && input->GetLines()->GetNumberOfCells() > 0)
    {
    this->BreakPolylines(input->GetLines(), lines, inputScalars,
                         firstLineScalar, lineScalars, colors[0]);
    }

  // Copy the polygons, convert strips to triangles
  vtkCellArray *polys = 0;
  int polyMax = 3;
  if ((input->GetPolys() && input->GetPolys()->GetNumberOfCells() > 0) ||
      (input->GetStrips() && input->GetStrips()->GetNumberOfCells() > 0))
    {
    // If there are line scalars, then poly scalars are needed too
    if (lineScalars)
      {
      polyScalars = vtkUnsignedCharArray::New();
      polyScalars->SetNumberOfComponents(3);
      }

    polys = vtkCellArray::New();
    this->CopyPolygons(input->GetPolys(), polys, inputScalars,
                       firstPolyScalar, polyScalars, colors[0]);
    this->BreakTriangleStrips(input->GetStrips(), polys, inputScalars,
                              firstStripScalar, polyScalars, colors[0]);

    // Check if the input has polys and quads or just triangles
    vtkIdType npts = 0;
    vtkIdType *pts = 0;
    vtkCellArray *inPolys = input->GetPolys();
    inPolys->InitTraversal();
    while (inPolys->GetNextCell(npts, pts))
      {
      if (npts > polyMax)
        {
        polyMax = npts;
        }
      }
    }

  // Get the clipping planes
  vtkPlaneCollection *planes = this->ClippingPlanes;

  // Arrays for storing the clipped lines and polys.
  vtkCellArray *newLines = vtkCellArray::New();
  vtkCellArray *newPolys = 0;
  if (polys)
    {
    newPolys = vtkCellArray::New();
    }

  // Make the locator and the points
  if (this->Locator == 0)
    {
    this->Locator = vtkMergePoints::New();
    }
  vtkIncrementalPointLocator *locator = this->Locator;
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetDataTypeToDouble();

  // The point scalars, needed for clipping (not for the output!)
  vtkDoubleArray *pointScalars = vtkDoubleArray::New();
  vtkPointData *inPointData = vtkPointData::New();
  inPointData->CopyScalarsOn();
  inPointData->SetScalars(pointScalars);
  pointScalars->Delete();
  pointScalars = 0;

  // The line scalars, for coloring the outline
  vtkCellData *inLineData = vtkCellData::New();
  inLineData->CopyScalarsOn();
  inLineData->SetScalars(lineScalars);
  if (lineScalars)
    {
    lineScalars->Delete();
    lineScalars = 0;
    }

  // The poly scalars, for coloring the faces
  vtkCellData *inPolyData = vtkCellData::New();
  inPolyData->CopyScalarsOn();
  inPolyData->SetScalars(polyScalars);
  if (polyScalars)
    {
    polyScalars->Delete();
    polyScalars = 0;
    }

  // Also create output attribute data
  vtkPointData *outPointData = vtkPointData::New();
  outPointData->CopyScalarsOn();

  vtkCellData *outLineData = vtkCellData::New();
  outLineData->CopyScalarsOn();

  vtkCellData *outPolyData = vtkCellData::New();
  outPolyData->CopyScalarsOn();

  // Go through the clipping planes and clip the input with each plane
  vtkCollectionSimpleIterator iter;
  int numPlanes = 0;
  if (planes)
    {
    planes->InitTraversal(iter);
    numPlanes = planes->GetNumberOfItems();
    }

  vtkPlane *plane = 0;
  for (int planeId = 0;
       planes && (plane = planes->GetNextPlane(iter));
       planeId++)
    {
    this->UpdateProgress((planeId + 1.0)/(numPlanes + 1.0));
    if (this->GetAbortExecute())
      {
      break;
      }

    // Is this the last cut plane?  If so, generate triangles.
    int triangulate = 5;
    if (planeId == numPlanes-1)
      {
      triangulate = polyMax;
      }

    // Is this the active plane?
    int active = (planeId == this->ActivePlaneId);

    // Convert the plane into an easy-to-evaluate function
    double pc[4];
    plane->GetNormal(pc);
    pc[3] = -vtkMath::Dot(pc, plane->GetOrigin());

    // Create the clip scalars by evaluating the plane at each point
    vtkIdType numPoints = points->GetNumberOfPoints();
    pointScalars = static_cast<vtkDoubleArray *>(inPointData->GetScalars());
    pointScalars->SetNumberOfValues(numPoints);
    for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
      {
      double p[3];
      points->GetPoint(pointId, p);
      double val = p[0]*pc[0] + p[1]*pc[1] + p[2]*pc[2] + pc[3];
      pointScalars->SetValue(pointId, val);
      }

    // Prepare the locator for merging points during clipping
    locator->InitPointInsertion(newPoints, input->GetBounds());

    // Prepare the output scalars
    outPointData->InterpolateAllocate(inPointData, 0, 0);
    outLineData->CopyAllocate(inLineData, 0, 0);
    outPolyData->CopyAllocate(inPolyData, 0, 0);

    // Clip the lines
    this->ClipLines(
      points, pointScalars, locator, lines, newLines,
      inPointData, outPointData, inLineData, outLineData);

    // Clip the polys
    if (polys)
      {
      // Get the number of lines remaining after the clipping
      vtkIdType numClipLines = newLines->GetNumberOfCells();

      // Cut the polys to generate more lines
      this->ClipAndContourPolys(
        points, pointScalars, locator, triangulate, polys, newPolys, newLines,
        inPointData, outPointData, inPolyData, outPolyData, outLineData);
      
      // Add scalars for the newly-created contour lines
      vtkUnsignedCharArray *scalars =
        vtkUnsignedCharArray::SafeDownCast(outLineData->GetScalars());

      if (scalars)
        {
        // Set the color to the active color if plane is active
        unsigned char *color = colors[1+active];
        unsigned char *activeColor = colors[2];

        vtkIdType numLines = newLines->GetNumberOfCells();
        for (vtkIdType lineId = numClipLines; lineId < numLines; lineId++)
          {
          unsigned char oldColor[3];
          scalars->GetTupleValue(lineId, oldColor);
          if (oldColor[0] != activeColor[0] ||
              oldColor[1] != activeColor[1] ||
              oldColor[2] != activeColor[2])
            {
            scalars->SetTupleValue(lineId, color);
            }
          }
        }

      // Generate new polys from the cut lines
      this->MakeCutPolys(newPoints, newLines, numClipLines, newPolys, pc,
                         outPolyData, colors[1 + active]);

      }

    // Swap the lines, points, etcetera: old output becomes new input
    vtkPoints *tmp0 = points;
    points = newPoints;
    newPoints = tmp0;
    newPoints->Initialize();

    vtkCellArray *tmp1 = lines;
    lines = newLines;
    newLines = tmp1;
    newLines->Initialize();

    if (polys)
      {
      vtkCellArray *tmp2 = polys;
      polys = newPolys;
      newPolys = tmp2;
      newPolys->Initialize();
      }

    vtkPointData *tmp3 = inPointData;
    inPointData = outPointData;
    outPointData = tmp3;
    outPointData->Initialize();

    vtkCellData *tmp4 = inLineData;
    inLineData = outLineData;
    outLineData = tmp4;
    outLineData->Initialize();

    vtkCellData *tmp5 = inPolyData;
    inPolyData = outPolyData;
    outPolyData = tmp5;
    outPolyData->Initialize();
    }

  output->SetPoints(points);
  points->Delete();

  // Get the line scalars
  vtkUnsignedCharArray *scalars = 
    vtkUnsignedCharArray::SafeDownCast(inLineData->GetScalars());

  if (this->GenerateOutline)
    {
    output->SetLines(lines);
    }
  else if (scalars)
    {
    // If not adding lines to output, clear the line scalars
    scalars->Initialize();
    }

  if (this->GenerateFaces)
    {
    output->SetPolys(polys);

    if (polys && scalars)
      {
      vtkUnsignedCharArray *pScalars = 
        vtkUnsignedCharArray::SafeDownCast(inPolyData->GetScalars());

      vtkIdType m = scalars->GetNumberOfTuples();
      vtkIdType n = pScalars->GetNumberOfTuples();

      if (n > 0)
        {
        unsigned char color[3];
        color[0] = color[1] = color[2] = 0;

        // This is just to expand the array
        scalars->InsertTupleValue(n+m-1, color);

        // Fill in the poly scalars
        for (vtkIdType i = 0; i < n; i++)
          {
          pScalars->GetTupleValue(i, color);
          scalars->SetTupleValue(i+m, color);
          }
        }
      }
   }

  lines->Delete();

  if (polys)
    {
    polys->Delete();
    }

  output->GetCellData()->SetScalars(scalars);

  locator->Initialize();
  newPoints->Delete();
  newLines->Delete();
  if (newPolys)
    {
    newPolys->Delete();
    }

  inPointData->Delete();
  outPointData->Delete();
  inLineData->Delete();
  outLineData->Delete();
  inPolyData->Delete();
  outPolyData->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::CreateColorValues(
  const double color1[3], const double color2[3], const double color3[3],
  unsigned char colors[3][3])
{
  // Convert colors from "double" to "unsigned char"

  const double *dcolors[3];
  dcolors[0] = color1;
  dcolors[1] = color2;
  dcolors[2] = color3;

  for (int i = 0; i < 3; i++)
    {
    for (int j = 0; j < 3; j++)
      {
      double val = dcolors[i][j];
      if (val < 0) { val = 0; }
      if (val > 1) { val = 1; }
      colors[i][j] = static_cast<unsigned char>(val*255);
      }
    }
}

//----------------------------------------------------------------------------
// Point interpolation for clipping and contouring, given the scalar
// values (v0, v1) for the two endpoints (p0, p1).  The use of this
// function guarantees perfect consistency in the results.
static void vtkCCSInterpolatePoint(
  const double *p0, const double *p1, double v0, double v1,
  double p[3], double& t)
{
  double s = 0;
  double *sp = &s;
  double *tp = &t;

  // This swap guarantees that exactly the same point is computed
  // for both line directions, as long as the endpoints are the same.
  if (v1 > 0)
    {
    const double *ptmp = p0;
    p0 = p1;
    p1 = ptmp;

    const double tmp = v0;
    v0 = v1;
    v1 = tmp;

    double *tmptp = tp;
    tp = sp;
    sp = tmptp;
    }

  *tp = v0/(v0 - v1);
  *sp = 1.0 - *tp;

  p[0] = (*sp)*p0[0] + (*tp)*p1[0];
  p[1] = (*sp)*p0[1] + (*tp)*p1[1];
  p[2] = (*sp)*p0[2] + (*tp)*p1[2];
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::ClipLines(
  vtkPoints *points, vtkDoubleArray *pointScalars,
  vtkIncrementalPointLocator *locator,
  vtkCellArray *inputCells, vtkCellArray *outputLines, 
  vtkPointData *inPointData, vtkPointData *outPointData,
  vtkCellData *inCellData, vtkCellData *outLineData)
{
  vtkIdType numCells = inputCells->GetNumberOfCells();
  vtkIdType numPts = 0;
  vtkIdType *pts = 0;

  inputCells->InitTraversal();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    inputCells->GetNextCell(numPts, pts);

    vtkIdType i1 = pts[0]; 
    double v1 = pointScalars->GetValue(i1);
    int c1 = (v1 > 0);

    for (vtkIdType i = 1; i < numPts; i++)
      {
      vtkIdType i0 = i1;
      double v0 = v1;
      int c0 = c1;

      i1 = pts[i];
      v1 = pointScalars->GetValue(i1);
      c1 = (v1 > 0);

      // If at least one point wasn't clipped
      if ( (c0 | c1) )
        {
        double p0[3], p1[3];
        points->GetPoint(i0, p0);
        points->GetPoint(i1, p1);

        vtkIdType linePts[2];
        linePts[0] = 0;
        linePts[1] = 0;

        // If only one end was clipped, interpolate new point
        if ( (c0 ^ c1) )
          {
          double t, p[3];
          vtkCCSInterpolatePoint(p0, p1, v0, v1, p, t);

          if (locator->InsertUniquePoint(p, linePts[c0]))
            {
            outPointData->InterpolateEdge(inPointData,linePts[c0],i0,i1,t);
            }
          }

        if (c0 && locator->InsertUniquePoint(p0, linePts[0]))
          {
          outPointData->CopyData(inPointData, i0, linePts[0]);
          }

        if (c1 && locator->InsertUniquePoint(p1, linePts[1]))
          {
          outPointData->CopyData(inPointData, i1, linePts[1]);
          }

        // If endpoints are different, insert the line segment
        if (linePts[0] != linePts[1])
          {
          vtkIdType newCellId = outputLines->InsertNextCell(2, linePts);
          outLineData->CopyData(inCellData, cellId, newCellId);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::ClipAndContourPolys(
  vtkPoints *points, vtkDoubleArray *pointScalars,
  vtkIncrementalPointLocator *locator, int triangulate,
  vtkCellArray *inputCells,
  vtkCellArray *outputPolys, vtkCellArray *outputLines, 
  vtkPointData *inPointData, vtkPointData *outPointData,
  vtkCellData *inCellData,
  vtkCellData *outPolyData, vtkCellData *outLineData)
{
  if (!this->IdList)
    {
    this->IdList = vtkIdList::New();
    }
  if (!this->Polygon)
    {
    this->Polygon = vtkPolygon::New();
    }

  vtkIdList *idList = this->IdList;
  vtkPolygon *polygon = this->Polygon;

  // How many sides for output polygons?
  int polyMax = VTK_INT_MAX;
  if (triangulate)
    {
    if (triangulate < 4)
      { // triangles only
      polyMax = 3;
      }
    else if (triangulate == 4)
      { // allow triangles and quads
      polyMax = 4;
      }
    }

  // Go through all cells and clip them.
  vtkIdType numCells = inputCells->GetNumberOfCells();
  vtkIdType numPts = 0;
  vtkIdType *pts = 0;

  inputCells->InitTraversal();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    inputCells->GetNextCell(numPts, pts);
    polygon->PointIds->Reset();
    polygon->Points->Reset();

    vtkIdType i1 = pts[numPts-1]; 
    double v1 = pointScalars->GetValue(i1);
    int c1 = (v1 > 0);

    double p1[3];
    points->GetPoint(i1, p1);

    // The ids for the current edge
    vtkIdType j0 = -1;
    vtkIdType j1 = -1;

    // Insert the first point (actually the last point) if it is
    // not clipped away 
    if (c1 && locator->InsertUniquePoint(p1, j0))
      {
      outPointData->CopyData(inPointData, i1, j0);
      }

    // To store the ids of the contour line
    vtkIdType linePts[2];
    linePts[0] = 0;
    linePts[1] = 0;

    for (vtkIdType i = 0; i < numPts; i++)
      {
      // Save previous point info
      vtkIdType i0 = i1;
      double v0 = v1;
      int c0 = c1;
      double p0[3];
      p0[0] = p1[0]; p0[1] = p1[1]; p0[2] = p1[2];

      // Generate new point info
      i1 = pts[i];
      v1 = pointScalars->GetValue(i1);
      c1 = (v1 > 0);
      points->GetPoint(i1, p1);

      // If at least one edge end point wasn't clipped
      if ( (c0 | c1) )
        {
        // If only one end was clipped, interpolate new point
        if ( (c0 ^ c1) )
          {
          double t, p[3];
          vtkCCSInterpolatePoint(p0, p1, v0, v1, p, t);

          if (locator->InsertUniquePoint(p, j1))
            {
            outPointData->InterpolateEdge(inPointData, j1, i0, i1, t);
            }
          if (j1 != j0)
            {
            polygon->PointIds->InsertNextId(j1);
            polygon->Points->InsertNextPoint(p);
            j0 = j1;
            }
          // Save as one end of the contour line
          linePts[c0] = j1;
          }

        if (c1)
          {
          if (locator->InsertUniquePoint(p1, j1))
            {
            outPointData->CopyData(inPointData, i1, j1);
            }
          if (j1 != j0)
            {
            polygon->PointIds->InsertNextId(j1);
            polygon->Points->InsertNextPoint(p1);
            j0 = j1;
            }
          }
        }
      }

    // Insert the clipped poly
    vtkIdType numPoints = polygon->PointIds->GetNumberOfIds();
    if (numPoints > polyMax)
      {
      // Triangulate the poly and insert triangles into output
      idList->Initialize();
      polygon->Triangulate(idList);
      vtkIdType m = idList->GetNumberOfIds();
      for (vtkIdType k = 0; k < m; k += 3)
        {
        vtkIdType newCellId = outputPolys->InsertNextCell(3);
        outputPolys->InsertCellPoint(
          polygon->PointIds->GetId(idList->GetId(k + 0)));
        outputPolys->InsertCellPoint(
          polygon->PointIds->GetId(idList->GetId(k + 1)));
        outputPolys->InsertCellPoint(
          polygon->PointIds->GetId(idList->GetId(k + 2)));
        outPolyData->CopyData(inCellData, cellId, newCellId);
        }
      }
    else if (numPoints > 2)
      {
      // Insert the polygon without triangulating it
      vtkIdType newCellId = outputPolys->InsertNextCell(polygon);
      outPolyData->CopyData(inCellData, cellId, newCellId);
      }

    // Insert the contour line if one was created
    if (linePts[0] != linePts[1])
      {
      vtkIdType newCellId = outputLines->InsertNextCell(2, linePts);
      outLineData->CopyData(inCellData, cellId, newCellId);
      }
    }

  // Free up the idList memory
  idList->Initialize();
  polygon->Points->Initialize();
  polygon->PointIds->Initialize();
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::BreakPolylines(
  vtkCellArray *inputLines, vtkCellArray *lines,
  vtkUnsignedCharArray *inputScalars, vtkIdType firstLineScalar,
  vtkUnsignedCharArray *scalars, const unsigned char color[3])
{
  // The color for the lines
  unsigned char cellColor[3];
  cellColor[0] = color[0];
  cellColor[1] = color[1];
  cellColor[2] = color[2];

  // Break the input lines into segments
  inputLines->InitTraversal();
  vtkIdType cellId = 0;
  vtkIdType npts, *pts;
  while (inputLines->GetNextCell(npts, pts))
    {
    if (inputScalars)
      {
      inputScalars->GetTupleValue(firstLineScalar + cellId++, cellColor);
      }

    for (vtkIdType i = 1; i < npts; i++)
      {
      lines->InsertNextCell(2);
      lines->InsertCellPoint(pts[i-1]);
      lines->InsertCellPoint(pts[i]);

      if (scalars)
        {
        scalars->InsertNextTupleValue(cellColor);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::CopyPolygons(
  vtkCellArray *inputPolys, vtkCellArray *polys,
  vtkUnsignedCharArray *inputScalars, vtkIdType firstPolyScalar,
  vtkUnsignedCharArray *polyScalars, const unsigned char color[3])
{
  if (!inputPolys)
    {
    return;
    }

  polys->DeepCopy(inputPolys);

  if (polyScalars)
    {
    unsigned char scalarValue[3];
    scalarValue[0] = color[0];
    scalarValue[1] = color[1];
    scalarValue[2] = color[2];

    vtkIdType n = polys->GetNumberOfCells();
    polyScalars->SetNumberOfTuples(n);

    if (inputScalars)
      {
      for (vtkIdType i = 0; i < n; i++)
        {
        inputScalars->GetTupleValue(i + firstPolyScalar, scalarValue);
        polyScalars->SetTupleValue(i, scalarValue);
        }
      }
    else
      {
      for (vtkIdType i = 0; i < n; i++)
        {
        polyScalars->SetTupleValue(i, scalarValue);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::BreakTriangleStrips(
  vtkCellArray *inputStrips, vtkCellArray *polys,
  vtkUnsignedCharArray *inputScalars, vtkIdType firstStripScalar,
  vtkUnsignedCharArray *polyScalars, const unsigned char color[3])
{
  if (!inputStrips)
    {
    return;
    }

  vtkIdType npts = 0;
  vtkIdType *pts = 0;

  inputStrips->InitTraversal();
 
  for (vtkIdType cellId = firstStripScalar;
       inputStrips->GetNextCell(npts, pts);
       cellId++)
    {
    vtkTriangleStrip::DecomposeStrip(npts, pts, polys);

    if (polyScalars)
      {
      unsigned char scalarValue[3];
      scalarValue[0] = color[0];
      scalarValue[1] = color[1];
      scalarValue[2] = color[2];

      if (inputScalars)
        {
        // If there are input scalars, use them instead of "color"
        inputScalars->GetTupleValue(cellId, scalarValue);
        }

      vtkIdType n = npts - 3;
      vtkIdType m = polyScalars->GetNumberOfTuples();
      if (n >= 0)
        {
        // First insert is just to allocate space
        polyScalars->InsertTupleValue(m+n, scalarValue);

        for (vtkIdType i = 0; i < n; i++)
          {
          polyScalars->SetTupleValue(m+i, scalarValue);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
// Everything below this point is support code for MakeCutPolys().
// It could be separated out into its own class for generating
// polygons from contours. 
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// A helper class: a bitfield that is always as large as needed.
// For our purposes this is much more convenient than a bool vector,
// which would have to be resized and range-checked externally.

class vtkCCSBitArray
{
public:
  void set(size_t bit, int val) {
    size_t n = (bit >> 5);
    size_t i = (bit & 0x1f);
    if (n >= bitstorage.size()) { bitstorage.resize(n+1); }
    unsigned int chunk = bitstorage[n];
    int bitval = 1;
    bitval <<= i;
    if (val) { chunk = chunk | bitval; }
    else { chunk = chunk & ~bitval; }
    bitstorage[n] = chunk;
  };

  int get(size_t bit) {
    size_t n = (bit >> 5);
    size_t i = (bit & 0x1f);
    if (n >= bitstorage.size()) { return 0; }
    unsigned int chunk = bitstorage[n];
    return ((chunk >> i) & 1);
  };

  void clear() {
    bitstorage.clear();
  };

private:
  vtkstd::vector<unsigned int> bitstorage;
};

//----------------------------------------------------------------------------
// Simple typedefs for stl-based polygons.

// A poly type that is just a vector of vtkIdType
typedef vtkstd::vector<vtkIdType> vtkCCSPoly;

// A poly group type that holds indices into a vector of polys.
// A poly group is used to represent a polygon with holes.
// The first member of the group is the outer poly, and all
// other members are the holes.
typedef vtkstd::vector<size_t> vtkCCSPolyGroup;

//----------------------------------------------------------------------------
// These are the prototypes for helper functions for manipulating
// polys that are stored in stl vectors.

// Take a set of lines, join them tip-to-tail to create polygons
static void vtkCCSMakePolysFromLines(
  vtkCellArray *lines, vtkIdType firstLine, vtkIdType numLines,
  vtkstd::vector<vtkCCSPoly> &newPolys,
  vtkstd::vector<size_t> &incompletePolys);

// Finish any incomplete polygons by trying to join loose ends
static void vtkCCSJoinLooseEnds(
  vtkstd::vector<vtkCCSPoly> &polys, vtkstd::vector<size_t> &incompletePolys,
  vtkPoints *points, const double normal[3]);

// Check for polygons that contain multiple loops, and split the loops apart
static void vtkCCSUntangleSelfIntersection(
  vtkstd::vector<vtkCCSPoly> &newPolys);

// Remove points that are not vertices of the polygon,
// i.e. remove any points that are on an edge but not at a corner.
// This simplifies all remaining steps and improves the triangulation.
// The original edges are appended to the originalEdges cell array,
// where each cell in this array will be a polyline consisting of two
// corner vertices and all the points in between.
static void vtkCCSFindTrueEdges(
  vtkstd::vector<vtkCCSPoly> &newPolys, vtkPoints *points,
  vtkCellArray *originalEdges);

// Returns 1 if the poly's normal matches the specified normal
static int vtkCCSCheckPolygonSense(
  vtkCCSPoly &polys, vtkPoints *points, const double normal[3]);

// Add a triangle to the output, and subdivide the triangle if one
// of the edges originally had more than two points, as indicated
// by originalEdges.  If scalars is not null, then add a scalar for
// each triangle.
static void vtkCCSInsertTriangle(
  vtkCellArray *polys, const vtkIdType pts[3], vtkCellArray *originalEdges,
  vtkUnsignedCharArray *scalars, const unsigned char color[3]);

// Check for polys within other polys, i.e. find polys that are holes and
// add them to the "polyGroup" of the poly that they are inside of.
static void vtkCCSMakeHoleyPolys(
  vtkstd::vector<vtkCCSPoly> &polys, vtkPoints *points,
  vtkstd::vector<vtkCCSPolyGroup> &polyGroups,
  const double normal[3]);

// For each poly that has holes, make two cuts between each hole and
// the outer poly in order to turn the polygon+hole into two polys.
static void vtkCCSCutHoleyPolys(
  vtkstd::vector<vtkCCSPoly> &polys, vtkPoints *points,
  vtkstd::vector<vtkCCSPolyGroup> &polyGroups,
  const double normal[3]);

//----------------------------------------------------------------------------
// This is a complex subroutine that takes a collection of lines that
// were formed by cutting a polydata with a plane, and generates
// a face that has those lines as its edges.  The lines must form one
// or more closed contours, but they need not be sorted.
//
// Only the lines from "firstLine" onward are used to create new polygons,
// and the new polygons are appended to "polys".  The normal of the cut
// plane must be provided so that the polys will be correctly oriented.
// New cell scalars will be appended to outCD.  These will be color
// scalars, where "color" specifies the color to be used.

void vtkClipClosedSurface::MakeCutPolys(
  vtkPoints *points, vtkCellArray *lines, vtkIdType firstLine,
  vtkCellArray *polys, const double normal[3], vtkCellData *outCD,
  const unsigned char color[3])
{
  // Need a temporary cell array to store some polylines
  if (!this->CellArray)
    {
    this->CellArray = vtkCellArray::New();
    }

  // Find the number of lines that were generated by the cut
  vtkIdType numLines = lines->GetNumberOfCells();
  vtkIdType numNewLines = numLines - firstLine;

  // If no cut lines were generated, there's nothing to do
  if (firstLine >= numLines)
    {
    return;
    }

  // Join all the new lines into connected groups, i.e. polygons.
  // If we are lucky these will be simple, convex polygons.  But
  // we can't count on that.

  vtkstd::vector<vtkCCSPoly> newPolys;
  vtkstd::vector<size_t> incompletePolys;
  vtkCCSMakePolysFromLines(lines, firstLine, numNewLines, newPolys,
                           incompletePolys);

  // Join any loose ends.  If the input was a closed surface then there
  // will not be any loose ends, so this is provided as a service to users
  // who want to clip a non-closed surface.
  vtkCCSJoinLooseEnds(newPolys, incompletePolys, points, normal);

  // Some polys might be self-intersecting.  Split the polys at each
  // intersection point.

  vtkCCSUntangleSelfIntersection(newPolys);

  // Some points might be in the middle of straight line segments.
  // These points can be removed without changing the shape of the
  // polys, and removing them makes triangulation more stable.
  // Unfortunately removing these points also means that the polys
  // will no longer form a watertight cap over the cut.

  vtkCellArray *originalEdges = this->CellArray;
  originalEdges->Initialize();
  vtkCCSFindTrueEdges(newPolys, points, originalEdges);

  // Next we have to check for polygons with holes, i.e. polygons that
  // have other polygons inside.  Each polygon is "grouped" with the
  // polygons that make up its holes.

  // Initialize each group to hold just one polygon.

  size_t numNewPolys = newPolys.size();
  vtkstd::vector<vtkCCSPolyGroup> polyGroups(numNewPolys);
  for (size_t i = 0; i < numNewPolys; i++)
    {
    polyGroups[i].push_back(i);
    }

  // Find out which polys are holes in larger polys.  Create a group
  // for each poly where the first member of the group is the larger
  // poly, and all other members are the holes.  The number of polyGroups
  // will be the same as the number of polys, and any polys that are
  // holes will have a matching empty group.

  vtkCCSMakeHoleyPolys(newPolys, points, polyGroups, normal);

  // Make cuts to create simple polygons out of the holey polys.
  // After this is done, each polyGroup will have exactly 1 polygon,
  // and no polys will be holes.

  vtkCCSCutHoleyPolys(newPolys, points, polyGroups, normal);

  // ------ Triangulation code ------

  // Need to add scalars for each cell that is created
  vtkUnsignedCharArray *scalars =
    vtkUnsignedCharArray::SafeDownCast(outCD->GetScalars());

  // Go through all polys and triangulate them
  for (size_t polyId = 0; polyId < numNewPolys; polyId++)
    {
    // If group is empty, then poly was a hole without a containing poly
    if (polyGroups[polyId].size() == 0)
      {
      continue;
      }

    vtkCCSPoly &poly = newPolys[polyId];
    size_t n = poly.size();

    // If the poly is a line, then skip it
    if (n < 3)
      {
      continue;
      }
    // If the poly is a triangle, then pass it
    else if (n == 3)
      {
      vtkIdType pts[3];
      pts[0] = poly[0];
      pts[1] = poly[1];
      pts[2] = poly[2];

      vtkCCSInsertTriangle(polys, pts, originalEdges, scalars, color);
      }
    // If the poly has 4 or more points, triangulate it
    else
      {
      // Need a polygon cell and idlist for triangulation
      if (this->Polygon == 0)
        {
        this->Polygon = vtkPolygon::New();
        }
      vtkPolygon *polygon = this->Polygon;

      if (this->IdList == 0)
        {
        this->IdList = vtkIdList::New();
        }
      vtkIdList *triangles = this->IdList;

      polygon->Points->SetDataTypeToDouble();
      polygon->Points->SetNumberOfPoints(n);
      polygon->PointIds->SetNumberOfIds(n);

      for (size_t j = 0; j < n; j++)
        {
        vtkIdType pointId = newPolys[polyId][j];
        double point[3];
        points->GetPoint(pointId, point);
        polygon->Points->SetPoint(static_cast<vtkIdType>(j), point);
        polygon->PointIds->SetId(static_cast<vtkIdType>(j), pointId);
        }

      triangles->Initialize();
      polygon->Triangulate(triangles);
      vtkIdType m = triangles->GetNumberOfIds();

      for (vtkIdType k = 0; k < m; k += 3)
        {
        vtkIdType pts[3];
        pts[0] = poly[static_cast<size_t>(triangles->GetId(k + 0))];
        pts[1] = poly[static_cast<size_t>(triangles->GetId(k + 1))];
        pts[2] = poly[static_cast<size_t>(triangles->GetId(k + 2))];

        vtkCCSInsertTriangle(polys, pts, originalEdges, scalars, color);
        }
      }
    }

  // Free up some memory

  if (this->Polygon)
    {
    this->Polygon->Points->Initialize();
    this->Polygon->PointIds->Initialize();
    }
  if (this->IdList)
    {
    this->IdList->Initialize();
    }
  if (this->CellArray)
    {
    this->CellArray->Initialize();
    }
}

// ---------------------------------------------------
// Here is the code for creating polygons from line segments.

void vtkCCSMakePolysFromLines(
  vtkCellArray *lines, vtkIdType firstLine, vtkIdType numLines,
  vtkstd::vector<vtkCCSPoly> &newPolys,
  vtkstd::vector<size_t> &incompletePolys)
{
  // Skip through the cell array until we get to the first line
  lines->InitTraversal();
  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  for (vtkIdType cellId = 0; cellId < firstLine; cellId++)
    {
    lines->GetNextCell(npts, pts);
    }

  vtkIdType firstLineLoc = lines->GetTraversalLocation();

  // Bitfield for marking lines as used
  vtkCCSBitArray usedLines;

  size_t numNewPolys = 0;
  vtkIdType remainingLines = numLines;
  while (remainingLines > 0)
    {
    // Create a new poly
    size_t polyId = numNewPolys++;
    newPolys.resize(numNewPolys);
    vtkCCSPoly &poly = newPolys[polyId];

    int completePoly = 0;
    int noLinesMatch = 0;
    while (!completePoly && !noLinesMatch && remainingLines > 0)
      {
      noLinesMatch = 1;
      lines->SetTraversalLocation(firstLineLoc);
      for (vtkIdType lineId = 0; lineId < numLines; lineId++)
        {
        lines->GetNextCell(npts, pts);

        if (usedLines.get(lineId))
          {
          continue;
          }

        // Number of points in the poly
        size_t npoly = poly.size();

        int usedLine = 1;

        if (poly.size() == 0)
          {
          poly.resize(static_cast<size_t>(npts));
          for (vtkIdType i = 0; i < npts; i++)
            { 
            poly[i] = pts[i];
            }
          }
        else if (pts[0] == poly[npoly-1] &&
                 pts[1] != poly[npoly-2])
          {
          vtkIdType n = npts;
          if (pts[npts-1] == poly[0])
            {
            n = npts-1;
            completePoly = 1;
            }

          poly.insert(poly.end(), n - 1, 0);

          for (vtkIdType k = 1; k < n; k++)
            {
            poly[npoly++] = pts[k];
            }
          }
        else if (pts[npts-1] == poly[0] &&
                 pts[npts-2] != poly[1])
          {
          vtkIdType n = npts - 1;
          poly.insert(poly.begin(), n, 0);

          for (vtkIdType k = 0; k < n; k++)
            {
            poly[k] = pts[k];
            }
          }
        else
          {
          usedLine = 0;
          }

        if (usedLine)
          {
          noLinesMatch = 0;
          usedLines.set(lineId, 1);
          remainingLines--;
          }
        }
      }

    // Check for incomplete polygons
    if (noLinesMatch)
      {
      incompletePolys.push_back(polyId);
      }
    }
}

// ---------------------------------------------------
// Join polys that have loose ends, as indicated by incompletePolys.
// Any polys created will have a normal opposite to the supplied normal,
// and any new edges that are created will be on the hull of the point set.
// Shorter edges will be preferred over long edges.

static void vtkCCSJoinLooseEnds(
  vtkstd::vector<vtkCCSPoly> &polys, vtkstd::vector<size_t> &incompletePolys,
  vtkPoints *points, const double normal[3])
{
  // Relative tolerance for checking whether an edge is on the hull
  const double tol = 1e-5;

  // A list of polys to remove when everything is done
  vtkstd::vector<size_t> removePolys;

  size_t n;
  while ( (n = incompletePolys.size()) )
    {
    vtkCCSPoly &poly1 = polys[incompletePolys[n-1]];
    vtkIdType pt1 = poly1[poly1.size()-1];
    double p1[3], p2[3];
    points->GetPoint(pt1, p1);

    double dMin = VTK_DOUBLE_MAX;
    size_t iMin = 0; 

    for (size_t i = 0; i < n; i++)
      {
      vtkCCSPoly &poly2 = polys[incompletePolys[i]];
      vtkIdType pt2 = poly2[0];
      points->GetPoint(pt2, p2);

      // The next few steps verify that edge [p1, p2] is on the hull
      double v[3];
      v[0] = p2[0] - p1[0]; v[1] = p2[1] - p1[1]; v[2] = p2[2] - p1[2];
      double d = vtkMath::Norm(v);
      v[0] /= d; v[1] /= d; v[2] /= d;

      // Compute the midpoint of the edge
      double pm[3];
      pm[0] = 0.5*(p1[0] + p2[0]);
      pm[1] = 0.5*(p1[1] + p2[1]);
      pm[2] = 0.5*(p1[2] + p2[2]);

      // Create a plane equation
      double pc[4];
      vtkMath::Cross(v, normal, pc);
      pc[3] = -vtkMath::Dot(pc, pm);

      // Check that all points are inside the plane.  If they aren't, then
      // the edge is not on the hull of the pointset.
      int badPoint = 0;
      size_t m = polys.size();
      for (size_t j = 0; j < m && !badPoint; j++)
        {
        vtkCCSPoly &poly = polys[j];
        size_t npts = poly.size();
        for (size_t k = 0; k < npts; k++)
          {
          vtkIdType ptId = poly[k];
          if (ptId != pt1 && ptId != pt2)
            {
            double p[3];
            points->GetPoint(ptId, p);
            double val = p[0]*pc[0] + p[1]*pc[1] + p[2]*pc[2] + pc[3];
            double r2 = vtkMath::Distance2BetweenPoints(p, pm);

            // Check distance from plane against the tolerance
            if (val < 0 && val*val > tol*tol*r2)
              {
              badPoint = 1;
              break;
              }
            }
          }

        // If no bad points, then this edge is a candidate
        if (!badPoint && d < dMin)
          {
          dMin = d;
          iMin = i;
          }
        }
      }

    // If a match was found, append the polys
    if (dMin < VTK_DOUBLE_MAX)
      {
      // Did the poly match with itself?
      if (iMin == n-1)
        {
        // Mark the poly as closed
        incompletePolys.pop_back();
        }
      else
        {
        size_t id2 = incompletePolys[iMin];

        // Combine the polys
        poly1.insert(poly1.end(), polys[id2].begin(), polys[id2].end());

        // Erase the second poly
        removePolys.push_back(id2);
        incompletePolys.erase(incompletePolys.begin() + iMin);
        }
      }
    else
      {
      // If no match, erase this poly from consideration
      removePolys.push_back(incompletePolys[n-1]);
      incompletePolys.pop_back();
      }
    }

  // Remove polys that couldn't be completed
  vtkstd::sort(removePolys.begin(), removePolys.end());
  size_t i = removePolys.size();
  while (i > 0)
    {
    // Remove items in reverse order
    polys.erase(polys.begin() + removePolys[--i]);
    }

  // Clear the incompletePolys vector, it's indices are no longer valid
  incompletePolys.clear();
}

// ---------------------------------------------------
// Check for self-intersection. Split the figure-eights.
// This assumes that all intersections occur at existing
// vertices, i.e. no new vertices will be created.

void vtkCCSUntangleSelfIntersection(
  vtkstd::vector<vtkCCSPoly> &newPolys)
{
  size_t numNewPolys = newPolys.size();
  for (size_t i = 0; i < numNewPolys; i++)
    {
    size_t n = newPolys[i].size();

    int foundMatch = 0;
    size_t idx1 = 0;
    size_t idx2 = 0;

    for (idx1 = 0; idx1 < n; idx1++)
      {
      vtkIdType firstId = newPolys[i][idx1];

      for (idx2 = idx1+1; idx2 < n ; idx2++)
        {
        vtkIdType secondId = newPolys[i][idx2];

        if (firstId == secondId)
          {
          foundMatch = 1;
          break;
          }
        }

      if (foundMatch) { break; }
      }

    if (foundMatch)
      {
      // Split off a new poly
      size_t m = idx2 - idx1;

      newPolys.resize(++numNewPolys);
      newPolys[numNewPolys-1].resize(n - m);

      for (size_t j = 0; j < idx1; j++)
        {
        newPolys[numNewPolys-1][j] = newPolys[i][j];
        }
      for (size_t k = idx2; k < n; k++)
        {
        newPolys[numNewPolys-1][k - m] = newPolys[i][k];
        }

      // The current poly, which is now intersection-free
      for (size_t l = 0; l < m; l++)
        {
        newPolys[i][l] = newPolys[i][l + idx1];
        }
      newPolys[i].resize(m);
      }
    }
}

// ---------------------------------------------------
// The polygons might have a lot of extra points, i.e. points
// in the middle of the edges.  Remove those points, but keep
// the original edges as polylines in the originalEdges array.
// Only original edges with more than two points will be kept.

void vtkCCSFindTrueEdges(
  vtkstd::vector<vtkCCSPoly> &newPolys, vtkPoints *points,
  vtkCellArray *originalEdges)
{
  // Tolerance^2 for angle to see if line segments are parallel
  const double tol2 = 1e-10;

  size_t numNewPolys = newPolys.size();
  for (size_t i = 0; i < numNewPolys; i++)
    {
    size_t n = newPolys[i].size();
    if (n < 3) { continue; }

    vtkCCSPoly newPoly;
    vtkIdType cornerPointId = 0;

    // Keep the partial edge from before the first corner is found
    vtkstd::vector<vtkIdType> partialEdge;
    int cellCount = 0;

    double p1[3], p2[3];
    double v1[3], v2[3];
    double l1, l2;

    points->GetPoint(newPolys[i][n-1], p2);
    points->GetPoint(newPolys[i][0], p1);
    v1[0] = p1[0] - p2[0];  v1[1] = p1[1] - p2[1];  v1[2] = p1[2] - p2[2];  
    l1 = vtkMath::Dot(v1, v1);

    for (size_t j = 0; j < n; j++)
      {
      size_t k = j+1;
      if (k == n) { k = 0; }

      points->GetPoint(newPolys[i][k], p2);
      v2[0] = p2[0] - p1[0];  v2[1] = p2[1] - p1[1];  v2[2] = p2[2] - p1[2];  
      l2 = vtkMath::Dot(v2, v2);

      // Dot product is |v1||v2|cos(theta)
      double c = vtkMath::Dot(v1, v2);

      // Keep the point if angle is greater than tolerance:
      // sin^2(theta) = (1 - cos^2(theta)), where
      // c*c = l1*l2*cos^2(theta)

      vtkIdType pointId = newPolys[i][j];

      if (c < 0 || (l1*l2 - c*c) > l1*l2*tol2)
        {
        newPoly.push_back(pointId);

        // Complete the previous edge only if the final point count
        // will be greater than two
        if (cellCount > 1)
          {
          originalEdges->InsertCellPoint(pointId);
          cellCount++;
          originalEdges->UpdateCellCount(cellCount);
          }
        else if (cellCount == 0)
          {
          partialEdge.push_back(pointId);
          }

        // Start a new edge with cornerPointId as a "virtual" point
        cornerPointId = pointId;
        cellCount = 1;
        }
      else if (cellCount > 0)
        {
        // First check to see if we have to add cornerPointId
        if (cellCount == 1)
          {
          originalEdges->InsertNextCell(1);
          originalEdges->InsertCellPoint(cornerPointId);
          }
        // Then add the new point
        originalEdges->InsertCellPoint(pointId);
        cellCount++;
        }
      else
        {
        // No corner yet, so save the point
        partialEdge.push_back(pointId);
        }

      p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
      v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
      l1 = l2;
      }

    // Add the partial edge to the end
    if (cellCount + partialEdge.size() > 2)
      {
      if (cellCount == 1)
        {
        originalEdges->InsertNextCell(1);
        originalEdges->InsertCellPoint(cornerPointId);
        }
      for (size_t ii = 0; ii < partialEdge.size(); ii++)
        {
        originalEdges->InsertCellPoint(partialEdge[ii]);
        cellCount++;
        }

      originalEdges->UpdateCellCount(cellCount);
      }

    newPolys[i] = newPoly;
    }
}

// ---------------------------------------------------
// Insert a triangle, and subdivide that triangle if one of
// its edges originally had more than two points before
// vtkCCSFindTrueEdges was called.  If scalars is not null,
// add a scalar for each triangle that is added.

void vtkCCSInsertTriangle(
  vtkCellArray *polys, const vtkIdType triPts[3], vtkCellArray *originalEdges,
  vtkUnsignedCharArray *scalars, const unsigned char color[3])
{
  int foundEdge = 0;

  vtkIdType npts = 0;
  vtkIdType *pts = 0; 
  originalEdges->InitTraversal();
  while (originalEdges->GetNextCell(npts, pts))
    {
    vtkIdType a = pts[0];

    vtkIdType c = triPts[2];
    vtkIdType d = triPts[0];
    vtkIdType e = triPts[1];

    // Check if the edge and the triangle share a point
    if ((a == c || a == d || a == e) && npts > 2)
      {
      vtkIdType b = pts[npts-1];

      for (int i = 0; i < 3 && !foundEdge; i++)
        {
        // "c to d" is the current edge
        d = triPts[i];

        // If a triangle edge matches an original edge
        if ( (a == c && b == d) || (a == d && b == c) )
          {
          // If original edge is same direction as triangle edge 
          vtkIdType jstart = 1;
          vtkIdType jstop = npts-1;
          vtkIdType jinc = 1;
          // Else if they are in opposite directions
          if (a != c)
            {
            jstart = npts-2;
            jstop = 0;
            jinc = -1;
            }

          // Make new sub-triangles
          vtkIdType newTri[3];
          newTri[0] = c;
          newTri[1] = pts[jstart];
          newTri[2] = e;

          // The first triangle's trailing edge must be checked
          vtkCCSInsertTriangle(polys, newTri, originalEdges,
                               scalars, color);
          newTri[0] = newTri[1];
          jstart += jinc;

          for (vtkIdType j = jstart; (jstop-jinc-j)*jinc >= 0; j += jinc)
            {
            newTri[1] = pts[j];

            vtkIdType cellId = polys->InsertNextCell(3);
            polys->InsertCellPoint(newTri[0]);
            polys->InsertCellPoint(newTri[1]);
            polys->InsertCellPoint(newTri[2]);

            if (scalars)
              {
              scalars->InsertTupleValue(cellId, color);
              }

            newTri[0] = newTri[1];
            }

          // The final triangle's leading edge must be checked
          newTri[1] = d;
          vtkCCSInsertTriangle(polys, newTri, originalEdges,
                               scalars, color);

          foundEdge = 1;
          break;
          }

        // Rotate the points
        e = c;
        c = d;
        }
      }
    }

  // If no triangle edges matched, then add without subdividing
  if (!foundEdge)
    {
    vtkIdType cellId = polys->InsertNextCell(3);
    polys->InsertCellPoint(triPts[0]);
    polys->InsertCellPoint(triPts[1]);
    polys->InsertCellPoint(triPts[2]);

    if (scalars)
      {
      scalars->InsertTupleValue(cellId, color);
      }
    }
}

// ---------------------------------------------------
// Check the sense of the polygon against the given normal.

int vtkCCSCheckPolygonSense(
  vtkCCSPoly &poly, vtkPoints *points, const double normal[3])
{
  // Compute the normal
  double pnormal[3], p0[3], p1[3], p2[3], v1[3], v2[3], v[3];
  pnormal[0] = 0.0; pnormal[1] = 0.0; pnormal[2] = 0.0;

  points->GetPoint(poly[0], p0);
  points->GetPoint(poly[1], p1);
  v1[0] = p1[0] - p0[0];  v1[1] = p1[1] - p0[1];  v1[2] = p1[2] - p0[2];  

  size_t n = poly.size();
  for (size_t jj = 2; jj < n; jj++)
    {
    points->GetPoint(poly[jj], p2);
    v2[0] = p2[0] - p0[0];  v2[1] = p2[1] - p0[1];  v2[2] = p2[2] - p0[2];  
    vtkMath::Cross(v1, v2, v);
    pnormal[0] += v[0]; pnormal[1] += v[1]; pnormal[2] += v[2];
    p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
    v1[0] = v2[0]; v1[1] = v2[1]; v1[2] = v2[2];
    }

  // Check the normal
  return (vtkMath::Dot(pnormal, normal) >= 0);
}

// ---------------------------------------------------
// Check whether innerPoly is inside outerPoly.
// The normal is needed to verify the polygon orientation.
// The values of pp, bounds, and tol2 must be precomputed
// by calling vtkCCSPrepareForPolyInPoly() on outerPoly.

int vtkCCSPolyInPoly(
  const vtkCCSPoly &outerPoly, const vtkCCSPoly &innerPoly,
  vtkPoints *points, const double normal[3],
  const double *pp, const double bounds[6],
  double tol2)
{
  // Find a vertex of poly "j" that isn't on the edge of poly "i".
  // This is necessary or the PointInPolygon might return "true"
  // based only on roundoff error.

  double p[3];
  int allPointsOnEdges = 1;
  size_t n = outerPoly.size();
  size_t m = innerPoly.size();

  for (size_t jj = 0; jj < m; jj++)
    {          
    points->GetPoint(innerPoly[jj], p);

    int pointOnEdge = 0;
    double q1[3], q2[3];
    points->GetPoint(outerPoly[n-1], q1);
    for (size_t ii = 0; ii < n; ii++)
      {
      points->GetPoint(outerPoly[ii], q2);
      double t, dummy[3];
      // This method returns distance squared
      if (vtkLine::DistanceToLine(p, q1, q2, t, dummy) < tol2)
        {
        pointOnEdge = 1;
        break;
        }
      q1[0] = q2[0]; q1[1] = q2[1]; q1[2] = q2[2];
      }
    if (!pointOnEdge)
      {
      allPointsOnEdges = 0;
      break;
      }
    }

  if (allPointsOnEdges)
    {
    return 1;
    }

  // There could also be a check to see if all the verts match.
  // If they do, both polys could be removed.
   
  return vtkPolygon::PointInPolygon(
    p, static_cast<int>(n), const_cast<double *>(pp),
    const_cast<double *>(bounds), const_cast<double *>(normal));
}

// ---------------------------------------------------
// Precompute values needed for the PolyInPoly check.
// The values that are returned are as follows:
// pp: an array of the polygon vertices
// bounds: the polygon bounds
// tol2: a tolerance value based on the size of the polygon
// (note: pp must be pre-allocated to the 3*outerPoly.size())

void vtkCCSPrepareForPolyInPoly(
  const vtkCCSPoly &outerPoly, vtkPoints *points,
  double *pp, double bounds[6], double &tol2)
{
  size_t n = outerPoly.size();

  if (n == 0)
    {
    return;
    }

  // Find the bounding box for the polygon
  points->GetPoint(outerPoly[0], pp);
  bounds[0] = pp[0]; bounds[1] = pp[0];
  bounds[2] = pp[1]; bounds[3] = pp[1];
  bounds[4] = pp[2]; bounds[5] = pp[2];

  for (size_t k = 1; k < n; k++)
    {
    double *p = &pp[3*k];
    points->GetPoint(outerPoly[k], p);

    if (p[0] < bounds[0]) { bounds[0] = p[0]; }
    if (p[0] > bounds[1]) { bounds[1] = p[0]; }
    if (p[1] < bounds[2]) { bounds[2] = p[1]; }
    if (p[1] > bounds[3]) { bounds[3] = p[1]; }
    if (p[2] < bounds[4]) { bounds[4] = p[2]; }
    if (p[2] > bounds[5]) { bounds[5] = p[2]; }
    }

  // Compute a tolerance based on the poly size
  double ps[3];
  ps[0] = bounds[1] - bounds[0];
  ps[1] = bounds[3] - bounds[2];
  ps[2] = bounds[5] - bounds[4];

  // Tolerance is for squared distance
  tol2 = (ps[0]*ps[0] + ps[1]*ps[1] + ps[2]*ps[2])*(1e-5 * 1e-5);
}

// ---------------------------------------------------
// Check for polygons within polygons.  Group the polygons
// if they are within each other.  Reverse the sense of 
// the interior "hole" polygons.  A hole within a hole
// will be reversed twice and will become its own group.

void vtkCCSMakeHoleyPolys(
  vtkstd::vector<vtkCCSPoly> &newPolys, vtkPoints *points,
  vtkstd::vector<vtkCCSPolyGroup> &polyGroups,
  const double normal[3])
{
  size_t numNewPolys = newPolys.size();
  if (numNewPolys <= 1)
    {
    return;
    }

  // Use bit arrays to keep track of inner polys
  vtkCCSBitArray polyReversed;
  vtkCCSBitArray innerPolys;

  // Find the maximum poly size
  size_t nmax = 1;
  for (size_t kk = 0; kk < numNewPolys; kk++)
    {
    size_t n = newPolys[kk].size();
    if (n > nmax) { nmax = n; }
    }

  // These are some values needed for poly-in-poly checks
  double *pp = new double[3*nmax];
  double bounds[6];
  double tol2;

  // Go through all polys
  for (size_t i = 0; i < numNewPolys; i++)
    {
    size_t n = newPolys[i].size();

    if (n < 3) { continue; }

    // Check if poly is reversed
    polyReversed.set(i,
      vtkCCSCheckPolygonSense(newPolys[i], points, normal));

    // Precompute some values needed for poly-in-poly checks
    vtkCCSPrepareForPolyInPoly(newPolys[i], points, pp, bounds, tol2);

    // Look for polygons inside of this one
    for (size_t j = 0; j < numNewPolys; j++)
      {
      size_t m = newPolys[j].size();
      if (j == i || m < 3) { continue; }

      // Make sure polygon i is not in polygon j
      int isInteriorPoly = 0;
      for (size_t k = 1; k < polyGroups[j].size(); k++)
        {
        if (polyGroups[j][k] == i)
          {
          isInteriorPoly = 1;
          break;
          }
        }

      if (isInteriorPoly)
        {
        continue;
        }

      if (vtkCCSPolyInPoly(newPolys[i], newPolys[j], points,
                           normal, pp, bounds, tol2))
        {
        // Add to group
        polyGroups[i].push_back(j);
        }
      }
    }

  delete [] pp;

  for (size_t j = 0; j < numNewPolys; j++)
    {
    // Remove the groups for reversed polys
    if (polyReversed.get(j))
      {
      polyGroups[j].clear();
      }
    // Polys inside the interior polys have their own groups, so remove
    // them from this group
    else if (polyGroups[j].size() > 1)
      {
      // Convert the group into a bit array, to make manipulation easier
      innerPolys.clear();
      for (size_t k = 1; k < polyGroups[j].size(); k++)
        {
        innerPolys.set(polyGroups[j][k], 1);
        }

      // Look for non-reversed polys inside this one
      for (size_t kk = 1; kk < polyGroups[j].size(); kk++)
        {
        // jj is the index of the inner poly
        size_t jj = polyGroups[j][kk];
        // If inner poly is not reversed then
        if (!polyReversed.get(jj))
          {
          // Remove that poly and all polys inside of it from the group
          for (size_t ii = 0; ii < polyGroups[jj].size(); ii++)
            {
            innerPolys.set(polyGroups[jj][ii], 0);
            }
          }
        }

      // Use the bit array to recreate the polyGroup
      polyGroups[j].clear();
      polyGroups[j].push_back(j);
      for (size_t jj = 0; jj < numNewPolys; jj++)
        {
        if (innerPolys.get(jj) != 0)
          {
          polyGroups[j].push_back(jj);
          }
        }
      }
    }
}

// ---------------------------------------------------
// Check line segment with point Ids (i, j) to make sure that it
// doesn't cut through the edges of any polys in the group.
// Return value of zero means check failed and the cut is not
// usable.

int vtkCCSCheckCut(
  vtkstd::vector<vtkCCSPoly> &polys, vtkPoints *points,
  vtkCCSPolyGroup &polyGroup, vtkIdType ptId1, vtkIdType ptId2)
{
  double p1[3], p2[3];

  points->GetPoint(ptId1, p1);
  points->GetPoint(ptId2, p2);

  for (size_t i = 0; i < polyGroup.size(); i++)
    {
    vtkCCSPoly &poly = polys[polyGroup[i]];
    size_t n = poly.size();

    double q1[3], q2[3];
    vtkIdType qtId1 = poly[n-1];
    points->GetPoint(qtId1, q1);

    for (size_t j = 0; j < n; j++)
      {
      vtkIdType qtId2 = poly[j];
      points->GetPoint(qtId2, q2);

      // If lines share an endpoint, they can't intersect,
      // so don't bother with the check.
      if (ptId1 != qtId1 && ptId1 != qtId2 &&
          ptId2 != qtId1 && ptId2 != qtId2)
        {
        double u, v;
        if (vtkLine::Intersection(p1, p2, q1, q2, u, v))
          {
          return 0;
          }
        }

      qtId1 = qtId2;
      q1[0] = q2[0]; q1[1] = q2[1]; q1[2] = q2[2];
      }
    }

  return 1;
}

// ---------------------------------------------------
// Check the quality of a cut between an outer and inner polygon.
// Larger values mean that the cut will produce triangles with
// sharp angles.  The range of values is [-1, 1], where the smallest
// values indicate the highest quality.

double vtkCCSCutQuality(
  vtkCCSPoly &outerPoly, vtkCCSPoly &innerPoly,
  size_t i, size_t j, vtkPoints *points)
{
  size_t n = outerPoly.size();
  size_t m = innerPoly.size();

  size_t a = ((i > 0) ? i-1 : n-1);
  size_t b = ((i < n-1) ? i+1 : 0);

  size_t c = ((j > 0) ? j-1 : m-1);
  size_t d = ((j < m-1) ? j+1 : 0);

  double p0[3], p1[3], p2[3];
  points->GetPoint(outerPoly[i], p1);
  points->GetPoint(innerPoly[j], p2);

  double v1[3], v2[3];
  v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];

  double l1 = sqrt(vtkMath::Dot(v1, v1));
  double l2;
  double qmax = -l1;
  double q;

  points->GetPoint(outerPoly[a], p0);
  v2[0] = p0[0] - p1[0]; v2[1] = p0[1] - p1[1]; v2[2] = p0[2] - p1[2];
  l2 = sqrt(vtkMath::Dot(v2, v2));
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2)/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(outerPoly[b], p0);
  v2[0] = p0[0] - p1[0]; v2[1] = p0[1] - p1[1]; v2[2] = p0[2] - p1[2];
  l2 = sqrt(vtkMath::Dot(v2, v2));
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2)/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(innerPoly[c], p0);
  v2[0] = p0[0] - p2[0]; v2[1] = p0[1] - p2[1]; v2[2] = p0[2] - p2[2];
  l2 = sqrt(vtkMath::Dot(v2, v2));
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2)/l2;
    if (q > qmax) { qmax = q; }
    }

  points->GetPoint(innerPoly[d], p0);
  v2[0] = p0[0] - p2[0]; v2[1] = p0[1] - p2[1]; v2[2] = p0[2] - p2[2];
  l2 = sqrt(vtkMath::Dot(v2, v2));
  if (l2 > 0)
    {
    q = vtkMath::Dot(v1, v2)/l2;
    if (q > qmax) { qmax = q; }
    }

  if (l1 > 0)
    {
    return qmax/l1;
    }

  return 1.0;
}

// ---------------------------------------------------
// After the holes have been identified, make cuts between the
// outer poly and each hole.  Make two cuts per hole.  The only
// strict requirement is that the cut must not intersect any
// edges, but it's best to make sure that no really sharp angles
// are created.

void vtkCCSCutHoleyPolys(
  vtkstd::vector<vtkCCSPoly> &polys, vtkPoints *points,
  vtkstd::vector<vtkCCSPolyGroup> &polyGroups, const double normal[3])
{
  // Go through all groups and cut out the first inner poly that is
  // found.  Every time an inner poly is cut out, the groupId counter
  // is reset because a cutting a poly creates a new group.
  size_t groupId = 0;
  while (groupId < polyGroups.size())
    {
    vtkCCSPolyGroup &polyGroup = polyGroups[groupId];

    // Only need to make a cut if the group size is greater than 1
    if (polyGroup.size() > 1)
      {
      // The first member of the group is the outer poly
      size_t outerPolyId = polyGroup[0];
      vtkCCSPoly &outerPoly = polys[outerPolyId];

      // The second member of the group is the first inner poly
      size_t innerPolyId = polyGroup[1];
      vtkCCSPoly &innerPoly = polys[innerPolyId];

      // Search for potential cuts (need to find two cuts)
      int cutId = 0;
      size_t cuts[2][2];
      cuts[0][0] = cuts[0][1] = 0;
      cuts[1][0] = cuts[1][1] = 0;

      for (size_t j = 0; j < innerPoly.size() && cutId < 2; j++)
        {
        double bestq = 1.0;
        cuts[cutId][0] = 0;
        cuts[cutId][1] = j; 

        for (size_t k = 0; k < outerPoly.size(); k++)
          {
          // If this is the second cut, do extra checks
          if (cutId > 0)
            {
            // Make sure cuts don't share an endpoint
            if (k == cuts[0][0])
              {
              continue;
              }

            // Make sure cuts don't intersect
            double p1[3], p2[3];
            points->GetPoint(outerPoly[cuts[0][0]], p1);
            points->GetPoint(innerPoly[cuts[0][1]], p2);

            double q1[3], q2[3];
            points->GetPoint(outerPoly[k], q1);
            points->GetPoint(innerPoly[j], q2);

            double u, v;
            if (vtkLine::Intersection(p1, p2, q1, q2, u, v))
              {
              continue;
              }
            }

          // These checks are done for both cuts
          if (vtkCCSCheckCut(polys, points, polyGroup,
                             outerPoly[k], innerPoly[j]))
            {
            // Look for the cut that produces the least-sharp triangles
            double q = vtkCCSCutQuality(outerPoly, innerPoly, k, j, points);
            if (q < bestq)
              {
              cuts[cutId][0] = k;
              bestq = q;
              }
            }
          }

        // If a suitable cut was found, do the next cut
        if (bestq < 1.0)
          {
          cutId++; 
          }
        }

      // Make sure that two good cuts were made
      if (cutId < 2)
        {
        // This error should never be generated
        vtkGenericWarningMacro("Triangulation failure for complex polygon.");
        }

      // Generate new polys from the cuts
      size_t n = outerPoly.size();
      size_t m = innerPoly.size();
      size_t idx;

      // Generate poly1
      vtkCCSPoly poly1;
      for (idx = cuts[0][0];;)
        {
        poly1.push_back(outerPoly[idx]);
        if (idx == cuts[1][0]) { break; }
        if (++idx >= n) { idx = 0; }
        }
      for (idx = cuts[1][1];;)
        {
        poly1.push_back(innerPoly[idx]);
        if (idx == cuts[0][1]) { break; }
        if (++idx >= m) { idx = 0; }
        }

      // Generate poly2
      vtkCCSPoly poly2;
      for (idx = cuts[1][0];;)
        {
        poly2.push_back(outerPoly[idx]);
        if (idx == cuts[0][0]) { break; }
        if (++idx >= n) { idx = 0; }
        }
      for (idx = cuts[0][1];;)
        {
        poly2.push_back(innerPoly[idx]);
        if (idx == cuts[1][1]) { break; }
        if (++idx >= m) { idx = 0; }
        }

      // Replace outerPoly and innerPoly with these new polys
      polys[outerPolyId] = poly1;
      polys[innerPolyId] = poly2;

      // Move innerPolyId into its own group
      polyGroup.erase(polyGroup.begin()+1);
      polyGroups[innerPolyId].push_back(innerPolyId);

      // If there are other interior polys in the group, find out whether
      // they are in poly1 or poly2
      if (polyGroup.size() > 1)
        {
        double *pp = new double[3*poly1.size()];
        double bounds[6];
        double tol2;
        vtkCCSPrepareForPolyInPoly(poly1, points, pp, bounds, tol2);

        size_t ii = 1;
        while (ii < polyGroup.size())
          {
          if (vtkCCSPolyInPoly(poly1, polys[polyGroup[ii]],
                               points, normal, pp, bounds, tol2))
            {
            // Keep this poly in polyGroup
            ii++;
            }
          else
            {
            // Move this poly to poly2 group
            polyGroups[innerPolyId].push_back(polyGroup[ii]);
            polyGroup.erase(polyGroup.begin()+ii);

            // Reduce the groupId to ensure that this new group
            // will get cut
            if (innerPolyId < groupId)
              {
              groupId = innerPolyId;
              }
            }
          }
        delete [] pp;

        // Continue without incrementing groupId
        continue;
        }
      }

    // Increment to the next group
    groupId++;
    }
}
