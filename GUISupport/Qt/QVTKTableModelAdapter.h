/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKTableModelAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QVTKTableModelAdapter
 * @brief An adapter to create a vtkTable from an QAbstractItemModel.
 *
 * An internal table is used to buffer the QAbstractItemModel. Any changes in the model are applied
 * to the internal table, so that it is always up-to-date with the model.
 *
 * Due to the structure of vtkTable it is not possible to have columns with different variable
 * types. The variable type of a column is determined from the first row in the model.
 *
 * The data is queried using QAbstractItemModel::data() using Qt::DisplayRole.
 *
 * All columns of the table must have unique names. They are queried using
 * QAbstractItemModel::headerData() using Qt::Horizontal as orientation and Qt::DisplayRole.
 *
 */

#ifndef QVTKTableModelAdapter_h
#define QVTKTableModelAdapter_h

#include "vtkGUISupportQtModule.h" // For export macro

#include <QAbstractItemModel>
#include <QObject>
#include <QPointer>

#include "vtkNew.h"   // For vtkNew
#include "vtkTable.h" // For vtkTable

class VTKGUISUPPORTQT_EXPORT QVTKTableModelAdapter : public QObject
{
  Q_OBJECT
public:
  QVTKTableModelAdapter(QObject* parent = nullptr);
  QVTKTableModelAdapter(QAbstractItemModel* model, QObject* parent = nullptr);

  ///@{
  /**
   * Get/set the Qt table model. It is expected that the QAbstractItemModel passed in is a
   * QAbstractTableModel subclass; however allowing this class to work with a QAbstractItemModel is
   * advantageous since it enables usage of proxy models, for example for sorting and filtering.
   */
  virtual void SetItemModel(QAbstractItemModel* model);
  QAbstractItemModel* GetItemModel() const;
  ///@}

  /**
   * Access to the vtkTable. Treat this as a const object, i.e. you should not modify it
   * outside of this class.
   */
  vtkTable* GetTable() const;

Q_SIGNALS:
  /**
   * Signal emitted when the internal vtkTable has changed. This signal can be used to connect to a
   * slot which handles rendering of an attached chart.
   */
  void tableChanged();

protected:
  /**
   * The default method for retrieving data for a tabel entry from the item model.
   */
  virtual QVariant modelData(int row, int col);

  /**
   * Return a suitable vtk array for the QVariant type.
   * Note to call Delete() on the returned array where necessary, see vtkObjectBase::Delete() for
   * details.
   */
  virtual vtkAbstractArray* NewArray(const QVariant& type);

  /**
   * Check that the correct array types are set for the columns.
   */
  virtual bool HasCorrectColumnArrays();

  /**
   * Sets the value of the cell given by row, column using the adequate QVariant type conversion.
   */
  virtual void SetCellValue(int row, int column, const QVariant& data);

  /**
   * Update the internal table from row0 to inclusive row1, and from column0 to inclusive column1
   * so it reflects the data in the model.
   */
  virtual void UpdateTable(int row0, int column0, int row1, int column1);

  QPointer<QAbstractItemModel> ItemModel;
  vtkNew<vtkTable> Table;

protected Q_SLOTS:
  virtual void onModified();
  virtual void onModelReset();
  virtual void onDataChanged(
    const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
  virtual void onHeaderDataChanged(Qt::Orientation orientation, int first, int last);
  virtual void onLayoutChanged(
    const QList<QPersistentModelIndex>& parents, QAbstractItemModel::LayoutChangeHint hint);
  virtual void onRowsInserted(const QModelIndex& parent, int first, int last);
  virtual void onRowsRemoved(const QModelIndex& parent, int first, int last);
  virtual void onRowsMoved(
    const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
  virtual void onColumnsInserted(const QModelIndex& parent, int first, int last);
  virtual void onColumnsRemoved(const QModelIndex& parent, int first, int last);
  virtual void onColumnsMoved(
    const QModelIndex& parent, int start, int end, const QModelIndex& destination, int column);
};

#endif
