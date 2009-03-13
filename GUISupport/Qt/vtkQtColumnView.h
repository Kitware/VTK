/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtColumnView.h

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
// .NAME vtkQtColumnView - A VTK view based on a Qt column view.
//
// .SECTION Description
// vtkQtColumnView is a VTK view using an underlying QColumnView. 
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtColumnView_h
#define __vtkQtColumnView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"

#include <QPointer>
#include "vtkQtAbstractModelAdapter.h"

class QItemSelection;
class QColumnView;
class vtkQtTreeModelAdapter;

class QVTK_EXPORT vtkQtColumnView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtColumnView *New();
  vtkTypeRevisionMacro(vtkQtColumnView, vtkQtView);
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
  vtkQtColumnView();
  ~vtkQtColumnView();

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
  
  QPointer<QColumnView> ColumnView;
  vtkQtTreeModelAdapter* TreeAdapter;
  bool Selecting;
  
  vtkQtColumnView(const vtkQtColumnView&);  // Not implemented.
  void operator=(const vtkQtColumnView&);  // Not implemented.
};

#endif
