/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtInitialization.h

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
 * @class   vtkQtInitialization
 * @brief   Initializes a Qt application.
 *
 *
 * Utility class that initializes Qt by creating an instance of
 * QApplication in its ctor, if one doesn't already exist.
 * This is mainly of use in ParaView with filters that use Qt in
 * their implementation - create an instance of vtkQtInitialization
 * prior to instantiating any filters that require Qt.
*/

#ifndef vtkQtInitialization_h
#define vtkQtInitialization_h

#include "vtkRenderingQtModule.h" // For export macro
#include "vtkObject.h"

class QApplication;

class VTKRENDERINGQT_EXPORT vtkQtInitialization : public vtkObject
{
public:
  static vtkQtInitialization* New();
  vtkTypeMacro(vtkQtInitialization, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkQtInitialization();
  ~vtkQtInitialization();

private:
  vtkQtInitialization(const vtkQtInitialization &) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtInitialization &) VTK_DELETE_FUNCTION;

  QApplication *Application;

};

#endif // vtkQtInitialization_h

