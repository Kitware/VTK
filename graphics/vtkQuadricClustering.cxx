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
  this->NumberOfXDivisions = 50;
  this->NumberOfYDivisions = 50;
  this->NumberOfZDivisions = 50;
  this->QuadricArray = NULL;
  this->NumberOfBinsUsed = 0;
  this->Log = vtkTimerLog::New();
  this->BinSizeSet = 0;
  this->OutputTriangles = vtkCellArray::New();
  this->InputList = NULL;
  this->NumberOfExpectedInputs = 1;
  this->AbortExecute = 0;
}

//----------------------------------------------------------------------------
vtkQuadricClustering::~vtkQuadricClustering()
{
  this->Log->Delete();
  this->OutputTriangles->Delete();
  if (this->InputList)
    {
    this->InputList->Delete();
    this->InputList = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::Execute()
{
  if (Debug)
    {
    this->Log->StartTimer();
    }

  if (this->GetInput() == NULL)
    {
    // The user may be calling StartAppend, Append, and EndAppend explicitly.
    return;
    }
  
  int numInputs = this->GetNumberOfInputs();
  int i;
  
  this->NumberOfExpectedInputs = this->GetNumberOfInputs();

  this->StartAppend();
  for (i = 0; i < numInputs; i++)
    {
    this->Append(this->GetInput(i));
    }
  this->EndAppend();
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::UpdateData(vtkDataObject *output)
{
  if (this->GetInput() == NULL)
    {
    // we do not want to release the data because user might
    // have called Append ...
    return;
    }

  this->vtkPolyDataToPolyDataFilter::UpdateData( output );
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::StartAppend()
{
  int i;
  
  this->QuadricArray = new VTK_POINT_QUADRIC[this->NumberOfXDivisions *
                                            this->NumberOfYDivisions *
                                            this->NumberOfZDivisions];
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
  vtkIdList *triPtIds = vtkIdList::New();
  int vertexId;
  
  if (numTris== 0)
    {
    vtkErrorMacro("No triangles to decimate");
    return;
    }
  
  if (this->Bounds[0] == 0 && this->Bounds[1] == 0 && this->Bounds[2] == 0 &&
      this->Bounds[3] == 0 && this->Bounds[4] == 0 && this->Bounds[5] == 0)
    {
    // Set the bounds from the input if they haven't been set already.  This is
    // to allow the user to set the bounds larger than the bounds of the input.
    // If the user sets the bounds smaller than those of the input, then all
    // the points outside the bounds will fall in the outermost bins.
    this->SetBounds(pd->GetBounds());
    }
    
  if (!this->BinSizeSet)
    {
    this->SetXBinSize((this->Bounds[1] - this->Bounds[0]) /
                      this->NumberOfXDivisions);
    this->SetYBinSize((this->Bounds[3] - this->Bounds[2]) /
                      this->NumberOfYDivisions);
    this->SetZBinSize((this->Bounds[5] - this->Bounds[4]) /
                      this->NumberOfZDivisions);
    this->BinSizeSet = 1;
    }
  
  inputTris->InitTraversal();
  
  for (i = 0; i < numTris && !this->AbortExecute ; i++)
    {
    if ( ! (i % 10000) ) 
      {
      vtkDebugMacro(<<"Visiting polygon #" << i);
      this->UpdateProgress(0.8/this->NumberOfExpectedInputs * i/numTris);
      if (this->GetAbortExecute())
        {
        this->AbortExecute = 1;
        break;
        }
      }
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
	triPtIds->InsertId(j, vertexId);
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
      this->OutputTriangles->InsertNextCell(triPtIds);
      }
    }
  
  triPtIds->Delete();
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::EndAppend()
{
  int i;
  vtkPoints *outputPoints = vtkPoints::New();
  float newPt[3];
  vtkPolyData *output = this->GetOutput();
  
  for (i = 0; i < this->NumberOfXDivisions *
         this->NumberOfYDivisions * this->NumberOfZDivisions &&
         !this->AbortExecute; i++)
    {
    if ( ! (i % 1000) ) 
      {
      vtkDebugMacro(<<"Finding point in bin #" << i);
      this->UpdateProgress (0.2*i/this->NumberOfXDivisions *
        this->NumberOfYDivisions * this->NumberOfZDivisions);
      if (this->GetAbortExecute())
        {
        this->AbortExecute = 1;
        break;
        }
      }

    if (this->QuadricArray[i].VertexId != -1)
      {
      this->ComputeRepresentativePoint(this->QuadricArray[i].Quadric, i,
                                       newPt);
      outputPoints->InsertPoint(this->QuadricArray[i].VertexId, newPt);
      }
    }
  
  output->SetPolys(this->OutputTriangles);
  output->SetPoints(outputPoints);
  outputPoints->Delete();
  delete [] this->QuadricArray;

  if ( Debug )
    {
    this->Log->StopTimer();
    vtkDebugMacro(<<"Execution took: "<<this->Log->GetElapsedTime()<<" seconds.");
    }  
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::AddInput(vtkPolyData *input)
{
  this->vtkProcessObject::AddInput(input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkQuadricClustering::GetInput(int idx)
{
  if (idx >= this->NumberOfInputs || idx < 0)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Inputs[idx]);
}

//----------------------------------------------------------------------------
// Remove a piece of a dataset from the list to decimate.
void vtkQuadricClustering::RemoveInput(vtkPolyData *pd)
{
  this->vtkProcessObject::RemoveInput(pd);
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkQuadricClustering::GetInputList()
{
  int idx;
  
  if (this->InputList)
    {
    this->InputList->Delete();
    }
  this->InputList = vtkDataSetCollection::New();
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    if (this->Inputs[idx] != NULL)
      {
      this->InputList->AddItem((vtkDataSet*)(this->Inputs[idx]));
      }
    }  
  
  return this->InputList;
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
    this->QuadricArray[binId].Quadric[i] += quadric[i];
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
  
#define VTK_SVTHRESHOLD 1E-3
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
void vtkQuadricClustering::SetNumberOfDivisions(int div[3])
{
  this->SetNumberOfXDivisions(div[0]);
  this->SetNumberOfYDivisions(div[1]);
  this->SetNumberOfZDivisions(div[2]);
}

//----------------------------------------------------------------------------
int *vtkQuadricClustering::GetNumberOfDivisions()
{
  static int div[3];
  this->GetNumberOfDivisions(div);
  return div;
}

//----------------------------------------------------------------------------
void vtkQuadricClustering::GetNumberOfDivisions(int div[3])
{
  div[0] = this->NumberOfXDivisions;
  div[1] = this->NumberOfYDivisions;
  div[2] = this->NumberOfZDivisions;
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
  os << indent << "Number of Expected Inputs: "
     << this->NumberOfExpectedInputs << "\n";
}
