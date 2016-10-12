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
/**
 * @class   vtkViewUpdater
 * @brief   Updates views automatically
 *
 *
 * vtkViewUpdater registers with annotation change events for a set of
 * annotation links, and updates all views when an annotation link fires an
 * annotation changed event. This is often needed when multiple views share
 * a selection with vtkAnnotationLink.
*/

#ifndef vtkViewUpdater_h
#define vtkViewUpdater_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkObject.h"

class vtkAnnotationLink;
class vtkView;

class VTKVIEWSINFOVIS_EXPORT vtkViewUpdater : public vtkObject
{
public:
  static vtkViewUpdater *New();
  vtkTypeMacro(vtkViewUpdater, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddView(vtkView* view);
  void RemoveView(vtkView* view);

  void AddAnnotationLink(vtkAnnotationLink* link);

protected:
  vtkViewUpdater();
  ~vtkViewUpdater();

private:
  vtkViewUpdater(const vtkViewUpdater&) VTK_DELETE_FUNCTION;
  void operator=(const vtkViewUpdater&) VTK_DELETE_FUNCTION;

  class vtkViewUpdaterInternals;
  vtkViewUpdaterInternals* Internals;

};

#endif
