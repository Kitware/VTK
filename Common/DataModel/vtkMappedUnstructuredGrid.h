/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMappedUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMappedUnstructuredGrid - Allows datasets with arbitrary storage
// layouts to be used with VTK.
//
// .SECTION Description
// This class fulfills the vtkUnstructuredGridBase API while delegating to a
// arbitrary implementation of the dataset topology. The purpose of
// vtkMappedUnstructuredGrid is to allow external data structures to be used
// directly in a VTK pipeline, e.g. for in-situ analysis of a running
// simulation.
//
// When introducing an external data structure into VTK, there are 3 principle
// components of the dataset to consider:
// - Points
// - Cells (topology)
// - Point/Cell attributes
//
// Points and attributes can be handled by subclassing vtkMappedDataArray and
// implementing that interface to adapt the external data structures into VTK.
// The vtkMappedDataArray subclasses can then be used as the vtkPoints's Data
// member (for points/nodes) or added directly to vtkPointData, vtkCellData, or
// vtkFieldData for attribute information. Filters used in the pipeline will
// need to be modified to remove calls to vtkDataArray::GetVoidPointer and use
// vtkTypedDataArrayIterators instead.
// See vtkDataArrayIteratorMacro.h for a (relatively) simple way to write
// processing algorithms that will use efficient raw memory accesses for
// standard VTK data arrays and safe iterators for non-standard data arrays in a
// single templated implementation.
//
// Introducing an arbitrary topology implementation into VTK requires the use of
// the vtkMappedUnstructuredGrid class. Unlike the data array counterpart, the
// mapped unstructured grid is not subclassed, but rather takes an adaptor
// class as a template argument. This is to allow cheap shallow copies of the
// data by passing the reference-counted implementation object to new instances
// of vtkMappedUnstructuredGrid.
//
// The implementation class should derive from vtkObject (for reference
// counting) and implement the usual vtkObject API requirements, such as a
// static New() method and PrintSelf function. The following methods must also
// be implemented:
// - vtkIdType GetNumberOfCells()
// - int GetCellType(vtkIdType cellId)
// - void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
// - void GetPointCells(vtkIdType ptId, vtkIdList *cellIds)
// - int GetMaxCellSize()
// - void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array)
// - int IsHomogeneous()
// - void Allocate(vtkIdType numCells, int extSize = 1000)
// - vtkIdType InsertNextCell(int type, vtkIdList *ptIds)
// - vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds)
// - vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
//                            vtkIdType nfaces, vtkIdType *faces)
// - void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts)
//
// These methods should provide the same functionality as defined in
// vtkUnstructuredGrid. See that class's documentation for more information.
//
// Note that since the implementation class is used as a compile-time template
// parameter in vtkMappedUnstructuredGrid, the above methods do not need be
// virtuals. The compiler will statically bind the calls, making dynamic vtable
// lookups unneccessary and giving a slight performance boost.
//
// Adapting a filter or algorithm to safely traverse the
// vtkMappedUnstructuredGrid's topology requires removing calls the following
// implementation-dependent vtkUnstructuredGrid methods:
// - vtkUnstructuredGrid::GetCellTypesArray()
// - vtkUnstructuredGrid::GetCellLocationsArray()
// - vtkUnstructuredGrid::GetCellLinks()
// - vtkUnstructuredGrid::GetCells()
// Access to the values returned by these methods should be replaced by the
// equivalent random-access lookup methods in the vtkUnstructuredGridBase API,
// or use vtkCellIterator (see vtkDataSet::NewCellIterator) for sequential
// access.
//
// A custom vtkCellIterator implementation may be specified for a particular
// vtkMappedUnstructuredGrid as the second template parameter. By default,
// vtkMappedUnstructuredGridCellIterator will be used, which increments an
// internal cell id counter and performs random-access lookup as-needed. More
// efficient implementations may be used with data structures better suited for
// sequential access, see vtkUnstructuredGridCellIterator for an example.
//
// A set of four macros are provided to generate a concrete subclass of
// vtkMappedUnstructuredGrid with a specified implementation, cell iterator,
// and export declaration. They are:
// - vtkMakeMappedUnstructuredGrid(_className, _impl)
//   - Create a subclass of vtkMappedUnstructuredGrid using _impl implemenation
//     that is named _className.
// - vtkMakeMappedUnstructuredGridWithIter(_className, _impl, _cIter)
//   - Create a subclass of vtkMappedUnstructuredGrid using _impl implemenation
//     and _cIter vtkCellIterator that is named _className.
// - vtkMakeExportedMappedUnstructuredGrid(_className, _impl, _exportDecl)
//   - Create a subclass of vtkMappedUnstructuredGrid using _impl implemenation
//     that is named _className. _exportDecl is used to decorate the class
//     declaration.
// - vtkMakeExportedMappedUnstructuredGridWithIter(_className, _impl, _cIter, _exportDecl)
//   - Create a subclass of vtkMappedUnstructuredGrid using _impl implemenation
//     and _cIter vtkCellIterator that is named _className. _exportDecl is used
//     to decorate the class declaration.
//
// To instantiate a vtkMappedUnstructuredGrid subclass created by the above
// macro, the follow pattern is encouraged:
//
// @code
// MyGrid.h:
// ----------------------------------------------------------------------
// class MyGridImplementation : public vtkObject
// {
// public:
//   ... (vtkObject required API) ...
//   ... (vtkMappedUnstructuredGrid Implementation required API) ...
//   void SetImplementationDetails(...raw data from external source...);
// };
//
// vtkMakeMappedUnstructuredGrid(MyGrid, MyGridImplementation)
//
// SomeSource.cxx
// ----------------------------------------------------------------------
// vtkNew<MyGrid> grid;
// grid->GetImplementation()->SetImplementationDetails(...);
// /* grid is now ready to use. */
// @endcode
//
// The vtkCPExodusIIElementBlock class provides an example of
// vtkMappedUnstructuredGrid usage, adapting the Exodus II data structures for
// the VTK pipeline.

