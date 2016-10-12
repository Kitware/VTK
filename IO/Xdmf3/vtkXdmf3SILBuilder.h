/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3SILBuilder.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXdmf3SILBuilder
 * @brief   helper to allow block selection
 *
 * vtkXdmf3Reader uses this to build up a datastructure that represents
 * block trees that correspond to the file. ParaView builds a GUI from
 * that to let the user select from the various block and types of blocks
 * that should or should not be loaded.
 *
 * This file is a helper for the vtkXdmf3Reader and vtkXdmf3Writer and
 * not intended to be part of VTK public API
*/

#ifndef vtkXdmf3SILBuilder_h
#define vtkXdmf3SILBuilder_h

#include "vtkIOXdmf3Module.h" // For export macro
#include "vtkType.h"

class vtkMutableDirectedGraph;
class vtkStringArray;
class vtkUnsignedCharArray;

class VTKIOXDMF3_EXPORT vtkXdmf3SILBuilder
{
public:
  vtkStringArray* NamesArray;
  vtkUnsignedCharArray* CrossEdgesArray;
  vtkMutableDirectedGraph* SIL;
  vtkIdType RootVertex;
  vtkIdType BlocksRoot;
  vtkIdType HierarchyRoot;
  vtkIdType VertexCount;

  /**
   * Initializes the data-structures.
   */
  void Initialize();

  //@{
  /**
   * Add vertex, child-edge or cross-edge to the graph.
   */
  vtkIdType AddVertex(const char* name);
  vtkIdType AddChildEdge(vtkIdType parent, vtkIdType child);
  vtkIdType AddCrossEdge(vtkIdType src, vtkIdType dst);
  //@}

  //@{
  /**
   * Returns the vertex id for the root vertex.
   */
  vtkIdType GetRootVertex();
  vtkIdType GetBlocksRoot();
  vtkIdType GetHierarchyRoot();
  //@}

  bool IsMaxedOut();

  vtkXdmf3SILBuilder();
  ~vtkXdmf3SILBuilder();
};

#endif //vtkXdmf3SILBuilder_h
// VTK-HeaderTest-Exclude: vtkXdmf3SILBuilder.h
