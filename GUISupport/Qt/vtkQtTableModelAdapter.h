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
/**
 * @class   vtkQtTableModelAdapter
 * @brief   Adapts a table to a Qt item model.
 *
 *
 * vtkQtTableModelAdapter is a QAbstractItemModel with a vtkTable as its
 * underlying data model.
 *
 * @sa
 * vtkQtAbstractModelAdapter vtkQtTreeModelAdapter
*/

#ifndef vtkQtTableModelAdapter_h
#define vtkQtTableModelAdapter_h

#include "vtkConfigure.h"
#include "vtkGUISupportQtModule.h" // For export macro
#include "vtkQtAbstractModelAdapter.h"
#include <QImage> // Needed for icon support

class vtkSelection;
class vtkTable;
class vtkVariant;

class QMimeData;

class VTKGUISUPPORTQT_EXPORT vtkQtTableModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtTableModelAdapter(QObject *parent = 0);
  vtkQtTableModelAdapter(vtkTable* table, QObject *parent = 0);
  ~vtkQtTableModelAdapter() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the VTK data object as input to this adapter
   */
  void SetVTKDataObject(vtkDataObject *data) VTK_OVERRIDE;
  vtkDataObject* GetVTKDataObject() const VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Selection conversion from VTK land to Qt land
   */
  vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const VTK_OVERRIDE;
  QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const VTK_OVERRIDE;
  //@}

  void SetKeyColumnName(const char* name) VTK_OVERRIDE;
  void SetColorColumnName(const char* name) VTK_OVERRIDE;
  void SetIconIndexColumnName(const char* name);

  enum
  {
    HEADER = 0,
    ITEM = 1
  };

  enum
  {
    COLORS = 0,
    ICONS = 1,
    NONE = 2
  };

  /**
   * Specify how to color rows if colors are provided by SetColorColumnName().
   * Default is the vertical header.
   */
  void SetDecorationLocation(int s);

  /**
   * Specify how to color rows if colors are provided by SetColorColumnName().
   * Default is the vertical header.
   */
  void SetDecorationStrategy(int s);

  bool GetSplitMultiComponentColumns() const;
  void SetSplitMultiComponentColumns(bool value);

  //@{
  /**
   * Set up the model based on the current table.
   */
  void setTable(vtkTable* table);
  vtkTable* table() const { return this->Table; }
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const VTK_OVERRIDE;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) VTK_OVERRIDE;
  Qt::ItemFlags flags(const QModelIndex &index) const VTK_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const VTK_OVERRIDE;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const VTK_OVERRIDE;
  QModelIndex parent(const QModelIndex &index) const VTK_OVERRIDE;
  int rowCount(const QModelIndex &parent = QModelIndex()) const VTK_OVERRIDE;
  int columnCount(const QModelIndex &parent = QModelIndex()) const VTK_OVERRIDE;
  //@}

  bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) VTK_OVERRIDE ;
  QMimeData * mimeData ( const QModelIndexList & indexes ) const VTK_OVERRIDE;
  QStringList mimeTypes () const VTK_OVERRIDE;
  Qt::DropActions supportedDropActions() const VTK_OVERRIDE;

  void SetIconSheet(QImage sheet);
  void SetIconSize(int w, int h);
  void SetIconSheetSize(int w, int h);

signals:
  void selectionDropped(vtkSelection*);

private:

  void getValue(int row, int column, vtkVariant& retVal) const;
  bool noTableCheck() const;
  void updateModelColumnHashTables();
  QVariant getColorIcon(int row) const;
  QVariant getIcon(int row) const;

  bool        SplitMultiComponentColumns;
  vtkTable*   Table;
  int         DecorationLocation;
  int         DecorationStrategy;
  QImage      IconSheet;
  int         IconSize[2];
  int         IconSheetSize[2];
  int         IconIndexColumn;

  class vtkInternal;
  vtkInternal* Internal;

  vtkQtTableModelAdapter(const vtkQtTableModelAdapter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtTableModelAdapter&) VTK_DELETE_FUNCTION;
};

#endif
