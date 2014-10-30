/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricClustering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadricClustering.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkFeatureEdges.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTimerLog.h"
#include "vtkTriangle.h"
#include <vtksys/hash_set.hxx> // keep track of inserted triangles

vtkStandardNewMacro(vtkQuadricClustering);

//----------------------------------------------------------------------------
// PIMPLd STL set for keeping track of inserted cells
struct vtkQuadricClusteringIdTypeHash {
  size_t operator()(vtkIdType val) const { return static_cast<size_t>(val); }
};
class vtkQuadricClusteringCellSet : public vtksys::hash_set<vtkIdType, vtkQuadricClusteringIdTypeHash> {};
typedef vtkQuadricClusteringCellSet::iterator vtkQuadricClusteringCellSetIterator;


//----------------------------------------------------------------------------
// Construct with default NumberOfDivisions to 50, DivisionSpacing to 1
// in all (x,y,z) directions. AutoAdjustNumberOfDivisions is set to ON.
// ComputeNumberOfDivisions to OFF. UseFeatureEdges and UseFeaturePoints
// are set to OFF by default
// The default behavior is also to compute an optimal position in each
// bin to produce the output triangles (this is also recommended)
vtkQuadricClustering::vtkQuadricClustering()
{
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] = 0.0;
  this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0.0;
  this->NumberOfXDivisions = 50;
  this->NumberOfYDivisions = 50;
  this->NumberOfZDivisions = 50;
  this->QuadricArray = NULL;
  this->NumberOfBinsUsed = 0;
  this->AbortExecute = 0;

  this->AutoAdjustNumberOfDivisions = 1;
  this->ComputeNumberOfDivisions = 0;
  this->DivisionOrigin[0] = 0.0;
  this->DivisionOrigin[1] = 0.0;
  this->DivisionOrigin[2] = 0.0;
  this->DivisionSpacing[0] = 1.0;
  this->DivisionSpacing[1] = 1.0;
  this->DivisionSpacing[2] = 1.0;

  this->UseFeatureEdges = 0;
  this->UseFeaturePoints = 0;
  this->FeaturePointsAngle = 30.0;
  this->UseInternalTriangles = 1;

  this->UseInputPoints = 0;

  this->PreventDuplicateCells = 1;
  this->CellSet = NULL;
  this->NumberOfBins = 0;

  this->OutputTriangleArray = NULL;
  this->OutputLines = NULL;

  // Used for matching boundaries.
  this->FeatureEdges = vtkFeatureEdges::New();
  this->FeatureEdges->FeatureEdgesOff();
  this->FeatureEdges->BoundaryEdgesOn();
  this->FeaturePoints = vtkPoints::New();

  this->InCellCount = this->OutCellCount = 0;
  this->CopyCellData = 0;
}

