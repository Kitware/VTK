/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMolecule.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMolecule - class describing a molecule
//
// .SECTION Description
//
// vtkMolecule and the convenience classes vtkAtom and vtkBond
// describe the geometry and connectivity of a molecule. The molecule
// can be constructed using the AppendAtom() and AppendBond() methods in one
// of two ways; either by fully specifying the atom/bond in a single
// call, or by incrementally setting the various attributes using the
// convience vtkAtom and vtkBond classes:
//
// Single call:
// \code
// vtkMolecule *mol = vtkMolecule::New();
// vtkAtom h1 = mol->AppendAtom(1, 0.0, 0.0, -0.5);
// vtkAtom h2 = mol->AppendAtom(1, 0.0, 0.0,  0.5);
// vtkBond b  = mol->AppendBond(h1, h2, 1);
// \endcode
//
// Incremental:
// \code
// vtkMolecule *mol = vtkMolecule::New();
//
// vtkAtom h1 = mol->AppendAtom();
// h1.SetAtomicNumber(1);
// h1.SetPosition(0.0, 0.0, -0.5);
//
// vtkAtom h2 = mol->AppendAtom();
// h2.SetAtomicNumber(1);
// vtkVector3d displacement (0.0, 0.0, 1.0);
// h2.SetPosition(h1.GetPositionAsVector3d() + displacement);
//
// vtkBond b  = mol->AppendBond(h1, h2, 1);
// \endcode
//
// Both of the above methods will produce the same molecule, two
// hydrogens connected with a 1.0 Angstrom single bond, aligned to the
// z-axis. The second example also demostrates the use of VTK's
// vtkVector class, which is fully supported by the Chemistry kit.
//
// The vtkMolecule object is intended to be used with the
// vtkMoleculeMapper class for visualizing molecular structure using
// common rendering techniques.
//
// \warning While direct use of the underlying vtkUndirectedGraph
// structure is possible due to vtkMolecule's public inheritance, this
// should not be relied upon and may change in the future.
//
// .SECTION See Also
// vtkAtom vtkBond vtkMoleculeMapper vtkPeriodicTable

#ifndef vtkMolecule_h
#define vtkMolecule_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUndirectedGraph.h"

//BTX
#include "vtkAtom.h" // Simple proxy class dependent on vtkMolecule
#include "vtkBond.h" // Simple proxy class dependent on vtkMolecule
//ETX
#include "vtkVector.h" // Small templated vector convenience class

class vtkPlane;
class vtkAbstractElectronicData;
class vtkPoints;
class vtkUnsignedShortArray;

class VTKCOMMONDATAMODEL_EXPORT vtkMolecule : public vtkUndirectedGraph
{
public:
  static vtkMolecule *New();
  vtkTypeMacro(vtkMolecule,vtkUndirectedGraph);
  void PrintSelf(ostream &os, vtkIndent indent);
  virtual void Initialize();

  // Description:
  // Return what type of dataset this is.
  virtual int GetDataObjectType() {return VTK_MOLECULE;}

//BTX
  // Description:
  // Add new atom with atomic number 0 (dummy atom) at origin. Return
  // a vtkAtom that refers to the new atom.
  vtkAtom AppendAtom()
  {
    return this->AppendAtom(0, vtkVector3f(0, 0, 0));
  }

  // Description:
  // Add new atom with the specified atomic number and position. Return a
  // vtkAtom that refers to the new atom.
  vtkAtom AppendAtom(unsigned short atomicNumber, const vtkVector3f &pos);

  // Description:
  // Convenience methods to append a new atom with the specified atomic number
  // and position.
  vtkAtom AppendAtom(unsigned short atomicNumber, double x, double y, double z)
  {
    return this->AppendAtom(atomicNumber, vtkVector3f(x, y, z));
  }

  // Description:
  // Return a vtkAtom that refers to the atom with the specified id.
  vtkAtom GetAtom(vtkIdType atomId);

  // Description:
  // Return the number of atoms in the molecule.
  vtkIdType GetNumberOfAtoms();

  // Description
  // Add a bond between the specified atoms, optionally setting the
  // bond order (default: 1). Return a vtkBond object referring to the
  // new bond.
  vtkBond AppendBond(vtkIdType atom1, vtkIdType atom2,
                     unsigned short order = 1);
  vtkBond AppendBond(const vtkAtom &atom1, const vtkAtom &atom2,
                     unsigned short order = 1)
  {
    return this->AppendBond(atom1.Id, atom2.Id, order);
  }

  // Description:
  // Return a vtkAtom that refers to the bond with the specified id.
  vtkBond GetBond(vtkIdType bondId);
//ETX

  // Description:
  // Return the number of bonds in the molecule.
  vtkIdType GetNumberOfBonds();

