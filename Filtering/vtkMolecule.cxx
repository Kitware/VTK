/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMolecule.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMolecule.h"

#include "vtkAbstractElectronicData.h"
#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMolecule);

//----------------------------------------------------------------------------
vtkMolecule::vtkMolecule()
{
  this->SetPoints(vtkPoints::New());
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkMolecule::Initialize()
{
  // Reset underlying data structure
  this->Superclass::Initialize();

  // Setup vertex data
  vtkDataSetAttributes *vertData = this->GetVertexData();
  vertData->AllocateArrays(1); // atomic nums

  // Atomic numbers
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::New();
  atomicNums->SetNumberOfComponents(1);
  atomicNums->SetName("Atomic Numbers");
  vertData->SetScalars(atomicNums);

  // Nuclear coordinates
  this->Points->Initialize();

  // Setup edge data
  vtkDataSetAttributes *edgeData = this->GetEdgeData();
  edgeData->AllocateArrays(1); // Bond orders

  vtkUnsignedShortArray *bondOrders = vtkUnsignedShortArray::New();
  bondOrders->SetNumberOfComponents(1);
  bondOrders->SetName("Bond Orders");
  edgeData->SetScalars(bondOrders);

  this->UpdateBondList();

  // Electronic data
  this->ElectronicData = NULL;

  this->Modified();
}

//----------------------------------------------------------------------------
vtkMolecule::~vtkMolecule()
{
  this->SetElectronicData(NULL);
  this->Points->Delete();
}

//----------------------------------------------------------------------------
void vtkMolecule::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkIndent subIndent = indent.GetNextIndent();

  os << indent << "Atoms:\n";
  for (vtkIdType i = 0; i < this->GetNumberOfAtoms(); ++i)
    {
    this->GetAtom(i).PrintSelf(os, subIndent);
    }

  os << indent << "Bonds:\n";
  for (vtkIdType i = 0; i < this->GetNumberOfBonds(); ++i)
    {
    os << subIndent << "===== Bond " << i << ": =====\n";
    this->GetBond(i).PrintSelf(os, subIndent);
    }
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::AddAtom(unsigned short atomicNumber, const float pos[3])
{
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  vtkIdType id;
  this->AddVertexInternal(0, &id);

  atomicNums->InsertValue(id, atomicNumber);
  vtkIdType coordID = this->Points->InsertNextPoint(pos);

  assert("point ids synced with vertex ids" && coordID == id);

  this->Modified();
  return vtkAtom(this, id);
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::AddAtom(unsigned short atomicNumber, const double pos[3])
{
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  vtkIdType id;
  this->AddVertexInternal(0, &id);

  atomicNums->InsertValue(id, atomicNumber);
  vtkIdType coordID = this->Points->InsertNextPoint(pos);

  assert("point ids synced with vertex ids" && coordID == id);

  this->Modified();
  return vtkAtom(this, id);
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::GetAtom(vtkIdType atomId)
{
  assert(atomId >= 0 && atomId < this->GetNumberOfAtoms());

  vtkAtom atom (this, atomId);
  return atom;
}

//----------------------------------------------------------------------------
unsigned short vtkMolecule::GetAtomAtomicNumber(vtkIdType id)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());

  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  return atomicNums->GetValue(id);
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomAtomicNumber(vtkIdType id, unsigned short atomicNum)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());

  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  atomicNums->SetValue(id, atomicNum);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMolecule::GetAtomPosition(vtkIdType id, double pos[3])
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  this->Points->GetPoint(id, pos);
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomPosition(vtkIdType id, const double pos[3])
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  this->Points->SetPoint(id, pos);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMolecule::GetAtomPosition(vtkIdType id, float pos[3])
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  double posd[3];
  // There is no float overload of vtkPoints::GetPoint
  this->Points->GetPoint(id, posd);
  pos[0] = posd[0]; pos[1] = posd[1]; pos[2] = posd[2];
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomPosition(vtkIdType id, const float pos[3])
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  this->Points->SetPoint(id, pos);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomPosition(vtkIdType id, double x, double y, double z)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  this->Points->SetPoint(id, x, y, z);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIdType vtkMolecule::GetNumberOfAtoms()
{
  return this->GetNumberOfVertices();
}

//----------------------------------------------------------------------------
vtkBond vtkMolecule::AddBond(const vtkIdType atom1, const vtkIdType atom2,
                             const unsigned short order)
{
  vtkUnsignedShortArray *bondOrders = vtkUnsignedShortArray::SafeDownCast
    (this->GetEdgeData()->GetScalars());
  assert(bondOrders);

  vtkEdgeType edgeType;
  this->AddEdgeInternal(atom1, atom2, false, 0, &edgeType);
  this->SetBondListDirty();

  vtkIdType id = edgeType.Id;
  bondOrders->InsertValue(id, order);
  this->Modified();
  return vtkBond(this, id, atom1, atom2);
}

//----------------------------------------------------------------------------
vtkBond vtkMolecule::GetBond(vtkIdType bondId)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  if (this->BondListIsDirty)
    {
      this->UpdateBondList();
    }
  vtkIdTypeArray *bonds = this->GetEdgeList();
  // An array with two components holding the bonded atom's ids
  vtkIdType *ids = bonds->GetPointer(2 * bondId);
  return vtkBond (this, bondId, ids[0], ids[1]);
}

