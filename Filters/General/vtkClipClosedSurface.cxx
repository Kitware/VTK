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
#include "vtkSignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkPlaneCollection.h"
#include "vtkMath.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkLine.h"
#include "vtkMatrix4x4.h"
#include "vtkContourTriangulator.h"

#include "vtkIncrementalOctreePointLocator.h"

#include <vector>
#include <algorithm>
#include <map>
#include <utility>

vtkStandardNewMacro(vtkClipClosedSurface);

vtkCxxSetObjectMacro(vtkClipClosedSurface,ClippingPlanes,vtkPlaneCollection);

//----------------------------------------------------------------------------
vtkClipClosedSurface::vtkClipClosedSurface()
{
  this->ClippingPlanes = 0;
  this->Tolerance = 1e-6;
  this->PassPointData = 0;

  this->ScalarMode = VTK_CCS_SCALAR_MODE_NONE;
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

  this->TriangulationErrorDisplay = 0;

  // A whole bunch of objects needed during execution
  this->IdList = 0;
}

//----------------------------------------------------------------------------
vtkClipClosedSurface::~vtkClipClosedSurface()
{
  if (this->ClippingPlanes) { this->ClippingPlanes->Delete(); }

  if (this->IdList) { this->IdList->Delete(); }
}

//----------------------------------------------------------------------------
const char *vtkClipClosedSurface::GetScalarModeAsString()
{
  switch (this->ScalarMode)
  {
    case VTK_CCS_SCALAR_MODE_NONE:
      return "None";
    case VTK_CCS_SCALAR_MODE_COLORS:
      return "Colors";
    case VTK_CCS_SCALAR_MODE_LABELS:
      return "Labels";
  }
  return "";
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

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "PassPointData: "
     << (this->PassPointData ? "On\n" : "Off\n" );

  os << indent << "GenerateOutline: "
     << (this->GenerateOutline ? "On\n" : "Off\n" );

  os << indent << "GenerateFaces: "
     << (this->GenerateFaces ? "On\n" : "Off\n" );

  os << indent << "ScalarMode: "
     << this->GetScalarModeAsString() << "\n";

  os << indent << "BaseColor: " << this->BaseColor[0] << ", "
     << this->BaseColor[1] << ", " << this->BaseColor[2] << "\n";

  os << indent << "ClipColor: " << this->ClipColor[0] << ", "
     << this->ClipColor[1] << ", " << this->ClipColor[2] << "\n";

  os << indent << "ActivePlaneId: " << this->ActivePlaneId << "\n";

  os << indent << "ActivePlaneColor: " << this->ActivePlaneColor[0] << ", "
     << this->ActivePlaneColor[1] << ", " << this->ActivePlaneColor[2] << "\n";

  os << indent << "TriangulationErrorDisplay: "
     << (this->TriangulationErrorDisplay ? "On\n" : "Off\n" );
}

