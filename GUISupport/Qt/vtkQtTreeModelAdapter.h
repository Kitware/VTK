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
/**
 * @class   vtkQtTreeModelAdapter
 * @brief   Adapts a tree to a Qt item model.
 *
 *
 * vtkQtTreeModelAdapter is a QAbstractItemModel with a vtkTree as its
 * underlying data model.
 *
 * @sa
 * vtkQtAbstractModelAdapter vtkQtTableModelAdapter
*/

#ifndef vtkQtTreeModelAdapter_h
#define vtkQtTreeModelAdapter_h

#include "vtkGUISupportQtModule.h" // For export macro

#include "vtkQtAbstractModelAdapter.h"
#include <QHash> // Needed for the decoration map
#include <QVector> // Needed for the index map
#include "vtkType.h" // Needed for vtkIdType

class vtkSelection;
class vtkTree;
class vtkAdjacentVertexIterator;

class QMimeData;

class VTKGUISUPPORTQT_EXPORT vtkQtTreeModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtTreeModelAdapter(QObject *parent = nullptr, vtkTree* tree = nullptr);
  ~vtkQtTreeModelAdapter() override;

  //@{
  /**
   * Set/Get the VTK data object as input to this adapter
   */
  void SetVTKDataObject(vtkDataObject *data) override;
  vtkDataObject* GetVTKDataObject() const override;
  //@}

  /**
   * Get the stored VTK data object modification time of when the
   * adaption to a Qt model was done. This is in general not the
   * same this as the data objects modification time. It is the mod
   * time of the object when it was placed into the Qt model adapter.
   * You can use this mtime as part of the checking to see whether
   * you need to update the adapter by call SetVTKDataObject again. :)
   */
  vtkMTimeType GetVTKDataObjectMTime() const;

  //@{
  /**
   * Selection conversion from VTK land to Qt land
   */
  vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const override;
  QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const override;
  //@}

  void SetKeyColumnName(const char* name) override;

  void SetColorColumnName(const char* name) override;

  /**
   * Set up the model based on the current tree.
   */
  void setTree(vtkTree* t);
  vtkTree* tree() const { return this->Tree; }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  //@{
  /**
   * If drag/drop is enabled in the view, the model will package up the current
   * pedigreeid vtkSelection into a QMimeData when items are dragged.
   * Currently only leaves of the tree can be dragged.
   */
  Qt::DropActions supportedDragActions() const override;
  QMimeData * mimeData ( const QModelIndexList & indexes ) const override;
  QStringList mimeTypes () const override;
  //@}

protected:
  void treeModified();
  void GenerateVTKIndexToQtModelIndex(vtkIdType vtk_index, QModelIndex qmodel_index);

  vtkTree* Tree;
  vtkAdjacentVertexIterator* ChildIterator;
  vtkMTimeType TreeMTime;
  QVector<QModelIndex> VTKIndexToQtModelIndex;
  QHash<QModelIndex, QVariant> IndexToDecoration;

private:
  vtkQtTreeModelAdapter(const vtkQtTreeModelAdapter &) = delete;
  void operator=(const vtkQtTreeModelAdapter&) = delete;
};

#endif
