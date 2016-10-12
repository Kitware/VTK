/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableView.h

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
 * @class   vtkQtTableView
 * @brief   A VTK view based on a Qt Table view.
 *
 *
 * vtkQtTableView is a VTK view using an underlying QTableView.
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for implementing
 * this class
*/

#ifndef vtkQtTableView_h
#define vtkQtTableView_h

#include "vtkViewsQtModule.h" // For export macro
#include "vtkQtView.h"

#include <QPointer> // Needed to hold the view
#include "vtkSmartPointer.h" // Needed for member variables

class vtkAddMembershipArray;
class vtkApplyColors;
class vtkDataObjectToTable;
class vtkIdTypeArray;
class QItemSelection;
class QSortFilterProxyModel;
class QTableView;
class vtkQtTableModelAdapter;

class VTKVIEWSQT_EXPORT vtkQtTableView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtTableView *New();
  vtkTypeMacro(vtkQtTableView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the main container of this view (a  QWidget).
   * The application typically places the view with a call
   * to GetWidget(): something like this
   * this->ui->box->layout()->addWidget(this->View->GetWidget());
   */
  virtual QWidget* GetWidget();

  /**
   * Have the view show/hide its column headers
   */
  void SetShowVerticalHeaders(bool);

  /**
   * Have the view show/hide its row headers
   */
  void SetShowHorizontalHeaders(bool);

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
   * Whether the table allows individual columns to be sorted upon
   * Sorting is enabled by default (turn off for large tables);
   */
  void SetSortingEnabled(bool);

  //@{
  /**
   * Whether or not to display all columns from the input table or to use the
   * ColumnName provided.
   * FIXME: This should be replaced with an Add/Remove column API.
   */
  void SetShowAll(bool);
  vtkGetMacro(ShowAll, bool);
  //@}

  //@{
  /**
   * The name of a single column to display.
   * FIXME: This should be replaced with an Add/Remove column API.
   */
  vtkSetStringMacro(ColumnName);
  vtkGetStringMacro(ColumnName);
  //@}

  void SetColumnVisibility(const QString &name, bool status);

  /**
   * Set whether or not the table view should split multi-component columns
   * into multiple single-component columns
   */
  void SetSplitMultiComponentColumns(bool value);

  /**
   * Get whether or not the table view splits multi-component columns into
   * multiple single-component columns
   */
  bool GetSplitMultiComponentColumns();

  //@{
  /**
   * Whether or not to sort selections that the view receives to the top
   */
  void SetSortSelectionToTop(bool value);
  vtkGetMacro(SortSelectionToTop, bool);
  //@}

  //@{
  /**
   * Whether or not to add an icon to the row header denoting the color
   * of an annotated row.
   */
  void SetApplyRowColors(bool value);
  vtkGetMacro(ApplyRowColors, bool);
  //@}

  /**
   * Updates the view.
   */
  virtual void Update();

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
   * Apply a view theme to this view.
   */
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  enum
  {
    SELECT_ITEMS = 0,
    SELECT_ROWS,
    SELECT_COLUMNS
  };

  //@{
  /**
   * The selection mode for this view.
   * SELECT_ITEMS (0) selects single items.
   * SELECT_ROWS (1) selects rows.
   * SELECT_COLUMNS (2) selects columns.
   * Linked selection only works when in the default mode
   * SELECT_ROWS. Selections from other modes may be retrieved
   * using GetSelectedItems().
   */
  virtual void SetSelectionBehavior(int type);
  virtual int GetSelectionBehavior();
  //@}

  /**
   * Fills the array with the selected items of the view.
   * If the selection behavior is SELECT_ITEMS,
   * arr will be a 2-component array containing (row,column)
   * for each selected item.
   * If the selection behavior is SELECT_ROWS or SELECT_COLUMNS,
   * arr will contain a list of row or column indices.
   */
  virtual void GetSelectedItems(vtkIdTypeArray* arr);

protected:
  vtkQtTableView();
  ~vtkQtTableView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();
  vtkMTimeType LastSelectionMTime;
  vtkMTimeType LastInputMTime;
  vtkMTimeType LastMTime;

  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);

  QPointer<QTableView> TableView;
  vtkQtTableModelAdapter* TableAdapter;
  QSortFilterProxyModel* TableSorter;
  int FieldType;
  bool ShowAll;
  char* ColumnName;
  bool InSelectionChanged;
  bool SortSelectionToTop;
  bool ApplyRowColors;
  char* ColorArrayNameInternal;

  vtkSmartPointer<vtkAddMembershipArray> AddSelectedColumn;
  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;
  vtkSmartPointer<vtkApplyColors> ApplyColors;

  vtkQtTableView(const vtkQtTableView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtTableView&) VTK_DELETE_FUNCTION;

};

#endif
