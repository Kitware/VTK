/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricClustering.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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

  this->UseInputPoints = 0;

  this->OutputTriangleArray = NULL;

  // Overide superclass so that append can be called directly.
  this->NumberOfRequiredInputs = 0;
}

//----------------------------------------------------------------------------
vtkQuadricClustering::~vtkQuadricClustering()
{
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
  int i;

  // Check for conditions that can occur if the Append methods 
  // are not called in the correct order.
  if (this->OutputTriangleArray)
    {
    this->OutputTriangleArray->Delete();
    this->OutputTriangleArray = NULL;
    vtkWarningMacro("Array already created.  Did you call EndAppend?");
    }

  this->OutputTriangleArray = vtkCellArray::New();

  // Copy over the bounds.
  for (i = 0; i < 6; ++i)
    {
    this->Bounds[i]= bounds[i];
    }

  this->XBinSize = (this->Bounds[1]-this->Bounds[0])/this->NumberOfXDivisions;
  this->YBinSize = (this->Bounds[3]-this->Bounds[2])/this->NumberOfYDivisions;
  this->ZBinSize = (this->Bounds[5]-this->Bounds[4])/this->NumberOfZDivisions;   

  this->NumberOfBinsUsed = 0;
  this->QuadricArray = new VTK_POINT_QUADRIC[this->NumberOfXDivisions *
                                            this->NumberOfYDivisions *
                                            this->NumberOfZDivisions];
  if (this->QuadricArray == NULL)
    {
    vtkErrorMacro("Could not allocate quadric grid.");
    return;
    }
  for (i = 0; i < this->NumberOfXDivisions * 
         this->NumberOfYDivisions * this->NumberOfZDivisions; i++)
    {
    this->QuadricArray[i].VertexId = -1;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Append(vtkPolyData *pd)
{
  vtkCellArray *inputTris = pd->GetPolys();
  int numTris = pd->GetNumberOfPolys(); // assuming we only have triangles
  vtkPoints *inputPoints = pd->GetPoints();
  int *cellPtIds;
  float triPts[3][3], quadric[9], quadric4x4[4][4];
  int i, j, numPts, binIds[3];
  int triPtIds[3];
  int vertexId, abort=0, tenth;
  
  // Check for mis-use of the Append methods.
  if (this->OutputTriangleArray == NULL)
    {
    vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

  if ( !numTris )
    {
    return;
    }      
  
  tenth = numTris/10 + 1;
  inputTris->InitTraversal();
  for (i = 0; i < numTris && !abort ; i++)
    {
    if ( !(i % tenth) ) 
      {
      vtkDebugMacro(<<"Visiting triangle #" << i);
      this->UpdateProgress(0.8 * i/numTris);
      abort = this->GetAbortExecute();
      }
    inputTris->GetNextCell(numPts, cellPtIds);
    for (j = 0; j < 3; j++)
      {
      inputPoints->GetPoint(cellPtIds[j], triPts[j]);
      binIds[j] = this->HashPoint(triPts[j]);
      }
    vtkTriangle::ComputeQuadric(triPts[0], triPts[1], triPts[2], quadric4x4);
    for (j = 0; j < 3; j++)
      {
      if (this->QuadricArray[binIds[j]].VertexId == -1)
        {
        this->InitializeQuadric(this->QuadricArray[binIds[j]].Quadric);
        vertexId = this->NumberOfBinsUsed;
        this->QuadricArray[binIds[j]].VertexId = vertexId;
        this->NumberOfBinsUsed++;
        }
      else
        {
        vertexId = this->QuadricArray[binIds[j]].VertexId;
        }
      triPtIds[j] = vertexId;
      quadric[0] = quadric4x4[0][0];
      quadric[1] = quadric4x4[0][1];
      quadric[2] = quadric4x4[0][2];
      quadric[3] = quadric4x4[0][3];
      quadric[4] = quadric4x4[1][1];
      quadric[5] = quadric4x4[1][2];
      quadric[6] = quadric4x4[1][3];
      quadric[7] = quadric4x4[2][2];
      quadric[8] = quadric4x4[2][3];
      this->AddQuadric(binIds[j], quadric);
      }
    if (binIds[0] != binIds[1] && binIds[0] != binIds[2] &&
      	binIds[1] != binIds[2])
      {
      this->OutputTriangleArray->InsertNextCell(3, triPtIds);
      }
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppend()
{
  int i, abortExecute=0, tenth, numBuckets;
  vtkPoints *outputPoints = vtkPoints::New();
  float newPt[3];
  vtkPolyData *output = this->GetOutput();
  
  // Check for mis use of the Append methods.
  if (this->OutputTriangleArray == NULL)
    {
    vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

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
  
  output->SetPolys(this->OutputTriangleArray);
  output->SetPoints(outputPoints);
  outputPoints->Delete();
  delete [] this->QuadricArray;
  this->QuadricArray = NULL;

  this->OutputTriangleArray->Delete();
  this->OutputTriangleArray = NULL;

  // Tell the data is is up to date 
  // (in case the user calls this method directly).
  output->DataHasBeenGenerated();
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
void vtkQuadricClustering::AddQuadric(int binId, float quadric[9])
{
  int i;
  
  for (i = 0; i < 9; i++)
    {
    this->QuadricArray[binId].Quadric[i] += (quadric[i] * 100000000.0);
    }
}

//----------------------------------------------------------------------------
int vtkQuadricClustering::HashPoint(float point[3])
{
  int xBinCoord, yBinCoord, zBinCoord, binId;
  
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
void vtkQuadricClustering::ComputeRepresentativePoint(float quadric[9],
						      int binId,
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
  vtkMath::Multiply3x3(W, V, tempMatrix);
  vtkMath::Multiply3x3(UT, tempMatrix, tempMatrix);
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
void vtkQuadricClustering::SetNumberOfDivisions(int divs[3])
{
  this->SetNumberOfXDivisions(divs[0]);
  this->SetNumberOfYDivisions(divs[1]);
  this->SetNumberOfZDivisions(divs[2]);
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
void vtkQuadricClustering::EndAppendUsingPoints(vtkPolyData *input)
{
  int         i, outPtId;
  vtkPoints   *inputPoints = input->GetPoints();
  vtkPoints   *outputPoints = vtkPoints::New();
  vtkPolyData *output = this->GetOutput();
  int         numPoints, numBins, binId;
  float       *minError, e, pt[3];
  float       *q;

  
  // Check for mis use of the Append methods.
  if (this->OutputTriangleArray == NULL)
    {
    vtkErrorMacro("Missing Array:  Did you call StartAppend?");
    return;
    }

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
      vtkErrorMacro("Point hash mismatch.");
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
  delete [] this->QuadricArray;
  this->QuadricArray = NULL;

  this->OutputTriangleArray->Delete();
  this->OutputTriangleArray = NULL;

  delete minError;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1]
     << " " << this->Bounds[2] << " " << this->Bounds[3] << " "
     << this->Bounds[4] << " " << this->Bounds[5] << "\n";
  os << indent << "UseInputPoints " << this->UseInputPoints << "\n";
  os << indent << "Number of X Divisions: " << this->NumberOfXDivisions
     << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfYDivisions
     << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfZDivisions
     << "\n";
}
