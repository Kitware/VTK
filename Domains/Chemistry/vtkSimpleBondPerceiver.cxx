/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleBondPerceiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSimpleBondPerceiver.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPeriodicTable.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedShortArray.h"


#include <vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSimpleBondPerceiver);

//----------------------------------------------------------------------------
vtkSimpleBondPerceiver::vtkSimpleBondPerceiver()
  : Tolerance(0.45)
{
}

//----------------------------------------------------------------------------
vtkSimpleBondPerceiver::~vtkSimpleBondPerceiver()
{
}

//----------------------------------------------------------------------------
void vtkSimpleBondPerceiver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

//----------------------------------------------------------------------------
int vtkSimpleBondPerceiver::RequestData(
  vtkInformation* ,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkMolecule *input = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(inputVector[0]));
  vtkMolecule *output = vtkMolecule::SafeDownCast
    (vtkDataObject::GetData(outputVector));

  // Copy input to output
  output->Initialize();
  output->DeepCopyStructure(input);
  output->ShallowCopyAttributes(input);

  // Get pointers to data
  vtkSmartPointer<vtkPoints> posArr = output->GetAtomicPositionArray();
  vtkSmartPointer<vtkUnsignedShortArray> numArr = output->GetAtomicNumberArray();

  // Cache atomic radii
  vtkNew<vtkPeriodicTable> pTab;
  std::vector<float> radii (numArr->GetNumberOfTuples());
  for (size_t i = 0; i < radii.size(); ++i)
  {
    radii[i] = pTab->GetCovalentRadius(
          numArr->GetValue(static_cast<vtkIdType>(i)));
  }

  // Check for bonds
  double diff[3];
  const vtkIdType numAtoms = output->GetNumberOfAtoms();
  vtkDebugMacro(<<"Checking for bonds with tolerance " << this->Tolerance);
  for (vtkIdType i = 0; i < numAtoms; ++i)
  {
    double *ipos = posArr->GetPoint(i);
    for (vtkIdType j = i+1; j < numAtoms; ++j)
    {
      double cutoff = radii[i] + radii[j] + this->Tolerance;
      posArr->GetPoint(j, diff);
      diff[0] -= ipos[0];
      diff[1] -= ipos[1];
      diff[2] -= ipos[2];

      if (fabs(diff[0]) > cutoff ||
          fabs(diff[1]) > cutoff ||
          fabs(diff[2]) > cutoff ||
          (numArr->GetValue(i) == 1 && numArr->GetValue(j) == 1))
        continue;

      // Check radius and add bond if needed
      double cutoffSq = cutoff * cutoff;
      double diffsq = diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2];
      if (diffsq < cutoffSq && diffsq > 0.1)
      {
        vtkDebugMacro(<<"Adding bond between " << i << " and " << j
                      << ". Distance: " << diffsq << "\n");
        output->AppendBond(i, j, 1);
      }
    }
  }

  return 1;
}
