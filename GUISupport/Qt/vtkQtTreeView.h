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
#include "vtkQtView.h"

#include <QPointer>
#include "vtkQtAbstractModelAdapter.h"


class QAbstractItemDelegate;
class QAbstractItemView;
class QColumnView;
class QItemSelection;
class QTreeView;
class QVBoxLayout;
class vtkQtTreeModelAdapter;

class QVTK_EXPORT vtkQtTreeView : public vtkQtView
{
Q_OBJECT

signals:
  void expanded(const QModelIndex&);
  void collapsed(const QModelIndex&);
  void updatePreviewWidget(const QModelIndex&);

public:
  static vtkQtTreeView *New();
  vtkTypeRevisionMacro(vtkQtTreeView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();
  
  // Description:
  // Have the view show/hide its column headers (default is ON)
  void SetShowHeaders(bool);

  // Description:
  // Have the view alternate its row colors (default is OFF)
  void SetAlternatingRowColors(bool);

  // Description:
  // Have the view alternate its row colors (default is OFF)
  void SetEnableDragDrop(bool);

  // Description:
  // Show the root node of the tree (default is OFF)
  void SetShowRootNode(bool);

  // Description:
  // Hide the column of the given index from being shown in the view
  void HideColumn(int i);

  // Description:
  // Set whether to use a QColumnView (QTreeView is the default)
  void SetUseColumnView(int state);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Set item delegate to something custom
  void SetItemDelegate(QAbstractItemDelegate* delegate);

protected:
  vtkQtTreeView();
  ~vtkQtTreeView();

  // Description:
  // Connects the algorithm output to the internal pipeline.
  // This view only supports a single representation.
  virtual void AddInputConnection(
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();
  unsigned long CurrentSelectionMTime;
  
  QPointer<QTreeView> TreeView;
  QPointer<QColumnView> ColumnView;
  QPointer<QWidget> Widget;
  QPointer<QVBoxLayout> Layout;
  vtkQtTreeModelAdapter* TreeAdapter;
  QAbstractItemView* View;
  bool Selecting;
  
  vtkQtTreeView(const vtkQtTreeView&);  // Not implemented.
  void operator=(const vtkQtTreeView&);  // Not implemented.
  
};

#endif
