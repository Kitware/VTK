/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDerivatives.cxx
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
#include <math.h>
#include "vtkCellDerivatives.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCellDerivatives* vtkCellDerivatives::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCellDerivatives");
  if(ret)
    {
    return (vtkCellDerivatives*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCellDerivatives;
}




vtkCellDerivatives::vtkCellDerivatives()
{
  this->VectorMode = VTK_VECTOR_MODE_COMPUTE_GRADIENT;
  this->TensorMode = VTK_TENSOR_MODE_COMPUTE_GRADIENT;
}

void vtkCellDerivatives::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkScalars *inScalars=pd->GetScalars();
  vtkVectors *inVectors=pd->GetVectors();
  vtkVectors *outVectors=NULL;
  vtkTensors *outTensors=NULL;
  int numCells=input->GetNumberOfCells();
  int computeScalarDerivs=1, computeVectorDerivs=1, subId;

  // Initialize
  vtkDebugMacro(<<"Computing cell derivatives");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Check input
  if ( numCells < 1 )
    {
    vtkErrorMacro("No cells to generate derivatives from");
    return;
    }

  // Figure out what to compute
  if ( !inScalars || this->VectorMode == VTK_VECTOR_MODE_PASS_VECTORS )
    {
    computeScalarDerivs = 0;
    }
  else
    {
    if ( this->VectorMode == VTK_VECTOR_MODE_COMPUTE_VORTICITY )
      {
      computeScalarDerivs = 0;
      }
    outVectors = vtkVectors::New();
    outVectors->SetNumberOfVectors(numCells);
    outVectors->GetData()->SetName("Vorticity");
    outCD->SetVectors(outVectors);
    outVectors->Delete(); //okay reference counted
    outCD->CopyVectorsOff();
    }

  if ( !inVectors || (this->TensorMode == VTK_TENSOR_MODE_PASS_TENSORS &&
              this->VectorMode != VTK_VECTOR_MODE_COMPUTE_VORTICITY) )
    {
    computeVectorDerivs = 0;
    }
  else
    {
    outTensors = vtkTensors::New();
    outTensors->SetNumberOfTensors(numCells);
    outTensors->GetData()->SetName("Tensors");
    outCD->SetTensors(outTensors);
    outTensors->Delete(); //okay reference counted
    outCD->CopyTensorsOff();
    }

  // If just passing data forget the loop
  if ( computeScalarDerivs || computeVectorDerivs )
    {
    float pcoords[3], derivs[9], w[3], *scalars, *vectors;
    vtkGenericCell *cell = vtkGenericCell::New();
    int cellId;
    vtkScalars *cellScalars=vtkScalars::New(); 
    cellScalars->Allocate(VTK_CELL_SIZE);
    cellScalars->GetData()->SetName("Scalars");
    vtkVectors *cellVectors=vtkVectors::New(); 
    cellVectors->Allocate(VTK_CELL_SIZE);
    cellVectors->GetData()->SetName("Vectors");
    vtkTensor *tens = vtkTensor::New();

    // Loop over all cells computing derivatives
    int progressInterval = numCells/20 + 1;
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( ! (cellId % progressInterval) ) 
        {
        vtkDebugMacro(<<"Computing cell #" << cellId);
        this->UpdateProgress ((float)cellId/numCells);
        }

      input->GetCell(cellId, cell);
      subId = cell->GetParametricCenter(pcoords);
      
      if ( computeScalarDerivs )
        {
        inScalars->GetScalars(cell->PointIds, cellScalars);
        scalars = ((vtkFloatArray *)cellScalars->GetData())->GetPointer(0);
        cell->Derivatives(subId, pcoords, scalars, 1, derivs);
        outVectors->SetVector(cellId, derivs);
        }

      if ( computeVectorDerivs )
        {
        inVectors->GetVectors(cell->PointIds, cellVectors);
        vectors = ((vtkFloatArray *)cellVectors->GetData())->GetPointer(0);
        cell->Derivatives(0, pcoords, vectors, 3, derivs);

        // Insert appropriate tensor
        if ( this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT)
          {
          tens->SetComponent(0,0, derivs[0]);
          tens->SetComponent(0,1, derivs[1]);
          tens->SetComponent(0,2, derivs[2]);
          tens->SetComponent(1,0, derivs[3]);
          tens->SetComponent(1,1, derivs[4]);
          tens->SetComponent(1,2, derivs[5]);
          tens->SetComponent(2,0, derivs[6]);
          tens->SetComponent(2,1, derivs[7]);
          tens->SetComponent(2,2, derivs[8]);
          
          outTensors->InsertTensor(cellId, tens);
          }
        else // this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN
          {
          tens->SetComponent(0,0, derivs[0]);
          tens->SetComponent(0,1, 0.5*(derivs[1]+derivs[3]));
          tens->SetComponent(0,2, 0.5*(derivs[2]+derivs[6]));
          tens->SetComponent(1,0, 0.5*(derivs[1]+derivs[3]));
          tens->SetComponent(1,1, derivs[4]);
          tens->SetComponent(1,2, 0.5*(derivs[5]+derivs[7]));
          tens->SetComponent(2,0, 0.5*(derivs[2]+derivs[6]));
          tens->SetComponent(2,1, 0.5*(derivs[5]+derivs[7]));
          tens->SetComponent(2,2, derivs[8]);
          
          outTensors->InsertTensor(cellId, tens);
          }

        if ( this->VectorMode == VTK_VECTOR_MODE_COMPUTE_VORTICITY )
          {
          w[0] = derivs[7] - derivs[5];
          w[1] = derivs[2] - derivs[6];
          w[2] = derivs[3] - derivs[1];
          outVectors->SetVector(cellId, w);
          }
        }
      }//for all cells

    cell->Delete();
    cellScalars->Delete();
    cellVectors->Delete();
    tens->Delete();
    }//if something to compute

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);
}

const char *vtkCellDerivatives::GetVectorModeAsString(void)
{
  if ( this->VectorMode == VTK_VECTOR_MODE_PASS_VECTORS )
    {
    return "PassVectors";
    }
  else if ( this->VectorMode == VTK_VECTOR_MODE_COMPUTE_GRADIENT )
    {
    return "ComputeGradient";
    }
  else //VTK_VECTOR_MODE_COMPUTE_VORTICITY
    {
    return "ComputeVorticity";
    }
}

const char *vtkCellDerivatives::GetTensorModeAsString(void)
{
  if ( this->TensorMode == VTK_TENSOR_MODE_PASS_TENSORS )
    {
    return "PassTensors";
    }
  else if ( this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT )
    {
    return "ComputeGradient";
    }
  else //VTK_TENSOR_MODE_COMPUTE_STRAIN
    {
    return "ComputeVorticity";
    }
}

void vtkCellDerivatives::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Vector Mode: " << this->GetVectorModeAsString() 
     << endl;

  os << indent << "Tensor Mode: " << this->GetTensorModeAsString() 
     << endl;
}

