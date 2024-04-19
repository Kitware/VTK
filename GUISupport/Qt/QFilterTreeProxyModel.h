// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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
#include "vtkGUISupportQtModule.h" // For export macro
#include <QSortFilterProxyModel>

class QModelIndex;
VTK_ABI_NAMESPACE_BEGIN

class VTKGUISUPPORTQT_EXPORT QFilterTreeProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  QFilterTreeProxyModel(QObject* p = nullptr);
  ~QFilterTreeProxyModel() override;

  // Description:
  // The 0-based level in the tree hierarchy to filter on. The root is level 0.
  void setFilterTreeLevel(int level);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
  int TreeLevel;
};

VTK_ABI_NAMESPACE_END
#endif
