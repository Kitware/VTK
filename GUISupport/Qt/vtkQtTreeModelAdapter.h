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
  vtkQtTreeModelAdapter(QObject *parent = 0, vtkTree* tree = 0);
  ~vtkQtTreeModelAdapter() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the VTK data object as input to this adapter
   */
  void SetVTKDataObject(vtkDataObject *data) VTK_OVERRIDE;
  vtkDataObject* GetVTKDataObject() const VTK_OVERRIDE;
  //@}

  /**
   * Get the stored VTK data object modification time of when the
   * adaption to a Qt model was done. This is in general not the
   * same this as the data objects modification time. It is the mod
   * time of the object when it was placed into the Qt model adapter.
   * You can use this mtime as part of the checking to see whether
   * you need to update the the adapter by call SetVTKDataObject again. :)
   */
  vtkMTimeType GetVTKDataObjectMTime() const;

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

  /**
   * Set up the model based on the current tree.
   */
  void setTree(vtkTree* t);
  vtkTree* tree() const { return this->Tree; }

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

  //@{
  /**
   * If drag/drop is enabled in the view, the model will package up the current
   * pedigreeid vtkSelection into a QMimeData when items are dragged.
   * Currently only leaves of the tree can be dragged.
   */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  Qt::DropActions supportedDragActions() const VTK_OVERRIDE;
#else
  Qt::DropActions supportedDragActions() const;
#endif
  QMimeData * mimeData ( const QModelIndexList & indexes ) const VTK_OVERRIDE;
  QStringList mimeTypes () const VTK_OVERRIDE;
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
  vtkQtTreeModelAdapter(const vtkQtTreeModelAdapter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtTreeModelAdapter&) VTK_DELETE_FUNCTION;
};

#endif
