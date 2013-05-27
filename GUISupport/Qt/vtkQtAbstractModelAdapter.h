/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAbstractModelAdapter.h

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
// .NAME vtkQtAbstractModelAdapter - Superclass for Qt model adapters.
//
// .SECTION Description
// vtkQtAbstractModelAdapter is the superclass for classes that adapt
// VTK objects to QAbstractItemModel. This class contains API for converting
// between QModelIndex and VTK ids, as well as some additional specialized
// functionality such as setting a column of data to use as the Qt header
// information.
//
// .SECTION See also
// vtkQtTableModelAdapter vtkQtTreeModelAdapter

#ifndef __vtkQtAbstractModelAdapter_h
#define __vtkQtAbstractModelAdapter_h

#include "vtkGUISupportQtModule.h" // For export macro
#include "QVTKWin32Header.h"
#include <QAbstractItemModel>
#include <QItemSelection>

class vtkDataObject;
class vtkSelection;
class QItemSelection;
class VTKGUISUPPORTQT_EXPORT vtkQtAbstractModelAdapter : public QAbstractItemModel
{
  Q_OBJECT

public:

  // The view types.
  enum {
    FULL_VIEW,
    DATA_VIEW
  };

  vtkQtAbstractModelAdapter(QObject* p) :
    QAbstractItemModel(p),
    ViewType(FULL_VIEW),
    KeyColumn(-1),
    ColorColumn(-1),
    DataStartColumn(-1),
    DataEndColumn(-1)
    { }

  // Description:
  // Set/Get the VTK data object as input to this adapter
  virtual void SetVTKDataObject(vtkDataObject *data) = 0;
  virtual vtkDataObject* GetVTKDataObject() const = 0;

  // Description:
  // Selection conversion from VTK land to Qt land
  virtual vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const = 0;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const = 0;

  // Description:
  // Set/Get the view type.
  // FULL_VIEW gives access to all the data.
  // DATA_VIEW gives access only to the data columns specified with SetDataColumnRange()
  // The default is FULL_VIEW.
  virtual void SetViewType(int type) { this->ViewType = type; }
  virtual int GetViewType() { return this->ViewType; }

  // Description:
  // Set/Get the key column.
  // The key column is used as the row headers in a table view,
  // and as the first column in a tree view.
  // Set to -1 for no key column.
  // The default is no key column.
  virtual void SetKeyColumn(int col) { this->KeyColumn = col; }
  virtual int GetKeyColumn() { return this->KeyColumn; }
  virtual void SetKeyColumnName(const char* name) = 0;

  // Description:
  // Set/Get the column storing the rgba color values for each row.
  // The color column is used as the row headers in a table view,
  // and as the first column in a tree view.
  // Set to -1 for no key column.
  // The default is no key column.
  virtual void SetColorColumn(int col) { this->ColorColumn = col; }
  virtual int GetColorColumn() { return this->ColorColumn; }
  virtual void SetColorColumnName(const char* name) = 0;

  // Description:
  // Set the range of columns that specify the main data matrix.
  // The data column range should not include the key column.
  // The default is no data columns.
  virtual void SetDataColumnRange(int c1, int c2)
    { this->DataStartColumn = c1; this->DataEndColumn = c2; }

  // We make the reset() method public because it isn't always possible for
  // an adapter to know when its input has changed, so it must be callable
  // by an outside entity.
  /// \sa beginResetModel, endResetModel
  /// \deprecated
  void reset() { QAbstractItemModel::beginResetModel(); QAbstractItemModel::endResetModel();}

  // We make the beginResetModel() and endResetModel() methods public because it
  // isn't always possible for an adapter to know when its input has changed,
  // so it must be callable by an outside entity.
  void beginResetModel() { QAbstractItemModel::beginResetModel(); }
  void endResetModel() { QAbstractItemModel::endResetModel(); }


signals:
  void modelChanged();

protected:

  // Description:
  // Map a column index in the QAbstractItemModel to a vtkTable column.
  // If the argument is out of range or cannot be mapped then
  // this method may return -1.
  virtual int ModelColumnToFieldDataColumn(int col) const;

  int ViewType;
  int KeyColumn;
  int ColorColumn;
  int DataStartColumn;
  int DataEndColumn;
};

#endif
