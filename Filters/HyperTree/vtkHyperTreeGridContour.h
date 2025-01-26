// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridContour
 * @brief   Extract cells from a hyper tree grid
 * where selected scalar value is within given range.
 *
 *
 * This filter extracts cells from a hyper tree grid that satisfy the
 * following contour: a cell is considered to be within range if its
 * value for the active scalar is within a specified range (inclusive).
 * The output remains a hyper tree grid.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkContourFilter
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was revised by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridContour_h
#define vtkHyperTreeGridContour_h

#include "vtkCellArray.h"              // For vtkCellArray
#include "vtkContourValues.h"          // Needed for inline methods
#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkNew.h"       // For vtkNew
#include "vtkPointData.h" // For vtkPointData

#include <vector> // For STL

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkCellData;
class vtkContourHelper;
class vtkDataArray;
class vtkHyperTreeGrid;
class vtkIdList;
class vtkIncrementalPointLocator;
class vtkLine;
class vtkPixel;
class vtkUnsignedCharArray;
class vtkVoxel;
class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridContour : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridContour* New();
  vtkTypeMacro(vtkHyperTreeGridContour, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator*);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);
  ///@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Modified GetMTime Because we delegate to vtkContourValues.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Methods (inlined) to set / get contour values.
   */
  void SetValue(int, double);
  double GetValue(int);
  double* GetValues();
  void GetValues(double*);
  void SetNumberOfContours(int);
  vtkIdType GetNumberOfContours();
  void GenerateValues(int, double[2]);
  void GenerateValues(int, double, double);
  ///@}

  enum CellStrategy3D
  {
    USE_VOXELS,
    USE_DECOMPOSED_POLYHEDRA
  };
  /**
   * Set the contour strategy to apply.
   * By default, strategy is USE_VOXELS.
   * This method is time-efficient but can lead to bad results in the 3D case, where generated dual
   * cells can be concave.
   * USE_DECOMPOSED_POLYHEDRA allow better results in such cases (3D HTGs only).
   * It takes advantage of the vtkPolyhedronUtilities::Decompose method to generate better contours.
   * The dowside is this method is much slower than USE_VOXELS.
   */
  vtkSetClampMacro(Strategy3D, int, USE_VOXELS, USE_DECOMPOSED_POLYHEDRA);

  ///@{
  /**
   * Set/Get whether or not the filter should use implicit arrays to store the
   * output contour values (stored as point data of the output contour).
   * Since these values are the same for each isosurface, some memory can be saved
   * by storing each value only once using an indexed array.
   *
   * @attention This option have no effect if there is more than 256 contour values.
   */
  vtkSetMacro(UseImplicitArrays, bool);
  vtkGetMacro(UseImplicitArrays, bool);
  vtkBooleanMacro(UseImplicitArrays, bool);
  ///@}

protected:
  vtkHyperTreeGridContour();
  ~vtkHyperTreeGridContour() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate isocontours of hyper tree grid.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively decide whether a cell is intersected by a contour
   */
  bool RecursivelyPreProcessTree(vtkHyperTreeGridNonOrientedCursor*);

  /**
   * Recursively descend into the tree down to the leaves to construct the contour (verts, lines,
   * polys). dualPointData represents the point data of the dual mesh, i.e. HTG cell data used for
   * contouring.
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedMooreSuperCursor*, vtkCellArray* verts,
    vtkCellArray* lines, vtkCellArray* polys, vtkPointData* dualPointData);

  /**
   * Storage for contour values.
   */
  vtkContourValues* ContourValues;

  /**
   * Storage for pre-selected cells to be processed
   */
  vtkBitArray* SelectedCells;

  /**
   * Sign of isovalue if cell not treated
   */
  vtkBitArray** CellSigns;

  /**
   * Spatial locator to merge points.
   */
  vtkIncrementalPointLocator* Locator;

  ///@{
  /**
   * Pointers needed to perform isocontouring
   */
  vtkContourHelper* Helper;
  vtkDataArray* CellScalars;
  vtkLine* Line;
  vtkPixel* Pixel;
  vtkVoxel* Voxel;
  vtkIdList* Leaves;
  ///@}

  /**
   * Storage for signs relative to current contour value
   */
  std::vector<bool> Signs;

  /**
   * Keep track of current index in output polydata
   */
  vtkIdType CurrentId;

  /**
   * Keep track of selected input scalars
   */
  vtkDataArray* InScalars;

  vtkBitArray* InMask;
  vtkUnsignedCharArray* InGhostArray;

  // Strategy used to represent dual cells in 3D
  int Strategy3D = USE_VOXELS;

private:
  vtkHyperTreeGridContour(const vtkHyperTreeGridContour&) = delete;
  void operator=(const vtkHyperTreeGridContour&) = delete;

  // Use implicit arrays to store contour values
  bool UseImplicitArrays = false;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkHyperTreeGridContour::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double vtkHyperTreeGridContour::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* vtkHyperTreeGridContour::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkHyperTreeGridContour::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkHyperTreeGridContour::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline vtkIdType vtkHyperTreeGridContour::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkHyperTreeGridContour::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkHyperTreeGridContour::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridContour_h
