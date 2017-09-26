/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkUniformGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkUniformGridGhostDataGenerator
 *
 *
 *  A concrete implementation of vtkDataSetGhostGenerator for generating ghost
 *  data on partitioned uniform grids on a single process. For a distributed
 *  data-set see vtkPUniformGridGhostDataGenerator.
 *
 * @warning
 *  <ol>
 *   <li>
 *    The input multi-block dataset must:
 *    <ul>
 *      <li> Have the whole-extent set </li>
 *      <li> Each block must be an instance of vtkUniformGrid </li>
 *      <li> Each block must have its corresponding global extent set in the
 *           meta-data using the PIECE_EXTENT() key </li>
 *      <li> The spacing of each block is the same </li>
 *      <li> All blocks must have the same fields loaded </li>
 *    </ul>
 *   </li>
 *   <li>
 *    The code currently does not handle the following cases:
 *    <ul>
 *      <li>Ghost cells along Periodic boundaries</li>
 *      <li>Growing ghost layers beyond the extents of the neighboring grid</li>
 *    </ul>
 *   </li>
 *  </ol>
 *
 * @sa
 *  vtkDataSetGhostGenerator, vtkPUniformGhostDataGenerator
*/

#ifndef vtkUniformGridGhostDataGenerator_h
#define vtkUniformGridGhostDataGenerator_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkDataSetGhostGenerator.h"

// Forward declarations
class vtkMultiBlockDataSet;
class vtkIndent;
class vtkStructuredGridConnectivity;

class VTKFILTERSGEOMETRY_EXPORT vtkUniformGridGhostDataGenerator :
  public vtkDataSetGhostGenerator
{
public:
  static vtkUniformGridGhostDataGenerator* New();
  vtkTypeMacro(vtkUniformGridGhostDataGenerator,vtkDataSetGhostGenerator);
  void PrintSelf(ostream& os, vtkIndent indent ) override;

protected:
  vtkUniformGridGhostDataGenerator();
  ~vtkUniformGridGhostDataGenerator() override;

  /**
   * Computes the global origin
   */
  void ComputeOrigin(vtkMultiBlockDataSet *in);

  /**
   * Computes the global spacing vector
   */
  void ComputeGlobalSpacingVector(vtkMultiBlockDataSet *in);

  /**
   * Registers the grid associated with this instance of multi-block.
   */
  void RegisterGrids(vtkMultiBlockDataSet *in);

  /**
   * Creates the output
   */
  void CreateGhostedDataSet(
      vtkMultiBlockDataSet *in,
      vtkMultiBlockDataSet *out );

  /**
   * Generates ghost layers.
   */
  void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out) override;

  double GlobalSpacing[3];
  double GlobalOrigin[3];
  vtkStructuredGridConnectivity *GridConnectivity;

private:
  vtkUniformGridGhostDataGenerator(const vtkUniformGridGhostDataGenerator&) = delete;
  void operator=(const vtkUniformGridGhostDataGenerator&) = delete;
};

#endif /* vtkUniformGridGhostDataGenerator_h */