//----------------------------------------------------------------------------
int vtkClipClosedSurface::ComputePipelineMTime(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector),
  int vtkNotUsed(requestFromOutputPort),
  vtkMTimeType* mtime)
{
  vtkMTimeType mTime = this->GetMTime();

  vtkPlaneCollection *planes = this->ClippingPlanes;
  vtkPlane *plane = 0;

  if (planes)
  {
    vtkMTimeType planesMTime = planes->GetMTime();
    if (planesMTime > mTime)
    {
      mTime = planesMTime;
    }

    vtkCollectionSimpleIterator iter;
    planes->InitTraversal(iter);
    while ( (plane = planes->GetNextPlane(iter)) )
    {
      vtkMTimeType planeMTime = plane->GetMTime();
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
// A helper class to quickly locate an edge, given the endpoint ids.
// It uses an stl map rather than a table partitioning scheme, since
// we have no idea how many entries there will be when we start.  So
// the performance is approximately log(n).

class vtkCCSEdgeLocatorNode
{
public:
  vtkCCSEdgeLocatorNode() :
    ptId0(-1), ptId1(-1), edgeId(-1), next(0) {};

  void FreeList() {
    vtkCCSEdgeLocatorNode *ptr = this->next;
    while (ptr)
    {
      vtkCCSEdgeLocatorNode *tmp = ptr;
      ptr = ptr->next;
      tmp->next = 0;
      delete tmp;
    }
  };

  vtkIdType ptId0;
  vtkIdType ptId1;
  vtkIdType edgeId;
  vtkCCSEdgeLocatorNode *next;
};


class vtkCCSEdgeLocator
{
private:
  typedef std::map<vtkIdType, vtkCCSEdgeLocatorNode> MapType;
  MapType EdgeMap;

public:
  static vtkCCSEdgeLocator *New() {
    return new vtkCCSEdgeLocator; };

  void Delete() {
    this->Initialize();
    delete this; };

  // Description:
  // Initialize the locator.
  void Initialize();

  // Description:
  // If edge (i0, i1) is not in the list, then it will be added and
  // a pointer for storing the new edgeId will be returned.
  // If edge (i0, i1) is in the list, then edgeId will be set to the
  // stored value and a null pointer will be returned.
  vtkIdType *InsertUniqueEdge(vtkIdType i0, vtkIdType i1, vtkIdType &edgeId);
};

void vtkCCSEdgeLocator::Initialize()
{
  for (MapType::iterator i = this->EdgeMap.begin();
       i != this->EdgeMap.end();
       ++i)
  {
    i->second.FreeList();
  }
  this->EdgeMap.clear();
}

vtkIdType *vtkCCSEdgeLocator::InsertUniqueEdge(
  vtkIdType i0, vtkIdType i1, vtkIdType &edgeId)
{
  // Ensure consistent ordering of edge
  if (i1 < i0)
  {
    vtkIdType tmp = i0;
    i0 = i1;
    i1 = tmp;
  }

  // Generate a integer key, try to make it unique
#ifdef VTK_USE_64BIT_IDS
  vtkIdType key = ((i1 << 32) ^ i0);
#else
  vtkIdType key = ((i1 << 16) ^ i0);
#endif

  vtkCCSEdgeLocatorNode *node = &this->EdgeMap[key];

  if (node->ptId1 < 0)
  {
    // Didn't find key, so add a new edge entry
    node->ptId0 = i0;
    node->ptId1 = i1;
    return &node->edgeId;
  }

  // Search through the list for i0 and i1
  if (node->ptId0 == i0 && node->ptId1 == i1)
  {
    edgeId = node->edgeId;
    return 0;
  }

  int i = 1;
  while (node->next != 0)
  {
    i++;
    node = node->next;

    if (node->ptId0 == i0 && node->ptId1 == i1)
    {
      edgeId = node->edgeId;
      return 0;
    }
  }

  // No entry for i1, so make one and return
  node->next = new vtkCCSEdgeLocatorNode;
  node = node->next;
  node->ptId0 = i0;
  node->ptId1 = i1;
  node->edgeId = static_cast<vtkIdType>(this->EdgeMap.size()-1);
  return &node->edgeId;
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

  // Create objects needed for temporary storage
  if (this->IdList == 0) { this->IdList = vtkIdList::New(); }

  // Get the input points
  vtkPoints *inputPoints = input->GetPoints();
  vtkIdType numPts = 0;
  int inputPointsType = VTK_FLOAT;
  if (inputPoints)
  {
    numPts = inputPoints->GetNumberOfPoints();
    inputPointsType = inputPoints->GetDataType();
  }

  // Force points to double precision, copy the point attributes
  vtkPoints *points = vtkPoints::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPts);

  vtkPointData *pointData = vtkPointData::New();
  vtkPointData *inPointData = 0;

  if (this->PassPointData)
  {
    inPointData = input->GetPointData();
    pointData->InterpolateAllocate(inPointData, numPts, 0);
  }

  for (vtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    double point[3];
    inputPoints->GetPoint(ptId, point);
    points->SetPoint(ptId, point);
    // Point data is not copied from input
    if (inPointData)
    {
      pointData->CopyData(inPointData, ptId, ptId);
    }
  }

  // An edge locator to avoid point duplication while clipping
  vtkCCSEdgeLocator *edgeLocator = vtkCCSEdgeLocator::New();

  // A temporary polydata for the contour lines that are triangulated
  vtkPolyData *tmpContourData = vtkPolyData::New();

  // The cell scalars
  vtkUnsignedCharArray *lineScalars = 0;
  vtkUnsignedCharArray *polyScalars = 0;
  vtkUnsignedCharArray *inputScalars = 0;

  // For input scalars: the offsets to the various cell types
  vtkIdType firstLineScalar = 0;
  vtkIdType firstPolyScalar = 0;
  vtkIdType firstStripScalar = 0;

  // Make the colors to be used on the data.
  int numberOfScalarComponents = 1;
  unsigned char colors[3][3];

  if (this->ScalarMode == VTK_CCS_SCALAR_MODE_COLORS)
  {
    numberOfScalarComponents = 3;
    this->CreateColorValues(this->BaseColor, this->ClipColor,
                            this->ActivePlaneColor, colors);
  }
  else if (this->ScalarMode == VTK_CCS_SCALAR_MODE_LABELS)
  {
    colors[0][0] = 0;
    colors[1][0] = 1;
    colors[2][0] = 2;
  }

  // This is set if we have to work with scalars.  The input scalars
  // will be copied if they are unsigned char with 3 components, otherwise
  // new scalars will be generated.
  if (this->ScalarMode)
  {
    // Make the scalars
    lineScalars = vtkUnsignedCharArray::New();
    lineScalars->SetNumberOfComponents(numberOfScalarComponents);

    vtkDataArray *tryInputScalars = input->GetCellData()->GetScalars();
    // Get input scalars if they are RGB color scalars
    if (tryInputScalars && tryInputScalars->IsA("vtkUnsignedCharArray") &&
        numberOfScalarComponents == 3 &&
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
      polyScalars->SetNumberOfComponents(numberOfScalarComponents);
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

  // The point scalars, needed for clipping (not for the output!)
  vtkDoubleArray *pointScalars = vtkDoubleArray::New();

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
    pointScalars->SetNumberOfValues(numPoints);
    for (vtkIdType pointId = 0; pointId < numPoints; pointId++)
    {
      double p[3];
      points->GetPoint(pointId, p);
      double val = p[0]*pc[0] + p[1]*pc[1] + p[2]*pc[2] + pc[3];
      pointScalars->SetValue(pointId, val);
    }

    // Prepare the output scalars
    outLineData->CopyAllocate(inLineData, 0, 0);
    outPolyData->CopyAllocate(inPolyData, 0, 0);

    // Reset the locator
    edgeLocator->Initialize();

    // Clip the lines
    this->ClipLines(points, pointScalars, pointData, edgeLocator,
                    lines, newLines, inLineData, outLineData);

    // Clip the polys
    if (polys)
    {
      // Get the number of lines remaining after the clipping
      vtkIdType numClipLines = newLines->GetNumberOfCells();

      // Cut the polys to generate more lines
      this->ClipAndContourPolys(points, pointScalars, pointData, edgeLocator,
                                triangulate, polys, newPolys, newLines,
                                inPolyData, outPolyData, outLineData);

      // Add scalars for the newly-created contour lines
      vtkUnsignedCharArray *scalars =
        vtkArrayDownCast<vtkUnsignedCharArray>(outLineData->GetScalars());

      if (scalars)
      {
        // Set the color to the active color if plane is active
        unsigned char *color = colors[1+active];
        unsigned char *activeColor = colors[2];

        vtkIdType numLines = newLines->GetNumberOfCells();
        for (vtkIdType lineId = numClipLines; lineId < numLines; lineId++)
        {
          unsigned char oldColor[3];
          scalars->GetTypedTuple(lineId, oldColor);
          if (numberOfScalarComponents != 3 ||
              oldColor[0] != activeColor[0] ||
              oldColor[1] != activeColor[1] ||
              oldColor[2] != activeColor[2])
          {
            scalars->SetTypedTuple(lineId, color);
          }
        }
      }

      // Generate new polys from the cut lines
      vtkIdType cellId = newPolys->GetNumberOfCells();
      vtkIdType numClipAndContourLines = newLines->GetNumberOfCells();

      // Create a polydata for the lines
      tmpContourData->SetPoints(points);
      tmpContourData->SetLines(newLines);
      tmpContourData->BuildCells();

      this->TriangulateContours(tmpContourData, numClipLines,
                                  numClipAndContourLines - numClipLines,
                                  newPolys, pc);

      // Add scalars for the newly-created polys
      scalars = vtkArrayDownCast<vtkUnsignedCharArray>(outPolyData->GetScalars());

      if (scalars)
      {
        unsigned char *color = colors[1+active];

        vtkIdType numCells = newPolys->GetNumberOfCells();
        if (numCells > cellId)
        {
          // The insert allocates space up to numCells-1
          scalars->InsertTypedTuple(numCells-1, color);
          for (;cellId < numCells; cellId++)
          {
            scalars->SetTypedTuple(cellId, color);
          }
        }
      }

      // Add scalars to any diagnostic lines that added by
      // TriangulateContours().  In usual operation, no lines are added.
      scalars = vtkArrayDownCast<vtkUnsignedCharArray>(outLineData->GetScalars());

      if (scalars)
      {
        unsigned char color[3] = { 0, 255, 255 };

        vtkIdType numCells = newLines->GetNumberOfCells();
        if (numCells > numClipAndContourLines)
        {
          // The insert allocates space up to numCells-1
          scalars->InsertTypedTuple(numCells-1, color);
          for (vtkIdType lineCellId = numClipAndContourLines;
               lineCellId < numCells; lineCellId++)
          {
            scalars->SetTypedTuple(lineCellId, color);
          }
        }
      }
    }

    // Swap the lines, points, etcetera: old output becomes new input
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

    vtkCellData *tmp4 = inLineData;
    inLineData = outLineData;
    outLineData = tmp4;
    outLineData->Initialize();

    vtkCellData *tmp5 = inPolyData;
    inPolyData = outPolyData;
    outPolyData = tmp5;
    outPolyData->Initialize();
  }

  // Delete the locator
  edgeLocator->Delete();

  // Delete the contour data container
  tmpContourData->Delete();

  // Delete the clip scalars
  pointScalars->Delete();

  // Get the line scalars
  vtkUnsignedCharArray *scalars =
    vtkArrayDownCast<vtkUnsignedCharArray>(inLineData->GetScalars());

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
        vtkArrayDownCast<vtkUnsignedCharArray>(inPolyData->GetScalars());

      vtkIdType m = scalars->GetNumberOfTuples();
      vtkIdType n = pScalars->GetNumberOfTuples();

      if (n > 0)
      {
        unsigned char color[3];
        color[0] = color[1] = color[2] = 0;

        // This is just to expand the array
        scalars->InsertTypedTuple(n+m-1, color);

        // Fill in the poly scalars
        for (vtkIdType i = 0; i < n; i++)
        {
          pScalars->GetTypedTuple(i, color);
          scalars->SetTypedTuple(i+m, color);
        }
      }
    }
  }

  lines->Delete();

  if (polys)
  {
    polys->Delete();
  }

  if (this->ScalarMode == VTK_CCS_SCALAR_MODE_COLORS)
  {
    scalars->SetName("Colors");
    output->GetCellData()->SetScalars(scalars);
  }
  else if (this->ScalarMode == VTK_CCS_SCALAR_MODE_LABELS)
  {
    // Don't use VTK_UNSIGNED_CHAR or they will look like color scalars
    vtkSignedCharArray *categories = vtkSignedCharArray::New();
    categories->DeepCopy(scalars);
    categories->SetName("Labels");
    output->GetCellData()->SetScalars(categories);
    categories->Delete();
  }
  else
  {
    output->GetCellData()->SetScalars(0);
  }

  newLines->Delete();
  if (newPolys)
  {
    newPolys->Delete();
  }

  inLineData->Delete();
  outLineData->Delete();
  inPolyData->Delete();
  outPolyData->Delete();

  // Finally, store the points in the output
  this->SqueezeOutputPoints(output, points, pointData, inputPointsType);
  output->Squeeze();

  points->Delete();
  pointData->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::SqueezeOutputPoints(
    vtkPolyData *output, vtkPoints *points, vtkPointData *pointData,
    int outputPointDataType)
{
  // Create a list of points used by cells
  vtkIdType n = points->GetNumberOfPoints();
  vtkIdType numNewPoints = 0;

  // The point data
  vtkPointData *outPointData = output->GetPointData();

  // A mapping from old pointIds to new pointIds
  vtkIdType *pointMap = new vtkIdType[n];
  for (vtkIdType i = 0; i < n; i++)
  {
    pointMap[i] = -1;
  }

  vtkIdType npts, *pts;
  vtkCellArray *cellArrays[4];
  cellArrays[0] = output->GetVerts();
  cellArrays[1] = output->GetLines();
  cellArrays[2] = output->GetPolys();
  cellArrays[3] = output->GetStrips();
  int arrayId;

  // Find all the newPoints that are used by cells
  for (arrayId = 0; arrayId < 4; arrayId++)
  {
    vtkCellArray *cellArray = cellArrays[arrayId];
    if (cellArray)
    {
      cellArray->InitTraversal();
      while (cellArray->GetNextCell(npts, pts))
      {
        for (vtkIdType ii = 0; ii < npts; ii++)
        {
          vtkIdType pointId = pts[ii];
          if (pointMap[pointId] < 0)
          {
            pointMap[pointId] = numNewPoints++;
          }
        }
      }
    }
  }

  // Create exactly the number of points that are required
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetDataType(outputPointDataType);
  newPoints->SetNumberOfPoints(numNewPoints);
  outPointData->CopyAllocate(pointData, numNewPoints, 0);

  for (vtkIdType pointId = 0; pointId < n; pointId++)
  {
    vtkIdType newPointId = pointMap[pointId];
    if (newPointId >= 0)
    {
      double p[3];
      points->GetPoint(pointId, p);
      newPoints->SetPoint(newPointId, p);
      outPointData->CopyData(pointData, pointId, newPointId);
    }
  }

  // Change the cell pointIds to reflect the new point array
  for (arrayId = 0; arrayId < 4; arrayId++)
  {
    vtkCellArray *cellArray = cellArrays[arrayId];
    if (cellArray)
    {
      cellArray->InitTraversal();
      while (cellArray->GetNextCell(npts, pts))
      {
        for (vtkIdType ii = 0; ii < npts; ii++)
        {
          vtkIdType pointId = pts[ii];
          pts[ii] = pointMap[pointId];
        }
      }
    }
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  delete [] pointMap;
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
int vtkClipClosedSurface::InterpolateEdge(
  vtkPoints *points, vtkPointData *pointData, vtkCCSEdgeLocator *locator,
  double tol, vtkIdType i0, vtkIdType i1, double v0, double v1,
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

  // After the above swap, i0 will be kept, and i1 will be clipped

  // Check to see if this point has already been computed
  vtkIdType *iptr = locator->InsertUniqueEdge(i0, i1, i);
  if (iptr == 0)
  {
    return 0;
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

  double tol2 = tol*tol;

  // Make sure that new point is far enough from kept point
  if (vtkMath::Distance2BetweenPoints(p, p0) < tol2)
  {
    i = i0;
    *iptr = i0;
    return 0;
  }

  if (vtkMath::Distance2BetweenPoints(p, p1) < tol2)
  {
    i = i1;
    *iptr = i1;
    return 0;
  }

  i = points->InsertNextPoint(p);
  pointData->InterpolateEdge(pointData, i, i0, i1, t);

  // Store the new index in the locator
  *iptr = i;

  return 1;
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::ClipLines(
  vtkPoints *points, vtkDoubleArray *pointScalars,
  vtkPointData *pointData, vtkCCSEdgeLocator *edgeLocator,
  vtkCellArray *inputCells, vtkCellArray *outputLines,
  vtkCellData *inCellData, vtkCellData *outLineData)
{
  vtkIdType numCells = inputCells->GetNumberOfCells();

  inputCells->InitTraversal();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    vtkIdType numPts = 0;
    vtkIdType *pts = 0;
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
        vtkIdType linePts[2];
        linePts[0] = i0;
        linePts[1] = i1;

        // If only one end was clipped, interpolate new point
        if ( (c0 ^ c1) )
        {
          vtkClipClosedSurface::InterpolateEdge(
            points, pointData, edgeLocator, this->Tolerance,
            i0, i1, v0, v1, linePts[c0]);
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
  vtkPoints *points, vtkDoubleArray *pointScalars, vtkPointData *pointData,
  vtkCCSEdgeLocator *edgeLocator, int triangulate,
  vtkCellArray *inputCells,
  vtkCellArray *outputPolys, vtkCellArray *outputLines,
  vtkCellData *inCellData,
  vtkCellData *outPolyData, vtkCellData *outLineData)
{
  vtkIdList *idList = this->IdList;

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

  int triangulationFailure = 0;

  // Go through all cells and clip them.
  vtkIdType numCells = inputCells->GetNumberOfCells();

  inputCells->InitTraversal();
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    vtkIdType numPts = 0;
    vtkIdType *pts = 0;
    inputCells->GetNextCell(numPts, pts);
    idList->Reset();

    vtkIdType i1 = pts[numPts-1];
    double v1 = pointScalars->GetValue(i1);
    int c1 = (v1 > 0);

    // The ids for the current edge: init j0 to -1 if i1 will be clipped
    vtkIdType j0 = (c1 ? i1 : -1);
    vtkIdType j1 = 0;

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

      // Generate new point info
      i1 = pts[i];
      v1 = pointScalars->GetValue(i1);
      c1 = (v1 > 0);

      // If at least one edge end point wasn't clipped
      if ( (c0 | c1) )
      {
        // If only one end was clipped, interpolate new point
        if ( (c0 ^ c1) )
        {
          vtkClipClosedSurface::InterpolateEdge(
            points, pointData, edgeLocator, this->Tolerance,
            i0, i1, v0, v1, j1);

          if (j1 != j0)
          {
            idList->InsertNextId(j1);
            j0 = j1;
          }

          // Save as one end of the contour line
          linePts[c0] = j1;
        }

        if (c1)
        {
          j1 = i1;

          if (j1 != j0)
          {
            idList->InsertNextId(j1);
            j0 = j1;
          }
        }
      }
    }

    // Insert the clipped poly
    vtkIdType numPoints = idList->GetNumberOfIds();

    if (numPoints > polyMax)
    {
      vtkIdType newCellId = outputPolys->GetNumberOfCells();

      // Triangulate the poly and insert triangles into output.
      if (!this->TriangulatePolygon(idList, points, outputPolys))
      {
        triangulationFailure = 1;
      }

      // Copy the attribute data to the triangle cells
      vtkIdType nCells = outputPolys->GetNumberOfCells();
      for (; newCellId < nCells; newCellId++)
      {
        outPolyData->CopyData(inCellData, cellId, newCellId);
      }
    }
    else if (numPoints > 2)
    {
      // Insert the polygon without triangulating it
      vtkIdType newCellId = outputPolys->InsertNextCell(idList);
      outPolyData->CopyData(inCellData, cellId, newCellId);
    }

    // Insert the contour line if one was created
    if (linePts[0] != linePts[1])
    {
      vtkIdType newCellId = outputLines->InsertNextCell(2, linePts);
      outLineData->CopyData(inCellData, cellId, newCellId);
    }
  }

  if (triangulationFailure && this->TriangulationErrorDisplay)
  {
    vtkErrorMacro("Triangulation failed, output may not be watertight");
  }

  // Free up the idList memory
  idList->Initialize();
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
      inputScalars->GetTypedTuple(firstLineScalar + cellId++, cellColor);
    }

    for (vtkIdType i = 1; i < npts; i++)
    {
      lines->InsertNextCell(2);
      lines->InsertCellPoint(pts[i-1]);
      lines->InsertCellPoint(pts[i]);

      if (scalars)
      {
        scalars->InsertNextTypedTuple(cellColor);
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
        inputScalars->GetTypedTuple(i + firstPolyScalar, scalarValue);
        polyScalars->SetTypedTuple(i, scalarValue);
      }
    }
    else
    {
      for (vtkIdType i = 0; i < n; i++)
      {
        polyScalars->SetTypedTuple(i, scalarValue);
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
        inputScalars->GetTypedTuple(cellId, scalarValue);
      }

      vtkIdType n = npts - 3;
      vtkIdType m = polyScalars->GetNumberOfTuples();
      if (n >= 0)
      {
        // First insert is just to allocate space
        polyScalars->InsertTypedTuple(m+n, scalarValue);

        for (vtkIdType i = 0; i < n; i++)
        {
          polyScalars->SetTypedTuple(m+i, scalarValue);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkClipClosedSurface::TriangulateContours(
  vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
  vtkCellArray *outputPolys, const double normal[3])
{
  // If no cut lines were generated, there's nothing to do
  if (numLines <= 0)
  {
    return;
  }

  double nnormal[3] = { -normal[0], -normal[1], -normal[2] };
  int rval = vtkContourTriangulator::TriangulateContours(
               data, firstLine, numLines, outputPolys, nnormal);

  if (rval == 0 && this->TriangulationErrorDisplay)
  {
    vtkErrorMacro("Triangulation failed, data may not be watertight.");
  }
}

// ---------------------------------------------------
int vtkClipClosedSurface::TriangulatePolygon(
  vtkIdList *polygon, vtkPoints *points, vtkCellArray *triangles)
{
  return vtkContourTriangulator::TriangulatePolygon(
           polygon, points, triangles);
}
