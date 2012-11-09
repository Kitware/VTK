/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkCellDistanceSelector,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellDistanceSelector - select neighbor cells up to a distance
//
// .SECTION Description
// This filter grows an input selection by iteratively selecting neighbor
// cells (a neighbor cell is a cell that shares a vertex/edge/face), up to
// a given topological distance to the selected neighborhood (number of times
// we add neighbor cells).
// This filter takes a vtkSelection and a vtkCompositeDataSet as inputs.
// It outputs a vtkSelection identifying all the selected cells.
//
// .SECTION Thanks
// This file has been initially developed in the frame of CEA's Love visualization software development <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Modified and integrated into VTK, Kitware SAS 2012
// Implementation by Thierry Carrard and Philippe Pebay

#ifndef VTK_CELL_DISTANCE_SELECTOR_H
#define VTK_CELL_DISTANCE_SELECTOR_H

#include "vtkFiltersSelectionModule.h" // For export macro
#include <vtkSelectionAlgorithm.h>
#include <vtkSmartPointer.h> // For smart pointers

class vtkDataSet;
class vtkSelection;
class vtkAlgorithmOutput;
class vtkDataArray;

// Description:
// Grows a selection, selecting neighbor cells, up to a user defined topological distance
class VTKFILTERSSELECTION_EXPORT vtkCellDistanceSelector : public vtkSelectionAlgorithm
{
 public:
  vtkTypeMacro(vtkCellDistanceSelector,vtkSelectionAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  static vtkCellDistanceSelector* New();

  // Description:
  // enumeration values to specify input port types
  enum InputPorts
    {
    INPUT_MESH      = 0,  //!< Port 0 is for input mesh
    INPUT_SELECTION = 1   //!< Port 1 is for input selection
    };

  // Description:
  // A convenience method to set the data object input connection to the producer output
  void SetInputMeshConnection( vtkAlgorithmOutput* in )
  { this->SetInputConnection( INPUT_MESH, in ); }

  // Description:
  // A convenience method to set the input data object
  void SetInputMesh( vtkDataObject* obj )
  { this->SetInputData( INPUT_MESH, obj ); }

  // Description:
  // A convenience method to set the selection input connection to the producer output
  void SetInputSelectionConnection( vtkAlgorithmOutput* in )
  { this->SetInputConnection( INPUT_SELECTION, in ); }

  // Description:
  // A convenience method to set the input selection
  void SetInputSelection( vtkSelection* obj )
  { this->SetInputData( INPUT_SELECTION, obj ); }

  // Description:
  // Tells how far (in term of topological distance) away from seed cells to expand the selection
  vtkSetMacro(Distance,int);
  vtkGetMacro(Distance,int);

  // Description:
  // If set, seed cells passed with SetSeedCells will be included in the final selection
  vtkSetMacro(IncludeSeed,int);
  vtkGetMacro(IncludeSeed,int);
  vtkBooleanMacro(IncludeSeed,int);

  // Description:
  // If set, intermediate cells (between seed cells and the selection boundary) will be included in the final selection
  vtkSetMacro(AddIntermediate,int);
  vtkGetMacro(AddIntermediate,int);
  vtkBooleanMacro(AddIntermediate,int);

 protected:
  vtkCellDistanceSelector ();
  virtual ~vtkCellDistanceSelector ();

  void AddSelectionNode(vtkSelection* output, vtkSmartPointer<vtkDataArray> outIndices, int partNumber, int d);

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  // Description:
  // Tological radius from seed cells to be used to select cells
  // Default: 1
  int Distance;

  // Description:
  // Decide whether seed cells are included in selection
  // Default: 1
  int IncludeSeed;

  // Description:
  // Decide whether at distance between 1 and Distance-1 are included in selection
  // Default: 1
  int AddIntermediate;

 private:
  vtkCellDistanceSelector(const vtkCellDistanceSelector &); // Not implemented
  void operator= (const vtkCellDistanceSelector &); // Not implemented
};

#endif /* VTK_CELL_DISTANCE_SELECTOR_H */
