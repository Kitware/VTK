/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSILBuilder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSILBuilder - helper class to build a SIL i.e. a directed graph used
// by reader producing composite datasets to describes the relationships among
// the blocks.
// .SECTION Description
// vtkSILBuilder is a helper class to build a SIL i.e. a directed graph used
// by reader producing composite datasets to describes the relationships among
// the blocks.
// Refer to http://www.paraview.org/Wiki/Block_Hierarchy_Meta_Data for details.

#ifndef vtkSILBuilder_h
#define vtkSILBuilder_h

#include "vtkIOXdmf2Module.h" // For export macro
#include "vtkObject.h"

class vtkUnsignedCharArray;
class vtkStringArray;
class vtkMutableDirectedGraph;

class VTKIOXDMF2_EXPORT vtkSILBuilder : public vtkObject
{
public:
  static vtkSILBuilder* New();
  vtkTypeMacro(vtkSILBuilder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Get/Set the graph to populate.
  void SetSIL(vtkMutableDirectedGraph*);
  vtkGetObjectMacro(SIL, vtkMutableDirectedGraph);

  // Description:
  // Initializes the data-structures.
  void Initialize();

  // Description:
  // Add vertex, child-edge or cross-edge to the graph.
  vtkIdType AddVertex(const char* name);
  vtkIdType AddChildEdge(vtkIdType parent, vtkIdType child);
  vtkIdType AddCrossEdge(vtkIdType src, vtkIdType dst);

  // Description:
  // Returns the vertex id for the root vertex.
  vtkGetMacro(RootVertex, vtkIdType);

//BTX
protected:
  vtkSILBuilder();
  ~vtkSILBuilder();

  vtkStringArray* NamesArray;
  vtkUnsignedCharArray* CrossEdgesArray;
  vtkMutableDirectedGraph* SIL;

  vtkIdType RootVertex;

private:
  vtkSILBuilder(const vtkSILBuilder&); // Not implemented.
  void operator=(const vtkSILBuilder&); // Not implemented.
//ETX
};

#endif
