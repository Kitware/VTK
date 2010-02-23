/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractPolyhedralMesh - extract 3D cells as polyhedron
// .SECTION Description
// vtkExtractPolyhedralMesh extracts from its input dataset all 3D cells and
// transforms them to a polyhedral cell type (VTK_POLYHEDRON). Cells of other
// topological dimension are passed through (if desired). The output type of 
// this filter is vtkUnstructuredGrid, with all 3D cells of polyhedral type.

// .SECTION See Also
// vtkPolyhedron vtkUnstructuredGrid

#ifndef __vtkExtractPolyhedralMesh_h
#define __vtkExtractPolyhedralMesh_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;

class VTK_GRAPHICS_EXPORT vtkExtractPolyhedralMesh : public vtkUnstructuredGridAlgorithm
{
public:
  // Description:
  // Construct object with ExtractInside turned on.
  static vtkExtractPolyhedralMesh *New();

  // Description:
  // Standard instanitable class methods.
  vtkTypeRevisionMacro(vtkExtractPolyhedralMesh,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Boolean controls whether to pass cells of topological dimension two or
  // less to the output. By default this variable is enabled.
  vtkSetMacro(ExtractNon3DCells,int);
  vtkGetMacro(ExtractNon3DCells,int);
  vtkBooleanMacro(ExtractNon3DCells,int);

protected:
  vtkExtractPolyhedralMesh();
  ~vtkExtractPolyhedralMesh();

  // Usual data generation method and pipeline methods
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Instance variables
  int ExtractNon3DCells;
  
private:
  vtkExtractPolyhedralMesh(const vtkExtractPolyhedralMesh&);  // Not implemented.
  void operator=(const vtkExtractPolyhedralMesh&);  // Not implemented.
};

#endif


