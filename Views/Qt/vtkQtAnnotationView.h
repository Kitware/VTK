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
/**
 * @class   vtkQtAnnotationView
 * @brief   A VTK view that displays the annotations
 *    on its annotation link.
 *
 *
 * vtkQtAnnotationView is a VTK view using an underlying QTableView.
 *
*/

#ifndef vtkQtAnnotationView_h
#define vtkQtAnnotationView_h

#include "vtkViewsQtModule.h" // For export macro
#include "vtkQtView.h"
#include <QObject> // Needed for the Q_OBJECT macro

#include <QPointer> // Needed to hold the view

class vtkQtAnnotationLayersModelAdapter;

class QItemSelection;
class QTableView;

class VTKVIEWSQT_EXPORT vtkQtAnnotationView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtAnnotationView *New();
  vtkTypeMacro(vtkQtAnnotationView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  QWidget* GetWidget() VTK_OVERRIDE;

  /**
   * Updates the view.
   */
  void Update() VTK_OVERRIDE;

protected:
  vtkQtAnnotationView();
  ~vtkQtAnnotationView() VTK_OVERRIDE;

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  vtkMTimeType LastInputMTime;

  QPointer<QTableView> View;
  vtkQtAnnotationLayersModelAdapter* Adapter;

  vtkQtAnnotationView(const vtkQtAnnotationView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtAnnotationView&) VTK_DELETE_FUNCTION;

};

#endif
