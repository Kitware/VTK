/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineSize.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkPipelineSize,vtkObject);

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


