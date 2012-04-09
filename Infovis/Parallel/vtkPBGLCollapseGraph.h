/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLCollapseGraph.h

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
// .NAME vtkPBGLCollapseGraph - collapse multiple vertices into a single vertex
//
// .SECTION Description
//
// Uses single input array specified with SetInputArrayToProcess(0,...)
// to collapse groups of vertices with the same value into a single vertex.

#ifndef __vtkPBGLCollapseGraph_h
#define __vtkPBGLCollapseGraph_h

#include "vtkGraphAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkPBGLCollapseGraph : public vtkGraphAlgorithm
{
public:
  static vtkPBGLCollapseGraph* New();
  vtkTypeMacro(vtkPBGLCollapseGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPBGLCollapseGraph();
  ~vtkPBGLCollapseGraph();

  virtual int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkPBGLCollapseGraph(const vtkPBGLCollapseGraph&); // Not implemented
  void operator=(const vtkPBGLCollapseGraph&);   // Not implemented
};

#endif

