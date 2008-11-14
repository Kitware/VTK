/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartMouseSelection.h

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

/// \file vtkQtChartMouseSelection.h
/// \date March 11, 2008

#ifndef _vtkQtChartMouseSelection_h
#define _vtkQtChartMouseSelection_h


#include "vtkQtChartExport.h"
#include "vtkQtChartMouseFunction.h"

class vtkQtChartArea;
class vtkQtChartMouseSelectionHandler;
class vtkQtChartMouseSelectionInternal;
class QMouseEvent;
class QString;
class QStringList;


/// \class vtkQtChartMouseSelection
/// \brief
///   The vtkQtChartMouseSelection class is used to select chart elements
///   based on the current selection mode.
class VTKQTCHART_EXPORT vtkQtChartMouseSelection :
    public vtkQtChartMouseFunction
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a mouse selection object.
  /// \param parent The parent object.
  vtkQtChartMouseSelection(QObject *parent=0);
  virtual ~vtkQtChartMouseSelection();

  /// \name Configuration Methods
  //@{
  virtual bool isCombinable() const {return false;}

  /// \brief
  ///   Gets the name of current selection mode.
  /// \return
  ///   The name of the current selection mode.
  const QString &getSelectionMode() const;

  /// \brief
  ///   Gets the list of selection modes.
  /// \return
  ///   The list of selection modes.
  const QStringList &getModeList() const;

  /// \brief
  ///   Adds a selection handler to the list.
  ///
  /// The selection mode list is rebuilt when a new handler is added.
  ///
  /// \param handler The new selection handler.
  void addHandler(vtkQtChartMouseSelectionHandler *handler);

  /// \brief
  ///   Inserts a selection handler into the list.
  /// \param index Where to insert the handler.
  /// \param handler The new selection handler.
  void insertHandler(int index, vtkQtChartMouseSelectionHandler *handler);

  /// \brief
  ///   Removes the given selection handler from the list.
  /// \param handler The selection handler to remove.
  void removeHandler(vtkQtChartMouseSelectionHandler *handler);
  //@}

  /// \name Interaction Methods
  //@{
  virtual bool mousePressEvent(QMouseEvent *e,  vtkQtChartArea *chart);
  virtual bool mouseMoveEvent(QMouseEvent *e,  vtkQtChartArea *chart);
  virtual bool mouseReleaseEvent(QMouseEvent *e,  vtkQtChartArea *chart);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,  vtkQtChartArea *chart);
  //@}

public slots:
  /// \brief
  ///   Sets the current selection mode.
  /// \param mode The name of the new selection mode.
  void setSelectionMode(const QString &mode);

signals:
  /// Emitted when the list of available modes changes.
  void modeListChanged();

  /// \brief
  ///   Emitted when the selection mode changes.
  /// \param mode The name of the new selection mode.
  void selectionModeChanged(const QString &mode);

private:
  /// Stores the mode data and selection handlers.
  vtkQtChartMouseSelectionInternal *Internal;

private:
  vtkQtChartMouseSelection(const vtkQtChartMouseSelection &);
  vtkQtChartMouseSelection &operator=(const vtkQtChartMouseSelection &);
};

#endif
