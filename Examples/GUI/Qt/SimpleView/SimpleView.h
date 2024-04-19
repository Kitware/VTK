// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#ifndef SimpleView_H
#define SimpleView_H

#include "vtkSmartPointer.h" // Required for smart pointer internal ivars.
#include <QMainWindow>

// Forward Qt class declarations
class Ui_SimpleView;

VTK_ABI_NAMESPACE_BEGIN
// Forward VTK class declarations
class vtkQtTableView;
VTK_ABI_NAMESPACE_END

class SimpleView : public QMainWindow
{
  Q_OBJECT

public:
  // Constructor/Destructor
  SimpleView();
  ~SimpleView() override;

public Q_SLOTS:

  virtual void slotOpenFile();
  virtual void slotExit();

protected:
protected Q_SLOTS:

private:
  vtkSmartPointer<vtkQtTableView> TableView;

  // Designer form
  Ui_SimpleView* ui;
};

#endif // SimpleView_H
