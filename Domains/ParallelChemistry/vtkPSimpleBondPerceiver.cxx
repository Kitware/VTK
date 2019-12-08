/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkYoungsMaterialInterfaceCEA.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPSimpleBondPerceiver.h"

#include "vtkDataSetAttributes.h"
#include "vtkDistributedPointCloudFilter.h"
#include "vtkFloatArray.h"
#include "vtkMPIController.h"
#include "vtkMolecule.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPeriodicTable.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkPSimpleBondPerceiver);

//----------------------------------------------------------------------------
static inline bool InBounds(const double* bounds, const double* p)
{
  return p[0] >= bounds[0] && p[0] <= bounds[1] && p[1] >= bounds[2] && p[1] <= bounds[3] &&
    p[2] >= bounds[4] && p[2] <= bounds[5];
}

//----------------------------------------------------------------------------
bool vtkPSimpleBondPerceiver::CreateGhosts(vtkMolecule* molecule)
{
  if (molecule == nullptr)
  {
    return false;
  }

  vtkMPIController* controller =
    vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController());
  if (controller == nullptr)
  {
    return true;
  }

  double domainBounds[6] = { 0., 0., 0., 0., 0., 0. };
  molecule->GetBounds(domainBounds);

  vtkNew<vtkPeriodicTable> table;
  static float MAX_VDW_RADIUS = table->GetMaxVDWRadius();

  double radius =
    this->IsToleranceAbsolute ? MAX_VDW_RADIUS + this->Tolerance : MAX_VDW_RADIUS * this->Tolerance;
  double outterBounds[6];
  outterBounds[0] = domainBounds[0] - radius;
  outterBounds[2] = domainBounds[2] - radius;
  outterBounds[4] = domainBounds[4] - radius;
  outterBounds[1] = domainBounds[1] + radius;
  outterBounds[3] = domainBounds[3] + radius;
  outterBounds[5] = domainBounds[5] + radius;

  vtkNew<vtkPolyData> inputPoly;
  vtkNew<vtkPoints> points;
  points->DeepCopy(molecule->GetAtomicPositionArray());
  inputPoly->SetPoints(points.Get());
  vtkDataSetAttributes* dataArray = inputPoly->GetPointData();
  dataArray->DeepCopy(molecule->GetVertexData());

  vtkNew<vtkPolyData> outputPoly;
  vtkDistributedPointCloudFilter::GetPointsInsideBounds(
    controller, inputPoly.Get(), outputPoly.Get(), outterBounds);

  molecule->Initialize(outputPoly->GetPoints(), outputPoly->GetPointData());

  molecule->AllocateAtomGhostArray();
  vtkUnsignedCharArray* atomGhostArray = molecule->GetAtomGhostArray();
  atomGhostArray->FillComponent(0, 0);

  molecule->AllocateBondGhostArray();
  vtkUnsignedCharArray* bondGhostArray = molecule->GetBondGhostArray();
  bondGhostArray->FillComponent(0, 0);

  if (!atomGhostArray && !bondGhostArray)
  {
    return false;
  }

  for (vtkIdType i = 0; i < molecule->GetNumberOfAtoms(); i++)
  {
    double p[3];
    molecule->GetPoint(i, p);
    if (!InBounds(domainBounds, p))
    {
      atomGhostArray->SetValue(i, 1);
      vtkNew<vtkOutEdgeIterator> it;
      molecule->GetOutEdges(i, it.Get());
      while (it->HasNext())
      {
        vtkOutEdgeType edge = it->Next();
        bondGhostArray->SetValue(edge.Id, 1);
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkPSimpleBondPerceiver::ComputeBonds(vtkMolecule* molecule)
{
  if (!this->CreateGhosts(molecule))
  {
    vtkWarningMacro(<< "Ghosts were not correctly initialized.");
  }

  this->Superclass::ComputeBonds(molecule);
}
