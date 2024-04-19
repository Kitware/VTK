// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2007 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef StatsView_H
#define StatsView_H

#include "vtkSmartPointer.h" // Required for smart pointer internal ivars.

#include <QMainWindow>

// Forward Qt class declarations
class Ui_StatsView;

VTK_ABI_NAMESPACE_BEGIN
// Forward VTK class declarations
class vtkRowQueryToTable;
class vtkQtTableView;
VTK_ABI_NAMESPACE_END

class StatsView : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  StatsView();
  ~StatsView() override;

public Q_SLOTS:

  virtual void slotOpenSQLiteDB();

protected:
protected Q_SLOTS:

private:
  // Methods
  void SetupSelectionLink();

  // Members
  vtkSmartPointer<vtkRowQueryToTable> RowQueryToTable;
  vtkSmartPointer<vtkQtTableView> TableView1;
  vtkSmartPointer<vtkQtTableView> TableView2;
  vtkSmartPointer<vtkQtTableView> TableView3;
  vtkSmartPointer<vtkQtTableView> TableView4;

  // Designer form
  Ui_StatsView* ui;
};

#endif // StatsView_H
