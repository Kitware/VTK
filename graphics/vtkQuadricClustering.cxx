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
  this->NumberOfXDivisions = 0;
  this->NumberOfYDivisions = 0;
  this->NumberOfZDivisions = 0;
  this->QuadricArray = NULL;
  this->BinIds = vtkIdList::New();
}

//----------------------------------------------------------------------------
vtkQuadricClustering::~vtkQuadricClustering()
{
  this->BinIds->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkCellArray *outputTris = vtkCellArray::New();
  vtkCellArray *inputTris = input->GetPolys();
  int numTris = input->GetNumberOfPolys(); // assuming we only have triangles
  int i, j, numPts, binIds[3];
  int *cellPtIds;
  vtkPoints *inputPoints = input->GetPoints();
  vtkPoints *outputPoints = vtkPoints::New();
  float triPts[3][3], quadric[4][4];
  vtkIdList *triPtIds = vtkIdList::New();
  float newPt[3];
  int vertexId;
  
  if (input == NULL)
    {
    vtkErrorMacro("No input");
    return;
    }
  if (this->NumberOfXDivisions <= 0 || this->NumberOfYDivisions <= 0 ||
      this->NumberOfZDivisions <= 0)
    {
    vtkErrorMacro("Number of divisions in X, Y, and Z dimensions must be set");
    return;
    }
  if (numTris == 0)
    {
    vtkErrorMacro("No triangles to decimate");
    return;
    }

  this->QuadricArray = new POINT_QUADRIC[this->NumberOfXDivisions *
					this->NumberOfYDivisions *
					this->NumberOfZDivisions];
  this->SetBounds(input->GetBounds());
  this->SetXBinSize((this->Bounds[1] - this->Bounds[0]) /
		    this->NumberOfXDivisions);
  this->SetYBinSize((this->Bounds[3] - this->Bounds[2]) /
		    this->NumberOfYDivisions);
  this->SetZBinSize((this->Bounds[5] - this->Bounds[4]) /
		    this->NumberOfZDivisions);
  
  inputTris->InitTraversal();
  for (i = 0; i < numTris; i++)
    {
    inputTris->GetNextCell(numPts, cellPtIds);
    for (j = 0; j < 3; j++)
      {
      inputPoints->GetPoint(cellPtIds[j], triPts[j]);
      binIds[j] = this->HashPoint(triPts[j]);
      }
    if (binIds[0] == binIds[1] || binIds[0] == binIds[2] ||
      	binIds[1] == binIds[2])
      {
      continue;
      }
    else
      {
      vtkTriangle::ComputeQuadric(triPts[0], triPts[1], triPts[2], quadric);
      for (j = 0; j < 3; j++)
        {
        if ((vertexId = this->BinIds->IsId(binIds[j])) == -1)
          {
          this->InitializeQuadric(this->QuadricArray[binIds[j]].Quadric);
          vertexId = this->BinIds->InsertNextId(binIds[j]);
          }
	triPtIds->InsertId(j, vertexId);
	this->QuadricArray[binIds[j]].VertexId = vertexId;
        this->AddQuadric(binIds[j], quadric);
        }
      outputTris->InsertNextCell(triPtIds);
      }
    }
  for (i = 0; i < this->BinIds->GetNumberOfIds(); i++)
    {
    this->ComputeRepresentativePoint(
      this->QuadricArray[this->BinIds->GetId(i)].Quadric,
      this->BinIds->GetId(i), newPt);
    outputPoints->InsertPoint(
      this->QuadricArray[this->BinIds->GetId(i)].VertexId, newPt);
    }
  
  output->SetPolys(outputTris);
  output->SetPoints(outputPoints);
  
  outputTris->Delete();
  triPtIds->Delete();
  outputPoints->Delete();
  delete [] this->QuadricArray;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::InitializeQuadric(float quadric[4][4])
{
  int i, j;
  
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      quadric[i][j] = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddQuadric(int binId, float quadric[4][4])
{
  int i, j;
  
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      this->QuadricArray[binId].Quadric[i][j] += quadric[i][j];
      }
    }
}

//----------------------------------------------------------------------------
int vtkQuadricClustering::HashPoint(float point[3])
{
  int xBinCoord, yBinCoord, zBinCoord, binId;
  
  if (point[0] < this->Bounds[1])
    {
    xBinCoord = int((point[0] - this->Bounds[0]) / this->XBinSize);
    }
  else
    {
    xBinCoord = this->NumberOfXDivisions - 1;
    }
  if (point[1] < this->Bounds[3])
    {
    yBinCoord = int((point[1] - this->Bounds[2]) / this->YBinSize);
    }
  else
    {
    yBinCoord = this->NumberOfYDivisions - 1;
    }
  if (point[2] < this->Bounds[5])
    {
    zBinCoord = int((point[2] - this->Bounds[4]) / this->ZBinSize);
    }
  else
    {
    zBinCoord = this->NumberOfZDivisions - 1;
    }
  
  binId = xBinCoord * this->NumberOfYDivisions * this->NumberOfZDivisions + 
    yBinCoord * this->NumberOfZDivisions + zBinCoord;
  
  return binId;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::ComputeRepresentativePoint(float quadric[4][4],
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
    b[i] = -quadric[3][i];
    cellCenter[i] = cellBounds[i*2] + (cellBounds[i*2+1] - cellBounds[i*2]) / 2.0;
    for (j = 0; j < 3; j++)
      {
      A[i][j] = quadric[i][j];
      }
    }
  
#define SVTHRESHOLD 1E-3
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
	if ( (w[i] / maxW) > SVTHRESHOLD)
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
void vtkQuadricClustering::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1]
     << " " << this->Bounds[2] << " " << this->Bounds[3] << " "
     << this->Bounds[4] << " " << this->Bounds[5] << "\n";
  os << indent << "Number of X Divisions: " << this->NumberOfXDivisions
     << "\n";
  os << indent << "Number of Y Divisions: " << this->NumberOfYDivisions
     << "\n";
  os << indent << "Number of Z Divisions: " << this->NumberOfZDivisions
     << "\n";
}
