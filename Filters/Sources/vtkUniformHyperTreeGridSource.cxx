/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHyperTreeGridSource.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformHyperTreeGridSource.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUniformHyperTreeGrid.h"

vtkStandardNewMacro(vtkUniformHyperTreeGridSource);

//----------------------------------------------------------------------------
vtkUniformHyperTreeGridSource::vtkUniformHyperTreeGridSource() {}

//----------------------------------------------------------------------------
vtkUniformHyperTreeGridSource::~vtkUniformHyperTreeGridSource() {}

//-----------------------------------------------------------------------------
void vtkUniformHyperTreeGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkUniformHyperTreeGridSource::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUniformHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkUniformHyperTreeGridSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // Retrieve the output
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  vtkUniformHyperTreeGrid* output = vtkUniformHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("pre: output_not_uniformHyperTreeGrid: " << outputDO->GetClassName());
    return 0;
  }

  output->Initialize();

  vtkPointData* outData = output->GetPointData();

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndexCnt.push_back(0);

  // When using descriptor-based definition, initialize descriptor parsing
  if (this->UseDescriptor)
  {
    // Calculate refined block size
    this->BlockSize = this->BranchFactor;
    for (unsigned int i = 1; i < this->Dimension; ++i)
    {
      this->BlockSize *= this->BranchFactor;
    }

    if (!this->DescriptorBits && !this->InitializeFromStringDescriptor())
    {
      return 0;
    }
    else if (this->DescriptorBits && !this->InitializeFromBitsDescriptor())
    {
      return 0;
    }
  } // if this->UseDescriptor

  // Set straightforward grid parameters
  output->SetTransposedRootIndexing(this->TransposedRootIndexing);
  output->SetBranchFactor(this->BranchFactor);

  //  Set parameters that depend on dimension
  switch (this->Dimension)
  {
    case 1:
    {
      // Set 1D grid size depending on orientation
      unsigned int axis = this->Orientation;
      unsigned int gs[] = { 1, 1, 1 };
      unsigned n = this->Dimensions[axis];
      gs[axis] = n;
      output->SetDimensions(gs);

      // Assign coordinates
      switch (axis)
      {
        case 0:
          output->SetGridScale(this->GridScale[axis], 0., 0.);
          break;
        case 1:
          output->SetGridScale(0., this->GridScale[axis], 0.);
          break;
        case 2:
          output->SetGridScale(0., 0., this->GridScale[axis]);
          break;
      } // switch (axis)
    }   // case 1
    break;
    case 2:
    {
      // Set grid size depending on orientation
      unsigned int n[3];
      memcpy(n, this->Dimensions, 3 * sizeof(unsigned int));
      n[this->Orientation] = 1;
      output->SetDimensions(n);

      unsigned int axis1 = (this->Orientation + 1) % 3;
      unsigned int axis2 = (this->Orientation + 2) % 3;

      // Assign coordinates
      switch (this->Orientation)
      {
        case 0:
          output->SetGridScale(0., this->GridScale[axis1], this->GridScale[axis2]);
          break;
        case 1:
          output->SetGridScale(this->GridScale[axis2], 0., this->GridScale[axis1]);
          break;
        case 2:
          output->SetGridScale(this->GridScale[axis1], this->GridScale[axis2], 0.);
          break;
      } // switch (this->Orientation)
    }   // case 2
    break;
    case 3:
    {
      // Set grid size
      output->SetDimensions(this->Dimensions);
      output->SetGridScale(this->GridScale[0], this->GridScale[1], this->GridScale[2]);
      break;
    } // case 3
    default:
      vtkErrorMacro(<< "Unsupported dimension: " << this->Dimension << ".");
      return 0;
  } // switch (this->Dimension)

  // Prepare array of doubles for depth values
  vtkNew<vtkDoubleArray> depthArray;
  depthArray->SetName("Depth");
  depthArray->SetNumberOfComponents(1);
  outData->SetScalars(depthArray);

  if (this->GenerateInterfaceFields)
  {
    // Prepare arrays of triples for interface surrogates
    vtkNew<vtkDoubleArray> normalsArray;
    normalsArray->SetName("Normals");
    normalsArray->SetNumberOfComponents(3);
    outData->SetVectors(normalsArray);

    vtkNew<vtkDoubleArray> interceptsArray;
    interceptsArray->SetName("Intercepts");
    interceptsArray->SetNumberOfComponents(3);
    outData->AddArray(interceptsArray);
  }

  if (!this->UseDescriptor)
  {
    // Prepare array of doubles for quadric values
    vtkNew<vtkDoubleArray> quadricArray;
    quadricArray->SetName("Quadric");
    quadricArray->SetNumberOfComponents(1);
    outData->AddArray(quadricArray);
  }

  // Iterate over constituting hypertrees
  if (!this->ProcessTrees(nullptr, outputDO))
  {
    return 0;
  }

  // Squeeze output data arrays
  for (int a = 0; a < outData->GetNumberOfArrays(); ++a)
  {
    outData->GetArray(a)->Squeeze();
  }

  this->LevelBitsIndexCnt.clear();
  this->LevelBitsIndex.clear();

  return 1;
}
