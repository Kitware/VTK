// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellLinks
 * @brief   object represents upward pointers from points to list of cells using each point
 *
 * vtkCellLinks is a supplemental object to vtkCellArray and vtkCellTypes,
 * enabling access from points to the cells using the points. vtkCellLinks is
 * a list of cell ids, each such link representing a dynamic list of cell ids
 * using the point. The information provided by this object can be used to
 * determine neighbors and construct other local topological information.
 *
 * @warning
 * vtkCellLinks supports incremental (i.e., "editable") operations such as
 * inserting a new cell, or deleting a point. Because of this, it is less
 * memory efficient, and slower to construct and delete than static classes
 * such as vtkStaticCellLinks or vtkStaticCellLinksTemplate. However these
 * other classes are typically meant for one-time (static) construction.
 *
 * @sa
 * vtkCellArray vtkCellTypes vtkStaticCellLinks vtkStaticCellLinksTemplate
 */

#ifndef vtkCellLinks_h
#define vtkCellLinks_h

#include "vtkAbstractCellLinks.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include <memory> // For shared_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCellArray;

class VTKCOMMONDATAMODEL_EXPORT vtkCellLinks : public vtkAbstractCellLinks
{
public:
  class Link
  {
  public:
    Link()
      : ncells(0)
      , cells(nullptr)
    {
    }
    ~Link() = default;
    vtkIdType ncells;
    vtkIdType* cells;
  };

  ///@{
  /**
   * Standard methods to instantiate, print, and obtain type information.
   */
  static vtkCellLinks* New();
  vtkTypeMacro(vtkCellLinks, vtkAbstractCellLinks);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Build the link list array from the input dataset.
   */
  void BuildLinks() override;

  /**
   * Allocate the specified number of links (i.e., number of points) that
   * will be built.
   */
  void Allocate(vtkIdType numLinks, vtkIdType ext = 1000);

  /**
   * Clear out any previously allocated data structures
   */
  void Initialize() override;

  /**
   * Get a link structure given a point id.
   */
  Link& GetLink(vtkIdType ptId) { return this->Array[ptId]; }

  /**
   * Get the number of cells using the point specified by ptId.
   */
  vtkIdType GetNcells(vtkIdType ptId) VTK_FUTURE_CONST { return this->Array[ptId].ncells; }

  /**
   * Return a list of cell ids using the point.
   */
  vtkIdType* GetCells(vtkIdType ptId) { return this->Array[ptId].cells; }

  ///@{
  /**
   * Select all cells with a point degree in the range [minDegree,maxDegree).
   * The degree is the number of cells using a point. The selection is
   * indicated through the provided unsigned char array, with a non-zero
   * value indicates selection. The memory allocated for cellSelection must
   * be the maximum cell id referenced in the links.
   */
  void SelectCells(vtkIdType minMaxDegree[2], unsigned char* cellSelection) override;
  ///@}

  /**
   * Insert a new point into the cell-links data structure. The size parameter
   * is the initial size of the list.
   */
  vtkIdType InsertNextPoint(int numLinks);

  /**
   * Insert a cell id into the list of cells (at the end) using the cell id
   * provided. (Make sure to extend the link list (if necessary) using the
   * method ResizeCellList().)
   */
  void InsertNextCellReference(vtkIdType ptId, vtkIdType cellId);

  /**
   * Delete point (and storage) by destroying links to using cells.
   */
  void DeletePoint(vtkIdType ptId);

  /**
   * Delete the reference to the cell (cellId) from the point (ptId). This
   * removes the reference to the cellId from the cell list, but does not
   * resize the list (recover memory with ResizeCellList(), if necessary).
   */
  void RemoveCellReference(vtkIdType cellId, vtkIdType ptId);

  /**
   * Add the reference to the cell (cellId) from the point (ptId). This
   * adds a reference to the cellId from the cell list, but does not resize
   * the list (extend memory with ResizeCellList(), if necessary).
   */
  void AddCellReference(vtkIdType cellId, vtkIdType ptId);

