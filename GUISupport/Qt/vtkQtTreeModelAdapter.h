/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTreeModelAdapter.h

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
// .NAME vtkQtTreeModelAdapter - Adapts a tree to a Qt item model.
//
// .SECTION Description
// vtkQtTreeModelAdapter is a QAbstractItemModel with a vtkTree as its
// underlying data model. 
//
// .SECTION See also
// vtkQtAbstractModelAdapter vtkQtTableModelAdapter

#ifndef __vtkQtTreeModelAdapter_h
#define __vtkQtTreeModelAdapter_h

#include "QVTKWin32Header.h"
#include "vtkType.h"
#include "vtkSelection.h"

#include "vtkQtAbstractModelAdapter.h"
#include <QHash>
#include <QVector>

class vtkTree;
class vtkAdjacentVertexIterator;

class QVTK_EXPORT vtkQtTreeModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtTreeModelAdapter(QObject *parent = 0, vtkTree* tree = 0);
  ~vtkQtTreeModelAdapter();
  
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

  // Description:
  // Set up the model based on the current tree.
  void setTree(vtkTree* t);
  vtkTree* tree() const { return this->Tree; }
  
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

protected:
  void treeModified();
  void GenerateVTKIndexToQtModelIndex(vtkIdType vtk_index, QModelIndex qmodel_index);
  
  vtkTree* Tree;
  vtkAdjacentVertexIterator* ChildIterator;
  unsigned long TreeMTime;
  QVector<QModelIndex> VTKIndexToQtModelIndex;
  QHash<QModelIndex, QVariant> IndexToDecoration;
  
private:
  vtkQtTreeModelAdapter(const vtkQtTreeModelAdapter &);  // Not implemented
  void operator=(const vtkQtTreeModelAdapter&);  // Not implemented.
};

#endif
