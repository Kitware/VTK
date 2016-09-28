/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkStructuredGridPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkStructuredGridPartitioner
 *
 *
 *  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a VTK structured grid dataset. The partition-
 *  ing method used is Recursive Coordinate Bisection (RCB) where each time the
 *  longest dimension is split.
 *
 * @sa
 *  vtkUniformGridPartitioner vtkRectilinearGridPartitioner
*/

#ifndef vtkStructuredGridPartitioner_h
#define vtkStructuredGridPartitioner_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkStructuredGrid;
class vtkPoints;

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredGridPartitioner :
  public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkStructuredGridPartitioner *New();
  vtkTypeMacro(vtkStructuredGridPartitioner, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &oss, vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get macro for the number of subdivisions.
   */
  vtkGetMacro(NumberOfPartitions,int);
  vtkSetMacro(NumberOfPartitions,int);
  //@}

  //@{
  /**
   * Set/Get macro for the number of ghost layers.
   */
  vtkGetMacro(NumberOfGhostLayers,int);
  vtkSetMacro(NumberOfGhostLayers,int);
  //@}

  //@{
  /**
   * Set/Get & boolean macro for the DuplicateNodes property.
   */
  vtkGetMacro(DuplicateNodes,int);
  vtkSetMacro(DuplicateNodes,int);
  vtkBooleanMacro(DuplicateNodes,int);
  //@}

protected:
  vtkStructuredGridPartitioner();
  ~vtkStructuredGridPartitioner() VTK_OVERRIDE;

  /**
   * Extracts the coordinates of the sub-grid from the whole grid.
   */
  vtkPoints* ExtractSubGridPoints(vtkStructuredGrid *wholeGrid,int subext[6]);

  // Standard Pipeline methods
  int RequestData(
     vtkInformation*,vtkInformationVector**,vtkInformationVector*) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  int DuplicateNodes;

private:
  vtkStructuredGridPartitioner(const vtkStructuredGridPartitioner &) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredGridPartitioner &) VTK_DELETE_FUNCTION;
};

#endif /* vtkStructuredGridPartitioner_h */
