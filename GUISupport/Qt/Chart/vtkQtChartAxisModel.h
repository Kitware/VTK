/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartAxisModel.h

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

/// \file vtkQtChartAxisModel.h
/// \date 2/5/2008

#ifndef _vtkQtChartAxisModel_h
#define _vtkQtChartAxisModel_h


#include "vtkQtChartExport.h"
#include <QObject>

class vtkQtChartAxisModelInternal;
class QVariant;


/// \class vtkQtChartAxisModel
/// \brief
///   The vtkQtChartAxisModel class stores the labels for a chart axis.
class VTKQTCHART_EXPORT vtkQtChartAxisModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart axis model.
  /// \param parent The parent object.
  vtkQtChartAxisModel(QObject *parent=0);
  virtual ~vtkQtChartAxisModel();

  /// \brief
  ///   Adds a label to the chart axis.
  /// \param label The label to add.
  void addLabel(const QVariant &label);

  /// \brief
  ///   Adds a label to the chart axis.
  /// \param index Where to insert the label.
  /// \param label The label to add.
  void insertLabel(int index, const QVariant &label);

  /// \brief
  ///   Removes a label from the chart axis.
  /// \param index The index of the label to remove.
  void removeLabel(int index);

  /// Removes all the labels from the chart axis.
  void removeAllLabels();

  /// \brief
  ///   Blocks the model modification signals.
  ///
  /// This method should be called before making multiple changes to
  /// the model. It will prevent the view from updating before the
  /// changes are complete. Once all the changes are made, the
  /// \c finishModifyingData method should be called to notify the
  /// view of the changes.
  ///
  /// \sa vtkQtChartAxisModel::finishModifyingData()
  void startModifyingData();

  /// \brief
  ///   Unblocks the model modification signals.
  ///
  /// The \c labelsReset signal is emitted to synchronize the view.
  ///
  /// \sa vtkQtChartAxisModel::startModifyingData()
  void finishModifyingData();

  /// \brief
  ///   Gets the number of labels in the chart axis.
  /// \return
  ///   The number of labels in the chart axis.
  int getNumberOfLabels() const;

  /// \brief
  ///   Gets a specified chart axis label.
  /// \param index Which chart axis to get.
  /// \param label Used to return the label.
  void getLabel(int index, QVariant &label) const;

  /// \brief
  ///   Gets the index of the given label.
  /// \param label The label value to find.
  /// \return
  ///   The index of the label or -1 if not found.
  int getLabelIndex(const QVariant &label) const;

signals:
  /// \brief
  ///   Emitted when a new label is added.
  /// \param index Where the label was added.
  void labelInserted(int index);

  /// \brief
  ///   Emitted before a label is removed.
  /// \param index The index being removed.
  void removingLabel(int index);

  /// \brief
  ///   Emitted after a label is removed.
  /// \param index The index being removed.
  void labelRemoved(int index);

  /// Emitted when the axis labels are reset.
  void labelsReset();

private:
  vtkQtChartAxisModelInternal *Internal; ///< Stores the list of labels.
  bool InModify;                         ///< True when blocking signals.
};

#endif
