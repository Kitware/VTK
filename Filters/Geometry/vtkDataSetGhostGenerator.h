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
/**
 * @class   vtkDataSetGhostGenerator
 *
 *
 *  An abstract class that provides common functionality and implements an
 *  interface for all ghost data generators. Ghost data generators accept as
 *  input a partitioned data-set, defined by a vtkMultiBlockDataSet, where each
 *  block corresponds to a partition. The output consists of vtkMultiBlockDataSet
 *  where each block holds the corresponding ghosted data-set. For more details,
 *  see concrete implementations.
 *
 * @sa
 * vtkUniformGridGhostDataGenerator, vtkStructuredGridGhostDataGenerator,
 * vtkRectilinearGridGhostDataGenerator
*/

#ifndef vtkDataSetGhostGenerator_h
#define vtkDataSetGhostGenerator_h

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get for number of ghost layers to generate.
   */
  vtkSetMacro( NumberOfGhostLayers, int );
  vtkGetMacro( NumberOfGhostLayers, int );
  //@}

  // Standard VTK pipeline routines
  int FillInputPortInformation(int port,vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int RequestData(
      vtkInformation *rqst, vtkInformationVector **inputVector,
      vtkInformationVector* outputVector ) VTK_OVERRIDE;

protected:
  vtkDataSetGhostGenerator();
  ~vtkDataSetGhostGenerator() VTK_OVERRIDE;

  /**
   * Generate ghost layers. Implemented by concrete implementations.
   */
  virtual void GenerateGhostLayers(
      vtkMultiBlockDataSet *in, vtkMultiBlockDataSet *out) = 0;


  int NumberOfGhostLayers;

private:
  vtkDataSetGhostGenerator(const vtkDataSetGhostGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetGhostGenerator&) VTK_DELETE_FUNCTION;

};

#endif /* vtkDataSetGhostGenerator_h */