  /**
   * Change the length of a point's link list (i.e., list of cells using a
   * point) by the size specified.
   */
  void ResizeCellList(vtkIdType ptId, int size);

  /**
   * Reclaim any unused memory.
   */
  void Squeeze() override;

  /**
   * Reset to a state of no entries without freeing the memory.
   */
  void Reset() override;

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this cell links array.
   * Used to support streaming and reading/writing data. The value
   * returned is guaranteed to be greater than or equal to the memory
   * required to actually represent the data represented by this object.
   * The information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Standard DeepCopy method.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void DeepCopy(vtkAbstractCellLinks* src) override;

  /**
   * Standard ShallowCopy method.
   *
   * Before you shallow copy, make sure to call SetDataSet()
   */
  void ShallowCopy(vtkAbstractCellLinks* src) override;

protected:
  vtkCellLinks();
  ~vtkCellLinks() override;

  /**
   * Increment the count of the number of cells using the point.
   */
  void IncrementLinkCount(vtkIdType ptId) { this->Array[ptId].ncells++; }

  void AllocateLinks(vtkIdType n);

  /**
   * Insert a cell id into the list of cells using the point.
   */
  void InsertCellReference(vtkIdType ptId, vtkIdType pos, vtkIdType cellId);

  std::shared_ptr<Link> ArraySharedPtr; // Shared Ptr to Array
  Link* Array;                          // pointer to data
  vtkIdType Size;                       // allocated size of data
  vtkIdType MaxId;                      // maximum index inserted thus far
  vtkIdType Extend;                     // grow array by this point
  Link* Resize(vtkIdType sz);           // function to resize data

  // Some information recorded at build time
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfCells;

private:
  vtkCellLinks(const vtkCellLinks&) = delete;
  void operator=(const vtkCellLinks&) = delete;
};

//----------------------------------------------------------------------------
inline void vtkCellLinks::InsertCellReference(vtkIdType ptId, vtkIdType pos, vtkIdType cellId)
{
  this->Array[ptId].cells[pos] = cellId;
}

//----------------------------------------------------------------------------
inline void vtkCellLinks::DeletePoint(vtkIdType ptId)
{
  this->Array[ptId].ncells = 0;
  delete[] this->Array[ptId].cells;
  this->Array[ptId].cells = nullptr;
}

//----------------------------------------------------------------------------
inline void vtkCellLinks::InsertNextCellReference(vtkIdType ptId, vtkIdType cellId)
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

//----------------------------------------------------------------------------
inline void vtkCellLinks::RemoveCellReference(vtkIdType cellId, vtkIdType ptId)
{
  vtkIdType* cells = this->Array[ptId].cells;
  vtkIdType ncells = this->Array[ptId].ncells;

  for (vtkIdType i = 0; i < ncells; i++)
  {
    if (cells[i] == cellId)
    {
      for (vtkIdType j = i; j < (ncells - 1); j++)
      {
        cells[j] = cells[j + 1];
      }
      this->Array[ptId].ncells--;
      break;
    }
  }
}

//----------------------------------------------------------------------------
inline void vtkCellLinks::AddCellReference(vtkIdType cellId, vtkIdType ptId)
{
  this->Array[ptId].cells[this->Array[ptId].ncells++] = cellId;
}

//----------------------------------------------------------------------------
inline void vtkCellLinks::ResizeCellList(vtkIdType ptId, int size)
{
  vtkIdType newSize = this->Array[ptId].ncells + size;
  vtkIdType* cells = new vtkIdType[newSize];
  memcpy(cells, this->Array[ptId].cells,
    static_cast<size_t>(this->Array[ptId].ncells) * sizeof(vtkIdType));
  delete[] this->Array[ptId].cells;
  this->Array[ptId].cells = cells;
}

VTK_ABI_NAMESPACE_END
#endif
