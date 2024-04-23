// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkRandomHyperTreeGridSource
 * @brief Builds a randomized but reproducible vtkHyperTreeGrid.
 */

#ifndef vtkRandomHyperTreeGridSource_h
#define vtkRandomHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vector>
#include <vtkNew.h> // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkExtentTranslator;
class vtkHyperTreeGridNonOrientedCursor;
class vtkMinimalStandardRandomSequence;

class VTKFILTERSSOURCES_EXPORT vtkRandomHyperTreeGridSource : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkRandomHyperTreeGridSource* New();
  vtkTypeMacro(vtkRandomHyperTreeGridSource, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The Dimensions of the output vtkHyperTreeGrid.
   * Default is 5x5x2.
   * @{
   */
  vtkGetVector3Macro(Dimensions, unsigned int);
  vtkSetVector3Macro(Dimensions, unsigned int);
  /**@}*/

  /**
   * The bounds of the output vtkHyperTreeGrid.
   * The default is {-10, 10, -10, 10, -10, 10}.
   */
  vtkGetVector6Macro(OutputBounds, double);
  vtkSetVector6Macro(OutputBounds, double);

  /**
   * A seed for the random number generator used to construct the output
   * vtkHyperTreeGrid.
   * The default is 0.
   * @{
   */
  vtkGetMacro(Seed, vtkTypeUInt32);
  vtkSetMacro(Seed, vtkTypeUInt32);
  /**@}*/

  /**
   * The maximum number of levels to allow in the output vtkHyperTreeGrid.
   * The default is 5.
   * @{
   */
  vtkGetMacro(MaxDepth, vtkIdType);
  vtkSetClampMacro(MaxDepth, vtkIdType, 1, VTK_ID_MAX);
  /**@}*/

  /**
   * The target fraction of nodes that will be split during generation.
   * Valid range is [0., 1.]. The default is 0.5.
   * @{
   */
  vtkGetMacro(SplitFraction, double);
  vtkSetClampMacro(SplitFraction, double, 0., 1.);
  /**@}*/

  /**
   * The target fraction of nodes that will be masked after generation.
   * Valid range is [0., 1.]. The default is 0.
   * The fraction represents the total space occupied by the HTG and not
   * its number of leaves/nodes. So a 0.5 fraction means half of the space
   * covered by the HTG should be masked. It is a target and the actual
   * masking fraction can differ up to an error margin depending on the
   * of tree in the HTG and the number of child of each node.
   * The error margin is : (1/numberOfTree + 1/NumberOfChild)
   * @{
   */
  vtkGetMacro(MaskedFraction, double);
  vtkSetClampMacro(MaskedFraction, double, 0., 1.);
  /**@}*/

  /**
   * The actual masked spatial fraction of the HTG.
   * It can be different from the MaskedFraction due to a margin of error.
   * @{
   */
  vtkGetMacro(ActualMaskedCellFraction, double);
  /**@}*/

protected:
  vtkRandomHyperTreeGridSource();
  ~vtkRandomHyperTreeGridSource() override;

  int RequestInformation(
    vtkInformation* req, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  int RequestData(
    vtkInformation* req, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  // We just do the work in RequestData.
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) final { return 1; }

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Recursively subdivides the leafs of the tree using a pseudo random number
   * generator and the SplitFraction property.
   * It also applies a mask depending on the same random number generator and
   * the MaskedFraction property.
   * Returns the spatial unmasked fraction of cells.
   */
  void SubdivideLeaves(vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId);

  bool ShouldRefine(vtkIdType level);

  unsigned int Dimensions[3];
  double OutputBounds[6];
  vtkTypeUInt32 Seed;
  vtkIdType MaxDepth;
  double SplitFraction;

private:
  vtkRandomHyperTreeGridSource(const vtkRandomHyperTreeGridSource&) = delete;
  void operator=(const vtkRandomHyperTreeGridSource&) = delete;

  vtkNew<vtkMinimalStandardRandomSequence> NodeRNG;
  // We have 2 different RNG for retrocompatibility, since the mask
  // has been added later on.
  vtkNew<vtkMinimalStandardRandomSequence> MaskRNG;
  vtkNew<vtkExtentTranslator> ExtentTranslator;
  vtkDoubleArray* Levels;
  double MaskedFraction = 0;
  double ActualMaskedCellFraction = 0;
  std::vector<double> MaskingCostPerLevel{ 1.0 };

  /**
   * Verify and returns if a node should be masked.
   * It is decided by looking at the number of siblings of the node that are currently masked.

   * @param siblingsMasked - the fraction of siblings currently masked at this level
   * @param level - the current depth level of the node to be masked
   * @param errorMargin - the error margin which should be the minimum space to be masked at the
   current level.
   * @returns If the fraction of sibling is higher than the expected masked fraction minus the error
   margin it will return false. Otherwise it will return true.
   */
  bool ShouldMask(double siblingsMasked = 0.0, int level = 0, double errorMargin = 0.0);

  /**
   * Generate the mask for the HTG.
   */
  double GenerateMask(vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId,
    double unmaskedFraction = 1.0, bool isParentMasked = false, double siblingsMasked = 0.0,
    double errorMargin = 0.0);

  /**
   * Fill the MaskingNodeCostPerLevel vector with the masking cost of each level up to the MaxDepth.
   */
  void InitializeMaskingNodeCostPerLevel();

  /**
   * Returns the weight of a node in the Hyper Tree.
   * Here we take the weight of a node as the space it occupies in the scene.
   * Since our structure is a grid, each node occupies the exact same space
   * than every other node at the same depth.
   * Knowing the branching Factor and the depth, we can compute
   * the fraction of space a node occupies in the Hyper Tree.
   */
  double GetMaskingNodeCost(int level);
};

VTK_ABI_NAMESPACE_END
#endif // vtkRandomHyperTreeGridSource_h
