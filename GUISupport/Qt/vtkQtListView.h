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
#include <QSortFilterProxyModel>
#include "vtkQtAbstractModelAdapter.h"
#include "vtkSmartPointer.h"

class vtkApplyColors;
class vtkDataObjectToTable;
class QItemSelection;
class QListView;
class vtkQtTableModelAdapter;

class QVTK_EXPORT vtkQtListView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtListView *New();
  vtkTypeMacro(vtkQtListView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();

  //BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5,
    };
  //ETX
  
  // Description:
  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
  vtkGetMacro(FieldType, int);
  void SetFieldType(int);

  // Description:
  // Enable drag and drop on this widget
  void SetEnableDragDrop(bool);

  // Description:
  // Have the view alternate its row colors
  void SetAlternatingRowColors(bool);

  // Description:
  // The strategy for how to decorate rows.
  // Should be one of vtkQtTableModelAdapter::COLORS, 
  // vtkQtTableModelAdapter::ICONS, or 
  // vtkQtTableModelAdapter::NONE. Default is NONE.
  void SetDecorationStrategy(int);

  // Description:
  // The array to use for coloring items in view.  Default is "color".
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  
  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  vtkBooleanMacro(ColorByArray, bool);

  // Description:
  // The column to display
  void SetVisibleColumn(int col);

  // Description:
  // The column used to filter on
  void SetFilterRegExp(const QRegExp& pattern);

  // Description:
  // Set the icon ivars. Only used if the decoration strategy is set to ICONS.
  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);
  void SetIconArrayName(const char* name);

  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtListView();
  ~vtkQtListView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();

  unsigned long LastSelectionMTime;
  unsigned long LastInputMTime;
  unsigned long LastMTime;

  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);
  vtkSetStringMacro(IconIndexArrayNameInternal);
  vtkGetStringMacro(IconIndexArrayNameInternal);

  QPointer<QListView> ListView;
  vtkQtTableModelAdapter* TableAdapter;
  QSortFilterProxyModel* TableSorter;
  char* ColorArrayNameInternal;
  char* IconIndexArrayNameInternal;
  char* VisibleColumnName;
  bool SortSelectionToTop;
  bool ApplyRowColors;
  int FieldType;
  int VisibleColumn;
  
//BTX
  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;
  vtkSmartPointer<vtkApplyColors> ApplyColors;
//ETX

  vtkQtListView(const vtkQtListView&);  // Not implemented.
  void operator=(const vtkQtListView&);  // Not implemented.
  
};

#endif
