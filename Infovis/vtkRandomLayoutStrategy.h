/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomLayoutStrategy.h

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
// .NAME vtkRandomLayoutStrategy - randomly places nodes in 2 or 3 dimensions
//
// .SECTION Description
//
// .SECION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
// layout capabilities.

#ifndef __vtkRandomLayoutStrategy_h
#define __vtkRandomLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkRandomLayoutStrategy : public vtkGraphLayoutStrategy 
{
public:
  static vtkRandomLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkRandomLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the region in space in which to place the final graph.
  // The GraphBounds only affects the results if AutomaticBoundsComputation
  // is off.
  vtkSetVector6Macro(GraphBounds,double);
  vtkGetVectorMacro(GraphBounds,double,6);

  // Description:
  // Turn on/off automatic graph bounds calculation. If this
  // boolean is off, then the manually specified GraphBounds is used.
  // If on, then the input's bounds us used as the graph bounds.
  vtkSetMacro(AutomaticBoundsComputation, int);
  vtkGetMacro(AutomaticBoundsComputation, int);
  vtkBooleanMacro(AutomaticBoundsComputation, int);

  // Turn on/off layout of graph in three dimensions. If off, graph
  // layout occurs in two dimensions. By default, three dimensional
  // layout is on.
  vtkSetMacro(ThreeDimensionalLayout, int);
  vtkGetMacro(ThreeDimensionalLayout, int);
  vtkBooleanMacro(ThreeDimensionalLayout, int);

  void SetGraph(vtkAbstractGraph *graph);
  void Layout();

protected:
  vtkRandomLayoutStrategy();
  ~vtkRandomLayoutStrategy();

  double GraphBounds[6];
  int   AutomaticBoundsComputation;
  int   ThreeDimensionalLayout;  //Boolean for a third dimension.
private:

  vtkRandomLayoutStrategy(const vtkRandomLayoutStrategy&);  // Not implemented.
  void operator=(const vtkRandomLayoutStrategy&);  // Not implemented.
};

#endif

