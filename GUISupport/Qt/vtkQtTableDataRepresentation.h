/* -*- Mode: C++; -*- */

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableDataRepresentation.h

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

// .NAME vtkQtTableDataRepresentation - Show table metadata in a QListView
//
// .SECTION Description
//
// This specializes vtkQtTableRepresentation to provide a data view of
// the table instead of a metadata view.  We do not yet tell you what
// exactly you should do with the data -- that's left for subclasses.
// Obvious possibilities are a spreadsheet view and a chart view.
//

#ifndef __vtkQtTableDataRepresentation_h
#define __vtkQtTableDataRepresentation_h

#include "QVTKWin32Header.h"
#include "vtkQtTableRepresentation.h"

// ----------------------------------------------------------------------

class QVTK_EXPORT vtkQtTableDataRepresentation : public vtkQtTableRepresentation
{
public:
  vtkTypeRevisionMacro(vtkQtTableDataRepresentation, vtkQtTableRepresentation);
  static vtkQtTableDataRepresentation *New();
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkQtTableDataRepresentation();
  ~vtkQtTableDataRepresentation();


  void SetModelType();

private:
  vtkQtTableDataRepresentation(const vtkQtTableDataRepresentation &);
  void operator=(const vtkQtTableDataRepresentation &);

};

#endif
