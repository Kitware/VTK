/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridGhostDataGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkStructuredGridGhostDataGenerator
 *
 *
 *  A concrete implementation of vtkDataSetGhostGenerator for generating ghost
 *  data on partitioned structured grids on a singled process. For a distributed
 *  data-set see vtkPStructuredGridGhostDataGenerator.
 *
 * @warning
 * <ol>
 *   <li>
 *    The input multi-block dataset must:
 *    <ul>
 *      <li> Have the whole-extent set </li>
 *      <li> Each block must be an instance of vtkStructuredGrid </li>
 *      <li> Each block must have its corresponding global extent set in the
 *           meta-data using the PIECE_EXTENT() key </li>
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
 * </ol>
 *
 * @sa
 * vtkDataSetGhostGenerator, vtkPStructuredGridGhostDataGenerator
*/

#ifndef vtkStructuredGridGhostDataGenerator_h
#define vtkStructuredGridGhostDataGenerator_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkDataSetGhostGenerator.h"

// Forward declarations
class vtkMultiBlockDataSet;
class vtkIndent;
class vtkStructuredGridConnectivity;

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredGridGhostDataGenerator :
  public vtkDataSetGhostGenerator
{
public:
  static vtkStructuredGridGhostDataGenerator* New();
  vtkTypeMacro(vtkStructuredGridGhostDataGenerator,vtkDataSetGhostGenerator);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkStructuredGridGhostDataGenerator();
  ~vtkStructuredGridGhostDataGenerator() VTK_OVERRIDE;

  /**
   * Registers the grid associated with this instance of multi-block.
   */
  void RegisterGrids(vtkMultiBlockDataSet *in);

  /**
   * Creates the output.
   */
  void CreateGhostedDataSet(
      vtkMultiBlockDataSet *in,
      vtkMultiBlockDataSet *out );

  /**
   * Generates ghost layers.
   */
  void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out) VTK_OVERRIDE;

  vtkStructuredGridConnectivity *GridConnectivity;
private:
  vtkStructuredGridGhostDataGenerator(const vtkStructuredGridGhostDataGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredGridGhostDataGenerator&) VTK_DELETE_FUNCTION;
};

#endif /* vtkStructuredGridGhostDataGenerator_h */
