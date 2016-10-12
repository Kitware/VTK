/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtDebugLeaksView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
class vtkObjectBase;
class vtkQtDebugLeaksModel;

class VTKGUISUPPORTQT_EXPORT vtkQtDebugLeaksView : public QWidget
{
  Q_OBJECT

public:

  vtkQtDebugLeaksView(QWidget *p=0);
  virtual ~vtkQtDebugLeaksView();

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

protected slots:

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

#endif
