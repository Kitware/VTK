/* -*- Mode: C++; -*- */

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableMetadataRepresentation.h

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

// .NAME vtkQtTableMetadataRepresentation - Show table metadata in a QListView
//
// .SECTION Description
//
// A table representing data to be charted will usually have a single
// column listing the titles of the various series.  This class pulls
// out those titles so that they can go into a vtkQtItemView where
// they may be toggled on and off.  The lookup table in the superclass
// is used to assign colors to each series.
//
// This class is meant to be used with vtkQtItemView.
//

#ifndef __vtkQtTableMetadataRepresentation_h
#define __vtkQtTableMetadataRepresentation_h

#include "QVTKWin32Header.h"
#include "vtkQtTableRepresentation.h"

// ----------------------------------------------------------------------

class QVTK_EXPORT vtkQtTableMetadataRepresentation : public vtkQtTableRepresentation
{
public:
  vtkTypeRevisionMacro(vtkQtTableMetadataRepresentation, vtkQtTableRepresentation);
  static vtkQtTableMetadataRepresentation *New();
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkQtTableMetadataRepresentation();
  ~vtkQtTableMetadataRepresentation();

  // Description:
  // Setup input connections to the view.
  virtual void SetupInputConnections();

  // Description:
  // Add this representation to the view.  That view needs to be a
  // vtkQtItemView or one of its subclasses.
  bool AddToView(vtkView *view);

  // Description:
  // Remove this representation from a view.
  bool RemoveFromView(vtkView *view);

  void SetModelType();

private:
  vtkQtTableMetadataRepresentation(const vtkQtTableMetadataRepresentation &);
  void operator=(const vtkQtTableMetadataRepresentation &);

};

#endif
  
