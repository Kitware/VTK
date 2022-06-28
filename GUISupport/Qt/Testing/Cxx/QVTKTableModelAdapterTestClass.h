/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtTableModelAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef QVTKTableModelAdapterTestClass_h
#define QVTKTableModelAdapterTestClass_h

/**
 * @class QVTKTableModelAdapterTestClass
 * @brief Class required by TestQVTKTableModelAdapter to run Qt tests involving event loop.
 *
 */

#include "QVTKTableModelAdapter.h"

#include <QAbstractTableModel>
#include <QStringList>
#include <QVector>

class QVTKTableModelAdapterTestClass : public QAbstractTableModel
{
  Q_OBJECT

public:
  QVTKTableModelAdapterTestClass(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
  bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

  bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;
  bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

  QVariant data(const QModelIndex& index, int role) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role) override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public Q_SLOTS:
  void runTests();

protected:
  int Errors;

  QVector<QVector<double>> ColumnData;
  QStringList ColumnNames;

  QVTKTableModelAdapter* TargetAdapter;

  void processEvents();
  void testClearTable();
  void testChangeHeader(int column, const QString& name);
  void testColumnInsertion(int column, const QStringList& names);
  void testColumnRemoval(int column, int n);
  void testRowInsertion(int row, int n);
  void testRowRemoval(int row, int n);

  void testInsertRemoveColumns();
  void testInsertRemoveRows();
};

#endif
