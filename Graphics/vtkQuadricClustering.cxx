/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricClustering.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkQuadricClustering.h"
#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkObjectFactory.h"
#include "vtkFeatureEdges.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkQuadricClustering* vtkQuadricClustering::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuadricClustering");
  if(ret)
    {
    return (vtkQuadricClustering*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuadricClustering;
}

//----------------------------------------------------------------------------
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

  this->OutputTriangleArray = NULL;
  this->OutputLines = NULL;

  // Overide superclass so that append can be called directly.
  this->NumberOfRequiredInputs = 0;

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
  if (this->QuadricArray)
    {
    delete [] this->QuadricArray;
    this->QuadricArray = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkTimerLog *tlog = NULL;

  if (input == NULL)
    {
    // The user may be calling StartAppend, Append, and EndAppend explicitly.
    return;
    }
  
  if (this->Debug)
    {
    tlog = vtkTimerLog::New();
    tlog->StartTimer();
    }

  this->StartAppend(input->GetBounds());

  this->Append(input);
  if (this->UseFeatureEdges)
    { // Adjust bin points that contain boundary edges.
    this->AppendFeatureQuadrics(this->GetInput());
    }

  if (this->UseInputPoints)
    {
    this->EndAppendUsingPoints(input);
    }
  else
    {
    this->EndAppend();
    }

  
  if ( this->Debug )
    {
    tlog->StopTimer();
    vtkDebugMacro(<<"Execution took: "<<tlog->GetElapsedTime()<<" seconds.");
    tlog->Delete();
    tlog = NULL;
    }  
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::StartAppend(float *bounds)
{
  vtkIdType i, numBins;

  // Copy over the bounds.
  for (i = 0; i < 6; ++i)
    {
    this->Bounds[i]= bounds[i];
    }

  if (this->ComputeNumberOfDivisions)
    {
    // extend the bounds so that it will not produce fractions of bins.
    float x, y, z;
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
    this->NumberOfXDivisions = (int)x;
    this->NumberOfYDivisions = (int)y;
    this->NumberOfZDivisions = (int)z;
    }
  else
    {
    this->DivisionOrigin[0] = bounds[0];
    this->DivisionOrigin[1] = bounds[2];
    this->DivisionOrigin[2] = bounds[4];
    this->DivisionSpacing[0] = (bounds[1]-bounds[0])/this->NumberOfXDivisions;
    this->DivisionSpacing[1] = (bounds[3]-bounds[2])/this->NumberOfYDivisions;
    this->DivisionSpacing[2] = (bounds[5]-bounds[4])/this->NumberOfZDivisions;
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

  this->XBinSize = (this->Bounds[1]-this->Bounds[0])/this->NumberOfXDivisions;
  this->YBinSize = (this->Bounds[3]-this->Bounds[2])/this->NumberOfYDivisions;
  this->ZBinSize = (this->Bounds[5]-this->Bounds[4])/this->NumberOfZDivisions;   

  this->NumberOfBinsUsed = 0;
  if (this->QuadricArray)
    {
    delete [] this->QuadricArray;
    this->QuadricArray = NULL;
    }
  this->QuadricArray = new VTK_POINT_QUADRIC[this->NumberOfXDivisions *
                                            this->NumberOfYDivisions *
                                            this->NumberOfZDivisions];
  if (this->QuadricArray == NULL)
    {
    vtkErrorMacro("Could not allocate quadric grid.");
    return;
    }
  numBins = this->NumberOfXDivisions 
              * this->NumberOfYDivisions * this->NumberOfZDivisions;
  for (i = 0; i < numBins; i++)
    {
    this->QuadricArray[i].VertexId = -1;
    this->QuadricArray[i].Dimension = 255;
    }

  // Allocate CellData here.
  if (this->CopyCellData && this->GetInput())
    {
    this->GetOutput()->GetCellData()->CopyAllocate(
            this->GetInput()->GetCellData(), this->NumberOfBinsUsed);
    this->InCellCount = this->OutCellCount = 0;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Append(vtkPolyData *pd)
{
  vtkCellArray *inputTris, *inputLines, *inputVerts;
  vtkPoints *inputPoints = pd->GetPoints();
  
  // Check for mis-use of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    //vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  inputVerts = pd->GetVerts();
  if (inputVerts)
    {
    this->AddVertices(inputVerts, inputPoints, 1);
    }

  inputLines = pd->GetLines();
  if (inputLines)
    {
    this->AddEdges(inputLines, inputPoints, 1);
    }

  inputTris = pd->GetPolys();
  if (inputTris)
    {
    this->AddPolygons(inputTris, inputPoints, 1);
    }

  inputTris = pd->GetStrips();
  if (inputTris)
    {
    this->AddTriangles(inputTris, inputPoints, 1);
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddTriangles(vtkCellArray *tris, vtkPoints *points,
                                        int geometryFlag)
{
  int j;
  vtkIdType numCells, i;
  vtkIdType *ptIds, numPts;
  float *pts[3];
  vtkIdType binIds[3];
  int odd;  // Used to flip order of every other triangle in a strip.

  numCells = tris->GetNumberOfCells();
  tris->InitTraversal();
  for (i = 0; i < numCells; ++i)
    {
    tris->GetNextCell(numPts, ptIds);
    pts[0] = points->GetPoint(ptIds[0]);
    binIds[0] = this->HashPoint(pts[0]);
    pts[1] = points->GetPoint(ptIds[1]);
    binIds[1] = this->HashPoint(pts[1]);
    // This internal loop handles triangle strips.
    odd = 0;
    for (j = 2; j < numPts; ++j)
      {
      pts[2] = points->GetPoint(ptIds[j]);
      binIds[2] = this->HashPoint(pts[2]);
      this->AddTriangle(binIds, pts[0], pts[1], pts[2], geometryFlag);
      pts[odd] = pts[2];
      binIds[odd] = binIds[2];
      // Toggle odd.
      odd = odd ? 0 : 1;
      }
    ++this->InCellCount;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddPolygons(vtkCellArray *polys, vtkPoints *points,
                                       int geometryFlag)
{
  vtkIdType numCells, i;
  int j;
  vtkIdType *ptIds, numPts;
  float **pts;
  vtkIdType binIds[3];

  numCells = polys->GetNumberOfCells();
  polys->InitTraversal();
  for (i = 0; i < numCells; ++i)
    {
    polys->GetNextCell(numPts, ptIds);
    pts = new float *[numPts];
    pts[0] = points->GetPoint(ptIds[0]);
    binIds[0] = this->HashPoint(pts[0]);
    for (j = 0; j < numPts-2; j++)
      {
      pts[j+1] = points->GetPoint(ptIds[j+1]);
      binIds[1] = this->HashPoint(pts[j+1]);
      pts[j+2] = points->GetPoint(ptIds[j+2]);
      binIds[2] = this->HashPoint(pts[j+2]);
      this->AddTriangle(binIds, pts[0], pts[j+1], pts[j+2], geometryFlag);
      }
    ++this->InCellCount;
    delete [] pts;
    }
}

//----------------------------------------------------------------------------
// The error function is the volume (squared) of the tetrahedron formed by the 
// triangle and the point.  We ignore constant factors across all coefficents, 
// and the constant coefficient.
// If geomertyFlag is 1 then the triangle is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddTriangle(vtkIdType *binIds, float *pt0,
                                       float *pt1, float *pt2,
                                       int geometryFlag)
{
  int i;
  vtkIdType triPtIds[3];
  float quadric[9], quadric4x4[4][4];
 
  // Compute the quadric.
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

  // Add the quadric to each of the three corner bins.
  for (i = 0; i < 3; ++i)
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
    // Now add the triangle to the geometry.
    for (i = 0; i < 3; i++)
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
      this->OutputTriangleArray->InsertNextCell(3, triPtIds);
      if (this->CopyCellData && this->GetInput())
        {
        this->GetOutput()->GetCellData()->CopyData(this->GetInput()->GetCellData(),
                                      this->InCellCount,
                                      this->OutCellCount++);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddEdges(vtkCellArray *edges, vtkPoints *points,
                                    int geometryFlag)
{
  int j;
  vtkIdType numCells, i;
  vtkIdType *ptIds, numPts;
  float *pt0, *pt1;
  vtkIdType binIds[2];

  // Add the edges to the error fuction.
  numCells = edges->GetNumberOfCells();
  edges->InitTraversal();
  for (i = 0; i < numCells; ++i)
    {
    edges->GetNextCell(numPts, ptIds);
    pt0 = points->GetPoint(ptIds[0]);
    binIds[0] = this->HashPoint(pt0);
    // This internal loop handles line strips.
    for (j = 1; j < numPts; ++j)
      {
      pt1 = points->GetPoint(ptIds[j]);
      binIds[1] = this->HashPoint(pt1);
      this->AddEdge(binIds, pt0, pt1, geometryFlag);
      ++this->InCellCount;
      pt0 = pt1;
      binIds[0] = binIds[1];
      }
    }
}
//----------------------------------------------------------------------------
// The error function is the square of the area of the triangle formed by the
// edge and the point.  We ignore constants across all terms.
// If geometryFlag is 1 then the edge is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddEdge(vtkIdType *binIds, float *pt0, float *pt1,
                                   int geometryFlag)
{
  int   i;
  vtkIdType edgePtIds[2];
  float length2, tmp;
  float d[3];
  float m[3];  // The mid point of the segement.(p1 or p2 could be used also).  
  float md;    // The dot product of m and d.
  float q[9];


  // Compute quadric for line segment
  // Line segment quadric is the area (squared) of the triangle (seg,pt).

  // Compute the direction vector of the segment.
  d[0] = pt1[0] - pt0[0];
  d[1] = pt1[1] - pt0[1];
  d[2] = pt1[2] - pt0[2];

  // Compute the length^2 of the line segement.
  length2 = d[0]*d[0] + d[1]*d[1] + d[2]*d[2];

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

  for (i = 0; i < 2; ++i)
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
    for (i = 0; i < 2; i++)
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
      if (this->CopyCellData && this->GetInput())
        {
        this->GetOutput()->GetCellData()->CopyData(this->GetInput()->GetCellData(),
                                      this->InCellCount,
                                      this->OutCellCount++);
        }
      }
    }
}


//----------------------------------------------------------------------------
void vtkQuadricClustering::AddVertices(vtkCellArray *verts, vtkPoints *points,
                                       int geometryFlag)
{
  int j;
  vtkIdType numCells, i;
  vtkIdType *ptIds, numPts;
  float *pt;
  vtkIdType binId;

  numCells = verts->GetNumberOfCells();
  verts->InitTraversal();
  for (i = 0; i < numCells; ++i)
    {
    verts->GetNextCell(numPts, ptIds);
    // Can there be poly vertices?
    for (j = 0; j < numPts; ++j)
      {
      pt = points->GetPoint(ptIds[j]);
      binId = this->HashPoint(pt);
      this->AddVertex(binId, pt, geometryFlag);
      ++this->InCellCount;
      }
    }
}
//----------------------------------------------------------------------------
// The error function is the length (point to vert) squared.
// We ignore constants across all terms.
// If geomertyFlag is 1 then the vert is added to the output.  Otherwise,
// only the quadric is affected.
void vtkQuadricClustering::AddVertex(vtkIdType binId, float *pt, 
                                     int geometryFlag)
{
  float q[9];

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

  // If the current quadric is from triangles, edges (or not initialized), then clear it out.
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
    int flag = 1;
    // Now add the vert to the geometry.
    // Get the vertex from the bin.
    if (this->QuadricArray[binId].VertexId == -1)
      {
      flag = 0;
      this->QuadricArray[binId].VertexId = this->NumberOfBinsUsed;
      this->NumberOfBinsUsed++;
      }
    }
}



//----------------------------------------------------------------------------
void vtkQuadricClustering::InitializeQuadric(float quadric[9])
{
  int i;
  
  for (i = 0; i < 9; i++)
    {
    quadric[i] = 0.0;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddQuadric(vtkIdType binId, float quadric[9])
{
  int i;
  
  for (i = 0; i < 9; i++)
    {
    this->QuadricArray[binId].Quadric[i] += (quadric[i] * 100000000.0);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkQuadricClustering::HashPoint(float point[3])
{
  vtkIdType binId;
  int xBinCoord, yBinCoord, zBinCoord;
  
  xBinCoord = int((point[0] - this->Bounds[0]) / this->XBinSize);
  if (xBinCoord < 0)
    {
    xBinCoord = 0;
    }
  if (xBinCoord >= this->NumberOfXDivisions)
    {
    xBinCoord = this->NumberOfXDivisions - 1;
    }

  yBinCoord = int((point[1] - this->Bounds[2]) / this->YBinSize);
  if (yBinCoord < 0)
    {
    yBinCoord = 0;
    }
  if (yBinCoord >= this->NumberOfYDivisions)
    {
    yBinCoord = this->NumberOfYDivisions - 1;
    }

  zBinCoord = int((point[2] - this->Bounds[4]) / this->ZBinSize);
  if (zBinCoord < 0)
    {
    zBinCoord = 0;
    }
  if (zBinCoord >= this->NumberOfZDivisions)
    {
    zBinCoord = this->NumberOfZDivisions - 1;
    }

  binId = xBinCoord * this->NumberOfYDivisions * this->NumberOfZDivisions + 
    yBinCoord * this->NumberOfZDivisions + zBinCoord;

  return binId;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppend()
{
  vtkIdType i, numBuckets, tenth;
  int abortExecute=0;
  vtkPoints *outputPoints;
  float newPt[3];
  vtkPolyData *output = this->GetOutput();
  
  // Check for mis use of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    //vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  outputPoints = vtkPoints::New();

  numBuckets = this->NumberOfXDivisions * this->NumberOfYDivisions * this->NumberOfZDivisions;
  tenth = numBuckets/10 + 1;
  for (i = 0; !abortExecute && i < numBuckets; i++ )
    {
    if ( ! (i % tenth) ) 
      {
      vtkDebugMacro(<<"Finding point in bin #" << i);
      this->UpdateProgress (0.8+0.2*i/numBuckets);
      abortExecute = this->GetAbortExecute();
      }

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

  this->EndAppendVertexGeometry(this->GetInput());

  // Tell the data is is up to date 
  // (in case the user calls this method directly).
  output->DataHasBeenGenerated();

  // Free the quadric array.
  if (this->QuadricArray)
    {
    delete [] this->QuadricArray;
    this->QuadricArray = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkQuadricClustering::ComputeRepresentativePoint(float quadric[9],
						      vtkIdType binId,
						      float point[3])
{
  int i, j;
  float A[3][3], U[3][3], UT[3][3], VT[3][3], V[3][3];
  float b[3], w[3];
  float W[3][3], tempMatrix[3][3];
  float cellCenter[3], tempVector[3];
  float cellBounds[6];
  int x, y, z;
  float quadric4x4[4][4];
      
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
  
  x = binId / (this->NumberOfYDivisions * this->NumberOfZDivisions);
  y = (binId - x * this->NumberOfYDivisions * this->NumberOfZDivisions) /
    this->NumberOfZDivisions;
  z = binId - this->NumberOfZDivisions * (x * this->NumberOfYDivisions + y);

  cellBounds[0] = this->Bounds[0] + x * this->XBinSize;
  cellBounds[1] = this->Bounds[0] + (x+1) * this->XBinSize;
  cellBounds[2] = this->Bounds[2] + y * this->YBinSize;
  cellBounds[3] = this->Bounds[2] + (y+1) * this->YBinSize;
  cellBounds[4] = this->Bounds[4] + z * this->ZBinSize;
  cellBounds[5] = this->Bounds[4] + (z+1) * this->ZBinSize;
  
  for (i = 0; i < 3; i++)
    {
    b[i] = -quadric4x4[3][i];
    cellCenter[i] = cellBounds[i*2] + (cellBounds[i*2+1] - cellBounds[i*2]) / 2.0;
    for (j = 0; j < 3; j++)
      {
      A[i][j] = quadric4x4[i][j];
      }
    }
  
#define VTK_SVTHRESHOLD 1E-2
  float invsig, maxW = 0.0;
  vtkMath::SingularValueDecomposition3x3(A, U, w, VT);
  for (i = 0; i < 3; i++)
    {
    if (w[i] > maxW)
      {
      maxW = w[i];
      }
    }
  for (i = 0; i < 3; i++)
    {
    for (j = 0; j < 3; j++)
      {
      if (i == j)
        {
        if ( (w[i] / maxW) > VTK_SVTHRESHOLD)
          {
          // If this is true, then w[i] != 0, so this division is ok.
          invsig = 1.0/w[i];
          W[i][j] = invsig;
          }
        else
          {
          W[i][j] = 0;
          }
        }
      else
        {
        W[i][j] = 0;
        }
      }
    }
  vtkMath::Transpose3x3(U, UT);
  vtkMath::Transpose3x3(VT, V);
  vtkMath::Multiply3x3(W, UT, tempMatrix);
  vtkMath::Multiply3x3(V, tempMatrix, tempMatrix);
  vtkMath::Multiply3x3(A, cellCenter, tempVector);
  for (i = 0; i < 3; i++)
    {
    tempVector[i] = b[i] - tempVector[i];
    }
  vtkMath::Multiply3x3(tempMatrix, tempVector, tempVector);
  for (i = 0; i < 3; i++)
    {
    point[i] = cellCenter[i] + tempVector[i];
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfDivisions(int div[3])
{
  this->SetNumberOfXDivisions(div[0]);
  this->SetNumberOfYDivisions(div[1]);
  this->SetNumberOfZDivisions(div[2]);
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::SetNumberOfXDivisions(int num)
{
  if (this->NumberOfXDivisions == num && this->ComputeNumberOfDivisions == 0)
    {
    return;
    }
  if (num < 2)
    {
    vtkErrorMacro("You cannot use less than two divisions.");
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
  if (num < 2)
    {
    vtkErrorMacro("You cannot use less than two divisions.");
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
  if (num < 2)
    {
    vtkErrorMacro("You cannot use less than two divisions.");
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
void vtkQuadricClustering::SetDivisionOrigin(float x, float y, float z)
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
void vtkQuadricClustering::SetDivisionSpacing(float x, float y, float z)
{
  if (this->ComputeNumberOfDivisions && this->DivisionSpacing[0] == x &&
      this->DivisionSpacing[1] == y && this->DivisionSpacing[2] == z)
    {
    return;
    }
  this->Modified();
  this->DivisionSpacing[0] = x;
  this->DivisionSpacing[1] = y;
  this->DivisionSpacing[2] = z;
  this->ComputeNumberOfDivisions = 1;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppendUsingPoints(vtkPolyData *input)
{
  vtkIdType   i;
  vtkIdType   outPtId;
  vtkPoints   *inputPoints;
  vtkPoints   *outputPoints;
  vtkPolyData *output = this->GetOutput();
  vtkIdType   numPoints, numBins;
  vtkIdType   binId;
  float       *minError, e, pt[3];
  float       *q;

  if (input == NULL || output == NULL)
    {
    return;
    }
  inputPoints = input->GetPoints();
  if (inputPoints == NULL)
    {
    return;
    }

  // Check for mis use of the Append methods.
  if (this->OutputTriangleArray == NULL || this->OutputLines == NULL)
    {
    //vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  outputPoints = vtkPoints::New();

  // Prepare to copy point data to output
  output->GetPointData()->CopyAllocate(input->GetPointData(), this->NumberOfBinsUsed);

  // Allocate and initialize an array to hold errors for each bin.
  numBins = this->NumberOfXDivisions * this->NumberOfYDivisions 
                  * this->NumberOfZDivisions;
  minError = new float[numBins];
  for (i = 0; i < numBins; ++i)
    {
    minError[i] = VTK_LARGE_FLOAT;
    }

  // Loop through the input points.
  numPoints = inputPoints->GetNumberOfPoints();
  for (i = 0; i < numPoints; ++i)
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

  this->EndAppendVertexGeometry(input);

  if (this->QuadricArray)
    {
    delete [] this->QuadricArray;
    this->QuadricArray = NULL;
    }

  delete minError;
}



//----------------------------------------------------------------------------
// This is not a perfect implementation, because it does not determine.
// Which vertex cell is the best for a bin.  The first detected is used.
void vtkQuadricClustering::EndAppendVertexGeometry(vtkPolyData *input)
{
  vtkCellArray *inVerts, *outVerts;
  vtkIdType *tmp = NULL;
  int        tmpLength = 0;
  int        tmpIdx;
  float *pt;
  int j;
  vtkIdType numCells, i;
  vtkIdType *ptIds, numPts;
  vtkIdType outPtId;
  vtkIdType binId, outCellId;

  inVerts = input->GetVerts();
  outVerts = vtkCellArray::New();

  numCells = inVerts->GetNumberOfCells();
  inVerts->InitTraversal();
  for (i = 0; i < numCells; ++i)
    {
    inVerts->GetNextCell(numPts, ptIds);
    if (tmpLength < numPts)
      {
      if (tmp)
        {
        delete tmp;
        tmp = NULL;
        }
      tmp = new vtkIdType[numPts];
      tmpLength = numPts;
      }
    tmpIdx = 0;
    for (j = 0; j < numPts; ++j)
      {
      pt = input->GetPoint(ptIds[j]);
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
      this->GetOutput()->GetCellData()->CopyData(input->GetCellData(),
                                      i, outCellId);
      }
    }

  if (tmp)
    {
    delete [] tmp;
    tmp = NULL;
    }

  if (outVerts->GetNumberOfCells() > 0)
    {
    this->GetOutput()->SetVerts(outVerts);
    }
  outVerts->Delete();
  outVerts = NULL;
}


//----------------------------------------------------------------------------
// This method is called after the execution, but before the vertex array
// is deleted. It changes some points to be based on the boundary edges.
void vtkQuadricClustering::AppendFeatureQuadrics(vtkPolyData *pd)
{
  vtkPolyData *input = vtkPolyData::New();
  vtkPoints *edgePts;
  vtkCellArray *edges;
  vtkIdType i;
  vtkIdType binId;
  float *featurePt;

  // Find the boundary edges.
  input->ShallowCopy(pd);
  this->FeatureEdges->SetInput(input);
  this->FeatureEdges->Update();
  edgePts = this->FeatureEdges->GetOutput()->GetPoints();
  edges = this->FeatureEdges->GetOutput()->GetLines();

  if (edges && edges->GetNumberOfCells() && edgePts)
    {
    this->AddEdges(edges, edgePts, 0);
    if (this->UseFeaturePoints)
      {
      this->FindFeaturePoints(edges, edgePts, this->FeaturePointsAngle);
      for (i = 0; i < this->FeaturePoints->GetNumberOfPoints(); i++)
        {
        featurePt = this->FeaturePoints->GetPoint(i);
        binId = this->HashPoint(featurePt);
        this->AddVertex(binId, featurePt, 0);
        }
      }
    }

  // Release data.
  this->FeatureEdges->SetInput(NULL);
  this->FeatureEdges->GetOutput()->ReleaseData();
  input->Delete();
}

// Find the feature points of a given set of edges.
// The points returned are (1) those used by only one edge, (2) those
// used by > 2 edges, and (3) those where the angle between 2 edges
// using this point is < angle.
void vtkQuadricClustering::FindFeaturePoints(vtkCellArray *edges,
                                             vtkPoints *edgePts,
                                             float vtkNotUsed(angle))
{
  vtkIdType i, pointIds[2];
  int j;
  vtkIdType *cellPts, numCellPts;
  vtkIdList *pointIdList = vtkIdList::New();
  vtkIdType numPts = edgePts->GetNumberOfPoints();
  vtkIdType numEdges = edges->GetNumberOfCells();
  vtkIdType edgeCount;
  vtkIdType **pointTable = new vtkIdType *[numPts];
  float featurePoint[3];
  float featureEdges[2][3];
  float point1[3], point2[3];
  vtkIdType *cellPointIds;
  float radAngle = vtkMath::DegreesToRadians() * this->FeaturePointsAngle;
  
  this->FeaturePoints->Allocate(numPts);
  
  for (i = 0; i < numPts; i++)
    {
    pointTable[i] = new vtkIdType[4];
    pointTable[i][1] = 0;
    }
  
  edges->InitTraversal();
  for (i = 0; i < numEdges; i++)
    {
    edges->GetNextCell(numCellPts, cellPts);
    for (j = 0; j < 2; j++)
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
  
  for (i = 0; i < numPts; i++)
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
      for (j = 0; j < 2; j++)
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
  pointIdList = NULL;
  for (i = 0; i < numPts; i++)
    {
    delete [] pointTable[i];
    pointTable[i] = NULL;
    }
  delete [] pointTable;
  pointTable = NULL;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1]
     << " " << this->Bounds[2] << " " << this->Bounds[3] << " "
     << this->Bounds[4] << " " << this->Bounds[5] << "\n";
  os << indent << "UseInputPoints " << this->UseInputPoints << "\n";

  if (this->ComputeNumberOfDivisions)
    {
    os << indent << "Using Spacing and Origin to setup bins\n";
    }
  else
    {
    os << indent << "Using input bounds and NumberOfDivisions to set up bins\n";
    }
  os << indent << "DivisionSpacing: " << this->DivisionSpacing[0] << ", " 
     << this->DivisionSpacing[1] << ", " << this->DivisionSpacing[2] << endl;
  os << indent << "DivisionOrigin: " << this->DivisionOrigin[0] << ", " 
     << this->DivisionOrigin[1] << ", " << this->DivisionOrigin[2] << endl;

  os << indent << "Number of X Divisions: " << this->NumberOfXDivisions
     << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfYDivisions
     << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfZDivisions
     << "\n";

  os << indent << "UseInternalTriangles: " << this->UseInternalTriangles << endl;

  os << indent << "UseFeatureEdges: " << this->UseFeatureEdges << endl;
  os << indent << "FeatureEdges: (" << this->FeatureEdges << ")\n";
  
  os << indent << "FeaturePointsAngle: " << this->FeaturePointsAngle << endl;
  os << indent << "UseFeaturePoints: " << this->UseFeaturePoints << endl;
  os << indent << "CopyCellData : " << this->CopyCellData << endl;
}
