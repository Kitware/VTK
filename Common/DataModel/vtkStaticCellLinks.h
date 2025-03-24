// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStaticCellLinks
 * @brief   object represents upward pointers from points
 * to list of cells using each point
 *
 * vtkStaticCellLinks is a supplemental object to vtkCellArray and
 * vtkCellTypes, enabling access from points to the cells using the
 * points. vtkStaticCellLinks is an array of links, each link represents a
 * list of cell ids using a particular point. The information provided by
 * this object can be used to determine cell neighbors and construct other
 * local topological information. This class is a faster implementation of
 * vtkCellLinks. However, it cannot be incrementally constructed; it is meant
 * to be constructed once (statically) and must be rebuilt if the cells
 * change.
 *
 * @warning
 * This is a drop-in replacement for vtkCellLinks using static link
 * construction. It uses the templated vtkStaticCellLinksTemplate class,
 * instantiating vtkStaticCellLinksTemplate with a vtkIdType template
 * parameter. Note that for best performance, the vtkStaticCellLinksTemplate
 * class may be used directly, instantiating it with the appropriate id
 * type. This class is also wrappable and can be used from an interpreted
 * language such as Python.
 *
 * @sa
 * vtkCellLinks vtkStaticCellLinksTemplate
 */

#ifndef vtkStaticCellLinks_h
#define vtkStaticCellLinks_h

#include "vtkAbstractCellLinks.h"
#include "vtkCommonDataModelModule.h"   // For export macro
#include "vtkStaticCellLinksTemplate.h" // For implementations

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkCellArray;

class VTKCOMMONDATAMODEL_EXPORT vtkStaticCellLinks : public vtkAbstractCellLinks
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type manipulation and printing.
   */
  static vtkStaticCellLinks* New();
  vtkTypeMacro(vtkStaticCellLinks, vtkAbstractCellLinks);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Build the link list array from the input dataset.
   */
  void BuildLinks() override;

  /**
   * Get the number of cells using the point specified by ptId.
   */
  vtkIdType GetNumberOfCells(vtkIdType ptId) { return this->Impl->GetNumberOfCells(ptId); }

  /**
   * Get the number of cells using the point specified by ptId. This is an
   * alias for GetNumberOfCells(); consistent with the vtkCellLinks API.
   */
  vtkIdType GetNcells(vtkIdType ptId) VTK_FUTURE_CONST
  {
    return this->Impl->GetNumberOfCells(ptId);
  }

  /**
   * Return a list of cell ids using the specified point.
   */
  vtkIdType* GetCells(vtkIdType ptId) { return this->Impl->GetCells(ptId); }

  ///@{
  /**
   * Select all cells with a point degree in the range [minDegree,maxDegree).
   * The degree is the number of cells using a point. The selection is
   * indicated through the provided unsigned char array, with a non-zero
   * value indicates selection. The memory allocated for cellSelection must
   * be the maximum cell id referenced in the links.
   */
  void SelectCells(vtkIdType minMaxDegree[2], unsigned char* cellSelection) override
  {
    this->Impl->SelectCells(minMaxDegree, cellSelection);
  }
  ///@}

  /**
   * Make sure any previously created links are cleaned up.
   */
  void Initialize() override
  {
    this->Impl->Initialize();
    this->Modified();
  }

  /**
   * Reclaim any unused memory.
   */
  void Squeeze() override {}

  /**
   * Reset to a state of no entries without freeing the memory.
   */
  void Reset() override {}

  /**
   * Return the memory in kibibytes (1024 bytes) consumed by this cell links array.
   * Used to support streaming and reading/writing data. The value
   * returned is guaranteed to be greater than or equal to the memory
   * required to actually represent the data represented by this object.
   * The information returned is valid only after the pipeline has
   * been updated.
   */
  unsigned long GetActualMemorySize() override { return this->Impl->GetActualMemorySize(); }

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
  vtkStaticCellLinks();
  ~vtkStaticCellLinks() override;

  vtkStaticCellLinksTemplate<vtkIdType>* Impl;

private:
  vtkStaticCellLinks(const vtkStaticCellLinks&) = delete;
  void operator=(const vtkStaticCellLinks&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
