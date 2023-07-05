// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQtDebugLeaksView
 * @brief   view class to display contents of vtkQtDebugLeaksModel
 *
 *
 * A widget the displays all vtkObjectBase derived objects that are alive in
 * memory.  The widget is designed to be a debugging tool that is instantiated
 * at program startup and displayed as a top level widget.  Simply create the
 * widget and call show().
 */

#ifndef vtkQtDebugLeaksView_h
#define vtkQtDebugLeaksView_h

#include "vtkGUISupportQtModule.h" // For export macro
#include <QWidget>

class QModelIndex;

VTK_ABI_NAMESPACE_BEGIN
class vtkObjectBase;
class vtkQtDebugLeaksModel;

class VTKGUISUPPORTQT_EXPORT vtkQtDebugLeaksView : public QWidget
{
  Q_OBJECT

public:
  vtkQtDebugLeaksView(QWidget* p = nullptr);
  ~vtkQtDebugLeaksView() override;

  vtkQtDebugLeaksModel* model();

  /**
   * Returns whether or not the regexp filter is enabled
   */
  bool filterEnabled() const;

  /**
   * Enabled or disables the regexp filter
   */
  void setFilterEnabled(bool value);

  /**
   * Returns the regexp filter line edit's current text
   */
  QString filterText() const;

  /**
   * Sets the current text in the regexp filter line edit
   */
  void setFilterText(const QString& text);

protected:
  virtual void onObjectDoubleClicked(vtkObjectBase* object);
  virtual void onClassNameDoubleClicked(const QString& className);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  void onCurrentRowChanged(const QModelIndex& current);
  void onRowDoubleClicked(const QModelIndex&);
  void onFilterTextChanged(const QString& filterText);
  void onFilterToggled();
  void onFilterHelp();

private:
  class qInternal;
  qInternal* Internal;

  Q_DISABLE_COPY(vtkQtDebugLeaksView);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkQtDebugLeaksView.h
