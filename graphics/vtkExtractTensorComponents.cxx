/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTensorComponents.cxx
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
#include "vtkExtractTensorComponents.h"
#include "vtkMath.h"
#include "vtkScalars.h"
#include "vtkVectors.h"
#include "vtkNormals.h"
#include "vtkTCoords.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkExtractTensorComponents* vtkExtractTensorComponents::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractTensorComponents");
  if(ret)
    {
    return (vtkExtractTensorComponents*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractTensorComponents;
}




// Construct object to extract nothing and to not pass tensor data
// through the pipeline.
vtkExtractTensorComponents::vtkExtractTensorComponents()
{
  this->PassTensorsToOutput = 0;

  this->ExtractScalars = 0;
  this->ExtractVectors = 0;
  this->ExtractNormals = 0;
  this->ExtractTCoords = 0;

  this->ScalarMode = VTK_EXTRACT_COMPONENT;
  this->ScalarComponents[0] = this->ScalarComponents[1] = 0;

  this->VectorComponents[0] = 0; this->VectorComponents[1] = 0;
  this->VectorComponents[2] = 1; this->VectorComponents[3] = 0;
  this->VectorComponents[4] = 2; this->VectorComponents[5] = 0;

  this->NormalizeNormals = 1;
  this->NormalComponents[0] = 0; this->NormalComponents[1] = 1;
  this->NormalComponents[2] = 1; this->NormalComponents[3] = 1;
  this->NormalComponents[4] = 2; this->NormalComponents[5] = 1;

  this->NumberOfTCoords = 2;
  this->TCoordComponents[0] = 0; this->TCoordComponents[1] = 2;
  this->TCoordComponents[2] = 1; this->TCoordComponents[3] = 2;
  this->TCoordComponents[4] = 2; this->TCoordComponents[5] = 2;
}

// Extract data from tensors.
//
void vtkExtractTensorComponents::Execute()
{
  vtkTensors *inTensors;
  vtkTensor *tensor;
  vtkDataSet *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkPointData *outPD = this->GetOutput()->GetPointData();
  float s, v[3];
  vtkScalars *newScalars=NULL;
  vtkVectors *newVectors=NULL;
  vtkNormals *newNormals=NULL;
  vtkTCoords *newTCoords=NULL;
  int ptId, numPts;
  float sx, sy, sz, txy, tyz, txz;

  // Initialize
  //
  vtkDebugMacro(<<"Extracting vector components!");

  // First, copy the input to the output as a starting point
  this->GetOutput()->CopyStructure( input );

  inTensors = pd->GetTensors();
  numPts = input->GetNumberOfPoints();

  if ( !inTensors || numPts < 1 )
    {
    vtkErrorMacro(<<"No data to extract!");
    return;
    }

  if ( !this->ExtractScalars && !this->ExtractVectors && 
  !this->ExtractNormals && !this->ExtractTCoords )
    {
    vtkWarningMacro(<<"No data is being extracted");
    }

  outPD->CopyAllOn();
  if ( !this->PassTensorsToOutput )
    {
    outPD->CopyTensorsOff();
    }
  if ( this->ExtractScalars )
    {
    outPD->CopyScalarsOff();
    newScalars = vtkScalars::New();
    newScalars->SetNumberOfScalars(numPts);
    }
  if ( this->ExtractVectors ) 
    {
    outPD->CopyVectorsOff();
    newVectors = vtkVectors::New();
    newVectors->SetNumberOfVectors(numPts);
    }
  if ( this->ExtractNormals ) 
    {
    outPD->CopyNormalsOff();
    newNormals = vtkNormals::New();
    newNormals->SetNumberOfNormals(numPts);
    }
  if ( this->ExtractTCoords ) 
    {
    outPD->CopyTCoordsOff();
    newTCoords = vtkTCoords::New();
    newTCoords->SetNumberOfTCoords(numPts);
    }
  outPD->PassData(pd);

  // Loop over all points extracting components of tensor
  //
  for (ptId=0; ptId < numPts; ptId++)
    {
    tensor = inTensors->GetTensor(ptId);

    if ( this->ExtractScalars )
      {
      if ( this->ScalarMode == VTK_EXTRACT_EFFECTIVE_STRESS )
        {
        sx = tensor->GetComponent(0,0);
        sy = tensor->GetComponent(1,1);
        sz = tensor->GetComponent(2,2);
        txy = tensor->GetComponent(0,1);
        tyz = tensor->GetComponent(1,2);
        txz = tensor->GetComponent(0,2);

        s = sqrt (0.16666667 * ((sx-sy)*(sx-sy) + (sy-sz)*(sy-sz) + (sz-sx)*(sz-sx) + 
                                6.0*(txy*txy + tyz*tyz + txz*txz)));
        }

      else if ( this->ScalarMode == VTK_EXTRACT_COMPONENT )
        {
        s = tensor->GetComponent(this->ScalarComponents[0],this->ScalarComponents[1]);
        }

      else //VTK_EXTRACT_EFFECTIVE_DETERMINANT
        {
        }
      newScalars->SetScalar(ptId, s);
      }//if extract scalars

    if ( this->ExtractVectors ) 
      {
      v[0] = tensor->GetComponent(this->VectorComponents[0],this->VectorComponents[1]);
      v[1] = tensor->GetComponent(this->VectorComponents[2],this->VectorComponents[3]);
      v[2] = tensor->GetComponent(this->VectorComponents[4],this->VectorComponents[5]);
      newVectors->SetVector(ptId, v);
      }

    if ( this->ExtractNormals ) 
      {
      v[0] = tensor->GetComponent(this->NormalComponents[0],this->NormalComponents[1]);
      v[1] = tensor->GetComponent(this->NormalComponents[2],this->NormalComponents[3]);
      v[2] = tensor->GetComponent(this->NormalComponents[4],this->NormalComponents[5]);
      newNormals->SetNormal(ptId, v);
      }

    if ( this->ExtractTCoords ) 
      {
      for ( int i=0; i < this->NumberOfTCoords; i++ )
        {
        v[i] = tensor->GetComponent(this->TCoordComponents[2*i],this->TCoordComponents[2*i+1]);
        }
      newTCoords->SetTCoord(ptId, v);
      }

    }//for all points

  // Send data to output
  //
  if ( this->ExtractScalars )
    {
    outPD->SetScalars(newScalars);
    newScalars->Delete();
    }
  if ( this->ExtractVectors ) 
    {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
    }
  if ( this->ExtractNormals ) 
    {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
    }
  if ( this->ExtractTCoords ) 
    {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
    }
}

void vtkExtractTensorComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Pass Tensors To Output: " << (this->PassTensorsToOutput ? "On\n" : "Off\n");

  os << indent << "Extract Scalars: " << (this->ExtractScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Extraction Mode: ";

  if ( this->ScalarMode == VTK_EXTRACT_COMPONENT )
    {
    os << "VTK_EXTRACT_COMPONENT\n";
    }
  else if ( this->ScalarMode == VTK_EXTRACT_EFFECTIVE_STRESS )
    {
    os << "VTK_EXTRACT_EFFECTIVE_STRESS\n";
    }
  else
    {
    os << "VTK_EXTRACT_DETERMINANT\n";
    }

  os << indent << "Scalar Components: \n";
  os << indent << "  (row,column): (" 
     << this->ScalarComponents[0] << ", " << this->ScalarComponents[1] << ")\n";

  os << indent << "Extract Vectors: " << (this->ExtractVectors ? "On\n" : "Off\n");
  os << indent << "Vector Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->VectorComponents[0] << ", " << this->VectorComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->VectorComponents[2] << ", " << this->VectorComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->VectorComponents[4] << ", " << this->VectorComponents[5] << ")\n";

  os << indent << "Extract Normals: " << (this->ExtractNormals ? "On\n" : "Off\n");
  os << indent << "Normalize Normals: " << (this->NormalizeNormals ? "On\n" : "Off\n");
  os << indent << "Normal Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->NormalComponents[0] << ", " << this->NormalComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->NormalComponents[2] << ", " << this->NormalComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->NormalComponents[4] << ", " << this->NormalComponents[5] << ")\n";

  os << indent << "Extract TCoords: " << (this->ExtractTCoords ? "On\n" : "Off\n");
  os << indent << "Number Of TCoords: (" << this->NumberOfTCoords << ")\n";
  os << indent << "TCoord Components: \n";
  os << indent << "  (row,column)0: (" 
     << this->TCoordComponents[0] << ", " << this->TCoordComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" 
     << this->TCoordComponents[2] << ", " << this->TCoordComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" 
     << this->TCoordComponents[4] << ", " << this->TCoordComponents[5] << ")\n";

}

