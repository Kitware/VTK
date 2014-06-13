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
// .NAME vtkStructuredGridPartitioner.h -- Partitions a structured grid by RCB
//
// .SECTION Description
//  A concrete implementation of vtkMultiBlockDataSetAlgorithm that provides
//  functionality for partitioning a VTK structured grid dataset. The partition-
//  ing method used is Recursive Coordinate Bisection (RCB) where each time the
//  longest dimension is split.
//
// .SECTION See Also
//  vtkUniformGridPartitioner vtkRectilinearGridPartitioner

#ifndef VTKSTRUCTUREDGRIDPARTITIONER_H_
#define VTKSTRUCTUREDGRIDPARTITIONER_H_

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
  void PrintSelf(ostream &oss, vtkIndent indent );

  // Description:
  // Set/Get macro for the number of subdivisions.
  vtkGetMacro(NumberOfPartitions,int);
  vtkSetMacro(NumberOfPartitions,int);

  // Description:
  // Set/Get macro for the number of ghost layers.
  vtkGetMacro(NumberOfGhostLayers,int);
  vtkSetMacro(NumberOfGhostLayers,int);

  // Description:
  // Set/Get & boolean macro for the DuplicateNodes property.
  vtkGetMacro(DuplicateNodes,int);
  vtkSetMacro(DuplicateNodes,int);
  vtkBooleanMacro(DuplicateNodes,int);

protected:
  vtkStructuredGridPartitioner();
  virtual ~vtkStructuredGridPartitioner();

  // Description:
  // Extracts the coordinates of the sub-grid from the whole grid.
  vtkPoints* ExtractSubGridPoints(vtkStructuredGrid *wholeGrid,int subext[6]);

  // Standard Pipeline methods
  virtual int RequestData(
     vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  int DuplicateNodes;

private:
  vtkStructuredGridPartitioner(const vtkStructuredGridPartitioner &); // Not implemented
  void operator=(const vtkStructuredGridPartitioner &); // Not implemented
};

#endif /* VTKSTRUCTUREDGRIDPARTITIONER_H_ */
