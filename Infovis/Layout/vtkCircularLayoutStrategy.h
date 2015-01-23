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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkCircularLayoutStrategy - Places vertices around a circle
//
// .SECTION Description
// Assigns points to the vertices around a circle with unit radius.

#ifndef vtkCircularLayoutStrategy_h
#define vtkCircularLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"

class VTKINFOVISLAYOUT_EXPORT vtkCircularLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkCircularLayoutStrategy *New();

  vtkTypeMacro(vtkCircularLayoutStrategy, vtkGraphLayoutStrategy);
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

