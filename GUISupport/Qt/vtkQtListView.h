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
// .NAME vtkQtListView - A VTK view based on a Qt List view.
//
// .SECTION Description
// vtkQtListView is a VTK view using an underlying QListView. 
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtListView_h
#define __vtkQtListView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"

#include <QPointer>
#include "vtkQtAbstractModelAdapter.h"

class QItemSelection;
class QListView;
class vtkQtTableModelAdapter;

class QVTK_EXPORT vtkQtListView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtListView *New();
  vtkTypeRevisionMacro(vtkQtListView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

  // Description:
  // Have the view alternate its row colors
  void SetAlternatingRowColors(bool);

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtListView();
  ~vtkQtListView();

  // Description:
  // Connects the algorithm output to the internal pipeline.
  // This view only supports a single representation.
  virtual void AddInputConnection( int port, int index,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection( int port, int index,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();
  unsigned long CurrentSelectionMTime;

  QPointer<QListView> ListView;
  vtkQtTableModelAdapter* ListAdapter;
  bool Selecting;
  
  vtkQtListView(const vtkQtListView&);  // Not implemented.
  void operator=(const vtkQtListView&);  // Not implemented.
  
};

#endif