#ifndef vtkMappedUnstructuredGrid_h
#define vtkMappedUnstructuredGrid_h

#include "vtkUnstructuredGridBase.h"

#include "vtkMappedUnstructuredGridCellIterator.h" // For default cell iterator
#include "vtkNew.h" // For vtkNew
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include "vtkTypeTemplate.h" // For vtkTypeTemplate

template <class Implementation,
          class CellIterator = vtkMappedUnstructuredGridCellIterator<Implementation> >
class vtkMappedUnstructuredGrid:
    public vtkTypeTemplate<vtkMappedUnstructuredGrid<Implementation, CellIterator>,
                           vtkUnstructuredGridBase>
{
public:
  typedef
    vtkTypeTemplate<vtkMappedUnstructuredGrid<Implementation, CellIterator>,
      vtkUnstructuredGridBase> Superclass;
  typedef Implementation ImplementationType;
  typedef CellIterator CellIteratorType;

  // Virtuals from various base classes:
  void PrintSelf(ostream &os, vtkIndent indent);
  void CopyStructure(vtkDataSet *pd);
  void ShallowCopy(vtkDataObject *src);
  vtkIdType GetNumberOfCells();
  vtkCell* GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  vtkCellIterator* NewCellIterator();
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);
  int GetMaxCellSize();
  void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array);
  int IsHomogeneous();
  void Allocate(vtkIdType numCells, int extSize = 1000);
  vtkIdType InsertNextCell(int type, vtkIdList *ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
                           vtkIdType nfaces, vtkIdType *faces);
  void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts);
  unsigned long GetMTime();

  void SetImplementation(ImplementationType *impl);
  ImplementationType *GetImplementation();

protected:
  vtkMappedUnstructuredGrid();
  ~vtkMappedUnstructuredGrid();

  // For convenience...
  typedef vtkMappedUnstructuredGrid<Implementation, CellIterator> ThisType;

  vtkSmartPointer<ImplementationType> Impl;

private:
  vtkMappedUnstructuredGrid(const vtkMappedUnstructuredGrid &); // Not implemented.
  void operator=(const vtkMappedUnstructuredGrid &);   // Not implemented.

  vtkNew<vtkGenericCell> TempCell;
};

#include "vtkMappedUnstructuredGrid.txx"

// We need to fake the superclass for the wrappers, otherwise they will choke on
// the template:
#ifndef __WRAP__

#define vtkMakeExportedMappedUnstructuredGrid(_className, _impl, _exportDecl) \
class _exportDecl _className : \
    public vtkMappedUnstructuredGrid<_impl> \
{ \
public: \
  vtkTypeMacro(_className, \
               vtkMappedUnstructuredGrid<_impl>) \
  static _className* New(); \
protected: \
  _className() \
  { \
    _impl *i = _impl::New(); \
    this->SetImplementation(i); \
    i->Delete(); \
  } \
  ~_className() {} \
private: \
  _className(const _className&); \
  void operator=(const _className&); \
};

#define vtkMakeExportedMappedUnstructuredGridWithIter(_className, _impl, _cIter, _exportDecl) \
class _exportDecl _className : \
  public vtkMappedUnstructuredGrid<_impl, _cIter> \
{ \
public: \
  vtkTypeMacro(_className, \
               vtkMappedUnstructuredGrid<_impl, _cIter>) \
  static _className* New(); \
protected: \
  _className() \
  { \
    _impl *i = _impl::New(); \
    this->SetImplementation(i); \
    i->Delete(); \
  } \
  ~_className() {} \
private: \
  _className(const _className&); \
  void operator=(const _className&); \
};

#else // __WRAP__

#define vtkMakeExportedMappedUnstructuredGrid(_className, _impl, _exportDecl) \
  class _exportDecl _className : \
  public vtkUnstructuredGridBase \
{ \
public: \
  vtkTypeMacro(_className, vtkUnstructuredGridBase) \
  static _className* New(); \
protected: \
  _className() {} \
  ~_className() {} \
private: \
  _className(const _className&); \
  void operator=(const _className&); \
};

#define vtkMakeExportedMappedUnstructuredGridWithIter(_className, _impl, _cIter, _exportDecl) \
  class _exportDecl _className : \
  public vtkUnstructuredGridBase \
{ \
public: \
  vtkTypeMacro(_className, vtkUnstructuredGridBase) \
  static _className* New(); \
protected: \
  _className() {} \
  ~_className() {} \
private: \
  _className(const _className&); \
  void operator=(const _className&); \
};

#endif // __WRAP__

#define vtkMakeMappedUnstructuredGrid(_className, _impl) \
  vtkMakeExportedMappedUnstructuredGrid(_className, _impl, )

#define vtkMakeMappedUnstructuredGridWithIter(_className, _impl, _cIter, _exportDecl) \
  vtkMakeExportedMappedUnstructuredGridWithIter(_className, _impl, _cIter, )

#endif //vtkMappedUnstructuredGrid_h

// VTK-HeaderTest-Exclude: vtkMappedUnstructuredGrid.h
