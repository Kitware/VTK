/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineSize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPipelineSize - compute the memory required by a pipeline


#ifndef __vtkPipelineSize_h
#define __vtkPipelineSize_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkObject.h"
class vtkAlgorithm;
class vtkPolyDataMapper;

class VTKFILTERSPARALLEL_EXPORT vtkPipelineSize : public vtkObject
{
public:
  static vtkPipelineSize* New();
  vtkTypeMacro(vtkPipelineSize,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compute an estimate of how much memory a pipline will require in
  // kilobytes (1024 bytes not 1000) This is only an estimate and the
  // calculations in here do not take into account the specifics of many
  // sources and filters.
  unsigned long GetEstimatedSize(vtkAlgorithm *input, int inputPort,
                                 int connection);

  // Description:
  // Determine how many subpieces a mapper should use to fit a target memory
  // limit. This takes into account the mapper's Piece and NumberOfPieces.
  unsigned long GetNumberOfSubPieces(unsigned long memoryLimit,
                                     vtkPolyDataMapper *mapper);

protected:
  vtkPipelineSize() {}
  void GenericComputeSourcePipelineSize(vtkAlgorithm *src,
                                        int outputPort,
                                        unsigned long size[3]);
  void ComputeSourcePipelineSize(vtkAlgorithm *src,
                                 int outputPort,
                                 unsigned long size[3]);
  void ComputeOutputMemorySize( vtkAlgorithm *src,
                                int outputPort,
                                unsigned long *inputSize,
                                unsigned long size[2] );
  void GenericComputeOutputMemorySize( vtkAlgorithm *src,
                                       int outputPort,
                                       unsigned long *inputSize,
                                       unsigned long size[2] );


private:
  vtkPipelineSize(const vtkPipelineSize&);  // Not implemented.
  void operator=(const vtkPipelineSize&);  // Not implemented.
};

#endif


