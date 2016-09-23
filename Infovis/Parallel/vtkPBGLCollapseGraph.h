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
/**
 * @class   vtkPBGLCollapseGraph
 * @brief   collapse multiple vertices into a single vertex
 *
 *
 *
 * Uses single input array specified with SetInputArrayToProcess(0,...)
 * to collapse groups of vertices with the same value into a single vertex.
 * @deprecated Not maintained as of VTK 6.2 and will be removed eventually.
*/

#ifndef vtkPBGLCollapseGraph_h
#define vtkPBGLCollapseGraph_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

#if !defined(VTK_LEGACY_REMOVE)
class VTKINFOVISPARALLEL_EXPORT vtkPBGLCollapseGraph : public vtkGraphAlgorithm
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
  vtkPBGLCollapseGraph(const vtkPBGLCollapseGraph&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPBGLCollapseGraph&) VTK_DELETE_FUNCTION;
};

#endif //VTK_LEGACY_REMOVE
#endif

