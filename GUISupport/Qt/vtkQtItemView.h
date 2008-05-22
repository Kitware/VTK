/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtItemView.h

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
// .NAME vtkQtItemView - Superclass for Qt widget-based views.
//
// .SECTION Description
// This superclass provides all the plumbing to integrate a Qt widget into
// the VTK view framework, including reporting selection changes and detecting
// selection changes from linked views.
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtItemView_h
#define __vtkQtItemView_h

#include "QVTKWin32Header.h"
#include "vtkView.h"

#include <QAbstractItemView>
#include "vtkQtAbstractModelAdapter.h"

// Description:
// A helper object which transfers the selection changed event
// to the view.
class vtkQtItemView;
class QItemSelectionModel;

class QVTK_EXPORT vtkQtSignalHandler : public QObject
{
Q_OBJECT
public:
  void setTarget(vtkQtItemView* t) { this->Target = t; }
public slots:
  void slotSelectionChanged(const QItemSelection&, const QItemSelection&);
private:
  vtkQtItemView* Target;
};

class QVTK_EXPORT vtkQtItemView : public vtkView
{
public:
  static vtkQtItemView *New();
  vtkTypeRevisionMacro(vtkQtItemView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set and get the underlying Qt view.
  virtual void SetItemView(QAbstractItemView*);
  QAbstractItemView* GetItemView();
  
  // Description:
  // Set and get the underlying Qt model adapter.
  virtual void SetItemModelAdapter(vtkQtAbstractModelAdapter* qma);
  vtkQtAbstractModelAdapter* GetItemModelAdapter();
  
  // Description:
  // Updates the view.
  virtual void Update();
  
protected:
  vtkQtItemView();
  ~vtkQtItemView();
  
  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Description:
  // Connects the algorithm output to the internal pipeline.
  // This view only supports a single representation.
  virtual void AddInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Called when the QItemView selection changes.
  void QtSelectionChanged(const QItemSelection&, const QItemSelection&);

  // Description:
  // Returns the selection model of the item view.
  // If no item view is defined (in the case of the vtkQtCategoryView), 
  // a pointer to an internal
  QItemSelectionModel* GetSelectionModel();
  
private:
  vtkQtItemView(const vtkQtItemView&);  // Not implemented.
  void operator=(const vtkQtItemView&);  // Not implemented.
  
  // Description:
  // Pointer to the item view
  QAbstractItemView *ItemViewPtr;

  // Description:  
  // Pointer to the model adapter
  vtkQtAbstractModelAdapter *ModelAdapterPtr;

  // Description: 
  // Pointer to the selection model for the item model
  QItemSelectionModel *SelectionModel;
  
  // Description:
  // Just a convenience function for making sure
  // that the view and model pointers are valid
  int CheckViewAndModelError();
  
  // Description:
  // Qt signal handling
  friend class vtkQtSignalHandler;
  vtkQtSignalHandler SignalHandler;
  
  // Description:
  // We need to keep track of whether were
  // in selection mode
  bool Selecting;
  
  // Description:
  // Does the selection consist of indices or values
  bool UseValueSelection;
  
  // Description:
  // If the selection is a value selection what array is used
  char *ValueSelectionArrayName;

  bool IOwnSelectionModel;
  
};

#endif
