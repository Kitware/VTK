/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPUniformGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkPUniformGridGhostDataGenerator
 *  uniform grids.
 *
 *
 *  A concrete implementation of vtkPDataSetGhostGenerator for generating ghost
 *  data on a partitioned and distributed domain of uniform grids.
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
 *      <li> The multi-block structure is consistent on all processes </li>
 *    </ul>
 *   </li>
 *   <li>
 *    The code currently does not handle the following cases:
 *    <ul>
 *      <li>Periodic boundaries</li>
 *      <li>Growing ghost layers beyond the extents of the neighboring grid</li>
 *    </ul>
 *   </li>
 *  </ol>
 *
 * @sa
 * vtkDataSetGhostGenerator,vtkUniformGhostDataGenerator,
 * vtkPDataSetGhostGenerator
*/

#ifndef vtkPUniformGridGhostDataGenerator_h
#define vtkPUniformGridGhostDataGenerator_h

#include "vtkFiltersParallelGeometryModule.h" // For export macro
#include "vtkPDataSetGhostGenerator.h"

class vtkMultiBlockDataSet;
class vtkIndent;
class vtkPStructuredGridConnectivity;

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPUniformGridGhostDataGenerator:
  public vtkPDataSetGhostGenerator
{
public:
  static vtkPUniformGridGhostDataGenerator* New();
  vtkTypeMacro(vtkPUniformGridGhostDataGenerator,vtkPDataSetGhostGenerator);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPUniformGridGhostDataGenerator();
  virtual ~vtkPUniformGridGhostDataGenerator();

  /**
   * Registers grids associated with this object instance on this process.
   */
  void RegisterGrids( vtkMultiBlockDataSet *in );

  /**
   * A collective operation that computes the global origin of the domain.
   */
  void ComputeOrigin(vtkMultiBlockDataSet *in);

  /**
   * A collective operations that computes the global spacing.
   */
  void ComputeGlobalSpacing(vtkMultiBlockDataSet *in);

  /**
   * Create ghosted data-set.
   */
  void CreateGhostedDataSet(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out );

  /**
   * Generates ghost-layers
   */
  virtual void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out);

  double GlobalSpacing[3];
  double GlobalOrigin[3];
  vtkPStructuredGridConnectivity *GridConnectivity;

private:
  vtkPUniformGridGhostDataGenerator(const vtkPUniformGridGhostDataGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPUniformGridGhostDataGenerator&) VTK_DELETE_FUNCTION;
};

#endif /* vtkPUniformGridGhostDataGenerator_h */
