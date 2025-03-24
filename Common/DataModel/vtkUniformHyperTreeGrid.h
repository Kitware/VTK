// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformHyperTreeGrid
 * @brief   A specifalized type of vtkHyperTreeGrid for the case
 * when root cells have uniform sizes in each direction
 *
 * @sa
 * vtkHyperTree vtkHyperTreeGrid vtkRectilinearGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, NexGen Analytics 2017
 * Modified to introduce Scales by Jacques-Bernard Lekien, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkUniformHyperTreeGrid_h
#define vtkUniformHyperTreeGrid_h

#include <algorithm> // std::min/std::max
#include <cmath>     // std::round
#include <limits>    // std::numeric_limits
#include <memory>    // std::shared_ptr

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHyperTreeGrid.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkHyperTreeGridScales;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformHyperTreeGrid : public vtkHyperTreeGrid
{
public:
  static vtkUniformHyperTreeGrid* New();
  vtkTypeMacro(vtkUniformHyperTreeGrid, vtkHyperTreeGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_HYPER_TREE_GRID; }

  /**
   * Copy the internal geometric and topological structure of a
   * vtkUniformHyperTreeGrid object.
   */
  void CopyStructure(vtkDataObject*) override;

  void Initialize() override;

  ///@{
  /**
   * Set/Get origin of grid
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Set/Get scale of root cells along each direction
   */
  void SetGridScale(double, double, double);
  void SetGridScale(double*);
  vtkGetVector3Macro(GridScale, double);
  ///@}

  /**
   * Set all scales at once when root cells are d-cubes
   */
  void SetGridScale(double);

  /**
   * Return a pointer to the grid bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetGridBounds(double bounds[6]) override;

  ///@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetXCoordinates(vtkDataArray* XCoordinates) override;
  vtkDataArray* GetXCoordinates() override;
  ///@}

  ///@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetYCoordinates(vtkDataArray* YCoordinates) override;
  vtkDataArray* GetYCoordinates() override;
  ///@}

  ///@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetZCoordinates(vtkDataArray* ZCoordinates) override;
  vtkDataArray* GetZCoordinates() override;
  ///@}

  ///@{
  /**
   * Augented services on Coordinates.
   */
  void CopyCoordinates(const vtkHyperTreeGrid* output) override;
  void SetFixedCoordinates(unsigned int axis, double value) override;
  ///@}

  /**
   * Convert the global index of a root to its Spatial coordinates origin and size.
   */
  void GetLevelZeroOriginAndSizeFromIndex(vtkIdType, double*, double*) override;

  /**
   * Convert the global index of a root to its Spatial coordinates origin and size.
   */
  void GetLevelZeroOriginFromIndex(vtkIdType, double*) override;

  /**
   * Create shallow copy of hyper tree grid.
   */
  void ShallowCopy(vtkDataObject*) override;

  /**
   * Create deep copy of hyper tree grid.
   */
  void DeepCopy(vtkDataObject*) override;

  /**
   * Return the actual size of the data bytes
   */
  unsigned long GetActualMemorySizeBytes() override;

  /**
   * Return tree located at given index of hyper tree grid
   * NB: This will construct a new HyperTree if grid slot is empty.
   */
  vtkHyperTree* GetTree(vtkIdType, bool create = false) override;

protected:
  /**
   * Constructor
   */
  vtkUniformHyperTreeGrid();

  /**
   * Destructor
   */
  ~vtkUniformHyperTreeGrid() override;

  /**
   * Grid Origin
   */
  double Origin[3];

  /**
   * Element sizes in each direction
   */
  double GridScale[3];

  ///@{
  /**
   * Keep track of whether coordinates have been explicitly computed
   */
  bool ComputedXCoordinates;
  bool ComputedYCoordinates;
  bool ComputedZCoordinates;
  ///@}

  unsigned int FindDichotomic(double value, unsigned char dim, double tol) const
  {
    unsigned int maxIdx = this->GetDimensions()[dim] - 1;
    if (value < (this->Origin[dim] - tol) ||
      value > (this->Origin[dim] + tol + this->GridScale[dim] * maxIdx))
    {
      return std::numeric_limits<unsigned int>::max();
    }

    long idx = std::round((value - this->Origin[dim]) / this->GridScale[dim]);
    return std::min(std::max(idx, 0l), static_cast<long>(maxIdx));
  }

  unsigned int FindDichotomicX(double value, double tolerance = 0.0) const override
  {
    return this->FindDichotomic(value, 0, tolerance);
  }
  unsigned int FindDichotomicY(double value, double tolerance = 0.0) const override
  {
    return this->FindDichotomic(value, 1, tolerance);
  }
  unsigned int FindDichotomicZ(double value, double tolerance = 0.0) const override
  {
    return this->FindDichotomic(value, 2, tolerance);
  }

  /**
   * Storage of pre-computed per-level cell scales
   */
  mutable std::shared_ptr<vtkHyperTreeGridScales> Scales;

private:
  vtkUniformHyperTreeGrid(const vtkUniformHyperTreeGrid&) = delete;
  void operator=(const vtkUniformHyperTreeGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
