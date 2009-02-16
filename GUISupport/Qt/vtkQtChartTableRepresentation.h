/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTableRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkQtChartTableRepresentation - Put a vtkTable into a QChartLayer
//
// .SECTION Description
//
// This is a specialization of vtkQtTableDataRepresentation to put the
// data into a QChartLayer.  The user must supply the QChartLayer to be
// used either through SetChartLayer() or via a subclass.

#ifndef __vtkQtChartTableRepresentation_h
#define __vtkQtChartTableRepresentation_h

#include "QVTKWin32Header.h"
#include "vtkQtTableDataRepresentation.h"

#include "vtkQtChartSeriesSelection.h"

class vtkQtAbstractModelAdapter;
class vtkAlgorithmOutput;
class vtkDataArray;
class vtkIdTypeArray;
class vtkIntArray;
class vtkLookupTable;
class vtkQtChartTableRepresentation;
class vtkQtItemView;
class vtkSelection;
class vtkSelectionLink;
class vtkView;

/*
// TODO Move the signal helper to its own class so we don't have to MOC compile
// vtkQtChartTableRepresentation.h (this header)
//BTX
class QVTK_EXPORT vtkQtChartRepresentationSignalHandler : public QObject
{
Q_OBJECT
public:
  void setTarget(vtkQtChartTableRepresentation* t) { this->Target = t; }
public slots:
  void selectedSeriesChanged(const vtkQtChartSeriesSelection&);
  void modelChanged();
private:
  vtkQtChartRepresentation* Target;
};
//ETX
*/

class QVTK_EXPORT vtkQtChartTableRepresentation : public vtkQtTableDataRepresentation
{
public:
  static vtkQtChartTableRepresentation *New();
  vtkTypeRevisionMacro(vtkQtChartTableRepresentation, vtkQtTableDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Sets the input pipeline connection to this representation.
  virtual void SetInputConnection(vtkAlgorithmOutput* conn);

  // Description:
  // Called by the handler when the layer selection changes.
  virtual void QtSelectedSeriesChanged(const vtkQtChartSeriesSelection &list);
  
  // Description:
  // Called by the handler whent the data model changes.
  virtual void QtModelChanged();

  // Description:
  // Update the current selection.
  virtual void Update();

  // Description:
  // Orients the table as being either columns-as-series or
  // rows-as-series oriented.
  void SetColumnsAsSeries(bool);
  vtkGetMacro(ColumnsAsSeries,int);

protected:
  vtkQtChartTableRepresentation();
  ~vtkQtChartTableRepresentation();
  
  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  bool RemoveFromView(vtkView* view);

  // Description:
  // Listens for selection changed events from the chart layer.
  //vtkQtChartTableRepresentationSignalHandler* Handler;

  bool ColumnsAsSeries;

private:

  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX

  vtkQtChartTableRepresentation(const vtkQtChartTableRepresentation&);  // Not implemented.
  void operator=(const vtkQtChartTableRepresentation&);  // Not implemented.
};

#endif
