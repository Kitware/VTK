/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAnnotationView.h

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
// .NAME vtkQtAnnotationView - A VTK view that displays the annotations
//    on its annotation link.
//
// .SECTION Description
// vtkQtAnnotationView is a VTK view using an underlying QTableView. 
//
// .SECTION Thanks

#ifndef __vtkQtAnnotationView_h
#define __vtkQtAnnotationView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"
#include <QObject>

#include <QPointer>
#include "vtkQtAnnotationLayersModelAdapter.h"

class QItemSelection;
class QTableView;

class QVTK_EXPORT vtkQtAnnotationView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtAnnotationView *New();
  vtkTypeMacro(vtkQtAnnotationView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtAnnotationView();
  ~vtkQtAnnotationView();

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  unsigned long LastInputMTime;
  
  QPointer<QTableView> View;
  vtkQtAnnotationLayersModelAdapter* Adapter;
  
  vtkQtAnnotationView(const vtkQtAnnotationView&);  // Not implemented.
  void operator=(const vtkQtAnnotationView&);  // Not implemented.
  
};

#endif
