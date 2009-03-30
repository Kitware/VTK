/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelectionModel.h

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

/// \file vtkQtChartSeriesSelectionModel.h
/// \date March 14, 2008

#ifndef _vtkQtChartSeriesSelectionModel_h
#define _vtkQtChartSeriesSelectionModel_h

#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartSeriesModel;
class vtkQtChartSeriesSelection;


/// \class vtkQtChartSeriesSelectionModel
/// \brief
///   The vtkQtChartSeriesSelectionModel class ties a series selection
///   to a series model.
class VTKQTCHART_EXPORT vtkQtChartSeriesSelectionModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a series selection model.
  /// \param parent The parent object.
  vtkQtChartSeriesSelectionModel(QObject *parent=0);
  virtual ~vtkQtChartSeriesSelectionModel();

  /// \brief
  ///   Gets the series model associated with the selection model.
  /// \return
  ///   The series model associated with the selection model.
  vtkQtChartSeriesModel *getModel() const {return this->Model;}

  /// \brief
  ///   Sets the series model associated with the selection model.
  /// \param model The new series model.
  void setModel(vtkQtChartSeriesModel *model);

  /// \brief
  ///   Gets whether or not the selection model is in an interactive
  ///   change.
  /// \return
  ///   Trure if the selection model is in an interactive change.
  /// \sa vtkQtChartSeriesSelectionModel::beginInteractiveChange()
  bool isInInteractiveChange() const {return this->InInteractMode;}

  /// \brief
  ///   Called to begin an interactive selection change.
  ///
  /// Interactive selection changes such as a selection box can send
  /// a lot of change signals as the user drags the mouse around. The
  /// chart needs to update the selection based on those signals in
  /// order for the user to see the changes. If an expensive process
  /// is attached to the selection change signal, this can cause a
  /// visible slow-down in the application. This method allows the
  /// selection to keep the chart painter up to date while allowing
  /// the expensive process to delay execution.
  ///
  /// The interactive controller should call this method before
  /// starting a change such as with a selection box. It should call
  /// the \c endInteractiveChange method when the interaction is done.
  /// The expensive process should listen to the \c selectionChanged
  /// and \c interactionFinished signals. The \c interactionFinished
  /// is emitted at the end of the interactive change. In order to
  /// keep track of non-interactive changes, the \c selectionChanged
  /// signal must be monitored. This signal will be emitted for every
  /// selection change. Therefore, the listening code should check
  /// to see if the model is in an interactive change before executing
  /// an expensive process.
  void beginInteractiveChange();

  /// \brief
  ///   Called to end an interactive selection change.
  /// \sa vtkQtChartSeriesSelectionModel::beginInteractiveChange()
  void endInteractiveChange();

  /// \brief
  ///   Gets whether or not the selection is empty.
  /// \return
  ///   True if the selection is empty.
  bool isSelectionEmpty() const;

  /// \brief
  ///   Gets the current selection.
  /// \return
  ///   A reference to the current selection.
  const vtkQtChartSeriesSelection &getSelection() const;

  /// Selects all the model series.
  void selectAllSeries();

  /// Selects all the model points.
  void selectAllPoints();

  /// Clears the selection.
  void selectNone();

  /// Inverts the selection.
  void selectInverse();

  /// \brief
  ///   Sets the current selection.
  /// \param selection The new selection.
  void setSelection(const vtkQtChartSeriesSelection &selection);

  /// \brief
  ///   Adds to the current selection.
  /// \param selection The selection to add.
  void addSelection(const vtkQtChartSeriesSelection &selection);

  /// \brief
  ///   Subtracts from the current selection.
  /// \param selection The selection to subtract.
  void subtractSelection(const vtkQtChartSeriesSelection &selection);

  /// \brief
  ///   Performs an exclusive or between the specified selection and
  ///   the current selection.
  /// \param selection The selection to modify the current selection.
  void xorSelection(const vtkQtChartSeriesSelection &selection);

signals:
  /// \brief
  ///   Emitted when the selection changes.
  /// \param list The list of selected series/points.
  void selectionChanged(const vtkQtChartSeriesSelection &list);

  /// \brief
  ///   Emitted when an interactive selection change is finished.
  ///
  /// This signal can be used to delay expensive processes until
  /// after the selection change is complete.
  ///
  /// \sa vtkQtChartSeriesSelectionModel::beginInteractiveChange()
  void interactionFinished();

public slots:
  /// \name Model Modification Handlers
  //@{
  /// \brief
  ///   Begins the model reset process.
  ///
  /// The selection model is cleared. The selection changed signal
  /// is emitted when the model finishes resetting.
  void beginModelReset();

  /// Ends the model reset process.
  void endModelReset();

  /// \brief
  ///   Begins the series insertion process.
  ///
  /// The indexes for the series in the selection model are updated
  /// to reflect the changes. The selection changed signal is emitted
  /// when the insertion is completed.
  ///
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void beginInsertSeries(int first, int last);

  /// \brief
  ///   Ends the series insertion process.
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void endInsertSeries(int first, int last);

  /// \brief
  ///   Begins the series removal process.
  ///
  /// Any reference to the removed range is deleted from the selection
  /// model. The selection indexes are updated to reflect the change.
  /// The selection changed signal is emitted when the removal is
  /// completed.
  ///
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void beginRemoveSeries(int first, int last);

  /// \brief
  ///   Ends the series removal process.
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void endRemoveSeries(int first, int last);
  //@}

private:
  /// Trims the selection to ranges valid for the model.
  void limitSelection();

private:
  /// Stores the series selection.
  vtkQtChartSeriesSelection *Selection;
  vtkQtChartSeriesModel *Model; ///< A pointer to the model.
  bool PendingSignal;           ///< Used during model changes.
  bool InInteractMode;          ///< True if in interact mode.
};

#endif
