/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomHyperTreeGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRandomHyperTreeGridSource
 * @brief Builds a randomized but reproducible vtkHyperTreeGrid.
 */

#ifndef vtkRandomHyperTreeGridSource_h
#define vtkRandomHyperTreeGridSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

#include <vtkNew.h> // For vtkNew

class vtkDoubleArray;
class vtkExtentTranslator;
class vtkHyperTreeCursor;
class vtkMinimalStandardRandomSequence;

class VTKFILTERSSOURCES_EXPORT vtkRandomHyperTreeGridSource
    : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkRandomHyperTreeGridSource* New();
  vtkTypeMacro(vtkRandomHyperTreeGridSource, vtkHyperTreeGridAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * The GridSize of the output vtkHyperTreeGrid.
   * Default is 5x5x2.
   * @{
   */
  vtkGetVector3Macro(GridSize, unsigned int)
  vtkSetVector3Macro(GridSize, unsigned int)
  /**@}*/

  /**
   * The bounds of the output vtkHyperTreeGrid.
   * The default is {-10, 10, -10, 10, -10, 10}.
   */
  vtkGetVector6Macro(OutputBounds, double)
  vtkSetVector6Macro(OutputBounds, double)

  /**
   * A seed for the random number generator used to construct the output
   * vtkHyperTreeGrid.
   * The default is 0.
   * @{
   */
  vtkGetMacro(Seed, vtkTypeUInt32)
  vtkSetMacro(Seed, vtkTypeUInt32)
  /**@}*/

  /**
   * The maximum number of levels to allow in the output vtkHyperTreeGrid.
   * The default is 5.
   * @{
   */
  vtkGetMacro(MaxDepth, vtkIdType)
  vtkSetClampMacro(MaxDepth, vtkIdType, 1, VTK_ID_MAX)
  /**@}*/

  /**
   * The target fraction of nodes that will be split during generation.
   * Valid range is [0., 1.]. The default is 0.5.
   * @{
   */
  vtkGetMacro(SplitFraction, double)
  vtkSetClampMacro(SplitFraction, double, 0., 1.)
  /**@}*/

protected:
  vtkRandomHyperTreeGridSource();
  ~vtkRandomHyperTreeGridSource() override;

  int RequestInformation(vtkInformation *req,
                         vtkInformationVector **inInfo,
                         vtkInformationVector *outInfo) override;

  int RequestData(vtkInformation *req,
                  vtkInformationVector **inInfo,
                  vtkInformationVector *outInfo) override;

  // We just do the work in RequestData.
  int ProcessTrees(vtkHyperTreeGrid *, vtkDataObject *) final { return 1; }

  int FillOutputPortInformation(int port, vtkInformation *info) override;

  void SubdivideLeaves(vtkHyperTreeCursor *cursor, vtkIdType treeId);

  bool ShouldRefine(vtkIdType level);

  unsigned int GridSize[3];
  double OutputBounds[6];
  vtkTypeUInt32 Seed;
  vtkIdType MaxDepth;
  double SplitFraction;

private:
  vtkRandomHyperTreeGridSource(const vtkRandomHyperTreeGridSource&) = delete;
  void operator=(const vtkRandomHyperTreeGridSource&) = delete;

  vtkNew<vtkMinimalStandardRandomSequence> RNG;
  vtkNew<vtkExtentTranslator> ExtentTranslator;
  vtkHyperTreeGrid *HTG;
  vtkDoubleArray *Levels;
};

#endif // vtkRandomHyperTreeGridSource_h
