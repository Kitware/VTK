/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLBreadthFirstSearch.h

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
// .NAME vtkPBGLBreadthFirstSearch - Breadth-first search on a distributed
// vtkGraph.
//
// .SECTION Description
//
// This VTK class uses the Parallel BGL  breadth_first_search 
// generic algorithm to perform a breadth first search from a given
// a 'source' vertex on the input graph (a distributed vtkGraph).
//
// .SECTION See Also
// vtkGraph vtkBoostBreadthFirstSearch vtkBoostGraphAdapter

#ifndef __vtkPBGLBreadthFirstSearch_h
#define __vtkPBGLBreadthFirstSearch_h

#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class vtkSelection;

class VTK_PARALLEL_EXPORT vtkPBGLBreadthFirstSearch : public vtkGraphAlgorithm 
{
public:
  static vtkPBGLBreadthFirstSearch *New();
  vtkTypeMacro(vtkPBGLBreadthFirstSearch, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Convenience methods for setting the origin selection input.
  void SetOriginSelection(vtkSelection *s);
  void SetOriginSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  // Description:
  // Set the index (into the vertex array) of the 
  // breadth first search 'origin' vertex.
  void SetOriginVertex(vtkIdType index);

  //BTX
  
  // Description:
  // Set the breadth first search 'origin' vertex.
  // This method is basically the same as above
  // but allows the application to simply specify
  // an array name and value, instead of having to
  // know the specific index of the vertex.
  void SetOriginVertex(vtkStdString arrayName, vtkVariant value);
  //ETX

  // Description:
  // Convenience method for setting the origin vertex
  // given an array name and string value.
  // This method is primarily for the benefit of the
  // VTK Parallel client/server layer, callers should
  // prefer to use SetOriginVertex() whenever possible.
  void SetOriginVertexString(char* arrayName, char* value);

  // Description:
  // Set the output array name. If no output array name is
  // set then the name 'BFS' is used.
  vtkSetStringMacro(OutputArrayName);

  // Description:
  // Use the vtkSelection from input port 1 as the origin vertex.
  // The selection should be a IDS selection with field type POINTS.
  // The first ID in the selection will be used for the origin vertex.
  // Default is off (origin is specified by SetOriginVertex(...)).
  vtkSetMacro(OriginFromSelection, bool);
  vtkGetMacro(OriginFromSelection, bool);
  vtkBooleanMacro(OriginFromSelection, bool);

  // Description:
  // Create an output selection containing the ID of a vertex based
  // on the output selection type. The default is to use the
  // the maximum distance from the starting vertex.  Defaults to off.
  vtkGetMacro(OutputSelection, bool);
  vtkSetMacro(OutputSelection, bool);
  vtkBooleanMacro(OutputSelection, bool);
  
  // Description:
  // Set the output selection type. The default is to use the
  // the maximum distance from the starting vertex "MAX_DIST_FROM_ROOT". 
  // But you can also specify other things like "ROOT","2D_MAX", etc
  vtkSetStringMacro(OutputSelectionType);

protected:
  vtkPBGLBreadthFirstSearch();
  ~vtkPBGLBreadthFirstSearch();

  virtual int RequestData(
    vtkInformation *, 
    vtkInformationVector **, 
    vtkInformationVector *);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);

  virtual int FillOutputPortInformation(
    int port, vtkInformation* info);
  
private:

  vtkIdType OriginVertexIndex;
  char* InputArrayName;
  char* OutputArrayName;
  //BTX
  vtkVariant OriginValue;
  //ETX
  bool OutputSelection;
  bool OriginFromSelection;
  char* OutputSelectionType;
  
  // Description:
  // Using the convenience function internally
  vtkSetStringMacro(InputArrayName);

  //BTX
  
  // Description:
  // This method is basically a helper function to find
  // the index of a specific value within a specific array
  vtkIdType GetVertexIndex(
    vtkAbstractArray *abstract,vtkVariant value);
  //ETX

  vtkPBGLBreadthFirstSearch(const vtkPBGLBreadthFirstSearch&);  // Not implemented.
  void operator=(const vtkPBGLBreadthFirstSearch&);  // Not implemented.
};

#endif
