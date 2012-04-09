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

#include "vtkViewsQtModule.h" // For export macro
#include "QVTKWin32Header.h"
#include "vtkQtView.h"

#include <QList>
#include <QPointer>
#include "vtkQtAbstractModelAdapter.h"
#include "vtkSmartPointer.h"
#include "QFilterTreeProxyModel.h"

class QAbstractItemDelegate;
class QAbstractItemView;
class QColumnView;
class QItemSelection;
class QTreeView;
class vtkApplyColors;
class QVBoxLayout;
class vtkQtTreeModelAdapter;
class QItemSelectionModel;

class VTKVIEWSQT_EXPORT vtkQtTreeView : public vtkQtView
{
Q_OBJECT

signals:
  void expanded(const QModelIndex&);
  void collapsed(const QModelIndex&);
  void updatePreviewWidget(const QModelIndex&);

public:
  static vtkQtTreeView *New();
  vtkTypeMacro(vtkQtTreeView, vtkQtView);
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
  // Show the column of the given index in the view
  void ShowColumn(int i);

  // Description:
  // Hide all but the first column in the view
  void HideAllButFirstColumn();

  // Description:
  // The column used to filter on
  void SetFilterColumn(int i);

  // Description:
  // The column used to filter on
  void SetFilterRegExp(const QRegExp& pattern);

  // Description:
  // The column used to filter on
  void SetFilterTreeLevel(int level);

  // Description:
  // Collapses the model item specified by the index.
  void Collapse( const QModelIndex & index );

  // Description:
  // Collapses all expanded items.
  void CollapseAll();

   // Description:
   //Expands the model item specified by the index.
   void Expand ( const QModelIndex & index );

   // Description:
   // Expands all expandable items.
   // Warning: if the model contains a large number of items,
   // this function will take some time to execute.
   void ExpandAll ();

   // Description:
   // Expands all expandable items to the given depth.
   void ExpandToDepth ( int depth );

   // Description:
   // Resizes the column given to the size of its contents.
   void ResizeColumnToContents ( int column );

  // Description:
  // Set whether to use a QColumnView (QTreeView is the default)
  void SetUseColumnView(int state);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Set item delegate to something custom
  void SetItemDelegate(QAbstractItemDelegate* delegate);

  // Description:
  // The array to use for coloring items in view.  Default is "color".
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();

  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  vtkBooleanMacro(ColorByArray, bool);

  virtual void ApplyViewTheme(vtkViewTheme* theme);

protected:
  vtkQtTreeView();
  ~vtkQtTreeView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();
  unsigned long CurrentSelectionMTime;
  unsigned long LastInputMTime;

  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);

  QPointer<QTreeView> TreeView;
  QPointer<QColumnView> ColumnView;
  QPointer<QWidget> Widget;
  QPointer<QVBoxLayout> Layout;
  QPointer<QItemSelectionModel> SelectionModel;
  QList<int> HiddenColumns;
  vtkQtTreeModelAdapter* TreeAdapter;
  QAbstractItemView* View;
  char* ColorArrayNameInternal;
  QFilterTreeProxyModel* TreeFilter;

  vtkSmartPointer<vtkApplyColors> ApplyColors;

  vtkQtTreeView(const vtkQtTreeView&);  // Not implemented.
  void operator=(const vtkQtTreeView&);  // Not implemented.

};

#endif
