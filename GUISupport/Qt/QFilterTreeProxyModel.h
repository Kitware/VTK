/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QFilterTreeProxyModel.h

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
// .NAME QFilterTreeProxyModel - An implementation of a QSortFilterProxyModel
//    tailored for hierarchical models. 
//
// .SECTION Description
// An implementation of a QSortFilterProxyModel tailored for hierarchical 
// models. It allows you to filter the model based on the content in a 
// certain column of a certain level in the tree. Indices above that level 
// in the tree are retained. Indices below the level are kept if their 
// ancestor at the tree level is kept. 
//
// .SECTION See also

#ifndef __QFilterTreeProxyModel_h
#define __QFilterTreeProxyModel_h

#include "QVTKWin32Header.h"
#include <QSortFilterProxyModel>

class QModelIndex;

class QVTK_EXPORT QFilterTreeProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:

  QFilterTreeProxyModel(QObject* p = 0);
  ~QFilterTreeProxyModel();

  // Description:
  // The 0-based level in the tree hierarchy to filter on. The root is level 0.
  void setFilterTreeLevel(int level);

protected:

  virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
  virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:

  int TreeLevel;
};

#endif
