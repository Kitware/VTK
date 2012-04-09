/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignCoordinatesLayoutStrategy.h

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
// .NAME vtkAssignCoordinatesLayoutStrategy - uses array values to set vertex locations
//
// .SECTION Description
// Uses vtkAssignCoordinates to use values from arrays as the x, y, and z coordinates.

#ifndef __vtkAssignCoordinatesLayoutStrategy_h
#define __vtkAssignCoordinatesLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkAssignCoordinates;

class VTK_INFOVIS_EXPORT vtkAssignCoordinatesLayoutStrategy : public vtkGraphLayoutStrategy 
{
public:
  static vtkAssignCoordinatesLayoutStrategy *New();
  vtkTypeMacro(vtkAssignCoordinatesLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The array to use for the x coordinate values.
  virtual void SetXCoordArrayName(const char* name);
  virtual const char* GetXCoordArrayName();

  // Description:
  // The array to use for the y coordinate values.
  virtual void SetYCoordArrayName(const char* name);
  virtual const char* GetYCoordArrayName();

  // Description:
  // The array to use for the z coordinate values.
  virtual void SetZCoordArrayName(const char* name);
  virtual const char* GetZCoordArrayName();

  // Description:
  // Perform the random layout.
  void Layout();

protected:
  vtkAssignCoordinatesLayoutStrategy();
  ~vtkAssignCoordinatesLayoutStrategy();

  //BTX
  vtkSmartPointer<vtkAssignCoordinates> AssignCoordinates;
  //ETX

private:
  vtkAssignCoordinatesLayoutStrategy(const vtkAssignCoordinatesLayoutStrategy&);  // Not implemented.
  void operator=(const vtkAssignCoordinatesLayoutStrategy&);  // Not implemented.
};

#endif

