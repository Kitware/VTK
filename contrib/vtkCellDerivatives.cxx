/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDerivatives.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkCellDerivatives.h"
#include "vtkFloatArray.h"

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
  int computeScalarDerivs=1, computeVectorDerivs=1;

  // Initialize
  vtkDebugMacro(<<"Computing cell derivatives");

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
    outCD->SetVectors(outVectors);
    outVectors->Delete(); //okay reference counted
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
    outCD->SetTensors(outTensors);
    outTensors->Delete(); //okay reference counted
    }

  // If just passing data forget the loop
  if ( computeScalarDerivs || computeVectorDerivs )
    {
    float pcoords[3], derivs[9], w[3], *scalars, *vectors;
    float a00, a01, a02, a10, a11, a12, a20, a21, a22;
    vtkGenericCell *cell = vtkGenericCell::New();
    int cellId;
    vtkScalars *cellScalars=vtkScalars::New(); 
    cellScalars->Allocate(VTK_CELL_SIZE);
    vtkVectors *cellVectors=vtkVectors::New(); 
    cellVectors->Allocate(VTK_CELL_SIZE);
    vtkTensor *tens = vtkTensor::New();

    // Assume that (0.5,0.5,0.5) is the (parametric) center of the cell
    pcoords[0] = pcoords[1] = pcoords[2] = 0.5; 

    // Loop over all cells computing derivatives
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( ! (cellId % 20000) ) 
        {
        vtkDebugMacro(<<"Computing cell #" << cellId);
        this->UpdateProgress ((float)cellId/numCells);
        }

      input->GetCell(cellId, cell);
      
      if ( computeScalarDerivs )
        {
        inScalars->GetScalars(cell->PointIds, cellScalars);
        scalars = ((vtkFloatArray *)cellScalars->GetData())->GetPointer(0);
        cell->Derivatives(0, pcoords, scalars, 1, derivs);
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
  outCD->PassNoReplaceData(cd);
}

char *vtkCellDerivatives::GetVectorModeAsString(void)
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

char *vtkCellDerivatives::GetTensorModeAsString(void)
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

