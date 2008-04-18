/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtListView.h

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
// .NAME vtkQtListView - A VTK view based on a Qt list view. 
//
// .SECTION Description
// vtkQtListView is a VTK view containing an underlying QListView.
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtListView_h
#define __vtkQtListView_h

#include "QVTKWin32Header.h"
#include "vtkQtItemView.h"

//class QListView;
//class vtkQtTableModelAdapter;
class QAbstractItemView;
class vtkQtAbstractModelAdapter;

class QVTK_EXPORT vtkQtListView : public vtkQtItemView
{
public:
  static vtkQtListView *New();
  vtkTypeRevisionMacro(vtkQtListView, vtkQtItemView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the underlying Qt view.
  virtual void SetItemView(QAbstractItemView*);
  
  // Description:
  // Set the underlying Qt model adapter.
  virtual void SetItemModelAdapter(vtkQtAbstractModelAdapter* qma);

protected:
  vtkQtListView();
  ~vtkQtListView();

private:
  vtkQtListView(const vtkQtListView&);  // Not implemented.
  void operator=(const vtkQtListView&);  // Not implemented.
  
  QAbstractItemView* ListViewPtr;
  vtkQtAbstractModelAdapter* TableAdapterPtr;
  
  bool IOwnListView;
  bool IOwnTableAdapter;
};

#endif
