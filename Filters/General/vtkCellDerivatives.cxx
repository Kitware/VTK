/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDerivatives.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellDerivatives.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTensor.h"

#include <math.h>

vtkStandardNewMacro(vtkCellDerivatives);

vtkCellDerivatives::vtkCellDerivatives()
{
  this->VectorMode = VTK_VECTOR_MODE_COMPUTE_GRADIENT;
  this->TensorMode = VTK_TENSOR_MODE_COMPUTE_GRADIENT;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);

  // by default process active point vectors
  this->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);
}

int vtkCellDerivatives::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkDataArray *inScalars=this->GetInputArrayToProcess(0, inputVector);
  vtkDataArray *inVectors=this->GetInputArrayToProcess(1, inputVector);
  vtkDoubleArray *outGradients=NULL;
  vtkDoubleArray *outVorticity=NULL;
  vtkDoubleArray *outTensors=NULL;
  vtkIdType numCells=input->GetNumberOfCells();
  int computeScalarDerivs=1, computeVectorDerivs=1, computeVorticity=1, subId;

  // Initialize
  vtkDebugMacro(<<"Computing cell derivatives");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Check input
  if ( numCells < 1 )
    {
    vtkErrorMacro("No cells to generate derivatives from");
    return 1;
    }

  // Figure out what to compute
  if ( inScalars && this->VectorMode == VTK_VECTOR_MODE_COMPUTE_GRADIENT )
    {
    outGradients = vtkDoubleArray::New();
    outGradients->SetNumberOfComponents(3);
    outGradients->SetNumberOfTuples(numCells);
    outGradients->SetName("ScalarGradient");
    }
  else
    {
    computeScalarDerivs = 0;
    }

  if ( inVectors && this->VectorMode == VTK_VECTOR_MODE_COMPUTE_VORTICITY )
    {
    outVorticity = vtkDoubleArray::New();
    outVorticity->SetNumberOfComponents(3);
    outVorticity->SetNumberOfTuples(numCells);
    outVorticity->SetName("Vorticity");
    }
  else
    {
    computeVorticity = 0;
    }

  if (inVectors && ( this->TensorMode == VTK_TENSOR_MODE_COMPUTE_GRADIENT ||
                     this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN ))
    {
    outTensors = vtkDoubleArray::New();
    outTensors->SetNumberOfComponents(9);
    outTensors->SetNumberOfTuples(numCells);
    if ( this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN )
      {
      outTensors->SetName("Strain");
      }
    else
      {
      outTensors->SetName("VectorGradient");
      }
    }
  else
    {
    computeVectorDerivs = 0;
    }

  // If just passing data forget the loop
  if ( computeScalarDerivs || computeVectorDerivs || computeVorticity )
    {
    double pcoords[3], derivs[9], w[3], *scalars, *vectors;
    vtkGenericCell *cell = vtkGenericCell::New();
    vtkIdType cellId;
    vtkDoubleArray *cellScalars=vtkDoubleArray::New();
    if ( computeScalarDerivs )
      {
      cellScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
      cellScalars->Allocate(cellScalars->GetNumberOfComponents()*VTK_CELL_SIZE);
      cellScalars->SetName("Scalars");
      }
    vtkDoubleArray *cellVectors=vtkDoubleArray::New();
    cellVectors->SetNumberOfComponents(3);
    cellVectors->Allocate(3*VTK_CELL_SIZE);
    cellVectors->SetName("Vectors");
    vtkTensor* tens = vtkTensor::New();

    // Loop over all cells computing derivatives
    vtkIdType progressInterval = numCells/20 + 1;
    for (cellId=0; cellId < numCells; cellId++)
      {
      if ( ! (cellId % progressInterval) )
        {
        vtkDebugMacro(<<"Computing cell #" << cellId);
        this->UpdateProgress (static_cast<double>(cellId)/numCells);
        }

      input->GetCell(cellId, cell);
      subId = cell->GetParametricCenter(pcoords);

      if ( computeScalarDerivs )
        {
        inScalars->GetTuples(cell->PointIds, cellScalars);
        scalars = cellScalars->GetPointer(0);
        cell->Derivatives(subId, pcoords, scalars, 1, derivs);
        outGradients->SetTuple(cellId, derivs);
        }

      if ( computeVectorDerivs || computeVorticity )
        {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        vectors = cellVectors->GetPointer(0);
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

          outTensors->InsertTuple(cellId, tens->T);
          }
        else if (this->TensorMode == VTK_TENSOR_MODE_COMPUTE_STRAIN)
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

          outTensors->InsertTuple(cellId, tens->T);
          }
        else if (this->TensorMode == VTK_TENSOR_MODE_PASS_TENSORS)
          {
          // do nothing.
          }

        if ( computeVorticity )
          {
          w[0] = derivs[7] - derivs[5];
          w[1] = derivs[2] - derivs[6];
          w[2] = derivs[3] - derivs[1];
          outVorticity->SetTuple(cellId, w);
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
  if (outGradients)
    {
    outCD->SetVectors(outGradients);
    outGradients->Delete();
    }
  if (outVorticity)
    {
    outCD->SetVectors(outVorticity);
    outVorticity->Delete();
    }
  if (outTensors)
    {
    outCD->SetTensors(outTensors);
    outTensors->Delete();
    }

  return 1;
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
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Vector Mode: " << this->GetVectorModeAsString()
     << endl;

  os << indent << "Tensor Mode: " << this->GetTensorModeAsString()
     << endl;
}

