/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTreeView.h

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
// .NAME vtkQtTreeView - A VTK view based on a Qt tree view.
//
// .SECTION Description
// vtkQtTreeView is a VTK view using an underlying QTreeView. 
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtTreeView_h
#define __vtkQtTreeView_h

#include "QVTKWin32Header.h"
#include "vtkQtItemView.h"

class QTreeView;
class vtkQtTreeModelAdapter;

class QVTK_EXPORT vtkQtTreeView : public vtkQtItemView
{
public:
  static vtkQtTreeView *New();
  vtkTypeRevisionMacro(vtkQtTreeView, vtkQtItemView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the underlying Qt view.
  virtual void SetItemView(QAbstractItemView*);
  
  // Description:
  // Set the underlying Qt model adapter.
  virtual void SetItemModelAdapter(vtkQtAbstractModelAdapter* qma);

protected:
  vtkQtTreeView();
  ~vtkQtTreeView();

private:
  vtkQtTreeView(const vtkQtTreeView&);  // Not implemented.
  void operator=(const vtkQtTreeView&);  // Not implemented.
  
  QTreeView* TreeViewPtr;
  vtkQtTreeModelAdapter* TreeAdapterPtr;
  
  bool IOwnTreeView;
  bool IOwnTreeAdapter;
};

#endif
