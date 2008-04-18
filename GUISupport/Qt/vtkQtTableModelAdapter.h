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

#include "QVTKWin32Header.h"
#include "vtkType.h"

#include "vtkQtAbstractModelAdapter.h"

#include "vtkSelection.h"
#include <QHash>

class vtkTable;

class QVTK_EXPORT vtkQtTableModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtTableModelAdapter(QObject *parent = 0);
  vtkQtTableModelAdapter(vtkTable* table, QObject *parent = 0);
  ~vtkQtTableModelAdapter();
  
  // Set/Get the VTK data object as input to this adapter
  virtual void SetVTKDataObject(vtkDataObject *data);
  virtual vtkDataObject* GetVTKDataObject() const;
  
  vtkIdType IdToPedigree(vtkIdType id) const;
  vtkIdType PedigreeToId(vtkIdType pedigree) const;
  QModelIndex PedigreeToQModelIndex(vtkIdType id) const;
  vtkIdType QModelIndexToPedigree(QModelIndex index) const;
  
  virtual void SetKeyColumnName(const char* name);

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
  void GenerateHashMap();
  bool noTableCheck() const;

  vtkTable* Table;
  QHash<vtkIdType, vtkIdType> IdToPedigreeHash;
  QHash<vtkIdType, QModelIndex> PedigreeToIndexHash;
  QHash<QModelIndex, vtkIdType> IndexToIdHash;
  
  QHash<QModelIndex, QVariant> IndexToDecoration;
  
  vtkQtTableModelAdapter(const vtkQtTableModelAdapter &);  // Not implemented
  void operator=(const vtkQtTableModelAdapter&);  // Not implemented.
};

#endif
