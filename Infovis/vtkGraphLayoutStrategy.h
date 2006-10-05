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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraphLayoutStrategy - abstract superclass for all graph layout strategies
//
// .SECTION Description
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
// layout capabilities.

#ifndef __vtkGraphLayoutStrategy_h
#define __vtkGraphLayoutStrategy_h

#include "vtkObject.h"

class vtkAbstractGraph;

class VTK_INFOVIS_EXPORT vtkGraphLayoutStrategy : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkGraphLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Setting the graph for the layout strategy
  virtual void SetGraph(vtkAbstractGraph *graph);
  
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
  
  

protected:
  vtkGraphLayoutStrategy();
  ~vtkGraphLayoutStrategy();
  
  vtkAbstractGraph *Graph;

private:

  vtkGraphLayoutStrategy(const vtkGraphLayoutStrategy&);  // Not implemented.
  void operator=(const vtkGraphLayoutStrategy&);  // Not implemented.
};

#endif

