/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtAnnotationLayersModelAdapter.h

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
 * @class   vtkQtAnnotationLayersModelAdapter
 * @brief   Adapts annotations to a Qt item model.
 *
 *
 * vtkQtAnnotationLayersModelAdapter is a QAbstractItemModel with a
 *    vtkAnnotationLayers as its underlying data model.
 *
 * @sa
 * vtkQtAbstractModelAdapter vtkQtTableModelAdapter
*/

#ifndef vtkQtAnnotationLayersModelAdapter_h
#define vtkQtAnnotationLayersModelAdapter_h

#include "vtkConfigure.h"
#include "vtkGUISupportQtModule.h" // For export macro
#include "vtkQtAbstractModelAdapter.h"

class vtkAnnotationLayers;
class vtkSelection;

class VTKGUISUPPORTQT_EXPORT vtkQtAnnotationLayersModelAdapter : public vtkQtAbstractModelAdapter
{
  Q_OBJECT

public:
  vtkQtAnnotationLayersModelAdapter(QObject *parent = nullptr);
  vtkQtAnnotationLayersModelAdapter(vtkAnnotationLayers* ann, QObject *parent = nullptr);
  ~vtkQtAnnotationLayersModelAdapter() override;

  //@{
  /**
   * Set/Get the VTK data object as input to this adapter
   */
  void SetVTKDataObject(vtkDataObject *data) override;
  vtkDataObject* GetVTKDataObject() const override;
  //@}

  //@{
  /**
   * Selection conversion from VTK land to Qt land
   */
  virtual vtkAnnotationLayers* QModelIndexListToVTKAnnotationLayers(
    const QModelIndexList qmil) const;
  virtual QItemSelection VTKAnnotationLayersToQItemSelection(
    vtkAnnotationLayers *vtkann) const;
  vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const override;
  QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const override;
  //@}

  void SetKeyColumnName(const char* name) override;
  void SetColorColumnName(const char* name) override;

  //@{
  /**
   * Set up the model based on the current table.
   */
  void setAnnotationLayers(vtkAnnotationLayers* annotations);
  vtkAnnotationLayers* annotationLayers() const { return this->Annotations; }
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
/*
  Qt::DropActions supportedDropActions() const;
  Qt::DropActions supportedDragActions() const;
  bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
  bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());
  virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) ;
  virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
  virtual QStringList mimeTypes () const ;
*/
private:
  //@}

  bool noAnnotationsCheck() const;

  vtkAnnotationLayers*   Annotations;

  vtkQtAnnotationLayersModelAdapter(const vtkQtAnnotationLayersModelAdapter &) = delete;
  void operator=(const vtkQtAnnotationLayersModelAdapter&) = delete;
};

#endif
