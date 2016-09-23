/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridBase
 * @brief   dataset represents arbitrary combinations
 * of all possible cell types. May be mapped onto a non-standard memory layout.
 *
 *
 * vtkUnstructuredGridBase defines the core vtkUnstructuredGrid API, omitting
 * functions that are implementation dependent.
 *
 * @sa
 * vtkMappedDataArray vtkUnstructuredGrid
*/

#ifndef vtkUnstructuredGridBase_h
#define vtkUnstructuredGridBase_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointSet.h"

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridBase : public vtkPointSet
{
public:
  vtkAbstractTypeMacro(vtkUnstructuredGridBase,vtkPointSet)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE
  {
    this->Superclass::PrintSelf(os, indent);
  }

  int GetDataObjectType() VTK_OVERRIDE { return VTK_UNSTRUCTURED_GRID_BASE; }

  /**
   * Allocate memory for the number of cells indicated. extSize is not used.
   */
  virtual void Allocate(vtkIdType numCells=1000, int extSize=1000) = 0;

  /**
   * Shallow and Deep copy.
   */
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;

  /**
   * Insert/create cell in object by type and list of point ids defining
   * cell topology. Most cells require just a type which implicitly defines
   * a set of points and their ordering. For non-polyhedron cell type, npts
   * is the number of unique points in the cell. pts are the list of global
   * point Ids. For polyhedron cell, a special input format is required.
   * npts is the number of faces in the cell. ptIds is the list of face stream:
   * (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   */
  virtual vtkIdType InsertNextCell(int type, vtkIdType npts,
                                   vtkIdType *ptIds) = 0;

  /**
   * Insert/create cell in object by a list of point ids defining
   * cell topology. Most cells require just a type which implicitly defines
   * a set of points and their ordering. For non-polyhedron cell type, ptIds
   * is the list of global Ids of unique cell points. For polyhedron cell,
   * a special ptIds input format is required:
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   */
  virtual vtkIdType InsertNextCell(int type, vtkIdList *ptIds) = 0;

  // Desciption:
  // Insert/create a polyhedron cell. npts is the number of unique points in
  // the cell. pts is the list of the unique cell point Ids. nfaces is the
  // number of faces in the cell. faces is the face-stream
  // [numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...].
  // All point Ids are global.
  virtual vtkIdType InsertNextCell(int type, vtkIdType npts, vtkIdType *ptIds,
                                   vtkIdType nfaces, vtkIdType *faces) = 0;

  /**
   * Replace the points defining cell "cellId" with a new set of points. This
   * operator is (typically) used when links from points to cells have not been
   * built (i.e., BuildLinks() has not been executed). Use the operator
   * ReplaceLinkedCell() to replace a cell when cell structure has been built.
   */
  virtual void ReplaceCell(vtkIdType cellId, int npts, vtkIdType *pts) = 0;

  /**
   * Fill vtkIdTypeArray container with list of cell Ids.  This
   * method traverses all cells and, for a particular cell type,
   * inserts the cell Id into the container.
   */
  virtual void GetIdsOfCellsOfType(int type, vtkIdTypeArray *array) = 0;

  /**
   * Traverse cells and determine if cells are all of the same type.
   */
  virtual int IsHomogeneous() = 0;

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkUnstructuredGridBase* GetData(vtkInformation* info);
  static vtkUnstructuredGridBase* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkUnstructuredGridBase();
  ~vtkUnstructuredGridBase() VTK_OVERRIDE;

private:
  vtkUnstructuredGridBase(const vtkUnstructuredGridBase&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridBase&) VTK_DELETE_FUNCTION;
};

#endif
