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
/**
 * @class   vtkAssignCoordinatesLayoutStrategy
 * @brief   uses array values to set vertex locations
 *
 *
 * Uses vtkAssignCoordinates to use values from arrays as the x, y, and z coordinates.
*/

#ifndef vtkAssignCoordinatesLayoutStrategy_h
#define vtkAssignCoordinatesLayoutStrategy_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkGraphLayoutStrategy.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkAssignCoordinates;

class VTKINFOVISLAYOUT_EXPORT vtkAssignCoordinatesLayoutStrategy : public vtkGraphLayoutStrategy
{
public:
  static vtkAssignCoordinatesLayoutStrategy *New();
  vtkTypeMacro(vtkAssignCoordinatesLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The array to use for the x coordinate values.
   */
  virtual void SetXCoordArrayName(const char* name);
  virtual const char* GetXCoordArrayName();
  //@}

  //@{
  /**
   * The array to use for the y coordinate values.
   */
  virtual void SetYCoordArrayName(const char* name);
  virtual const char* GetYCoordArrayName();
  //@}

  //@{
  /**
   * The array to use for the z coordinate values.
   */
  virtual void SetZCoordArrayName(const char* name);
  virtual const char* GetZCoordArrayName();
  //@}

  /**
   * Perform the random layout.
   */
  void Layout() override;

protected:
  vtkAssignCoordinatesLayoutStrategy();
  ~vtkAssignCoordinatesLayoutStrategy() override;

  vtkSmartPointer<vtkAssignCoordinates> AssignCoordinates;

private:
  vtkAssignCoordinatesLayoutStrategy(const vtkAssignCoordinatesLayoutStrategy&) = delete;
  void operator=(const vtkAssignCoordinatesLayoutStrategy&) = delete;
};

#endif

