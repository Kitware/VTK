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
#include "vtkFloatArray.h"
#include "vtkGraphInternals.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMatrix3x3.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMolecule);

//----------------------------------------------------------------------------
vtkMolecule::vtkMolecule()
  : ElectronicData(nullptr)
  , Lattice(nullptr)
  , LatticeOrigin(0., 0., 0.)
  , AtomGhostArray(nullptr)
  , BondGhostArray(nullptr)
  , AtomicNumberArrayName(nullptr)
  , BondOrdersArrayName(nullptr)
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkMolecule::Initialize()
{
  // Reset underlying data structure
  this->Superclass::Initialize();

  // Setup vertex data
  vtkDataSetAttributes* vertData = this->GetVertexData();
  vertData->AllocateArrays(1); // atomic nums

  // Atomic numbers
  this->SetAtomicNumberArrayName("Atomic Numbers");
  vtkNew<vtkUnsignedShortArray> atomicNums;
  atomicNums->SetNumberOfComponents(1);
  atomicNums->SetName(this->GetAtomicNumberArrayName());
  vertData->SetScalars(atomicNums);

  // Nuclear coordinates
  vtkPoints* points = vtkPoints::New();
  this->SetPoints(points);
  points->Delete();

  // Setup edge data
  vtkDataSetAttributes* edgeData = this->GetEdgeData();
  edgeData->AllocateArrays(1); // Bond orders

  this->SetBondOrdersArrayName("Bond Orders");
  vtkNew<vtkUnsignedShortArray> bondOrders;
  bondOrders->SetNumberOfComponents(1);
  bondOrders->SetName(this->GetBondOrdersArrayName());
  edgeData->SetScalars(bondOrders);

  this->UpdateBondList();

  // Electronic data
  this->SetElectronicData(nullptr);

  this->Modified();
}

//----------------------------------------------------------------------------
vtkMolecule::~vtkMolecule()
{
  this->SetElectronicData(nullptr);
  delete[] this->AtomicNumberArrayName;
  delete[] this->BondOrdersArrayName;
}

//----------------------------------------------------------------------------
void vtkMolecule::PrintSelf(ostream& os, vtkIndent indent)
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

  os << indent << "Lattice:\n";
  if (this->HasLattice())
  {
    double* m = this->Lattice->GetData();
    os << subIndent << "a: " << m[0] << " " << m[3] << " " << m[6] << "\n";
    os << subIndent << "b: " << m[1] << " " << m[4] << " " << m[7] << "\n";
    os << subIndent << "c: " << m[2] << " " << m[5] << " " << m[8] << "\n";
    os << subIndent << "origin: " << this->LatticeOrigin[0] << " " << this->LatticeOrigin[1] << " "
       << this->LatticeOrigin[2] << "\n";
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

  os << indent << "Atomic number array name : " << this->GetAtomicNumberArrayName() << "\n";
  os << indent << "Bond orders array name : " << this->GetBondOrdersArrayName();
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::AppendAtom(unsigned short atomicNumber, double x, double y, double z)
{
  vtkUnsignedShortArray* atomicNums = this->GetAtomicNumberArray();

  assert(atomicNums);

  vtkIdType id;
  this->AddVertexInternal(nullptr, &id);

  atomicNums->InsertValue(id, atomicNumber);
  vtkIdType coordID = this->Points->InsertNextPoint(x, y, z);
  (void)coordID;
  assert("point ids synced with vertex ids" && coordID == id);

  this->Modified();
  return vtkAtom(this, id);
}

//----------------------------------------------------------------------------
vtkAtom vtkMolecule::GetAtom(vtkIdType atomId)
{
  assert(atomId >= 0 && atomId < this->GetNumberOfAtoms());

  vtkAtom atom(this, atomId);
  return atom;
}

//----------------------------------------------------------------------------
unsigned short vtkMolecule::GetAtomAtomicNumber(vtkIdType id)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());

  vtkUnsignedShortArray* atomicNums = this->GetAtomicNumberArray();

  return atomicNums->GetValue(id);
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomAtomicNumber(vtkIdType id, unsigned short atomicNum)
{
  assert(id >= 0 && id < this->GetNumberOfAtoms());

  vtkUnsignedShortArray* atomicNums = this->GetAtomicNumberArray();

  atomicNums->SetValue(id, atomicNum);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMolecule::SetAtomPosition(vtkIdType id, const vtkVector3f& pos)
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
  vtkFloatArray* positions = vtkArrayDownCast<vtkFloatArray>(this->Points->GetData());
  assert(positions != nullptr);
  float* data = positions->GetPointer(id * 3);
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
void vtkMolecule::GetAtomPosition(vtkIdType id, double pos[3])
{
  this->Points->GetPoint(id, pos);
}

//----------------------------------------------------------------------------
vtkIdType vtkMolecule::GetNumberOfAtoms()
{
  return this->GetNumberOfVertices();
}

//----------------------------------------------------------------------------
vtkBond vtkMolecule::AppendBond(
  const vtkIdType atom1, const vtkIdType atom2, const unsigned short order)
{
  vtkUnsignedShortArray* bondOrders = this->GetBondOrdersArray();

  assert(bondOrders);

  vtkEdgeType edgeType;
  this->AddEdgeInternal(atom1, atom2, false, nullptr, &edgeType);
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

  vtkIdTypeArray* bonds = this->GetBondList();
  // An array with two components holding the bonded atom's ids
  vtkIdType* ids = bonds->GetPointer(2 * bondId);
  return vtkBond(this, bondId, ids[0], ids[1]);
}

//----------------------------------------------------------------------------
void vtkMolecule::SetBondOrder(vtkIdType bondId, unsigned short order)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  vtkUnsignedShortArray* bondOrders = this->GetBondOrdersArray();
  assert(bondOrders);

  this->Modified();
  bondOrders->InsertValue(bondId, order);
}

