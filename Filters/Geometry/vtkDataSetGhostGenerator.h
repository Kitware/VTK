/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkDataSetGhostGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkDataSetGhostGenerator.h -- Base class for ghost data generators
//
// .SECTION Description
//  An abstract class that provides common functionality and implements an
//  interface for all ghost data generators. Ghost data generators accept as
//  input a partitioned data-set, defined by a vtkMultiBlockDataSet, where each
//  block corresponds to a partition. The output consists of vtkMultiBlockDataSet
//  where each block holds the corresponding ghosted data-set. For more details,
//  see concrete implementations.
//
// .SECTION See Also
// vtkUniformGridGhostDataGenerator, vtkStructuredGridGhostDataGenerator,
// vtkRectilinearGridGhostDataGenerator

#ifndef VTKDATASETGHOSTGENERATOR_H_
#define VTKDATASETGHOSTGENERATOR_H_

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

// Forward Declarations
class vtkInformation;
class vtkInformationVector;
class vtkMultiBlockDataSet;

class VTKFILTERSGEOMETRY_EXPORT vtkDataSetGhostGenerator:
  public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkDataSetGhostGenerator,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get for number of ghost layers to generate.
  vtkSetMacro( NumberOfGhostLayers, int );
  vtkGetMacro( NumberOfGhostLayers, int );

  // Standard VTK pipeline routines
  virtual int FillInputPortInformation(int port,vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(
      vtkInformation *rqst, vtkInformationVector **inputVector,
      vtkInformationVector* outputVector );

protected:
  vtkDataSetGhostGenerator();
  virtual ~vtkDataSetGhostGenerator();

  // Description:
  // Generate ghost layers. Implemented by concrete implementations.
  virtual void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out) = 0;


  int NumberOfGhostLayers;

private:
  vtkDataSetGhostGenerator(const vtkDataSetGhostGenerator&); // Not implemented
  void operator=(const vtkDataSetGhostGenerator&); // Not implemented

};

#endif /* VTKDATASETGHOSTGENERATOR_H_ */
