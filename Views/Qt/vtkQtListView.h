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
/**
 * @class   vtkQtListView
 * @brief   A VTK view based on a Qt List view.
 *
 *
 * vtkQtListView is a VTK view using an underlying QListView.
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for implementing
 * this class
*/

#ifndef vtkQtListView_h
#define vtkQtListView_h

#include "vtkViewsQtModule.h" // For export macro
#include "vtkQtView.h"

#include <QPointer> // Needed for the internal list view
#include <QImage> // Needed for the icon methods
#include "vtkSmartPointer.h" // Needed for member variables

class vtkApplyColors;
class vtkDataObjectToTable;
class QItemSelection;
class QSortFilterProxyModel;
class QListView;
class vtkQtTableModelAdapter;

class VTKVIEWSQT_EXPORT vtkQtListView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtListView *New();
  vtkTypeMacro(vtkQtListView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  QWidget* GetWidget() override;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5,
  };

  //@{
  /**
   * The field type to copy into the output table.
   * Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
   */
  vtkGetMacro(FieldType, int);
  void SetFieldType(int);
  //@}

  /**
   * Enable drag and drop on this widget
   */
  void SetEnableDragDrop(bool);

  /**
   * Have the view alternate its row colors
   */
  void SetAlternatingRowColors(bool);

  /**
   * The strategy for how to decorate rows.
   * Should be one of vtkQtTableModelAdapter::COLORS,
   * vtkQtTableModelAdapter::ICONS, or
   * vtkQtTableModelAdapter::NONE. Default is NONE.
   */
  void SetDecorationStrategy(int);

  //@{
  /**
   * The array to use for coloring items in view.  Default is "color".
   */
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  //@}

  //@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorByArray(bool vis);
  bool GetColorByArray();
  vtkBooleanMacro(ColorByArray, bool);
  //@}

  /**
   * The column to display
   */
  void SetVisibleColumn(int col);

  /**
   * The column used to filter on
   */
  void SetFilterRegExp(const QRegExp& pattern);

  //@{
  /**
   * Set the icon ivars. Only used if the decoration strategy is set to ICONS.
   */
  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);
  void SetIconArrayName(const char* name);
  //@}

  void ApplyViewTheme(vtkViewTheme* theme) override;

  /**
   * Updates the view.
   */
  void Update() override;

protected:
  vtkQtListView();
  ~vtkQtListView() override;

  void AddRepresentationInternal(vtkDataRepresentation* rep) override;
  void RemoveRepresentationInternal(vtkDataRepresentation* rep) override;

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();

  vtkMTimeType LastSelectionMTime;
  vtkMTimeType LastInputMTime;
  vtkMTimeType LastMTime;

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

  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;
  vtkSmartPointer<vtkApplyColors> ApplyColors;

  vtkQtListView(const vtkQtListView&) = delete;
  void operator=(const vtkQtListView&) = delete;

};

#endif
