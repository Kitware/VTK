// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPExodusIIElementBlock
 * @brief   Uses an Exodus II element block as a
 *  vtkMappedUnstructuredGrid's implementation.
 *
 *
 * This class allows raw data arrays returned by the Exodus II library to be
 * used directly in VTK without repacking the data into the vtkUnstructuredGrid
 * memory layout. Use the vtkCPExodusIIInSituReader to read an Exodus II file's
 * data into this structure.
 */

#ifndef vtkCPExodusIIElementBlock_h
#define vtkCPExodusIIElementBlock_h

#include "vtkIOExodusModule.h" // For export macro
#include "vtkObject.h"

#include "vtkMappedUnstructuredGrid.h" // For mapped unstructured grid wrapper

#include <string> // For std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericCell;

class VTKIOEXODUS_EXPORT vtkCPExodusIIElementBlockImpl : public vtkObject
{
public:
  static vtkCPExodusIIElementBlockImpl* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkCPExodusIIElementBlockImpl, vtkObject);

  /**
   * Set the Exodus element block data. 'elements' is the array returned from
   * ex_get_elem_conn. 'type', 'numElements', and 'nodesPerElement' are obtained
   * from ex_get_elem_block. Returns true or false depending on whether or not
   * the element type can be translated into a VTK cell type. This object takes
   * ownership of the elements array unless this function returns false.
   */
  bool SetExodusConnectivityArray(
    int* elements, const std::string& type, int numElements, int nodesPerElement);

  // API for vtkMappedUnstructuredGrid's implementation.
  vtkIdType GetNumberOfCells();
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds);
  void GetFaceStream(vtkIdType cellId, vtkIdList* ptIds);
  void GetPolyhedronFaces(vtkIdType cellId, vtkCellArray* faces);
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds);
  int GetMaxCellSize();
  void GetIdsOfCellsOfType(int type, vtkIdTypeArray* array);
  int IsHomogeneous();

  // This container is read only -- these methods do nothing but print a
  // warning.
  void Allocate(vtkIdType numCells, int extSize = 1000);
  vtkIdType InsertNextCell(int type, vtkIdList* ptIds);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[])
    VTK_SIZEHINT(ptIds, npts);
  vtkIdType InsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[], vtkCellArray* faces)
    VTK_SIZEHINT(ptIds, npts);

  void ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

protected:
  vtkCPExodusIIElementBlockImpl();
  ~vtkCPExodusIIElementBlockImpl() override;

private:
  vtkCPExodusIIElementBlockImpl(const vtkCPExodusIIElementBlockImpl&) = delete;
  void operator=(const vtkCPExodusIIElementBlockImpl&) = delete;

  // Convert between Exodus node ids and VTK point ids.
  static vtkIdType NodeToPoint(const int& id) { return static_cast<vtkIdType>(id - 1); }
  static int PointToNode(const vtkIdType& id) { return static_cast<int>(id + 1); }

  // Convenience methods to get pointers into the element array.
  int* GetElementStart(vtkIdType cellId) const
  {
    return this->Elements + (cellId * this->CellSize);
  }
  int* GetElementEnd(vtkIdType cellId) const
  {
    return this->Elements + (cellId * this->CellSize) + this->CellSize;
  }
  int* GetStart() const { return this->Elements; }
  int* GetEnd() const { return this->Elements + (this->NumberOfCells * this->CellSize); }

  int* Elements;
  int CellType;
  int CellSize;
  vtkIdType NumberOfCells;
};

vtkMakeExportedMappedUnstructuredGrid(
  vtkCPExodusIIElementBlock, vtkCPExodusIIElementBlockImpl, VTKIOEXODUS_EXPORT);

VTK_ABI_NAMESPACE_END
#endif // vtkCPExodusIIElementBlock_h
