/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatedVelocityField.cxx
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

#include "vtkInterpolatedVelocityField.h"
#include "vtkObjectFactory.h"

vtkInterpolatedVelocityField* vtkInterpolatedVelocityField::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInterpolatedVelocityField");
  if(ret)
    {
    return (vtkInterpolatedVelocityField*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInterpolatedVelocityField;
}

vtkInterpolatedVelocityField::vtkInterpolatedVelocityField()
{
  this->NumFuncs = 3; // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->DataSet = 0;
  this->Weights = 0;
  this->GenCell = vtkGenericCell::New();
  this->LastCellId = -1;
  this->CacheHit = 0;
  this->CacheMiss = 0;
  this->Caching = 1; // Caching on by default

  this->Cell = vtkGenericCell::New();
}

vtkInterpolatedVelocityField::~vtkInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetDataSet(0);
  this->GenCell->Delete();
  delete[] this->Weights;
  this->Weights = 0;

  this->Cell->Delete();
}

void vtkInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkFunctionSet::PrintSelf(os, indent);
  if ( this->DataSet )
    {
    os << indent << "Data Set: " << this->DataSet << endl;
    }
  else
    {
    os << indent << "Data Set: (none)" << endl;
    }
  if ( this->GenCell )
    {
    os << indent << "Last cell: " << this->GenCell << endl;
    }
  else
    {
    os << indent << "Last cell: (none)" << endl;
    }
  os << indent << "Weights: " << this->Weights << endl;
  os << indent << "Last cell Id: " << this->LastCellId << endl;
  os << indent << "Cache hit: " << this->CacheHit << endl;
  os << indent << "Cache miss: " << this->CacheMiss << endl;
  os << indent << "Caching: ";
  if ( this->Caching )
    {
    os << "on." << endl;
    }
  else
    {
    os << "off." << endl;
    }

}

// Evaluate u,v,w at x,y,z,t
int vtkInterpolatedVelocityField::FunctionValues(float* x, float* f)
{
  int i, j, subId , numPts, id;
  vtkDataArray* vectors;
  float vec[3];
  float dist2;
  int ret;

  for(i=0; i<3; i++)
    {
    f[i] = 0;
    }

  // See if a dataset has been specified and if there are input vectors
  if ( !this->DataSet || 
       !(vectors = this->DataSet->GetPointData()->GetVectors()) )
    {
    vtkErrorMacro(<<"Can't evaluate dataset!");
    return 0;
    }

  if (this->Caching)
    {
    // See if the point is in the cached cell
    if (this->LastCellId == -1 || 
	!(ret=this->GenCell->EvaluatePosition(x, 0, subId,
					      this->LastPCoords, dist2, 
					      this->Weights))
	|| ret == -1)
      {
      // if not, find and get it
	if (this->LastCellId != - 1 )
	  {
	  this->DataSet->GetCell(this->LastCellId, this->Cell);

	  this->LastCellId = 
	    this->DataSet->FindCell(x, this->Cell, this->GenCell, -1, 0, 
				    subId, this->LastPCoords, this->Weights);
	  if (this->LastCellId != - 1)
	    {
	    this->DataSet->GetCell(this->LastCellId, this->GenCell);
	    }
	  else
	    {
	    return 0;
	    }
	  this->CacheMiss++;
	  }
	else
	  {
	  return 0;
	  }
      }
    else
      {
      this->CacheHit++;
      }
    }
  else
    {
    // if caching is off, find the cell and get it
    this->LastCellId = 
      this->DataSet->FindCell(x, 0, this->GenCell, -1, 0, 
			      subId, this->LastPCoords, this->Weights);
    this->DataSet->GetCell(this->LastCellId, this->GenCell);
    }

  // if the cell is valid
  if (this->LastCellId >= 0)
    {
    numPts = this->GenCell->GetNumberOfPoints();
    // interpolate the vectors
    for (j=0; j < numPts; j++)
      {
      id = this->GenCell->PointIds->GetId(j);
      vectors->GetTuple(id, vec);
      for (i=0; i < 3; i++)
	{
	f[i] +=  vec[i] * this->Weights[j];
	}
      }
    }
  // if not, return false
  else
    {
    return 0;
    }

  return 1;
}

void vtkInterpolatedVelocityField::SetDataSet(vtkDataSet* dataset)
{
  if (this->DataSet != dataset)
    {
    if (this->DataSet != 0) { this->DataSet->UnRegister(this); }
    this->DataSet = dataset;
    if (this->DataSet != 0) { this->DataSet->Register(this); }
    this->Modified();
    delete[] this->Weights;
    this->Weights = 0; 
    // Allocate space for the largest cell
    if (this->DataSet != 0) 
      {
      this->Weights = new float[this->DataSet->GetMaxCellSize()];
      }
    }
}


int vtkInterpolatedVelocityField::GetLastWeights(float* w)
{
  int j, numPts;

  // If last cell is valid, fill w with the interpolation weights
  // and return true
  if (this->LastCellId >= 0)
    {
    numPts = this->GenCell->GetNumberOfPoints();
    for (j=0; j < numPts; j++)
      {
      w[j] = this->Weights[j];
      }
    return 1;
    }
  // otherwise, return false
  else
    {
    return 0;
    }
}

int vtkInterpolatedVelocityField::GetLastLocalCoordinates(float pcoords[3])
{
  int j;

  // If last cell is valid, fill p with the local coordinates
  // and return true
  if (this->LastCellId >= 0)
    {
    for (j=0; j < 3; j++)
      {
      pcoords[j] = this->LastPCoords[j];
      }
    return 1;
    }
  // otherwise, return false
  else
    {
    return 0;
    }
}


  














