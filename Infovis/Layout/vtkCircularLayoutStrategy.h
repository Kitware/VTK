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
/**
 * @class   vtkCircularLayoutStrategy
 * @brief   Places vertices around a circle
 *
 *
 * Assigns points to the vertices around a circle with unit radius.
*/

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

  /**
   * Perform the layout.
   */
  void Layout();

protected:
  vtkCircularLayoutStrategy();
  ~vtkCircularLayoutStrategy();

private:
  vtkCircularLayoutStrategy(const vtkCircularLayoutStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCircularLayoutStrategy&) VTK_DELETE_FUNCTION;
};

#endif

