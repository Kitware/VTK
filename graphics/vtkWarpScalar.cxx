/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalar.cxx
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
#include "vtkWarpScalar.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkWarpScalar* vtkWarpScalar::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWarpScalar");
  if(ret)
    {
    return (vtkWarpScalar*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWarpScalar;
}

vtkWarpScalar::vtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;
}

float *vtkWarpScalar::DataNormal(vtkIdType id, vtkNormals *normals)
{
  return normals->GetNormal(id);
}

float *vtkWarpScalar::InstanceNormal(vtkIdType vtkNotUsed(id), 
				     vtkNormals *vtkNotUsed(normals))
{
  return this->Normal;
}

float *vtkWarpScalar::ZNormal(vtkIdType vtkNotUsed(id), 
			      vtkNormals *vtkNotUsed(normals))
{
  static float zNormal[3]={0.0,0.0,1.0};
  return zNormal;
}

void vtkWarpScalar::Execute()
{
  vtkPoints *inPts;
  vtkNormals *inNormals;
  vtkScalars *inScalars;
  vtkPoints *newPts;
  vtkPointData *pd;
  int i;
  vtkIdType ptId, numPts;
  float *x, *n, s, newX[3];
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
  
  vtkDebugMacro(<<"Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inNormals = pd->GetNormals();
  inScalars = pd->GetScalars();

  if ( !inPts || !inScalars )
    {
    vtkErrorMacro(<<"No data to warp");
    return;
    }

  numPts = inPts->GetNumberOfPoints();

  if ( inNormals && !this->UseNormal )
    {
    this->PointNormal = &vtkWarpScalar::DataNormal;
    vtkDebugMacro(<<"Using data normals");
    }
  else if ( this->XYPlane )
    {
    this->PointNormal = &vtkWarpScalar::ZNormal;
    vtkDebugMacro(<<"Using x-y plane normal");
    }
  else
    {
    this->PointNormal = &vtkWarpScalar::InstanceNormal;
    vtkDebugMacro(<<"Using Normal instance variable");
    }

  newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( ! (ptId % 10000) ) 
      {
      this->UpdateProgress ((float)ptId/numPts);
      if (this->GetAbortExecute())
	{
	break;
	}
      }

    x = inPts->GetPoint(ptId);
    n = (this->*(this->PointNormal))(ptId,inNormals);
    if ( this->XYPlane )
      {
      s = x[2];
      }
    else
      {
      s = inScalars->GetScalar(ptId);
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    newPts->SetPoint(ptId, newX);
    }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry 
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry 
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  newPts->Delete();
}

void vtkWarpScalar::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " 
     << this->Normal[1] << ", " << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
}
