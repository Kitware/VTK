/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetTessellator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataSetTessellator - tessellates generic, higher-order datasets into linear cells
// .SECTION Description

// vtkGenericDataSetTessellator is a filter that subdivides a
// vtkGenericDataSet into linear elements (i.e., linear VTK
// cells). Tetrahedras are produced from 3D cells; triangles from 2D cells;
// and lines from 1D cells. The subdivision process depends on the cell
// tessellator associated with the input generic dataset, and its associated
// error metric. (These can be specified by the user if necessary.)
//
// This filter is typically used to convert a higher-order, complex dataset
// represented by a vtkGenericDataSet into a conventional vtkDataSet that can
// be operated on by linear VTK graphics filters (end of pipeline for
// rendering).

// .SECTION See Also
// vtkGenericCellTessellator vtkGenericSubdivisionErrorMetric

#ifndef __vtkGenericDataSetTessellator_h
#define __vtkGenericDataSetTessellator_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkPointData;
class vtkIncrementalPointLocator;

class VTK_GENERIC_FILTERING_EXPORT vtkGenericDataSetTessellator : public vtkUnstructuredGridAlgorithm
{
public:
  // Description:
  // Standard VTK methods.
  static vtkGenericDataSetTessellator *New();
  vtkTypeMacro(vtkGenericDataSetTessellator,
                       vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Turn on/off generation of a cell centered attribute with ids of the
  // original cells (as an input cell is tessellated into several linear
  // cells).
  // The name of the data array is "OriginalIds". It is true by default.
  vtkSetMacro(KeepCellIds, int);
  vtkGetMacro(KeepCellIds, int);
  vtkBooleanMacro(KeepCellIds, int);
  
  
  // Description:
  // Turn on/off merging of coincident points. Note that is merging is
  // on, points with different point attributes (e.g., normals) are merged,
  // which may cause rendering artifacts.
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);

  // Description:
  // Set / get a spatial locator for merging points. By
  // default an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();
  
  // Description:
  // Return the MTime also considering the locator.
  unsigned long GetMTime();
  
protected:
  vtkGenericDataSetTessellator();
  ~vtkGenericDataSetTessellator();
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  int FillInputPortInformation(int, vtkInformation*);
  
  // See Set/Get KeepCellIds() for explanations.
  int KeepCellIds;
  
  // Used internal by vtkGenericAdaptorCell::Tessellate()
  vtkPointData *InternalPD;
  
  int Merging;
  vtkIncrementalPointLocator *Locator;
  
private:
  vtkGenericDataSetTessellator(const vtkGenericDataSetTessellator&);  // Not implemented.
  void operator=(const vtkGenericDataSetTessellator&);  // Not implemented.
};

#endif
