/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineSize.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPipelineSize - compute the memory required by a pipeline


#ifndef __vtkPipelineSize_h
#define __vtkPipelineSize_h

#include "vtkObject.h"
class vtkSource;
class vtkDataObject;
class vtkPolyDataMapper;

class VTK_PARALLEL_EXPORT vtkPipelineSize : public vtkObject
{
public:
  // Description:
  static vtkPipelineSize* New();
  vtkTypeRevisionMacro(vtkPipelineSize,vtkObject);

  // Description:
  // Compute an estimate of how much memory a pipline will require in 
  // kilobytes (1024 bytes not 1000) This is only an estimate and the 
  // calculations in here do not take into account the specifics of many
  // sources and filters.
  unsigned long GetEstimatedSize(vtkDataObject *input);

  // Description:
  // Determine how many subpieces a mapper should use to fit a target memory 
  // limit. This takes into account the mapper's Piece and NumberOfPieces.
  unsigned long GetNumberOfSubPieces(unsigned long memoryLimit, 
                                     vtkPolyDataMapper *mapper);
  
protected:
  vtkPipelineSize() {};
  void GenericComputeSourcePipelineSize(vtkSource *src, 
                                        vtkDataObject *output,
                                        unsigned long size[3]);
  void ComputeSourcePipelineSize(vtkSource *src, 
                                 vtkDataObject *output,
                                 unsigned long size[3]);
  void ComputeOutputMemorySize( vtkSource *src,
                                vtkDataObject *output,
                                unsigned long *inputSize,
                                unsigned long size[2] );
  void GenericComputeOutputMemorySize( vtkSource *src,
                                       vtkDataObject *output,
                                       unsigned long *inputSize,
                                       unsigned long size[2] );
  void ComputeDataPipelineSize(vtkDataObject *input,
                               unsigned long sizes[3]);

    
private:
  vtkPipelineSize(const vtkPipelineSize&);  // Not implemented.
  void operator=(const vtkPipelineSize&);  // Not implemented.
};

#endif


