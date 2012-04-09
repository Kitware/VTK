/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughLayoutStrategy.h

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
// .NAME vtkPassThroughLayoutStrategy - a layout strategy that does absolutely nothing
//
// .SECTION Description
// Yes, this incredible strategy does absoluted nothing to the data
// so in affect passes through the graph untouched. This strategy
// is useful in the cases where the graph is already laid out.

#ifndef __vtkPassThroughLayoutStrategy_h
#define __vtkPassThroughLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkPassThroughLayoutStrategy : public vtkGraphLayoutStrategy 
{
public:
  static vtkPassThroughLayoutStrategy *New();

  vtkTypeMacro(vtkPassThroughLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // This strategy sets up some data structures
  // for faster processing of each Layout() call
  virtual void Initialize();
  
  // Description:
  // This is the layout method where the graph that was
  // set in SetGraph() is laid out. The method can either
  // entirely layout the graph or iteratively lay out the
  // graph. If you have an iterative layout please implement
  // the IsLayoutComplete() method.
  virtual void Layout();
  
  // Description:
  // I'm an iterative layout so this method lets the caller
  // know if I'm done laying out the graph
  virtual int IsLayoutComplete() {return 1;}

protected:
  vtkPassThroughLayoutStrategy();
  ~vtkPassThroughLayoutStrategy();

private:

  vtkPassThroughLayoutStrategy(const vtkPassThroughLayoutStrategy&);  // Not implemented.
  void operator=(const vtkPassThroughLayoutStrategy&);  // Not implemented.
};

#endif

