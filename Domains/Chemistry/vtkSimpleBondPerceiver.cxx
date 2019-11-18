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
#include "vtkOctreePointLocator.h"
#include "vtkPeriodicTable.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"

#include <vector>

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkSimpleBondPerceiver);

//----------------------------------------------------------------------------
vtkSimpleBondPerceiver::vtkSimpleBondPerceiver()
  : Tolerance(0.45)
  , IsToleranceAbsolute(true)
{
}

//----------------------------------------------------------------------------
vtkSimpleBondPerceiver::~vtkSimpleBondPerceiver() = default;

//----------------------------------------------------------------------------
void vtkSimpleBondPerceiver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "IsToleranceAbsolute: " << this->IsToleranceAbsolute << "\n";
}

//----------------------------------------------------------------------------
int vtkSimpleBondPerceiver::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMolecule* input = vtkMolecule::SafeDownCast(vtkDataObject::GetData(inputVector[0]));
  if (!input)
  {
    vtkErrorMacro(<< "Input vtkMolecule does not exists.");
    return 0;
  }

  vtkMolecule* output = vtkMolecule::SafeDownCast(vtkDataObject::GetData(outputVector));
  if (!output)
  {
    vtkErrorMacro(<< "Output vtkMolecule does not exists.");
    return 0;
  }

  // Copy input to output
  output->Initialize();
  output->DeepCopyStructure(input);
  output->ShallowCopyAttributes(input);

  this->ComputeBonds(output);

  return 1;
}

//----------------------------------------------------------------------------
void vtkSimpleBondPerceiver::ComputeBonds(vtkMolecule* molecule)
{
  if (!molecule)
  {
    vtkWarningMacro(<< "vtkMolecule to fill is not defined.");
    return;
  }

  vtkPoints* atomPositions = molecule->GetPoints();

  if (atomPositions->GetNumberOfPoints() == 0)
  {
    // nothing to do.
    return;
  }

  vtkNew<vtkPolyData> moleculePolyData;
  moleculePolyData->SetPoints(atomPositions);
  vtkNew<vtkOctreePointLocator> locator;
  locator->SetDataSet(moleculePolyData.Get());
  locator->BuildLocator();

  vtkUnsignedCharArray* ghostAtoms = molecule->GetAtomGhostArray();
  vtkUnsignedCharArray* ghostBonds = molecule->GetBondGhostArray();

  vtkIdType nbAtoms = molecule->GetNumberOfAtoms();
  vtkNew<vtkIdList> neighborsIdsList;
  vtkNew<vtkPeriodicTable> periodicTable;
  int nbElementsPeriodicTable = periodicTable->GetNumberOfElements();

  /**
   * Main algorithm:
   *  - loop on each atom.
   *  - use locator to determine potential pair: consider atoms in a radius of 2*covalentRadius
   *  - for each potential pair, compute atomic radius (with tolerance) and distance
   *  - if (d < r1 + r2) add a bond. Do not add twice a same bond. Do not create bond between two
   * ghost atoms.
   *  - if one of the two atoms is a ghost, mark bond as ghost
   */
  for (vtkIdType i = 0; i < nbAtoms; i++)
  {
    bool isGhostAtom = (ghostAtoms ? (ghostAtoms->GetTuple1(i) != 0) : false);
    vtkIdType atomicNumber = molecule->GetAtomAtomicNumber(i);

    if (atomicNumber < 1 || atomicNumber > nbElementsPeriodicTable)
    {
      continue;
    }

    double covalentRadius = this->GetCovalentRadiusWithTolerance(periodicTable, atomicNumber);
    double atomPosition[3];
    atomPositions->GetPoint(i, atomPosition);
    neighborsIdsList->SetNumberOfIds(0);
    locator->FindPointsWithinRadius(2 * covalentRadius, atomPosition, neighborsIdsList.Get());

    vtkIdType nbNeighbors = neighborsIdsList->GetNumberOfIds();
    vtkIdType* neighborsPtr = neighborsIdsList->GetPointer(0);
    for (vtkIdType j = 0; j < nbNeighbors; ++j)
    {
      vtkIdType neighId = neighborsPtr[j];
      bool isGhostNeigh = (ghostAtoms ? (ghostAtoms->GetTuple1(neighId) != 0) : false);
      vtkIdType atomicNumberNeigh = molecule->GetAtomAtomicNumber(neighId);

      if (atomicNumberNeigh < 1 || (atomicNumberNeigh > nbElementsPeriodicTable) ||
        (isGhostAtom && isGhostNeigh))
      {
        continue;
      }

      double covalentRadiusNeigh =
        this->GetCovalentRadiusWithTolerance(periodicTable, atomicNumberNeigh);
      double radiusSumSquare =
        (covalentRadius + covalentRadiusNeigh) * (covalentRadius + covalentRadiusNeigh);
      double doubleNeighbourRadiusSquare = 4 * covalentRadiusNeigh * covalentRadiusNeigh;
      double atomPositionNeigh[3];
      molecule->GetAtom(neighId).GetPosition(atomPositionNeigh);
      double distanceSquare = vtkMath::Distance2BetweenPoints(atomPosition, atomPositionNeigh);

      /**
       * Bond may have already been created:
       *  - neighId <= i : we already check bonds for atom 'neighId' in a previous iteration.
       *  - distanceSquare <= doubleNeighbourRadiusSquare : atom 'i' was in the list for potential
       *    pair with neighId.
       *  So if the bond i-neighId is possible, it was added the first time and we can continue.
       *
       * Distance can be to big:
       *  - distanceSquare > radiusSumSquare
       */
      if ((neighId <= i && distanceSquare <= doubleNeighbourRadiusSquare) ||
        distanceSquare > radiusSumSquare)
      {
        continue;
      }

      molecule->AppendBond(i, neighId);
      if (ghostBonds)
      {
        ghostBonds->InsertNextValue(isGhostAtom || isGhostNeigh ? 1 : 0);
      }
    }
  }
}

//----------------------------------------------------------------------------
double vtkSimpleBondPerceiver::GetCovalentRadiusWithTolerance(
  vtkPeriodicTable* table, vtkIdType atomicNumber)
{
  return this->IsToleranceAbsolute ? table->GetCovalentRadius(atomicNumber) + this->Tolerance / 2
                                   : table->GetCovalentRadius(atomicNumber) * this->Tolerance;
}