//----------------------------------------------------------------------------
void vtkMolecule::SetBondOrder(vtkIdType bondId, unsigned short order)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  vtkUnsignedShortArray *bondOrders = vtkUnsignedShortArray::SafeDownCast
    (this->GetEdgeData()->GetScalars());

  assert(bondOrders);

  this->Modified();
  return bondOrders->SetValue(bondId, order);
}

//----------------------------------------------------------------------------
unsigned short vtkMolecule::GetBondOrder(vtkIdType bondId)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  vtkUnsignedShortArray *bondOrders = vtkUnsignedShortArray::SafeDownCast
    (this->GetEdgeData()->GetScalars());

  assert(bondOrders);

  return bondOrders->GetValue(bondId);
}

//----------------------------------------------------------------------------
double vtkMolecule::GetBondLength(vtkIdType bondId)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  // Get list of bonds
  vtkIdTypeArray *bonds = this->GetEdgeList();
  // An array of length two holding the bonded atom's ids
  vtkIdType *ids = bonds->GetPointer(bondId);

  // Get positions
  vtkVector3d pos1 = this->GetAtomPositionAsVector3d(ids[0]);
  vtkVector3d pos2 = this->GetAtomPositionAsVector3d(ids[1]);

  return (pos2 - pos1).Norm();
}

//----------------------------------------------------------------------------
vtkPoints * vtkMolecule::GetAtomicPositionArray()
{
  return this->Points;
}

//----------------------------------------------------------------------------
vtkUnsignedShortArray * vtkMolecule::GetAtomicNumberArray()
{
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  return atomicNums;
}

//----------------------------------------------------------------------------
vtkIdType vtkMolecule::GetNumberOfBonds()
{
  return this->GetNumberOfEdges();
}

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMolecule, ElectronicData, vtkAbstractElectronicData);

//----------------------------------------------------------------------------
void vtkMolecule::UpdateBondList()
{
  this->BuildEdgeList();
  this->BondListIsDirty = false;
}

bool vtkMolecule::GetPlaneFromBond(vtkAtom atom1, vtkAtom atom2,
                                   const double normal[3], vtkPlane *plane)
{
  if (plane == NULL)
    return false;

  // TODO Remove or restore this when subtracting vectors is supported again
  // vtkVector3d v (atom1.GetPositionAsVector3d() -
  //                atom2.GetPositionAsVector3d());
  double pos1[3];
  double pos2[3];
  atom1.GetPosition(pos1);
  atom2.GetPosition(pos2);
  vtkVector3d v (pos1[0] - pos2[0], pos1[1] - pos2[1], pos1[2] - pos2[2]);
  // end vtkVector reimplementation TODO

  vtkVector3d n_i (normal);
  vtkVector3d unitV (v.Normalized());

  // Check if vectors are (nearly) parallel
  if (unitV.Compare(n_i.Normalized(), 1e-7))
    return false;

  // calculate projection of n_i onto v
  // TODO Remove or restore this when scalar mult. is supported again
  // vtkVector3d proj (unitV * n_i.Dot(unitV));
  double n_iDotUnitV = n_i.Dot(unitV);
  vtkVector3d proj (unitV[0] * n_iDotUnitV, unitV[1] * n_iDotUnitV,
                    unitV[2] * n_iDotUnitV);
  // end vtkVector reimplementation TODO

  // Calculate actual normal:
  // TODO Remove/restore this when subtraction is supported again
  // vtkVector3d realNormal (n_i - proj);
  vtkVector3d realNormal (n_i[0]-proj[0], n_i[1]-proj[1], n_i[2]-proj[2]);
  // end vtkVector reimplementation TODO

  // Create plane:
  double pos[3];
  atom1.GetPosition(pos);
  plane->SetOrigin(pos);
  plane->SetNormal(realNormal.GetData());
  return true;
}