//----------------------------------------------------------------------------
unsigned short vtkMolecule::GetBondOrder(vtkIdType bondId)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  vtkUnsignedShortArray* bondOrders = this->GetBondOrdersArray();

  return bondOrders ? bondOrders->GetValue(bondId) : 0;
}

//----------------------------------------------------------------------------
double vtkMolecule::GetBondLength(vtkIdType bondId)
{
  assert(bondId >= 0 && bondId < this->GetNumberOfBonds());

  // Get list of bonds
  vtkIdTypeArray* bonds = this->GetBondList();
  // An array of length two holding the bonded atom's ids
  vtkIdType* ids = bonds->GetPointer(bondId);

  // Get positions
  vtkVector3f pos1 = this->GetAtomPosition(ids[0]);
  vtkVector3f pos2 = this->GetAtomPosition(ids[1]);

  return (pos2 - pos1).Norm();
}

//----------------------------------------------------------------------------
vtkPoints* vtkMolecule::GetAtomicPositionArray()
{
  return this->Points;
}

//----------------------------------------------------------------------------
vtkUnsignedShortArray* vtkMolecule::GetAtomicNumberArray()
{
  vtkUnsignedShortArray* atomicNums = vtkArrayDownCast<vtkUnsignedShortArray>(
    this->GetVertexData()->GetScalars(this->GetAtomicNumberArrayName()));

  assert(atomicNums);

  return atomicNums;
}

//----------------------------------------------------------------------------
vtkUnsignedShortArray* vtkMolecule::GetBondOrdersArray()
{
  return vtkArrayDownCast<vtkUnsignedShortArray>(
    this->GetBondData()->GetScalars(this->GetBondOrdersArrayName()));
}

//----------------------------------------------------------------------------
vtkIdType vtkMolecule::GetNumberOfBonds()
{
  return this->GetNumberOfEdges();
}

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMolecule, ElectronicData, vtkAbstractElectronicData);

