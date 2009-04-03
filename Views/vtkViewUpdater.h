/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewUpdater.h

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
// .NAME vtkViewUpdater - Updates views automatically
//
// .SECTION Description
// vtkViewUpdater registers with selection change events for a set of
// views, and updates all views when a view fires a selection changed event.
// This is often needed when multiple views share a selection with
// vtkSelectionLink.

#ifndef __vtkViewUpdater_h
#define __vtkViewUpdater_h

#include "vtkObject.h"

class vtkView;

class VTK_VIEWS_EXPORT vtkViewUpdater : public vtkObject
{
public:
  static vtkViewUpdater *New();
  vtkTypeRevisionMacro(vtkViewUpdater, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddView(vtkView* view);

protected:
  vtkViewUpdater();
  ~vtkViewUpdater();

private:
  vtkViewUpdater(const vtkViewUpdater&);  // Not implemented.
  void operator=(const vtkViewUpdater&);  // Not implemented.

//BTX
  class vtkViewUpdaterInternals;
  vtkViewUpdaterInternals* Internals;
//ETX
};

#endif
