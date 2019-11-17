/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * JB modify for introduce Scales by Jacques-Bernard Lekien, CEA 2018.
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkUniformHyperTreeGrid_h
#define vtkUniformHyperTreeGrid_h

#include "limits.h" // UINT_MAX

#include <memory> // std::shared_ptr

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkHyperTreeGrid.h"

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
  int GetDataObjectType() override { return VTK_UNIFORM_HYPER_TREE_GRID; }

  /**
   * Copy the internal geometric and topological structure of a
   * vtkUniformHyperTreeGrid object.
   */
  void CopyStructure(vtkDataObject*) override;

  virtual void Initialize() override;

  //@{
  /**
   * Set/Get origin of grid
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Set/Get scale of root cells along each direction
   */
  void SetGridScale(double, double, double);
  void SetGridScale(double*);
  vtkGetVector3Macro(GridScale, double);
  //@}

  /**
   * Set all scales at once when root cells are d-cubes
   */
  void SetGridScale(double);

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetBounds() VTK_SIZEHINT(6) override;

  //@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetXCoordinates(vtkDataArray* XCoordinates) override;
  vtkDataArray* GetXCoordinates() override;
  /* JB A faire pour les Get !
  const vtkDataArray* GetXCoordinates() const override {
    throw std::domain_error("Cannot use GetZCoordinates on UniformHyperTreeGrid");
  }
  */
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetYCoordinates(vtkDataArray* YCoordinates) override;
  vtkDataArray* GetYCoordinates() override;
  /* JB A faire pour les Get !
  const vtkDataArray* GetYCoordinates() const override {
    throw std::domain_error("Cannot use GetZCoordinates on UniformHyperTreeGrid");
  }
  */
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   * NB: Set method deactivated in the case of uniform grids.
   * Use SetSize() instead.
   */
  void SetZCoordinates(vtkDataArray* ZCoordinates) override;
  vtkDataArray* GetZCoordinates() override;
  /* JB A faire pour les Get !
  const vtkDataArray* GetZCoordinates() const override {
    throw std::domain_error("Cannot use GetZCoordinates on UniformHyperTreeGrid");
  }
  */
  // JB A faire pour les autre Get !
  //@}

  //@{
  /**
   * JB Augented services on Coordinates.
   */
  void CopyCoordinates(const vtkHyperTreeGrid* output) override;
  void SetFixedCoordinates(unsigned int axis, double value) override;
  //@}

  /**
   * Convert the global index of a root to its Spacial coordinates origin and size.
   */
  void GetLevelZeroOriginAndSizeFromIndex(vtkIdType, double*, double*) override;

  /**
   * Convert the global index of a root to its Spacial coordinates origin and size.
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

  //@{
  /**
   * Keep track of whether coordinates have been explicitly computed
   */
  bool ComputedXCoordinates;
  bool ComputedYCoordinates;
  bool ComputedZCoordinates;
  //@}

  unsigned int FindDichotomicX(double value) const override
  {
    if (value < this->Origin[0] ||
      value > this->Origin[0] + this->GridScale[0] * (this->GetDimensions()[0] - 1))
    {
      return UINT_MAX;
    }
    return round((value - this->Origin[0]) / this->GridScale[0]);
  }
  unsigned int FindDichotomicY(double value) const override
  {
    if (value < this->Origin[1] ||
      value > this->Origin[1] + this->GridScale[1] * (this->GetDimensions()[1] - 1))
    {
      return UINT_MAX;
    }
    return round((value - this->Origin[1]) / this->GridScale[1]);
  }
  unsigned int FindDichotomicZ(double value) const override
  {
    if (value < this->Origin[2] ||
      value > this->Origin[2] + this->GridScale[2] * (this->GetDimensions()[2] - 1))
    {
      return UINT_MAX;
    }
    return round((value - this->Origin[2]) / this->GridScale[2]);
  }

  /**
   * JB Storage of pre-computed per-level cell scales
   */
  mutable std::shared_ptr<vtkHyperTreeGridScales> Scales;

private:
  vtkUniformHyperTreeGrid(const vtkUniformHyperTreeGrid&) = delete;
  void operator=(const vtkUniformHyperTreeGrid&) = delete;
};

#endif
