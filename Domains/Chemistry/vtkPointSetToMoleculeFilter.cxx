/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetToMoleculeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointSetToMoleculeFilter.h"

#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkPointSetToMoleculeFilter);

//----------------------------------------------------------------------------
vtkPointSetToMoleculeFilter::vtkPointSetToMoleculeFilter()
{
  this->SetNumberOfInputPorts(1);

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
int vtkPointSetToMoleculeFilter::FillInputPortInformation(int vtkNotUsed(port),
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPointSetToMoleculeFilter::RequestData(vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkPointSet* input = vtkPointSet::SafeDownCast(vtkDataObject::GetData(inputVector[0]));
  vtkMolecule* output = vtkMolecule::SafeDownCast(vtkDataObject::GetData(outputVector));

  if (!input)
  {
    vtkErrorMacro(<< "No input provided.");
    return 0;
  }

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (input->GetNumberOfPoints() > 0 && !inScalars)
  {
    vtkErrorMacro(<< "vtkPointSetToMoleculeFilter does not have atomic numbers as input.");
    return 0;
  }

  return output->Initialize(input->GetPoints(), inScalars, input->GetPointData());
}
