// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkHyperTreeGridPreConfiguredSource
 * @brief Helper class for generating a curated set of HyperTree Grids (HTGs) for testing purposes
 *
 * Provides a set of public methods for generating some commonly used HTG setups.
 */

#include "vtkFiltersSourcesModule.h" //for export macro
#include "vtkHyperTreeGridAlgorithm.h"

#ifndef vtkHyperTreeGridPreConfiguredSource_h
#define vtkHyperTreeGridPreConfiguredSource_h

VTK_ABI_NAMESPACE_BEGIN
class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkDoubleArray;

class VTKFILTERSSOURCES_EXPORT vtkHyperTreeGridPreConfiguredSource
  : public vtkHyperTreeGridAlgorithm
{
public:
  /**
   * Standard object factory setup
   */
  vtkTypeMacro(vtkHyperTreeGridPreConfiguredSource, vtkHyperTreeGridAlgorithm);
  static vtkHyperTreeGridPreConfiguredSource* New();

  ///@{
  /**
   * Helper methods for generating HTGs
   */
  void GenerateUnbalanced(vtkHyperTreeGrid* HTG, unsigned int dim, unsigned int factor,
    unsigned int depth, const std::vector<double>& extent,
    const std::vector<unsigned int>& subdivisions);

  void GenerateBalanced(vtkHyperTreeGrid* HTG, unsigned int dim, unsigned int factor,
    unsigned int depth, const std::vector<double>& extent,
    const std::vector<unsigned int>& subdivisions);
  ///@}

  ///@{
  /**
   * An enum type for referencing preconfigured HTGs
   */
  enum HTGType
  {
    UNBALANCED_3DEPTH_2BRANCH_2X3,
    BALANCED_3DEPTH_2BRANCH_2X3,
    UNBALANCED_2DEPTH_3BRANCH_3X3,
    BALANCED_4DEPTH_3BRANCH_2X2,
    UNBALANCED_3DEPTH_2BRANCH_3X2X3,
    BALANCED_2DEPTH_3BRANCH_3X3X2,
    CUSTOM
  };
  ///@}

  ///@{
  /**
   * An enum type for configuring the type of generation for the CUSTOM HTG type
   */
  enum HTGArchitecture
  {
    UNBALANCED,
    BALANCED
  };
  ///@}

  /**
   * Get/Set HyperTreeGrid mode
   */
  vtkGetEnumMacro(HTGMode, HTGType);
  vtkSetEnumMacro(HTGMode, HTGType);

  ///@{
  /**
   * Get/Set for custom architecture
   */
  vtkGetEnumMacro(CustomArchitecture, HTGArchitecture);
  vtkSetEnumMacro(CustomArchitecture, HTGArchitecture);

  /**
   * Get/Set for custom dimension
   */
  vtkGetMacro(CustomDim, unsigned int);
  vtkSetMacro(CustomDim, unsigned int);

  /**
   * Get/Set for custom branching factor
   */
  vtkGetMacro(CustomFactor, unsigned int);
  vtkSetMacro(CustomFactor, unsigned int);

  /**
   * Get/Set for custom depth
   */
  vtkGetMacro(CustomDepth, unsigned int);
  vtkSetMacro(CustomDepth, unsigned int);

  /**
   * Get/Set for custom extent in coordinate space
   */
  vtkGetVector6Macro(CustomExtent, double);
  vtkSetVector6Macro(CustomExtent, double);

  /**
   * Get/Set for custom subdivisions of the extent
   */
  vtkGetVector3Macro(CustomSubdivisions, unsigned int);
  vtkSetVector3Macro(CustomSubdivisions, unsigned int);
  ///@}

  ///@{
  /**
   *  Helper functions for generating the different types of HTGs
   */
  void GenerateUnbalanced3DepthQuadTree2x3(vtkHyperTreeGrid* HTG);

  void GenerateBalanced3DepthQuadTree2x3(vtkHyperTreeGrid* HTG);

  void GenerateUnbalanced2Depth3BranchTree3x3(vtkHyperTreeGrid* HTG);

  void GenerateBalanced4Depth3BranchTree2x2(vtkHyperTreeGrid* HTG);

  void GenerateUnbalanced3DepthOctTree3x2x3(vtkHyperTreeGrid* HTG);

  void GenerateBalanced2Depth3BranchTree3x3x2(vtkHyperTreeGrid* HTG);

  int GenerateCustom(vtkHyperTreeGrid* HTG);
  ///@}

protected:
  /**
   * Constructor setup
   */
  vtkHyperTreeGridPreConfiguredSource();
  ~vtkHyperTreeGridPreConfiguredSource() override = default;
  vtkHyperTreeGridPreConfiguredSource(const vtkHyperTreeGridPreConfiguredSource&) = delete;

  void operator=(const vtkHyperTreeGridPreConfiguredSource&) = delete;

  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  ///@{
  /**
   * Common preprocessing for setting up the HyperTreeGrid for all types
   */
  void Preprocess(vtkHyperTreeGrid* HTG, unsigned int dim, unsigned int factor,
    const std::vector<double>& extent, const std::vector<unsigned int>& subdivisions);

  /**
   * Recursive helper for the BALANCED architecture
   */
  void RecurseBalanced(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkDoubleArray* levels, int maxDepth);
  ///@}

  ///@{
  /**
   * The pre-configuration mode of the generator
   */
  HTGType HTGMode;

  /**
   * All members related to the CUSTOM HTGType
   */
  HTGArchitecture CustomArchitecture;
  unsigned int CustomDim;
  unsigned int CustomFactor;
  unsigned int CustomDepth;
  double CustomExtent[6];
  unsigned int CustomSubdivisions[3];
  ///@}

}; // vtkHyperTreeGridPreConfiguredSource
VTK_ABI_NAMESPACE_END

#endif // vtkHyperTreeGridPreConfiguredSource_h
