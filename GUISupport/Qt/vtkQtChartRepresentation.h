/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartRepresentation.h

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
// .NAME vtkQtChartRepresentation - Put a vtkTable into a QChartLayer
//
// .SECTION Description
//
// This is a specialization of vtkQtTableDataRepresentation to put the
// data into a QChartLayer.  The user must supply the QChartLayer to be
// used either through SetChartLayer() or via a subclass.

#ifndef __vtkQtChartRepresentation_h
#define __vtkQtChartRepresentation_h

#include "QVTKWin32Header.h"
#include "vtkQtTableDataRepresentation.h"

#include <QObject>
#include <QList>
#include "vtkQtChartSeriesSelection.h"

class vtkQtChartSeriesLayer;
class QItemSelection;
class vtkQtAbstractModelAdapter;
class vtkAlgorithmOutput;
class vtkDataArray;
class vtkIdTypeArray;
class vtkIntArray;
class vtkLookupTable;
class vtkQtChartRepresentation;
class vtkQtItemView;
class vtkSelection;
class vtkSelectionLink;
class vtkView;

class QVTK_EXPORT vtkQtChartRepresentationSignalHandler : public QObject
{
Q_OBJECT
public:
  void setTarget(vtkQtChartRepresentation* t) { this->Target = t; }
public slots:
  void selectedSeriesChanged(const vtkQtChartSeriesSelection&);
  void modelChanged();
private:
  vtkQtChartRepresentation* Target;
};

class QVTK_EXPORT vtkQtChartRepresentation : public vtkQtTableDataRepresentation
{
public:
  static vtkQtChartRepresentation *New();
  vtkTypeRevisionMacro(vtkQtChartRepresentation, vtkQtTableDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the underlying chart layer for this representation.
  virtual void SetChartLayer(vtkQtChartSeriesLayer* layer);
  virtual vtkQtChartSeriesLayer* GetChartLayer() { return this->ChartLayer; }

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
  vtkQtChartRepresentation();
  ~vtkQtChartRepresentation();
  
  // Description:
  // Sets the input pipeline connections for this representation.
  virtual void SetupInputConnections();

  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  bool RemoveFromView(vtkView* view);
  
  virtual void CreateSeriesColors();

  // Description:
  // The underlying chart layer.
  vtkQtChartSeriesLayer* ChartLayer;

  // Description:
  // Listens for selection changed events from the chart layer.
  vtkQtChartRepresentationSignalHandler* Handler;

  // Description:
  // A map from chart series id to VTK id.
  vtkIdTypeArray* SeriesToVTKMap;

  // Description:
  // A map from VTK id to chart series id.
  vtkIntArray* VTKToSeriesMap;

  bool ColumnsAsSeries;

private:
  vtkQtChartRepresentation(const vtkQtChartRepresentation&);  // Not implemented.
  void operator=(const vtkQtChartRepresentation&);  // Not implemented.
};

#endif
