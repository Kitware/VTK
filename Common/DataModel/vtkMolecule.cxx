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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkUnsignedShortArray.h"
#include "vtkFloatArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <assert.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMolecule);

//----------------------------------------------------------------------------
vtkMolecule::vtkMolecule()
{
  vtkPoints *points = vtkPoints::New();
  this->SetPoints(points);
  points->Delete();
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
  vtkNew<vtkUnsignedShortArray> atomicNums;
  atomicNums->SetNumberOfComponents(1);
  atomicNums->SetName("Atomic Numbers");
  vertData->SetScalars(atomicNums.GetPointer());

  // Nuclear coordinates
  this->Points->Initialize();

  // Setup edge data
  vtkDataSetAttributes *edgeData = this->GetEdgeData();
  edgeData->AllocateArrays(1); // Bond orders

  vtkNew<vtkUnsignedShortArray> bondOrders;
  bondOrders->SetNumberOfComponents(1);
  bondOrders->SetName("Bond Orders");
  edgeData->SetScalars(bondOrders.GetPointer());

  this->UpdateBondList();

  // Electronic data
  this->ElectronicData = NULL;

  this->Modified();
}

//----------------------------------------------------------------------------
vtkMolecule::~vtkMolecule()
{
  this->SetElectronicData(NULL);
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

  os << indent << "Electronic Data:\n";
  if (this->ElectronicData)
    {
    this->ElectronicData->PrintSelf(os, subIndent);
    }
  else
    {
    os << subIndent << "Not set.\n";
    }
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::AppendAtom(unsigned short atomicNumber,
                                const vtkVector3f &pos)
{
  vtkUnsignedShortArray *atomicNums = vtkUnsignedShortArray::SafeDownCast
    (this->GetVertexData()->GetScalars());

  assert(atomicNums);

  vtkIdType id;
  this->AddVertexInternal(0, &id);

  atomicNums->InsertValue(id, atomicNumber);
  vtkIdType coordID = this->Points->InsertNextPoint(pos.GetData());
  (void)coordID;
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
void vtkMolecule::SetAtomPosition(vtkIdType id, const vtkVector3f &pos)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  this->Points->SetPoint(id, pos.GetData());
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
vtkVector3f vtkMolecule::GetAtomPosition(vtkIdType id)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());
  vtkFloatArray *positions = vtkFloatArray::SafeDownCast(this->Points->GetData());
  assert(positions != NULL);
  float *data = static_cast<float *>(positions->GetVoidPointer(id * 3));
  return vtkVector3f(data);
}

//----------------------------------------------------------------------------
void vtkMolecule::GetAtomPosition(vtkIdType id, float pos[3])
{
  vtkVector3f position = this->GetAtomPosition(id);
  pos[0] = position.GetX();
  pos[1] = position.GetY();
  pos[2] = position.GetZ();
}

//----------------------------------------------------------------------------
vtkIdType vtkMolecule::GetNumberOfAtoms()
{
  return this->GetNumberOfVertices();
}

//----------------------------------------------------------------------------
vtkBond vtkMolecule::AppendBond(const vtkIdType atom1, const vtkIdType atom2,
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
  vtkVector3f pos1 = this->GetAtomPosition(ids[0]);
  vtkVector3f pos2 = this->GetAtomPosition(ids[1]);

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
void vtkMolecule::ShallowCopy(vtkDataObject *obj)
{
  vtkMolecule *m = vtkMolecule::SafeDownCast(obj);
  if (!m)
    {
    vtkErrorMacro("Can only shallow copy from vtkMolecule or subclass.");
    return;
    }
  this->ShallowCopyStructure(m);
  this->ShallowCopyAttributes(m);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopy(vtkDataObject *obj)
{
  vtkMolecule *m = vtkMolecule::SafeDownCast(obj);
  if (!m)
    {
    vtkErrorMacro("Can only deel copy from vtkMolecule or subclass.");
    return;
    }
  this->DeepCopyStructure(m);
  this->DeepCopyAttributes(m);
}


//----------------------------------------------------------------------------
void vtkMolecule::ShallowCopyStructure(vtkMolecule *m)
{
  this->CopyStructureInternal(m, false);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopyStructure(vtkMolecule *m)
{
  this->CopyStructureInternal(m, true);
}

//----------------------------------------------------------------------------
void vtkMolecule::ShallowCopyAttributes(vtkMolecule *m)
{
  this->CopyAttributesInternal(m, false);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopyAttributes(vtkMolecule *m)
{
  this->CopyAttributesInternal(m, true);
}

//----------------------------------------------------------------------------
void vtkMolecule::CopyStructureInternal(vtkMolecule *m, bool deep)
{
  // Call superclass
  if (deep)
    {
    this->Superclass::DeepCopy(m);
    }
  else
    {
    this->Superclass::ShallowCopy(m);
    }
  }

//----------------------------------------------------------------------------
void vtkMolecule::CopyAttributesInternal(vtkMolecule *m, bool deep)
{
  if (deep)
    {
    if (m->ElectronicData)
      this->ElectronicData->DeepCopy(m->ElectronicData);
    }
  else
    {
    this->SetElectronicData(m->ElectronicData);
    }
}

//----------------------------------------------------------------------------
void vtkMolecule::UpdateBondList()
{
  this->BuildEdgeList();
  this->BondListIsDirty = false;
}

//----------------------------------------------------------------------------
bool vtkMolecule::GetPlaneFromBond(const vtkBond &bond,
                                   const vtkVector3f &normal,
                                   vtkPlane *plane)
{
  return vtkMolecule::GetPlaneFromBond(bond.GetBeginAtom(), bond.GetEndAtom(),
                                       normal, plane);
}

//----------------------------------------------------------------------------
bool vtkMolecule::GetPlaneFromBond(const vtkAtom &atom1, const vtkAtom &atom2,
                                   const vtkVector3f &normal, vtkPlane *plane)
{
  if (plane == NULL)
    {
    return false;
    }

  vtkVector3f v(atom1.GetPosition() - atom2.GetPosition());

  vtkVector3f n_i(normal);
  vtkVector3f unitV(v.Normalized());

  // Check if vectors are (nearly) parallel
  if (unitV.Compare(n_i.Normalized(), 1e-7))
    {
    return false;
    }

  // calculate projection of n_i onto v
  // TODO Remove or restore this when scalar mult. is supported again
  // vtkVector3d proj (unitV * n_i.Dot(unitV));
  double n_iDotUnitV = n_i.Dot(unitV);
  vtkVector3f proj (unitV[0] * n_iDotUnitV, unitV[1] * n_iDotUnitV,
                    unitV[2] * n_iDotUnitV);
  // end vtkVector reimplementation TODO

  // Calculate actual normal:
  vtkVector3f realNormal(n_i - proj);

  // Create plane:
  vtkVector3f pos(atom1.GetPosition());
  plane->SetOrigin(pos.Cast<double>().GetData());
  plane->SetNormal(realNormal.Cast<double>().GetData());
  return true;
}
