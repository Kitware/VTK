/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTableModelAdapter.h

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
// .NAME vtkQtTableModelAdapter - Adapts a table to a Qt item model.
//
// .SECTION Description
// vtkQtTableModelAdapter is a QAbstractItemModel with a vtkTable as its
// underlying data model.
//
// .SECTION See also
// vtkQtAbstractModelAdapter vtkQtTreeModelAdapter

#ifndef __vtkQtTableModelAdapter_h
#define __vtkQtTableModelAdapter_h

#include "vtkQtAbstractModelAdapter.h"

class vtkTable;
class vtkVariant;
class QVTK_EXPORT vtkQtTableModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtTableModelAdapter(QObject *parent = 0);
  vtkQtTableModelAdapter(vtkTable* table, QObject *parent = 0);
  ~vtkQtTableModelAdapter();
  
  // Description:
  // Set/Get the VTK data object as input to this adapter
  virtual void SetVTKDataObject(vtkDataObject *data);
  virtual vtkDataObject* GetVTKDataObject() const;
  
  // Description:
  // Selection conversion from VTK land to Qt land
  virtual vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const;
  
  virtual void SetKeyColumnName(const char* name);
  virtual void SetColorColumnName(const char* name);

  bool GetSplitMultiComponentColumns() const;
  void SetSplitMultiComponentColumns(bool value);

  // Description:
  // Set up the model based on the current table.
  void setTable(vtkTable* table);
  vtkTable* table() const { return this->Table; }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &index) const;
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:

  void getValue(int row, int column, vtkVariant& retVal) const;
  bool noTableCheck() const;
  void updateModelColumnHashTables();

  bool        SplitMultiComponentColumns;
  vtkTable*   Table;
  char*       ColorColumnName;

  class vtkInternal;
  vtkInternal* Internal;
  
  vtkQtTableModelAdapter(const vtkQtTableModelAdapter &);  // Not implemented
  void operator=(const vtkQtTableModelAdapter&);  // Not implemented.
};

#endif
