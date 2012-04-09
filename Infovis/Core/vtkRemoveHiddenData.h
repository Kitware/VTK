/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveHiddenData.h

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
// .NAME vtkRemoveHiddenData - Removes the rows/edges/vertices of
// input data flagged by ann.
//
// .SECTION Description
// Output only those rows/vertices/edges of the input vtkDataObject that
// are visible, as defined by the vtkAnnotation::HIDE() flag of the input
// vtkAnnotationLayers.
// Inputs:
//    Port 0 - vtkDataObject
//    Port 1 - vtkAnnotationLayers (optional)
//

#ifndef __vtkRemoveHiddenData_h
#define __vtkRemoveHiddenData_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h"  // For Smartpointer

class vtkExtractSelectedGraph;
class vtkExtractSelectedRows;

class VTKINFOVISCORE_EXPORT vtkRemoveHiddenData : public vtkPassInputTypeAlgorithm
{
public:
  static vtkRemoveHiddenData *New();
  vtkTypeMacro(vtkRemoveHiddenData, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkRemoveHiddenData();
  ~vtkRemoveHiddenData();

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkRemoveHiddenData(const vtkRemoveHiddenData&);  // Not implemented.
  void operator=(const vtkRemoveHiddenData&);  // Not implemented.

//BTX
  vtkSmartPointer<vtkExtractSelectedGraph> ExtractGraph;
  vtkSmartPointer<vtkExtractSelectedRows> ExtractTable;
//ETX
};

#endif
