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
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <vtkNew.h> // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkExtentTranslator;
class vtkHyperTreeGridNonOrientedCursor;
class vtkMinimalStandardRandomSequence;

class VTKFILTERSSOURCES_EXPORT VTK_MARSHALAUTO vtkRandomHyperTreeGridSource
  : public vtkHyperTreeGridAlgorithm
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
  void SubdivideLeaves(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId, vtkDoubleArray*);

  unsigned int Dimensions[3];
  double OutputBounds[6];
  vtkTypeUInt32 Seed;
  vtkIdType MaxDepth;
  double SplitFraction;

private:
  vtkRandomHyperTreeGridSource(const vtkRandomHyperTreeGridSource&) = delete;
  void operator=(const vtkRandomHyperTreeGridSource&) = delete;

  vtkNew<vtkMinimalStandardRandomSequence> NodeRNG;
  vtkNew<vtkMinimalStandardRandomSequence> MaskRNG;

  bool ShouldRefine();
  bool ShouldMask();

  double MaskedFraction = 0;
};

VTK_ABI_NAMESPACE_END
#endif // vtkRandomHyperTreeGridSource_h
