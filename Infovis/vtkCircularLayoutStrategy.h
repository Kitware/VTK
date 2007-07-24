/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCircularLayoutStrategy.h

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
// .NAME vtkCircularLayoutStrategy - Places vertices around a circle
//
// .SECTION Description
// Assigns points to the vertices around a circle with unit radius.

#ifndef __vtkCircularLayoutStrategy_h
#define __vtkCircularLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkCircularLayoutStrategy : public vtkGraphLayoutStrategy 
{
public:
  static vtkCircularLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkCircularLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout.
  void Layout();
  
protected:
  vtkCircularLayoutStrategy();
  ~vtkCircularLayoutStrategy();

private:
  vtkCircularLayoutStrategy(const vtkCircularLayoutStrategy&);  // Not implemented.
  void operator=(const vtkCircularLayoutStrategy&);  // Not implemented.
};

#endif

