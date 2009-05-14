/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLegendModel.h

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

/// \file vtkQtChartLegendModel.h
/// \date February 12, 2008

#ifndef _vtkQtChartLegendModel_h
#define _vtkQtChartLegendModel_h


#include "vtkQtChartExport.h"
#include <QObject>
#include <QPixmap> // Needed for return type
#include <QString> // Needed for return type

class vtkQtChartLegendModelInternal;
class vtkQtPointMarker;
class QPen;


/// \class vtkQtChartLegendModel
/// \brief
///   The vtkQtChartLegendModel class stores the data for a chart legend.
class VTKQTCHART_EXPORT vtkQtChartLegendModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart legend model.
  /// \param parent The parent object.
  vtkQtChartLegendModel(QObject *parent=0);
  virtual ~vtkQtChartLegendModel();

  /// \brief
  ///   Adds an entry to the chart legend.
  /// \param icon The series identifying image.
  /// \param text The series label.
  /// \return
  ///   The id for the inserted entry or zero for failure.
  int addEntry(const QPixmap &icon, const QString &text, bool visible);

  /// \brief
  ///   Inserts an entry into the chart legend.
  /// \param index Where to place the new entry.
  /// \param icon The series identifying image.
  /// \param text The series label.
  /// \return
  ///   The id for the inserted entry or zero for failure.
  int insertEntry(
    int index, const QPixmap &icon, const QString &text, bool visible);

  /// \brief
  ///   Removes an entry from the chart legend.
  /// \param index The index of the entry to remove.
  void removeEntry(int index);

  /// Removes all the entries from the legend.
  void removeAllEntries();

  /// \brief
  ///   Blocks the model modification signals.
  ///
  /// This method should be called before making multiple changes to
  /// the model. It will prevent the view from updating before the
  /// changes are complete. Once all the changes are made, the
  /// \c finishModifyingData method should be called to notify the
  /// view of the changes.
  ///
  /// \sa vtkQtChartLegendModel::finishModifyingData()
  void startModifyingData();

  /// \brief
  ///   Unblocks the model modification signals.
  ///
  /// The \c entriesReset signal is emitted to synchronize the view.
  ///
  /// \sa vtkQtChartLegendModel::startModifyingData()
  void finishModifyingData();

  /// \brief
  ///   Gets the number of entries in the legend.
  /// \return
  ///   The number of entries in the legend.
  int getNumberOfEntries() const;

  /// \brief
  ///   Gets the index for the given id.
  /// \param id The entry identifier.
  /// \return
  ///   The index for the entry that matches the id or -1 if there is
  ///   no matching entry.
  int getIndexForId(unsigned int id) const;

  /// \brief
  ///   Gets the icon for the given index.
  /// \param index The index of the entry.
  /// \return
  ///   The icon for the given index or a null pixmap if the index is
  ///   out of bounds.
  QPixmap getIcon(int index) const;

  /// \brief
  ///   Sets the icon for the given index.
  /// \param index The index of the entry.
  /// \param icon The new series icon.
  void setIcon(int index, const QPixmap &icon);

  /// \brief
  ///   Gets the text for the given index.
  /// \param index The index of the entry.
  /// \return
  ///   The text for the given index or a null string if the index is
  ///   out of bounds.
  QString getText(int index) const;

  /// \brief
  ///   Sets the text for the given index.
  /// \param index The index of the entry.
  /// \param text The new series label.
  void setText(int index, const QString &text);

  /// \brief 
  ///   Sets if the given entry is visible.
  /// \param index The index of the entry.
  /// \param visible The visibility of the entry.
  void setVisible(int index, bool visible);

  /// \brief 
  ///   Returns if the given entry is visible.
  bool getVisible(int index) const;

signals:
  /// \brief
  ///   Emitted when a new entry is added.
  /// \param index Where the entry was added.
  void entryInserted(int index);

  /// \brief
  ///   Emitted before an entry is removed.
  /// \param index The index being removed.
  void removingEntry(int index);

  /// \brief
  ///   Emitted after an entry is removed.
  /// \param index The index being removed.
  void entryRemoved(int index);

  /// Emitted when the legend entries are reset.
  void entriesReset();

  /// \brief
  ///   Emitted when the icon for an entry has changed.
  /// \param index The index of the entry that changed.
  void iconChanged(int index);

  /// \brief
  ///   Emitted when the text for an entry has changed.
  /// \param index The index of the entry that changed.
  void textChanged(int index);

  /// \brief
  ///   Emitted when the visibility of an entry changes.
  void visibilityChanged(int index);

private:
  vtkQtChartLegendModelInternal *Internal; ///< Stores the legend items.
  bool InModify;                        ///< True when blocking signals.
};

#endif
