/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutStrategy.h

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
// .NAME vtkGraphLayoutStrategy - abstract superclass for all graph layout strategies
//
// .SECTION Description
// All graph layouts should subclass from this class.  vtkGraphLayoutStrategy
// works as a plug-in to the vtkGraphLayout algorithm.  The Layout()
// function should perform some reasonable "chunk" of the layout.
// This allows the user to be able to see the progress of the layout.
// Use IsLayoutComplete() to tell the user when there is no more layout
// to perform.
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
// layout capabilities.

#ifndef __vtkGraphLayoutStrategy_h
#define __vtkGraphLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

class vtkGraph;

class VTKINFOVISLAYOUT_EXPORT vtkGraphLayoutStrategy : public vtkObject
{
public:
  vtkTypeMacro(vtkGraphLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Setting the graph for the layout strategy
  virtual void SetGraph(vtkGraph *graph);

  // Description:
  // This method allows the layout strategy to
  // do initialization of data structures
  // or whatever else it might want to do.
  virtual void Initialize() {};

  // Description:
  // This is the layout method where the graph that was
  // set in SetGraph() is laid out. The method can either
  // entirely layout the graph or iteratively lay out the
  // graph. If you have an iterative layout please implement
  // the IsLayoutComplete() method.
  virtual void Layout()=0;

  // Description:
  // If your concrete class is iterative than
  // you should overload IsLayoutComplete()
  // otherwise it simply returns 1 by default;
  virtual int IsLayoutComplete() {return 1;}

  // Description:
  // Whether to use edge weights in the layout or not.
  virtual void SetWeightEdges(bool state);
  vtkGetMacro(WeightEdges, bool);

  // Description:
  // Set/Get the field to use for the edge weights.
  virtual void SetEdgeWeightField(const char* field);
  vtkGetStringMacro(EdgeWeightField);

protected:
  vtkGraphLayoutStrategy();
  ~vtkGraphLayoutStrategy();

  vtkGraph *Graph;
  char     *EdgeWeightField;
  bool     WeightEdges;
private:

  vtkGraphLayoutStrategy(const vtkGraphLayoutStrategy&);  // Not implemented.
  void operator=(const vtkGraphLayoutStrategy&);  // Not implemented.
};

#endif

