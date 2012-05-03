/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLShortestPaths.h

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
// .NAME vtkPBGLShortestPaths - Compute the shortest paths from the origin
// vertex to all other vertices in a distributed vtkGraph.
//
// .SECTION Description
// This VTK class uses the Parallel BGL's implementation of the
// delta-stepping algorithm generic algorithm to compute shortest
// paths from a given 'source' vertex on the input graph (a
// distributed vtkGraph). Delta-stepping, discovered by Meyer and
// Sanders, is a parallel form of Dijkstra's shortest paths algorithm,
// based on a multi-level bucket structure that permits edges to be
// relaxed in parallel.

#ifndef __vtkPBGLShortestPaths_h
#define __vtkPBGLShortestPaths_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class vtkSelection;

class VTKINFOVISPARALLEL_EXPORT vtkPBGLShortestPaths : public vtkGraphAlgorithm
{
public:
  static vtkPBGLShortestPaths *New();
  vtkTypeMacro(vtkPBGLShortestPaths, vtkGraphAlgorithm);
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

  // Description:
  // Set the breadth first search 'origin' vertex.
  // This method is basically the same as above
  // but allows the application to simply specify
  // an array name and value, instead of having to
  // know the specific index of the vertex.
  void SetOriginVertex(vtkStdString arrayName, vtkVariant value);

  // Description:
  // Convenience method for setting the origin vertex
  // given an array name and string value.
  // This method is primarily for the benefit of the
  // VTK Parallel client/server layer, callers should
  // prefer to use SetOriginVertex() whenever possible.
  void SetOriginVertexString(char* arrayName, char* value);

  // Description:
  // Set the name of the edge-weight input array, which must name an
  // array that is part of the edge data of the input graph and
  // contains numeric data. If the edge-weight array is not of type
  // vtkDoubleArray, the array will be copied into a temporary
  // vtkDoubleArray.
  vtkSetStringMacro(EdgeWeightArrayName);

  // Description:
  // Sets the value of delta, which is the width of each "bucket"
  // within the multi-level bucket structure used internally by this
  // algorithm. Large values of delta correspond with wider buckets,
  // exposing more parallelism than smaller values. However, values
  // that are too large will cause the algorithm to compute (and,
  // later, correct) paths that are longer than the shortest
  // path. While the value of delta will not affect the correctness of
  // the results of this algorithm, delta can have a significant
  // impact on performance. If no value of delta is provided, this
  // algorithm employs the heuristics provided by Meyer and Sanders to
  // automatically determine a delta.
  vtkSetMacro(Delta,double);

  // Description:
  // Set the name of the predecessor output array, which contains the
  // predecessor of each vertex within the shortest paths tree. To
  // determine the shortest path from the origin to a particular
  // vertex, walk the predecessor array backwards. If no predecessor
  // array name is set then the name 'Predecessor' is used.
  vtkSetStringMacro(PredecessorArrayName);

  // Description:
  // Set the name of the shortest path length output array, containing
  // the length of the shortest path from the origin vertex to each of
  // the other vertices in the graph. The origin will always have
  // path-length 0, while vertices unreachable from the origin will
  // have infinite path-length.  If no path length array name is set
  // then the name 'PathLength' is used.
  vtkSetStringMacro(PathLengthArrayName);

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

  // Description:
  // This option causes a temporary edge-weight array to be created
  // with uniform edge weights of 1.0 at each edge.  This option should
  // preempt a given edge weight array via EdgeWeightArrayName.
  // Defaults to off.
  vtkSetMacro(UseUniformEdgeWeights, bool);
  vtkGetMacro(UseUniformEdgeWeights, bool);
  vtkBooleanMacro(UseUniformEdgeWeights, bool);


protected:
  vtkPBGLShortestPaths();
  ~vtkPBGLShortestPaths();

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
  char* EdgeWeightArrayName;
  double Delta;
  char* PredecessorArrayName;
  char* PathLengthArrayName;
  vtkVariant OriginValue;
  bool OutputSelection;
  bool OriginFromSelection;
  bool UseUniformEdgeWeights;
  char* OutputSelectionType;

  // Description:
  // Using the convenience function internally
  vtkSetStringMacro(InputArrayName);

  // Description:
  // This method is basically a helper function to find
  // the index of a specific value within a specific array
  vtkIdType GetVertexIndex(
    vtkAbstractArray *abstract,vtkVariant value);

  vtkPBGLShortestPaths(const vtkPBGLShortestPaths&);  // Not implemented.
  void operator=(const vtkPBGLShortestPaths&);  // Not implemented.
};

#endif