  // Description:
  // Return the atomic number of the atom with the specified id.
  unsigned short GetAtomAtomicNumber(vtkIdType atomId);

  // Description:
  // Set the atomic number of the atom with the specified id.
  void SetAtomAtomicNumber(vtkIdType atomId,
                           unsigned short atomicNum);

  // Description:
  // Set the position of the atom with the specified id.
  void SetAtomPosition(vtkIdType atomId, const vtkVector3f &pos);
  void SetAtomPosition(vtkIdType atomId, double x, double y, double z);

  // Description:
  // Get the position of the atom with the specified id.
  vtkVector3f GetAtomPosition(vtkIdType atomId);
  void GetAtomPosition(vtkIdType atomId, float pos[3]);

  // Description
  // Get/Set the bond order of the bond with the specified id
  void SetBondOrder(vtkIdType bondId, unsigned short order);
  unsigned short GetBondOrder(vtkIdType bondId);

  // Description
  // Get the bond length of the bond with the specified id
  //
  // \note If the associated vtkBond object is already available,
  // vtkBond::GetBondLength is potentially much faster than this
  // function, as a list of all bonds may need to be constructed to
  // locate the appropriate bond.
  // \sa UpdateBondList()
  double GetBondLength(vtkIdType bondId);

  // Description:
  // Access the raw arrays used in this vtkMolecule instance
  vtkPoints * GetAtomicPositionArray();
  vtkUnsignedShortArray * GetAtomicNumberArray();

//BTX
  // Description:
  // Set/Get the AbstractElectronicData-subclassed object for this molecule.
  vtkGetObjectMacro(ElectronicData, vtkAbstractElectronicData);
  virtual void SetElectronicData(vtkAbstractElectronicData*);
//ETX

  // Description:
  // Shallow copies the data object into this molecule.
  virtual void ShallowCopy(vtkDataObject *obj);

  // Description:
  // Deep copies the data object into this molecule.
  virtual void DeepCopy(vtkDataObject *obj);

  // Description:
  // Shallow copies the atoms and bonds from @a m into @a this.
  virtual void ShallowCopyStructure(vtkMolecule *m);

  // Description:
  // Deep copies the atoms and bonds from @a m into @a this.
  virtual void DeepCopyStructure(vtkMolecule *m);

  // Description:
  // Shallow copies attributes (i.e. everything besides atoms and bonds) from
  // @a m into @a this.
  virtual void ShallowCopyAttributes(vtkMolecule *m);

  // Description:
  // Deep copies attributes (i.e. everything besides atoms and bonds) from
  // @a m into @a this.
  virtual void DeepCopyAttributes(vtkMolecule *m);

  // Description:
  // Obtain the plane that passes through the indicated bond with the given
  // normal. If the plane is set successfully, the function returns true.
  //
  // If the normal is not orthogonal to the bond, a new normal will be
  // constructed in such a way that the plane will be orthogonal to
  // the plane spanned by the bond vector and the input normal vector.
  //
  // This ensures that the plane passes through the bond, and the
  // normal is more of a "hint" indicating the orientation of the plane.
  //
  // The new normal (n) is defined as the input normal vector (n_i) minus
  // the projection of itself (proj[n_i]_v) onto the bond vector (v):
  //
  // @verbatim
  //              v ^
  //                |  n = (n_i - proj[n_j]_v)
  // proj[n_i]_v ^  |----x
  //             |  |   /
  //             |  |  / n_i
  //             |  | /
  //             |  |/
  // @endverbatim
  //
  // If n_i is parallel to v, a warning will be printed and no plane will be
  // added. Obviously, n_i must not be parallel to v.
  static bool GetPlaneFromBond(const vtkBond &bond, const vtkVector3f &normal,
                               vtkPlane *plane);
  static bool GetPlaneFromBond(const vtkAtom &atom1, const vtkAtom &atom2,
                               const vtkVector3f &normal, vtkPlane *plane);

 protected:
  vtkMolecule();
  ~vtkMolecule();

  // Description:
  // Copy bonds and atoms.
  virtual void CopyStructureInternal(vtkMolecule *m, bool deep);

  // Description:
  // Copy everything but bonds and atoms.
  virtual void CopyAttributesInternal(vtkMolecule *m, bool deep);

  // Description:
  // The graph superclass does not provide fast random access to the
  // edge (bond) data. All random access is performed using a lookup
  // table that must be rebuilt periodically. These allow for lazy
  // building of the lookup table
  bool BondListIsDirty;
  void SetBondListDirty() {this->BondListIsDirty = true;}
  void UpdateBondList();

  friend class vtkAtom;
  friend class vtkBond;

  vtkAbstractElectronicData *ElectronicData;

private:
  vtkMolecule(const vtkMolecule&);    // Not implemented.
  void operator=(const vtkMolecule&); // Not implemented.
};

#endif