//----------------------------------------------------------------------------
void vtkMolecule::ShallowCopy(vtkDataObject* obj)
{
  vtkMolecule* m = vtkMolecule::SafeDownCast(obj);
  if (!m)
  {
    vtkErrorMacro("Can only shallow copy from vtkMolecule or subclass.");
    return;
  }
  this->ShallowCopyStructure(m);
  this->ShallowCopyAttributes(m);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopy(vtkDataObject* obj)
{
  vtkMolecule* m = vtkMolecule::SafeDownCast(obj);
  if (!m)
  {
    vtkErrorMacro("Can only deep copy from vtkMolecule or subclass.");
    return;
  }
  this->DeepCopyStructure(m);
  this->DeepCopyAttributes(m);
}

//----------------------------------------------------------------------------
bool vtkMolecule::CheckedShallowCopy(vtkGraph* g)
{
  bool result = this->Superclass::CheckedShallowCopy(g);
  this->BondListIsDirty = true;
  return result;
}

//----------------------------------------------------------------------------
bool vtkMolecule::CheckedDeepCopy(vtkGraph* g)
{
  bool result = this->Superclass::CheckedDeepCopy(g);
  this->BondListIsDirty = true;
  return result;
}

//----------------------------------------------------------------------------
void vtkMolecule::ShallowCopyStructure(vtkMolecule* m)
{
  this->CopyStructureInternal(m, false);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopyStructure(vtkMolecule* m)
{
  this->CopyStructureInternal(m, true);
}

//----------------------------------------------------------------------------
void vtkMolecule::ShallowCopyAttributes(vtkMolecule* m)
{
  this->CopyAttributesInternal(m, false);
}

//----------------------------------------------------------------------------
void vtkMolecule::DeepCopyAttributes(vtkMolecule* m)
{
  this->CopyAttributesInternal(m, true);
}

//----------------------------------------------------------------------------
void vtkMolecule::CopyStructureInternal(vtkMolecule* m, bool deep)
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

  if (!m->HasLattice())
  {
    this->ClearLattice();
  }
  else
  {
    if (deep)
    {
      vtkNew<vtkMatrix3x3> newLattice;
      newLattice->DeepCopy(m->Lattice);
      this->SetLattice(newLattice);
    }
    else
    {
      this->SetLattice(m->Lattice);
    }
    this->LatticeOrigin = m->LatticeOrigin;
  }

  this->BondListIsDirty = true;
}

//----------------------------------------------------------------------------
void vtkMolecule::CopyAttributesInternal(vtkMolecule* m, bool deep)
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
vtkIdTypeArray* vtkMolecule::GetBondList()
{
  // Create the edge list if it doesn't exist, or is marked as dirty.
  vtkIdTypeArray* edgeList = this->BondListIsDirty ? nullptr : this->GetEdgeList();
  if (!edgeList)
  {
    this->UpdateBondList();
    edgeList = this->GetEdgeList();
  }
  assert(edgeList != nullptr);
  return edgeList;
}

//----------------------------------------------------------------------------
bool vtkMolecule::GetPlaneFromBond(const vtkBond& bond, const vtkVector3f& normal, vtkPlane* plane)
{
  return vtkMolecule::GetPlaneFromBond(bond.GetBeginAtom(), bond.GetEndAtom(), normal, plane);
}

//----------------------------------------------------------------------------
bool vtkMolecule::GetPlaneFromBond(
  const vtkAtom& atom1, const vtkAtom& atom2, const vtkVector3f& normal, vtkPlane* plane)
{
  if (plane == nullptr)
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
  vtkVector3f proj(unitV[0] * n_iDotUnitV, unitV[1] * n_iDotUnitV, unitV[2] * n_iDotUnitV);
  // end vtkVector reimplementation TODO

  // Calculate actual normal:
  vtkVector3f realNormal(n_i - proj);

  // Create plane:
  vtkVector3f pos(atom1.GetPosition());
  plane->SetOrigin(pos.Cast<double>().GetData());
  plane->SetNormal(realNormal.Cast<double>().GetData());
  return true;
}

//------------------------------------------------------------------------------
bool vtkMolecule::HasLattice()
{
  return this->Lattice != nullptr;
}

//------------------------------------------------------------------------------
void vtkMolecule::ClearLattice()
{
  this->SetLattice(nullptr);
}

//------------------------------------------------------------------------------
void vtkMolecule::SetLattice(vtkMatrix3x3* matrix)
{
  if (!matrix)
  {
    if (this->Lattice)
    {
      // If we're clearing a matrix, zero out the origin:
      this->LatticeOrigin = vtkVector3d(0., 0., 0.);
      this->Lattice = nullptr;
      this->Modified();
    }
  }
  else if (this->Lattice != matrix)
  {
    this->Lattice = matrix;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMolecule::SetLattice(const vtkVector3d& a, const vtkVector3d& b, const vtkVector3d& c)
{
  if (this->Lattice == nullptr)
  {
    this->Lattice.TakeReference(vtkMatrix3x3::New());
    this->Modified();
  }

  double* mat = this->Lattice->GetData();
  if (mat[0] != a[0] || mat[1] != b[0] || mat[2] != c[0] || mat[3] != a[1] || mat[4] != b[1] ||
    mat[5] != c[1] || mat[6] != a[2] || mat[7] != b[2] || mat[8] != c[2])
  {
    mat[0] = a[0];
    mat[1] = b[0];
    mat[2] = c[0];
    mat[3] = a[1];
    mat[4] = b[1];
    mat[5] = c[1];
    mat[6] = a[2];
    mat[7] = b[2];
    mat[8] = c[2];
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkMatrix3x3* vtkMolecule::GetLattice()
{
  return this->Lattice;
}

//------------------------------------------------------------------------------
void vtkMolecule::GetLattice(vtkVector3d& a, vtkVector3d& b, vtkVector3d& c)
{
  if (this->Lattice)
  {
    double* mat = this->Lattice->GetData();
    a[0] = mat[0];
    a[1] = mat[3];
    a[2] = mat[6];
    b[0] = mat[1];
    b[1] = mat[4];
    b[2] = mat[7];
    c[0] = mat[2];
    c[1] = mat[5];
    c[2] = mat[8];
  }
  else
  {
    a = b = c = vtkVector3d(0., 0., 0.);
  }
}

//------------------------------------------------------------------------------
void vtkMolecule::GetLattice(vtkVector3d& a, vtkVector3d& b, vtkVector3d& c, vtkVector3d& origin)
{
  if (this->Lattice)
  {
    double* mat = this->Lattice->GetData();
    a[0] = mat[0];
    a[1] = mat[3];
    a[2] = mat[6];
    b[0] = mat[1];
    b[1] = mat[4];
    b[2] = mat[7];
    c[0] = mat[2];
    c[1] = mat[5];
    c[2] = mat[8];
    origin = this->LatticeOrigin;
  }
  else
  {
    a = b = c = origin = vtkVector3d(0., 0., 0.);
  }
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkMolecule::GetAtomGhostArray()
{
  return vtkArrayDownCast<vtkUnsignedCharArray>(
    this->GetVertexData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
}

//----------------------------------------------------------------------------
void vtkMolecule::AllocateAtomGhostArray()
{
  if (this->GetAtomGhostArray() == nullptr)
  {
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfComponents(1);
    ghosts->SetNumberOfTuples(this->GetNumberOfAtoms());
    ghosts->FillComponent(0, 0);
    this->GetVertexData()->AddArray(ghosts);
  }
  else
  {
    this->GetAtomGhostArray()->SetNumberOfTuples(this->GetNumberOfAtoms());
  }
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkMolecule::GetBondGhostArray()
{
  return vtkArrayDownCast<vtkUnsignedCharArray>(
    this->GetEdgeData()->GetArray(vtkDataSetAttributes::GhostArrayName()));
}

//----------------------------------------------------------------------------
void vtkMolecule::AllocateBondGhostArray()
{
  if (this->GetBondGhostArray() == nullptr)
  {
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfComponents(1);
    ghosts->SetNumberOfTuples(this->GetNumberOfBonds());
    ghosts->FillComponent(0, 0);
    this->GetEdgeData()->AddArray(ghosts);
  }
  else
  {
    this->GetBondGhostArray()->SetNumberOfTuples(this->GetNumberOfBonds());
  }
}

//----------------------------------------------------------------------------
int vtkMolecule::Initialize(
  vtkPoints* atomPositions, vtkDataArray* atomicNumberArray, vtkDataSetAttributes* atomData)
{
  // Start with default initialization the molecule
  this->Initialize();

  // if no atomicNumberArray given, look for one in atomData
  if (!atomicNumberArray && atomData)
  {
    atomicNumberArray = atomData->GetArray(this->GetAtomicNumberArrayName());
  }

  if (!atomPositions && !atomicNumberArray)
  {
    vtkDebugMacro(<< "Atom position and atomic numbers were not found: skip atomic data.");
    return 1;
  }

  if (!atomPositions || !atomicNumberArray)
  {
    vtkDebugMacro(<< "Empty atoms or atomic numbers.");
    return 0;
  }

  // ensure it is a short array
  vtkNew<vtkUnsignedShortArray> newAtomicNumberShortArray;
  if (vtkUnsignedShortArray::SafeDownCast(atomicNumberArray))
  {
    newAtomicNumberShortArray->ShallowCopy(atomicNumberArray);
  }
  else
  {
    vtkIdType nbPoints = atomicNumberArray->GetNumberOfTuples();
    newAtomicNumberShortArray->SetNumberOfComponents(1);
    newAtomicNumberShortArray->SetNumberOfTuples(nbPoints);
    newAtomicNumberShortArray->SetName(atomicNumberArray->GetName());
    for (vtkIdType i = 0; i < nbPoints; i++)
    {
      newAtomicNumberShortArray->SetTuple1(i, atomicNumberArray->GetTuple1(i));
    }
  }

  int nbAtoms = atomPositions->GetNumberOfPoints();
  if (nbAtoms != newAtomicNumberShortArray->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of atoms not equal to number of atomic numbers.");
    return 0;
  }
  if (atomData && nbAtoms != atomData->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of atoms not equal to number of atom properties.");
    return 0;
  }

  static const std::string atomicNumberName = this->GetAtomicNumberArrayName();

  // update atoms positions
  this->ForceOwnership();
  this->Internals->Adjacency.resize(nbAtoms, vtkVertexAdjacencyList());
  this->SetPoints(atomPositions);

  // if atom properties exists, copy it in VertexData and look for an atomic number in its arrays.
  if (atomData)
  {
    this->GetVertexData()->ShallowCopy(atomData);

    // if atomData contains an array with the atomic number name,
    // copy this array with a new name as we will overwrite it.
    vtkDataArray* otherArray = atomData->GetArray(atomicNumberName.c_str());
    if (otherArray && otherArray != static_cast<vtkAbstractArray*>(atomicNumberArray))
    {
      this->GetVertexData()->RemoveArray(atomicNumberName.c_str());

      // create a new name for the copied array.
      std::string newName = std::string("Original ") + atomicNumberName;

      // if the new name is available create a copy of the array with another name, and add it
      // else no backup is done, array will be replaced.
      if (!atomData->GetArray(newName.c_str()))
      {
        vtkDataArray* otherArrayCopy = otherArray->NewInstance();
        otherArrayCopy->ShallowCopy(otherArray);
        otherArrayCopy->SetName(newName.c_str());
        this->GetVertexData()->AddArray(otherArrayCopy);
        otherArrayCopy->Delete();
      }
      else
      {
        vtkWarningMacro(<< "Array '" << atomicNumberName << "' was replaced.");
      }
    }
  }

  // add atomic number array: if given array has the expected name, add it directly
  // (it will replace the current one). Else copy it and add it with atomic number name.
  if (atomicNumberName == newAtomicNumberShortArray->GetName())
  {
    this->GetVertexData()->AddArray(newAtomicNumberShortArray);
  }
  else
  {
    vtkNew<vtkUnsignedShortArray> atomicNumberArrayCopy;
    atomicNumberArrayCopy->ShallowCopy(newAtomicNumberShortArray);
    atomicNumberArrayCopy->SetName(atomicNumberName.c_str());
    this->GetVertexData()->AddArray(atomicNumberArrayCopy.Get());
  }

  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
int vtkMolecule::Initialize(vtkMolecule* molecule)
{
  if (molecule == nullptr)
  {
    this->Initialize();
    return 1;
  }

  return this->Initialize(
    molecule->GetPoints(), molecule->GetAtomicNumberArray(), molecule->GetVertexData());
}

//----------------------------------------------------------------------------
vtkMolecule* vtkMolecule::GetData(vtkInformation* info)
{
  return info ? vtkMolecule::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//----------------------------------------------------------------------------
vtkMolecule* vtkMolecule::GetData(vtkInformationVector* v, int i)
{
  return vtkMolecule::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
unsigned long vtkMolecule::GetActualMemorySize()
{
  unsigned long size = this->Superclass::GetActualMemorySize();
  if (this->ElectronicData)
  {
    size += this->ElectronicData->GetActualMemorySize();
  }
  if (this->AtomGhostArray)
  {
    size += this->AtomGhostArray->GetActualMemorySize();
  }
  if (this->BondGhostArray)
  {
    size += this->BondGhostArray->GetActualMemorySize();
  }
  return size;
}
