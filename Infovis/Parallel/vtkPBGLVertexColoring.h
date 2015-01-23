/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLVertexColoring.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkPBGLVertexColoring - Compute a vertex coloring for a distributed,
// undirected vtkGraph, where each vertex has a color distinct from the
// colors of its adjacent vertices.
//
// .SECTION Description
// This VTK class uses the Parallel BGL's implementation of Boman et al's
// parallel vertex coloring algorithm.
//
// @deprecated Not maintained as of VTK 6.2 and will be removed eventually.

#ifndef vtkPBGLVertexColoring_h
#define vtkPBGLVertexColoring_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class vtkSelection;

#if !defined(VTK_LEGACY_REMOVE)
class VTKINFOVISPARALLEL_EXPORT vtkPBGLVertexColoring : public vtkGraphAlgorithm
{
public:
  static vtkPBGLVertexColoring *New();
  vtkTypeMacro(vtkPBGLVertexColoring, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Sets the block size of the algorithm, which is the number of
  // vertices whose colors will be assigned before the processes
  // resynchronize.
  vtkSetMacro(BlockSize,vtkIdType);

  // Description:
  // Retrieve the block size of the algorithm.
  vtkGetMacro(BlockSize,vtkIdType);

  // Description:
  // Set the name of the vertex color output array, which contains the
  // color of each vertex (a non-negative value). If no color
  // array name is set then the name 'Color' is used.
  vtkSetStringMacro(ColorArrayName);

protected:
  vtkPBGLVertexColoring();
  ~vtkPBGLVertexColoring();

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);

  virtual int FillOutputPortInformation(
    int port, vtkInformation* info);

private:

  vtkIdType BlockSize;
  char* ColorArrayName;

  vtkPBGLVertexColoring(const vtkPBGLVertexColoring&);  // Not implemented.
  void operator=(const vtkPBGLVertexColoring&);  // Not implemented.
};

#endif //VTK_LEGACY_REMOVE
#endif