//----------------------------------------------------------------------------
vtkQuadricClustering::~vtkQuadricClustering()
{
  this->FeatureEdges->Delete();
  this->FeatureEdges = NULL;
  this->FeaturePoints->Delete();
  this->FeaturePoints = NULL;
  delete this->CellSet;
  this->CellSet = NULL;
  delete [] this->QuadricArray;
  this->QuadricArray = NULL;
  if (this->OutputTriangleArray)
    {
    this->OutputTriangleArray->Delete();
    this->OutputTriangleArray = NULL;
    }
  if (this->OutputLines)
    {
    this->OutputLines->Delete();
    this->OutputLines = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkQuadricClustering::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = 0;
  if (inInfo)
    {
    input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkTimerLog *tlog=NULL;

  if (!input || (input->GetNumberOfPoints() == 0))
    {
    // The user may be calling StartAppend, Append, and EndAppend explicitly.
    return 1;
    }

  if (input->CheckAttributes())
    {
    // avoid crashing if input is not all we expect (is not consistent).
    return 1;
    }

  if (this->Debug)
    {
    tlog = vtkTimerLog::New();
    tlog->StartTimer();
    }

  // Lets limit the number of divisions based on
  // the number of points in the input.
  // (To minimize chance of overflow, force math in vtkIdType type,
  // which is sometimes bigger than int, and never smaller.)
  vtkIdType target = input->GetNumberOfPoints();
  vtkIdType numDiv = static_cast<vtkIdType>(this->NumberOfXDivisions)
                        * this->NumberOfYDivisions
                        * this->NumberOfZDivisions
                        / 2;
  if (this->AutoAdjustNumberOfDivisions && numDiv > target)
    {
    double factor = pow(((double)numDiv/(double)target),0.33333);
    this->NumberOfDivisions[0] =
      (int)(0.5+(double)(this->NumberOfXDivisions)/factor);
    this->NumberOfDivisions[0] = (this->NumberOfDivisions[0] > 0 ? this->NumberOfDivisions[0] : 1);
    this->NumberOfDivisions[1] =
      (int)(0.5+(double)(this->NumberOfYDivisions)/factor);
    this->NumberOfDivisions[1] = (this->NumberOfDivisions[1] > 0 ? this->NumberOfDivisions[1] : 1);
    this->NumberOfDivisions[2] =
      (int)(0.5+(double)(this->NumberOfZDivisions)/factor);
    this->NumberOfDivisions[2] = (this->NumberOfDivisions[2] > 0 ? this->NumberOfDivisions[2] : 1);
    }
  else
    {
    this->NumberOfDivisions[0] = this->NumberOfXDivisions;
    this->NumberOfDivisions[1] = this->NumberOfYDivisions;
    this->NumberOfDivisions[2] = this->NumberOfZDivisions;
    }

  this->UpdateProgress(.01);

  this->StartAppend(input->GetBounds());
  this->UpdateProgress(.2);
  this->SliceSize = this->NumberOfDivisions[0]*this->NumberOfDivisions[1];

  this->Append(input);
  if (this->UseFeatureEdges)
    { // Adjust bin points that contain boundary edges.
    this->AppendFeatureQuadrics(input, output);
    }

  if (this->UseInputPoints)
    {
    this->EndAppendUsingPoints(input, output);
    }
  else
    {
    this->EndAppend();
    }

  // Free up some memory.
  delete [] this->QuadricArray;
  this->QuadricArray = NULL;

  if ( this->Debug )
    {
    tlog->StopTimer();
    vtkDebugMacro(<<"Execution took: "<<tlog->GetElapsedTime()<<" seconds.");
    tlog->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::StartAppend(double *bounds)
{
  // If there are duplicate triangles. remove them
  if ( this->PreventDuplicateCells )
    {
    this->CellSet = new vtkQuadricClusteringCellSet;
    this->NumberOfBins =
      this->NumberOfDivisions[0]*this->NumberOfDivisions[1]*this->NumberOfDivisions[2];
    }

  // Copy over the bounds.
  for (vtkIdType i = 0; i < 6; ++i)
    {
    this->Bounds[i]= bounds[i];
    }

  if (this->ComputeNumberOfDivisions)
    {
    // extend the bounds so that it will not produce fractions of bins.
    double x, y, z;
    x = floor((bounds[0]-this->DivisionOrigin[0])/this->DivisionSpacing[0]);
    y = floor((bounds[2]-this->DivisionOrigin[1])/this->DivisionSpacing[1]);
    z = floor((bounds[4]-this->DivisionOrigin[2])/this->DivisionSpacing[2]);
    this->Bounds[0] = this->DivisionOrigin[0]+(x * this->DivisionSpacing[0]);
    this->Bounds[2] = this->DivisionOrigin[1]+(y * this->DivisionSpacing[1]);
    this->Bounds[4] = this->DivisionOrigin[2]+(z * this->DivisionSpacing[2]);
    x = ceil((bounds[1]-this->Bounds[0])/this->DivisionSpacing[0]);
    y = ceil((bounds[3]-this->Bounds[2])/this->DivisionSpacing[1]);
    z = ceil((bounds[5]-this->Bounds[4])/this->DivisionSpacing[2]);
    this->Bounds[1] = this->Bounds[0] + (x * this->DivisionSpacing[0]);
    this->Bounds[3] = this->Bounds[2] + (y * this->DivisionSpacing[1]);
    this->Bounds[5] = this->Bounds[4] + (z * this->DivisionSpacing[2]);
    this->NumberOfDivisions[0] = (int)x > 0 ? (int)x : 1;
    this->NumberOfDivisions[1] = (int)y > 0 ? (int)y : 1;
    this->NumberOfDivisions[2] = (int)z > 0 ? (int)z : 1;
    }
  else
    {
    this->DivisionOrigin[0] = bounds[0];
    this->DivisionOrigin[1] = bounds[2];
    this->DivisionOrigin[2] = bounds[4];
    this->DivisionSpacing[0] = (bounds[1]-bounds[0])/this->NumberOfDivisions[0];
    this->DivisionSpacing[1] = (bounds[3]-bounds[2])/this->NumberOfDivisions[1];
    this->DivisionSpacing[2] = (bounds[5]-bounds[4])/this->NumberOfDivisions[2];
    }

  // Check for conditions that can occur if the Append methods
  // are not called in the correct order.
  if (this->OutputTriangleArray)
    {
    this->OutputTriangleArray->Delete();
    this->OutputTriangleArray = NULL;
    //vtkWarningMacro("Array already created.  Did you call EndAppend?");
    }
  if (this->OutputLines)
    {
    this->OutputLines->Delete();
    this->OutputLines = NULL;
    //vtkWarningMacro("Array already created.  Did you call EndAppend?");
    }

  this->OutputTriangleArray = vtkCellArray::New();
  this->OutputLines = vtkCellArray::New();

  this->XBinSize = (this->Bounds[1]-this->Bounds[0])/this->NumberOfDivisions[0];
  this->YBinSize = (this->Bounds[3]-this->Bounds[2])/this->NumberOfDivisions[1];
  this->ZBinSize = (this->Bounds[5]-this->Bounds[4])/this->NumberOfDivisions[2];

  this->XBinStep = (this->XBinSize > 0.0) ? (1.0/this->XBinSize) : 0.0;
  this->YBinStep = (this->YBinSize > 0.0) ? (1.0/this->YBinSize) : 0.0;
  this->ZBinStep = (this->ZBinSize > 0.0) ? (1.0/this->ZBinSize) : 0.0;

  this->NumberOfBinsUsed = 0;
  delete [] this->QuadricArray;
  this->QuadricArray =
    new vtkQuadricClustering::PointQuadric[this->NumberOfDivisions[0] *
                                          this->NumberOfDivisions[1] *
                                          this->NumberOfDivisions[2]];
  if (this->QuadricArray == NULL)
    {
    vtkErrorMacro("Could not allocate quadric grid.");
    return;
    }

  vtkInformation *inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkInformation *outInfo = this->GetExecutive()->GetOutputInformation(0);
  vtkPolyData *input = 0;
  if (inInfo)
    {
    input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Allocate CellData here.
  if (this->CopyCellData && input)
    {
    output->GetCellData()->CopyAllocate(
      input->GetCellData(), this->NumberOfBinsUsed);
    this->InCellCount = this->OutCellCount = 0;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Append(vtkPolyData *pd)
{
  vtkCellArray *inputPolys, *inputStrips, *inputLines, *inputVerts;
  vtkPoints *inputPoints = pd->GetPoints();

  // Check for mis-use of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    vtkDebugMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  vtkInformation *outInfo =
    this->GetExecutive()->GetOutputInformation(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  inputVerts = pd->GetVerts();
  if (inputVerts)
    {
    this->AddVertices(inputVerts, inputPoints, 1, pd, output);
    }
  this->UpdateProgress(.40);

  inputLines = pd->GetLines();
  if (inputLines)
    {
    this->AddEdges(inputLines, inputPoints, 1, pd, output);
    }
  this->UpdateProgress(.60);

  inputPolys = pd->GetPolys();
  if (inputPolys)
    {
    this->AddPolygons(inputPolys, inputPoints, 1, pd, output);
    }
  this->UpdateProgress(.80);

  inputStrips = pd->GetStrips();
  if (inputStrips)
    {
    this->AddStrips(inputStrips, inputPoints, 1, pd, output);
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddPolygons(vtkCellArray *polys, vtkPoints *points,
                                       int geometryFlag,
                                       vtkPolyData *input, vtkPolyData *output)
{
  vtkIdType *ptIds = 0;
  vtkIdType numPts = 0;
  double pts0[3], pts1[3], pts2[3];
  vtkIdType binIds[3];

  double total = polys->GetNumberOfCells();
  double curr = 0;
  double step = total / 10;
  if (step < 1000.0)
    {
    step = 1000.0;
    }
  double cstep = step;

  for ( polys->InitTraversal(); polys->GetNextCell(numPts, ptIds); )
    {
    points->GetPoint(ptIds[0],pts0);
    binIds[0] = this->HashPoint(pts0);
    for (vtkIdType j=0; j < numPts-2; j++)//creates triangles; assumes poly is convex
      {
      points->GetPoint(ptIds[j+1],pts1);
      binIds[1] = this->HashPoint(pts1);
      points->GetPoint(ptIds[j+2],pts2);
      binIds[2] = this->HashPoint(pts2);
      this->AddTriangle(binIds, pts0, pts1, pts2, geometryFlag, input, output);
      }
    ++this->InCellCount;
    if ( curr > cstep )
      {
      this->UpdateProgress(.6 + .2 * curr / total);
      cstep += step;
      }
    curr += 1;
    }//for all polygons
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddStrips(vtkCellArray *strips, vtkPoints *points,
                                     int geometryFlag,
                                     vtkPolyData *input, vtkPolyData *output)
{
  vtkIdType *ptIds = 0;
  vtkIdType numPts = 0;
  double pts[3][3];
  vtkIdType binIds[3];
  int odd;  // Used to flip order of every other triangle in a strip.

  for ( strips->InitTraversal(); strips->GetNextCell(numPts, ptIds); )
    {
    points->GetPoint(ptIds[0], pts[0]);
    binIds[0] = this->HashPoint(pts[0]);
    points->GetPoint(ptIds[1], pts[1]);
    binIds[1] = this->HashPoint(pts[1]);
    // This internal loop handles triangle strips.
    odd = 0;
    for (vtkIdType j = 2; j < numPts; ++j)
      {
      points->GetPoint(ptIds[j], pts[2]);
      binIds[2] = this->HashPoint(pts[2]);
      this->AddTriangle(binIds, pts[0], pts[1], pts[2], geometryFlag, input,
                        output);
      pts[odd][0] = pts[2][0];
      pts[odd][1] = pts[2][1];
      pts[odd][2] = pts[2][2];
      binIds[odd] = binIds[2];
      // Toggle odd.
      odd = odd ? 0 : 1;
      }
    ++this->InCellCount;
    }
}

//----------------------------------------------------------------------------
inline void vtkQuadricClustering::InitializeQuadric(double quadric[9])
{
  quadric[0] = 0.0;
  quadric[1] = 0.0;
  quadric[2] = 0.0;
  quadric[3] = 0.0;
  quadric[4] = 0.0;
  quadric[5] = 0.0;
  quadric[6] = 0.0;
  quadric[7] = 0.0;
  quadric[8] = 0.0;
}

//----------------------------------------------------------------------------
// The error function is the volume (squared) of the tetrahedron formed by the
// triangle and the point.  We ignore constant factors across all coefficents,
// and the constant coefficient.
// If geomertyFlag is 1 then the triangle is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddTriangle(vtkIdType *binIds, double *pt0, double *pt1,
                                       double *pt2, int geometryFlag,
                                       vtkPolyData *input, vtkPolyData *output)
{
  // Special condition for fast execution.
  // Only add triangles that traverse three bins to quadrics.
  if (this->UseInternalTriangles == 0)
    {
    if (binIds[0] == binIds[1] || binIds[0] == binIds[2] ||
        binIds[1] == binIds[2])
      {
      return;
      }
    }

  // Compute the quadric.
  double quadric[9], quadric4x4[4][4];
  vtkTriangle::ComputeQuadric(pt0, pt1, pt2, quadric4x4);
  quadric[0] = quadric4x4[0][0];
  quadric[1] = quadric4x4[0][1];
  quadric[2] = quadric4x4[0][2];
  quadric[3] = quadric4x4[0][3];
  quadric[4] = quadric4x4[1][1];
  quadric[5] = quadric4x4[1][2];
  quadric[6] = quadric4x4[1][3];
  quadric[7] = quadric4x4[2][2];
  quadric[8] = quadric4x4[2][3];

  // Add the quadric to each of the three corner bins.
  for (int i = 0; i < 3; ++i)
    {
    // If the current quadric is not initialized, then clear it out.
    if (this->QuadricArray[binIds[i]].Dimension > 2)
      {
      this->QuadricArray[binIds[i]].Dimension = 2;
      // Initialize the coeff
      this->InitializeQuadric(this->QuadricArray[binIds[i]].Quadric);
      }
    if (this->QuadricArray[binIds[i]].Dimension == 2)
      { // Points and segments supercede triangles.
      this->AddQuadric(binIds[i], quadric);
      }
    }

  if (geometryFlag)
    {
    vtkIdType triPtIds[3];
    // Now add the triangle to the geometry.
    for (int i = 0; i < 3; i++)
      {
      // Get the vertex from each bin.
      if (this->QuadricArray[binIds[i]].VertexId == -1)
        {
        this->QuadricArray[binIds[i]].VertexId = this->NumberOfBinsUsed;
        this->NumberOfBinsUsed++;
        }
      triPtIds[i] = this->QuadricArray[binIds[i]].VertexId;
      }
    // This comparison could just as well be on triPtIds.
    if (binIds[0] != binIds[1] && binIds[0] != binIds[2] &&
        binIds[1] != binIds[2])
      {
      if ( this->PreventDuplicateCells )
        {
        vtkIdType minIdx = ( binIds[0]<binIds[1] ? (binIds[0]<binIds[2] ? 0 : 2) :
                             (binIds[1]<binIds[2] ? 1 : 2) );
        vtkIdType midIdx = 0;
        vtkIdType maxIdx = 0;
        switch ( minIdx )
          {
          case 0:
            if ( binIds[1] > binIds[2] )
              {
              maxIdx = 1;
              midIdx = 2;
              }
            else
              {
              maxIdx = 2;
              midIdx = 1;
              }
            break;
          case 1:
            if ( binIds[0] > binIds[2] )
              {
              maxIdx = 0;
              midIdx = 2;
              }
            else
              {
              maxIdx = 2;
              midIdx = 0;
              }
            break;
          case 2:
            if ( binIds[0] > binIds[1] )
              {
              maxIdx = 0;
              midIdx = 1;
              }
            else
              {
              maxIdx = 1;
              midIdx = 0;
              }
            break;
          }
        // TODO: this arithmetic overflows with the TestQuadricLODActor test.
        vtkIdType idx = binIds[minIdx] + this->NumberOfBins*binIds[midIdx] +
                        this->NumberOfBins*this->NumberOfBins*binIds[maxIdx];
        if ( this->CellSet->find(idx) == this->CellSet->end() )
          {
          this->CellSet->insert(idx);
          this->OutputTriangleArray->InsertNextCell(3, triPtIds);
          if (this->CopyCellData && input)
            {
            output->GetCellData()->
              CopyData(input->GetCellData(), this->InCellCount,this->OutCellCount++);
            }//if cell data
          }//if not a duplicate
        }
      else //don't check for duplicates
        {
        this->OutputTriangleArray->InsertNextCell(3, triPtIds);
        if (this->CopyCellData && input)
          {
          output->GetCellData()->
            CopyData(input->GetCellData(), this->InCellCount,this->OutCellCount++);
          }//if cell data
        }//don't check for duplicates
      }//if not duplicate vertices
    }//if this should be inserted
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddEdges(vtkCellArray *edges, vtkPoints *points,
                                    int geometryFlag,
                                    vtkPolyData *input, vtkPolyData *output)
{
  vtkIdType numCells;
  vtkIdType *ptIds = 0;
  vtkIdType numPts = 0;
  double pt0[3], pt1[3];
  vtkIdType binIds[2];

  // Add the edges to the error fuction.
  numCells = edges->GetNumberOfCells();
  edges->InitTraversal();
  for (vtkIdType i = 0; i < numCells; ++i)
    {
    edges->GetNextCell(numPts, ptIds);
    if(numPts != 0)
      {
      points->GetPoint(ptIds[0], pt0);
      binIds[0] = this->HashPoint(pt0);
      // This internal loop handles line strips.
      for (vtkIdType j = 1; j < numPts; ++j)
        {
        points->GetPoint(ptIds[j], pt1);
        binIds[1] = this->HashPoint(pt1);
        this->AddEdge(binIds, pt0, pt1, geometryFlag, input, output);
        pt0[0] = pt1[0];
        pt0[1] = pt1[1];
        pt0[2] = pt1[2];
        binIds[0] = binIds[1];
        }
      }
    ++this->InCellCount;
    }
}
//----------------------------------------------------------------------------
// The error function is the square of the area of the triangle formed by the
// edge and the point.  We ignore constants across all terms.
// If geometryFlag is 1 then the edge is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddEdge(vtkIdType *binIds, double *pt0, double *pt1,
                                   int geometryFlag,
                                   vtkPolyData *input, vtkPolyData *output)
{
  vtkIdType edgePtIds[2];
  double length2, tmp;
  double d[3];
  double m[3];  // The mid point of the segement.(p1 or p2 could be used also).
  double md;    // The dot product of m and d.
  double q[9];

  // Compute quadric for line segment.
  // Line segment quadric is the area (squared) of the triangle (seg,pt)
  // Compute the direction vector of the segment.
  d[0] = pt1[0] - pt0[0];
  d[1] = pt1[1] - pt0[1];
  d[2] = pt1[2] - pt0[2];

  // Compute the length^2 of the line segement.
  length2 = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];

  if (length2 == 0.0)
    { // Coincident points.  Avoid divide by zero.
    return;
    }

  // Normalize the direction vector.
  tmp = 1.0 / sqrt(length2);
  d[0] = d[0] * tmp;
  d[1] = d[1] * tmp;
  d[2] = d[2] * tmp;

  // Compute the mid point of the segment.
  m[0] = 0.5 * (pt1[0] + pt0[0]);
  m[1] = 0.5 * (pt1[1] + pt0[1]);
  m[2] = 0.5 * (pt1[2] + pt0[2]);

  // Compute dot(m, d);
  md = m[0]*d[0] + m[1]*d[1] + m[2]*d[2];

  // We save nine coefficients of the error function cooresponding to:
  // 0: Px^2
  // 1: PxPy
  // 2: PxPz
  // 3: Px
  // 4: Py^2
  // 5: PyPz
  // 6: Py
  // 7: Pz^2
  // 8: Pz
  // We ignore the constant because it disappears with the derivative.
  q[0] = length2*(1.0 - d[0]*d[0]);
  q[1] = -length2*(d[0]*d[1]);
  q[2] = -length2*(d[0]*d[2]);
  q[3] = length2*(d[0]*md - m[0]);
  q[4] = length2*(1.0 - d[1]*d[1]);
  q[5] = -length2*(d[1]*d[2]);
  q[6] = length2*(d[1]*md - m[1]);
  q[7] = length2*(1.0 - d[2]*d[2]);
  q[8] = length2*(d[2]*md - m[2]);

  for (int i = 0; i < 2; ++i)
    {
    // If the current quadric is from triangles (or not initialized), then clear it out.
    if (this->QuadricArray[binIds[i]].Dimension > 1)
      {
      this->QuadricArray[binIds[i]].Dimension = 1;
      // Initialize the coeff
      this->InitializeQuadric(this->QuadricArray[binIds[i]].Quadric);
      }
    if (this->QuadricArray[binIds[i]].Dimension == 1)
      { // Points supercede segements.
      this->AddQuadric(binIds[i], q);
      }
    }

  if (geometryFlag)
    {
    // Now add the edge to the geometry.
    for (int i = 0; i < 2; i++)
      {
      // Get the vertex from each bin.
      if (this->QuadricArray[binIds[i]].VertexId == -1)
        {
        this->QuadricArray[binIds[i]].VertexId = this->NumberOfBinsUsed;
        this->NumberOfBinsUsed++;
        }
      edgePtIds[i] = this->QuadricArray[binIds[i]].VertexId;
      }
    // This comparison could just as well be on edgePtIds.
    if (binIds[0] != binIds[1])
      {
      this->OutputLines->InsertNextCell(2, edgePtIds);
      if (this->CopyCellData && input)
        {
        output->GetCellData()->
          CopyData(input->GetCellData(),this->InCellCount,
                   this->OutCellCount++);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddVertices(vtkCellArray *verts, vtkPoints *points,
                                       int geometryFlag, vtkPolyData *input,
                                       vtkPolyData *output)
{
  vtkIdType numCells;
  vtkIdType *ptIds = 0;
  vtkIdType numPts = 0;
  double pt[3];
  vtkIdType binId;

  numCells = verts->GetNumberOfCells();
  double cstep = (double)numCells / 10.0;
  if (cstep < 1000.0)
    {
    cstep = 1000.0;
    }
  double next = cstep;
  double curr = 0;

  verts->InitTraversal();
  for (vtkIdType i = 0; i < numCells; ++i)
    {
    verts->GetNextCell(numPts, ptIds);
    // Can there be poly vertices?
    for (vtkIdType j = 0; j < numPts; ++j)
      {
      points->GetPoint(ptIds[j], pt);
      binId = this->HashPoint(pt);
      this->AddVertex(binId, pt, geometryFlag, input, output);
      }
    ++this->InCellCount;

    if ( curr > next )
      {
      this->UpdateProgress(.2 + .2 * curr / (double)numCells);
      next += cstep;
      }
    curr += 1;
    }
}

//----------------------------------------------------------------------------
// The error function is the length (point to vert) squared.
// We ignore constants across all terms.
// If geomertyFlag is 1 then the vert is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddVertex(vtkIdType binId, double *pt,
                                     int geometryFlag,
                                     vtkPolyData *input, vtkPolyData *output)
{
  double q[9];

  // Compute quadric for the vertex.

  // We save nine coefficients of the error function cooresponding to:
  // 0: Px^2
  // 1: PxPy
  // 2: PxPz
  // 3: Px
  // 4: Py^2
  // 5: PyPz
  // 6: Py
  // 7: Pz^2
  // 8: Pz
  // We ignore the constant because it disappears with the derivative.
  q[0] = 1.0;
  q[1] = 0.0;
  q[2] = 0.0;
  q[3] = -pt[0];
  q[4] = 1.0;
  q[5] = 0.0;
  q[6] = -pt[1];
  q[7] = 1.0;
  q[8] = -pt[2];

  // If the current quadric is from triangles, edges (or not initialized),
  // then clear it out.
  if (this->QuadricArray[binId].Dimension > 0)
    {
    this->QuadricArray[binId].Dimension = 0;
    // Initialize the coeff
    this->InitializeQuadric(this->QuadricArray[binId].Quadric);
    }
  if (this->QuadricArray[binId].Dimension == 0)
    { // Points supercede all other types of quadrics.
    this->AddQuadric(binId, q);
    }

  if (geometryFlag)
    {
    // Now add the vert to the geometry.
    // Get the vertex from the bin.
    if (this->QuadricArray[binId].VertexId == -1)
      {
      this->QuadricArray[binId].VertexId = this->NumberOfBinsUsed;
      this->NumberOfBinsUsed++;

      if (this->CopyCellData && input)
        {
        output->GetCellData()->
          CopyData(input->GetCellData(), this->InCellCount,
                   this->OutCellCount++);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddQuadric(vtkIdType binId, double quadric[9])
{
  double *q = this->QuadricArray[binId].Quadric;

  for (int i=0; i<9; i++)
    {
    q[i] += (quadric[i] * 100000000.0);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkQuadricClustering::HashPoint(double point[3])
{
  vtkIdType xBinCoord = static_cast<vtkIdType>(
                         (point[0] - this->Bounds[0]) * this->XBinStep);
  if (xBinCoord < 0)
    {
    xBinCoord = 0;
    }
  else if (xBinCoord >= this->NumberOfDivisions[0])
    {
    xBinCoord = this->NumberOfDivisions[0] - 1;
    }

  vtkIdType yBinCoord = static_cast<vtkIdType>(
                         (point[1] - this->Bounds[2]) * this->YBinStep);
  if (yBinCoord < 0)
    {
    yBinCoord = 0;
    }
  else if (yBinCoord >= this->NumberOfDivisions[1])
    {
    yBinCoord = this->NumberOfDivisions[1] - 1;
    }

  vtkIdType zBinCoord = static_cast<vtkIdType>(
                         (point[2] - this->Bounds[4]) * this->ZBinStep);
  if (zBinCoord < 0)
    {
    zBinCoord = 0;
    }
  else if (zBinCoord >= this->NumberOfDivisions[2])
    {
    zBinCoord = this->NumberOfDivisions[2] - 1;
    }

  // vary x fastest, then y, then z
  vtkIdType binId = xBinCoord + yBinCoord*this->NumberOfDivisions[0] +
          zBinCoord*this->SliceSize;

  return binId;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppend()
{
  vtkInformation *inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkInformation *outInfo = this->GetExecutive()->GetOutputInformation(0);
  vtkPolyData *input = 0;
  if (inInfo)
    {
    input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numBuckets;
  int abortExecute=0;
  vtkPoints *outputPoints;
  double newPt[3];
  numBuckets = this->NumberOfDivisions[0] * this->NumberOfDivisions[1] *
                this->NumberOfDivisions[2];
  double step = (double)numBuckets / 10.0;
  if (step < 1000.0)
    {
    step = 1000.0;
    }
  double cstep = 0;

  // Check for mis use of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    vtkDebugMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  // Clean up
  if ( this->PreventDuplicateCells )
    {
    delete this->CellSet;
    this->CellSet = NULL;
    }

  // Compute the representative points for each bin
  outputPoints = vtkPoints::New();
  for (vtkIdType i = 0; !abortExecute && i < numBuckets; i++ )
    {
    if (cstep > step)
      {
      cstep = 0;
      vtkDebugMacro(<<"Finding point in bin #" << i);
      this->UpdateProgress (0.8+0.2*i/numBuckets);
      abortExecute = this->GetAbortExecute();
      }
    ++cstep;

    if (this->QuadricArray[i].VertexId != -1)
      {
      this->ComputeRepresentativePoint(this->QuadricArray[i].Quadric, i, newPt);
      outputPoints->InsertPoint(this->QuadricArray[i].VertexId, newPt);
      }
    }

  // Set up the output data object.
  output->SetPoints(outputPoints);
  outputPoints->Delete();

  if (this->OutputTriangleArray->GetNumberOfCells() > 0)
    {
    output->SetPolys(this->OutputTriangleArray);
    }
  this->OutputTriangleArray->Delete();
  this->OutputTriangleArray = NULL;

  if (this->OutputLines->GetNumberOfCells() > 0)
    {
    output->SetLines(this->OutputLines);
    }
  this->OutputLines->Delete();
  this->OutputLines = NULL;

  this->EndAppendVertexGeometry(input, output);

  // Tell the data is is up to date
  // (in case the user calls this method directly).
  output->DataHasBeenGenerated();

  // Free the quadric array.
  delete [] this->QuadricArray;
  this->QuadricArray = NULL;
}


//----------------------------------------------------------------------------
void vtkQuadricClustering::ComputeRepresentativePoint(double quadric[9],
                                                      vtkIdType binId,
                                                      double point[3])
{
  double A[3][3], U[3][3], UT[3][3], VT[3][3], V[3][3];
  double b[3], w[3];
  double W[3][3], tempMatrix[3][3];
  double cellCenter[3], tempVector[3];
  double cellBounds[6];
  double quadric4x4[4][4];

  quadric4x4[0][0] = quadric[0];
  quadric4x4[0][1] = quadric4x4[1][0] = quadric[1];
  quadric4x4[0][2] = quadric4x4[2][0] = quadric[2];
  quadric4x4[0][3] = quadric4x4[3][0] = quadric[3];
  quadric4x4[1][1] = quadric[4];
  quadric4x4[1][2] = quadric4x4[2][1] = quadric[5];
  quadric4x4[1][3] = quadric4x4[3][1] = quadric[6];
  quadric4x4[2][2] = quadric[7];
  quadric4x4[2][3] = quadric4x4[3][2] = quadric[8];
  quadric4x4[3][3] = 1;  // arbitrary value

  vtkIdType x = binId % this->NumberOfDivisions[0];
  vtkIdType y = (binId / this->NumberOfDivisions[0]) % this->NumberOfDivisions[1];
  vtkIdType z = binId / this->SliceSize;

  cellBounds[0] = this->Bounds[0] + x * this->XBinSize;
  cellBounds[1] = this->Bounds[0] + (x+1) * this->XBinSize;
  cellBounds[2] = this->Bounds[2] + y * this->YBinSize;
  cellBounds[3] = this->Bounds[2] + (y+1) * this->YBinSize;
  cellBounds[4] = this->Bounds[4] + z * this->ZBinSize;
  cellBounds[5] = this->Bounds[4] + (z+1) * this->ZBinSize;

  for (int i = 0; i < 3; i++)
    {
    b[i] = -quadric4x4[3][i];
    cellCenter[i] = (cellBounds[i*2+1] + cellBounds[i*2]) / 2.0;
    for (int j = 0; j < 3; j++)
      {
      A[i][j] = quadric4x4[i][j];
      }
    }

  // Compute diagonal matrix W
  //
#define VTK_SVTHRESHOLD 1.0E-3
  double maxW = 0.0, temp;
  vtkMath::SingularValueDecomposition3x3(A, U, w, VT);

  // Find maximum (magnitude) eigenvalue from SVD
  for (int i = 0; i < 3; i++)
    {
    if ((temp = fabs(w[i])) > maxW)
      {
      maxW = temp;
      }
    }
  // Initialize matrix
  for (int i = 0; i < 3; i++)
    {
    for (int j = 0; j < 3; j++)
      {
      if (i == j)
        {
        if ( fabs(w[i] / maxW) > VTK_SVTHRESHOLD)
          {
          // If this is true, then w[i] != 0, so this division is ok.
          W[i][j] = 1.0/w[i];
          }
        else
          {
          W[i][j] = 0.0;
          }
        }
      else
        {
        W[i][j] = 0.0;
        }
      }
    }
#undef VTK_SVTHRESHOLD

  vtkMath::Transpose3x3(U, UT);
  vtkMath::Transpose3x3(VT, V);
  vtkMath::Multiply3x3(W, UT, tempMatrix);
  vtkMath::Multiply3x3(V, tempMatrix, tempMatrix);
  vtkMath::Multiply3x3(A, cellCenter, tempVector);
  for (int i = 0; i < 3; i++)
    {
    tempVector[i] = b[i] - tempVector[i];
    }
  vtkMath::Multiply3x3(tempMatrix, tempVector, tempVector);

  // Make absolutely sure that the point lies in the vicinity (enclosing
  // sphere) of the bin.  If not, then clamp the point to the enclosing sphere.
  double deltaMag = vtkMath::Norm(tempVector);
  double radius = sqrt(this->XBinSize*this->XBinSize +
    this->YBinSize*this->YBinSize +
    this->ZBinSize*this->ZBinSize) / 2.0;
  if (deltaMag > radius)
    {
    tempVector[0] *= radius / deltaMag;
    tempVector[1] *= radius / deltaMag;
    tempVector[2] *= radius / deltaMag;
    }

  point[0] = cellCenter[0] + tempVector[0];
  point[1] = cellCenter[1] + tempVector[1];
  point[2] = cellCenter[2] + tempVector[2];
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfDivisions(int div0, int div1, int div2)
{
  this->SetNumberOfXDivisions(div0);
  this->SetNumberOfYDivisions(div1);
  this->SetNumberOfZDivisions(div2);
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfXDivisions(int num)
{
  if (this->NumberOfXDivisions == num && this->ComputeNumberOfDivisions == 0)
    {
    return;
    }
  if (num < 1)
    {
    vtkErrorMacro("You cannot use less than one division.");
    return;
    }
  this->Modified();
  this->NumberOfXDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfYDivisions(int num)
{
  if (this->NumberOfYDivisions == num && this->ComputeNumberOfDivisions == 0)
    {
    return;
    }
  if (num < 1)
    {
    vtkErrorMacro("You cannot use less than one division.");
    return;
    }
  this->Modified();
  this->NumberOfYDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfZDivisions(int num)
{
  if (this->NumberOfZDivisions == num && this->ComputeNumberOfDivisions == 0)
    {
    return;
    }
  if (num < 1)
    {
    vtkErrorMacro("You cannot use less than one division.");
    return;
    }
  this->Modified();
  this->NumberOfZDivisions = num;
  this->ComputeNumberOfDivisions = 0;
}


//----------------------------------------------------------------------------
int *vtkQuadricClustering::GetNumberOfDivisions()
{
  static int divs[3];
  this->GetNumberOfDivisions(divs);
  return divs;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::GetNumberOfDivisions(int divs[3])
{
  divs[0] = this->NumberOfXDivisions;
  divs[1] = this->NumberOfYDivisions;
  divs[2] = this->NumberOfZDivisions;
}



//----------------------------------------------------------------------------
void vtkQuadricClustering::SetDivisionOrigin(double x, double y, double z)
{
  if (this->ComputeNumberOfDivisions && this->DivisionOrigin[0] == x &&
      this->DivisionOrigin[1] == y && this->DivisionOrigin[2] == z)
    {
    return;
    }
  this->Modified();
  this->DivisionOrigin[0] = x;
  this->DivisionOrigin[1] = y;
  this->DivisionOrigin[2] = z;
  this->ComputeNumberOfDivisions = 1;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetDivisionSpacing(double x, double y, double z)
{
  if (this->ComputeNumberOfDivisions && this->DivisionSpacing[0] == x &&
      this->DivisionSpacing[1] == y && this->DivisionSpacing[2] == z)
    {
    return;
    }
  if ( x <= 0 )
    {
    vtkErrorMacro( << "Spacing (x) should be larger than 0.0, setting to 1.0" );
    x = 1.0;
    }
  if ( y <= 0 )
    {
    vtkErrorMacro( << "Spacing (y) should be larger than 0.0, setting to 1.0" );
    y = 1.0;
    }
  if ( z <= 0 )
    {
    vtkErrorMacro( << "Spacing (z) should be larger than 0.0, setting to 1.0" );
    z = 1.0;
    }
  this->Modified();
  this->DivisionSpacing[0] = x;
  this->DivisionSpacing[1] = y;
  this->DivisionSpacing[2] = z;
  this->ComputeNumberOfDivisions = 1;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppendUsingPoints(vtkPolyData *input,
                                                vtkPolyData *output)
{
  vtkIdType   outPtId;
  vtkPoints   *inputPoints;
  vtkPoints   *outputPoints;
  vtkIdType   numPoints, numBins;
  vtkIdType   binId;
  double       *minError, e, pt[3];
  double       *q;

  inputPoints = input->GetPoints();
  if (inputPoints == NULL)
    {
    return;
    }

  // Check for misuse of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    vtkDebugMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  // Clean up
  if ( this->PreventDuplicateCells )
    {
    delete this->CellSet;
    this->CellSet = NULL;
    }

  outputPoints = vtkPoints::New();

  // Prepare to copy point data to output
  output->GetPointData()->
    CopyAllocate(input->GetPointData(), this->NumberOfBinsUsed);

  // Allocate and initialize an array to hold errors for each bin.
  numBins = this->NumberOfDivisions[0] * this->NumberOfDivisions[1]
                  * this->NumberOfDivisions[2];
  minError = new double[numBins];
  for (vtkIdType i = 0; i < numBins; ++i)
    {
    minError[i] = VTK_DOUBLE_MAX;
    }

  // Loop through the input points.
  numPoints = inputPoints->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    inputPoints->GetPoint(i, pt);
    binId = this->HashPoint(pt);
    outPtId = this->QuadricArray[binId].VertexId;
    // Sanity check.
    if (outPtId == -1)
      {
      // This condition happens when there are points in the input that are
      // not used in any triangles, and therefore are never added to the
      // 3D hash structure.
      continue;
      }

    // Compute the error for this point.  Note: the constant term is ignored.
    // It will be the same for every point in this bin, and it
    // is not stored in the quadric array anyway.
    q = this->QuadricArray[binId].Quadric;
    e = q[0]*pt[0]*pt[0] + 2.0*q[1]*pt[0]*pt[1] + 2.0*q[2]*pt[0]*pt[2] + 2.0*q[3]*pt[0]
          + q[4]*pt[1]*pt[1] + 2.0*q[5]*pt[1]*pt[2] + 2.0*q[6]*pt[1]
          + q[7]*pt[2]*pt[2] + 2.0*q[8]*pt[2];
    if (e < minError[binId])
      {
      minError[binId] = e;
      outputPoints->InsertPoint(outPtId, pt);

      // Since this is the same point as the input point, copy point data here too.
      output->GetPointData()->CopyData(input->GetPointData(),i,outPtId);
      }
    }

  output->SetPolys(this->OutputTriangleArray);
  output->SetPoints(outputPoints);
  outputPoints->Delete();
  this->OutputTriangleArray->Delete();
  this->OutputTriangleArray = NULL;

  if (this->OutputLines->GetNumberOfCells() > 0)
    {
    output->SetLines(this->OutputLines);
    }
  this->OutputLines->Delete();
  this->OutputLines = NULL;

  this->EndAppendVertexGeometry(input, output);

  delete [] this->QuadricArray;
  this->QuadricArray = NULL;

  delete [] minError;
}

//----------------------------------------------------------------------------
// This is not a perfect implementation, because it does not determine
// which vertex cell is the best for a bin.  The first detected is used.
void vtkQuadricClustering::EndAppendVertexGeometry(vtkPolyData *input,
                                                   vtkPolyData *output)
{
  vtkCellArray *inVerts, *outVerts;
  vtkIdType *tmp = NULL;
  vtkIdType  tmpLength = 0;
  vtkIdType  tmpIdx;
  double pt[3];
  vtkIdType *ptIds = 0;
  vtkIdType numPts = 0;
  vtkIdType outPtId;
  vtkIdType binId, cellId, outCellId;

  inVerts = input->GetVerts();
  outVerts = vtkCellArray::New();

  for (cellId=0, inVerts->InitTraversal(); inVerts->GetNextCell(numPts, ptIds); cellId++)
    {
    if (tmpLength < numPts)
      {
      delete [] tmp;
      tmp = new vtkIdType[numPts];
      tmpLength = numPts;
      }
    tmpIdx = 0;
    for (vtkIdType j = 0; j < numPts; ++j)
      {
      input->GetPoint(ptIds[j], pt);
      binId = this->HashPoint(pt);
      outPtId = this->QuadricArray[binId].VertexId;
      if (outPtId >= 0)
        {
        // Do not use this point.  Destroy infomration in Quadric array.
        this->QuadricArray[binId].VertexId = -1;
        tmp[tmpIdx] = outPtId;
        ++tmpIdx;
        }
      }
    if (tmpIdx > 0)
      {
      // add poly vertex to output.
      outCellId = outVerts->InsertNextCell(tmpIdx, tmp);
      output->GetCellData()->
        CopyData(input->GetCellData(), cellId, outCellId);
      }
    }

  delete [] tmp;

  if (outVerts->GetNumberOfCells() > 0)
    {
    output->SetVerts(outVerts);
    }
  outVerts->Delete();
}


//----------------------------------------------------------------------------
// This method is called after the execution, but before the vertex array
// is deleted. It changes some points to be based on the boundary edges.
void vtkQuadricClustering::AppendFeatureQuadrics(vtkPolyData *pd,
                                                 vtkPolyData *output)
{
  vtkPolyData *input = vtkPolyData::New();
  vtkPoints *edgePts;
  vtkCellArray *edges;
  vtkIdType binId;
  double featurePt[3];

  // Find the boundary edges.
  input->ShallowCopy(pd);
  this->FeatureEdges->SetInputData(input);
  this->FeatureEdges->Update();
  edgePts = this->FeatureEdges->GetOutput()->GetPoints();
  edges = this->FeatureEdges->GetOutput()->GetLines();

  if (edges && edges->GetNumberOfCells() && edgePts)
    {
    this->AddEdges(edges, edgePts, 0, pd, output);
    if (this->UseFeaturePoints)
      {
      this->FindFeaturePoints(edges, edgePts, this->FeaturePointsAngle);
      for (vtkIdType i = 0; i < this->FeaturePoints->GetNumberOfPoints(); i++)
        {
        this->FeaturePoints->GetPoint(i, featurePt);
        binId = this->HashPoint(featurePt);
        this->AddVertex(binId, featurePt, 0, input, output);
        }
      }
    }

  // Release data.
  this->FeatureEdges->SetInputConnection(0, 0);
  this->FeatureEdges->GetOutput()->ReleaseData();
  input->Delete();
}

// Find the feature points of a given set of edges.
// The points returned are (1) those used by only one edge, (2) those
// used by > 2 edges, and (3) those where the angle between 2 edges
// using this point is < angle.
void vtkQuadricClustering::FindFeaturePoints(vtkCellArray *edges,
                                             vtkPoints *edgePts,
                                             double vtkNotUsed(angle))
{
  vtkIdType pointIds[2];
  vtkIdType *cellPts = 0;
  vtkIdType numCellPts;
  vtkIdList *pointIdList = vtkIdList::New();
  vtkIdType numPts = edgePts->GetNumberOfPoints();
  vtkIdType numEdges = edges->GetNumberOfCells();
  vtkIdType edgeCount;
  vtkIdType **pointTable = new vtkIdType *[numPts];
  double featurePoint[3];
  double featureEdges[2][3];
  double point1[3], point2[3];
  vtkIdType *cellPointIds;
  double radAngle = vtkMath::RadiansFromDegrees( this->FeaturePointsAngle );

  this->FeaturePoints->Allocate(numPts);

  for (vtkIdType i = 0; i < numPts; i++)
    {
    pointTable[i] = new vtkIdType[4];
    pointTable[i][1] = 0;
    }

  edges->InitTraversal();
  for (vtkIdType i = 0; i < numEdges; i++)
    {
    edges->GetNextCell(numCellPts, cellPts);
    for (int j = 0; j < 2; j++)
      {
      pointIds[j] = pointIdList->InsertUniqueId(cellPts[j]);
      pointTable[pointIds[j]][0] = cellPts[j];
      edgeCount = pointTable[pointIds[j]][1];
      if (edgeCount < 2)
        {
        pointTable[pointIds[j]][edgeCount+2] = i;
        }
      pointTable[pointIds[j]][1]++;
      }
    }

  for (vtkIdType i = 0; i < numPts; i++)
    {
    if (pointTable[i][1] == 1)
      {
      edgePts->GetPoint(pointTable[i][0], featurePoint);
      }
    else if (pointTable[i][1] > 2)
      {
      edgePts->GetPoint(pointTable[i][0], featurePoint);
      }
    else if (pointTable[i][1] == 2)
      {
      for (int j = 0; j < 2; j++)
        {
        edges->GetCell(3*pointTable[i][j+2], numCellPts, cellPointIds);
        if (cellPointIds[0] == pointTable[i][0])
          {
          edgePts->GetPoint(cellPointIds[0], point1);
          edgePts->GetPoint(cellPointIds[1], point2);
          }
        else
          {
          edgePts->GetPoint(cellPointIds[1], point1);
          edgePts->GetPoint(cellPointIds[0], point2);
          }
        featureEdges[j][0] = point2[0] - point1[0];
        featureEdges[j][1] = point2[1] - point1[1];
        featureEdges[j][2] = point2[2] - point1[2];
        vtkMath::Normalize(featureEdges[j]);
        }
      if (acos(vtkMath::Dot(featureEdges[0], featureEdges[1])) < radAngle)
        {
        edgePts->GetPoint(pointTable[i][0], featurePoint);
        }
      }
    }

  pointIdList->Delete();
  for (vtkIdType i = 0; i < numPts; i++)
    {
    delete [] pointTable[i];
    }
  delete [] pointTable;
}

//----------------------------------------------------------------------------
int vtkQuadricClustering::FillInputPortInformation(int port,
                                                   vtkInformation *info)
{
  int retval = this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return retval;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1]
     << " " << this->Bounds[2] << " " << this->Bounds[3] << " "
     << this->Bounds[4] << " " << this->Bounds[5] << "\n";
  os << indent << "Use Input Points: "
     << (this->UseInputPoints ? "On\n" : "Off\n");

  if (this->ComputeNumberOfDivisions)
    {
    os << indent << "Using Spacing and Origin to construct bins\n";
    }
  else
    {
    os << indent << "Using input bounds and NumberOfDivisions to construct bins\n";
    }
  os << indent << "Division Spacing: " << this->DivisionSpacing[0] << ", "
     << this->DivisionSpacing[1] << ", " << this->DivisionSpacing[2] << endl;
  os << indent << "Division Origin: " << this->DivisionOrigin[0] << ", "
     << this->DivisionOrigin[1] << ", " << this->DivisionOrigin[2] << endl;

  os << indent << "Number of X Divisions: " << this->NumberOfXDivisions
     << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfYDivisions
     << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfZDivisions
     << "\n";

  os << indent << "Auto Adjust Number Of Divisions: "
     << (this->AutoAdjustNumberOfDivisions ? "On\n" : "Off\n");

  os << indent << "Use Internal Triangles: "
     << (this->UseInternalTriangles ? "On\n" : "Off\n");

  os << indent << "Use Feature Edges: " << this->UseFeatureEdges << endl;
  os << indent << "FeatureEdges: (" << this->FeatureEdges << ")\n";

  os << indent << "Feature Points Angle: " << this->FeaturePointsAngle << endl;
  os << indent << "Use Feature Points: "
     << (this->UseFeaturePoints ? "On\n" : "Off\n");
  os << indent << "Copy Cell Data : " << this->CopyCellData << endl;

  os << indent << "Prevent Duplicate Cells : "
     << (this->PreventDuplicateCells ? "On\n" : "Off\n");
}

