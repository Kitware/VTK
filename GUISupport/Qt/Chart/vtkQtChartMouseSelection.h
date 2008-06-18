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

class vtkQtChartContentsSpace;
class vtkQtChartMouseBox;
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

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the chart mouse box object.
  /// \return
  ///   A pointer to the chart mouse box object.
  vtkQtChartMouseBox *getMouseBox() const {return this->MouseBox;}

  /// \brief
  ///   Sets the chart mouse box object.
  ///
  /// The mouse box can be used to create a selection box.
  ///
  /// \param box The chart mouse box object to use.
  virtual void setMouseBox(vtkQtChartMouseBox *box) {this->MouseBox = box;}

  virtual bool isCombinable() const {return false;}
  //@}

  /// \name Configuration Methods
  //@{
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

  void addHandler(vtkQtChartMouseSelectionHandler *handler);

  void insertHandler(int index, vtkQtChartMouseSelectionHandler *handler);

  void removeHandler(vtkQtChartMouseSelectionHandler *handler);
  //@}

  /// \name Interaction Methods
  //@{
  virtual bool mousePressEvent(QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual bool mouseMoveEvent(QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual bool mouseReleaseEvent(QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
  virtual bool mouseDoubleClickEvent(QMouseEvent *e,
      vtkQtChartContentsSpace *contents);
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
  vtkQtChartMouseBox *MouseBox; ///< Stores the mouse box.
};

#endif
