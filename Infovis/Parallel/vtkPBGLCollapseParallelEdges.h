/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLCollapseParallelEdges.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   vtkPBGLCollapseParallelEdges
 * @brief   collapse multiple vertices into a single vertex
 *
 *
 *
 * Uses single input array specified with SetInputArrayToProcess(0,...)
 * to collapse groups of vertices with the same value into a single vertex.
*/

#ifndef vtkPBGLCollapseParallelEdges_h
#define vtkPBGLCollapseParallelEdges_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class VTKINFOVISPARALLEL_EXPORT vtkPBGLCollapseParallelEdges : public vtkGraphAlgorithm
{
public:
  static vtkPBGLCollapseParallelEdges* New();
  vtkTypeMacro(vtkPBGLCollapseParallelEdges,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPBGLCollapseParallelEdges();
  ~vtkPBGLCollapseParallelEdges();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkPBGLCollapseParallelEdges(const vtkPBGLCollapseParallelEdges&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPBGLCollapseParallelEdges&) VTK_DELETE_FUNCTION;
};

#endif

