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
// cells (a neighbor cell is a cell that shares a vertex/edge/face).
// The topological distance of the selected neighborhood (number of times 
// we add neighbor cells).
//
// .SECTION Thanks
// This file has been developed in the frame of CEA's Love visualization software development <br>
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br>
// BP12, F-91297 Arpajon, France. <br>
// Initial implementation by Thierry Carrard and Philippe Pebay

#ifndef VTK_CELL_DISTANCE_SELECTOR_H
#define VTK_CELL_DISTANCE_SELECTOR_H

#include "vtkCommonExecutionModelModule.h" // For export macro
#include <vtkSelectionAlgorithm.h>
#include <vtkSmartPointer.h>

class vtkDataSet;
class vtkSelection;
class vtkAlgorithmOutput;
class vtkDataArray;

// Description:
// Grows a selection, selecting neighbor cells, up to a user defined topological distance
class VTK_EXPORT vtkCellDistanceSelector : public vtkSelectionAlgorithm
{
   public:
      vtkTypeMacro(vtkCellDistanceSelector,vtkSelectionAlgorithm);
      void PrintSelf(ostream& os, vtkIndent indent);

      static vtkCellDistanceSelector* New();

      // Description:
      // Set the data object the initial selection refers to
      void SetDataObjectConnection(vtkAlgorithmOutput *in);

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

      int Distance;
      int IncludeSeed;
      int AddIntermediate;

   private:
      vtkCellDistanceSelector (const vtkCellDistanceSelector &); // Not implemented
      vtkCellDistanceSelector & operator= (const vtkCellDistanceSelector &); // Not implemented
} ;

#endif /* VTK_CELL_DISTANCE_SELECTOR_H */
