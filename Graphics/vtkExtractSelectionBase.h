/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectionBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectionBase - abstract base class for all extract selection
// filters.
// .SECTION Description
// vtkExtractSelectionBase is an abstract base class for all extract selection
// filters. It defines some properties common to all extract selection filters.

#ifndef __vtkExtractSelectionBase_h
#define __vtkExtractSelectionBase_h

#include "vtkDataObjectAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkExtractSelectionBase : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkExtractSelectionBase, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to specify the selection connection (2nd input
  // port)
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
    {
    this->SetInputConnection(1, algOutput);
    }

  // Description:
  // This flag tells the extraction filter not to convert the selected
  // output into an unstructured grid, but instead to produce a vtkInsidedness
  // array and add it to the input dataset. Default value is false(0).
  vtkSetMacro(PreserveTopology, int);
  vtkGetMacro(PreserveTopology, int);
  vtkBooleanMacro(PreserveTopology, int);

//BTX
protected:
  vtkExtractSelectionBase();
  ~vtkExtractSelectionBase();

  // Description:
  // Sets up empty output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int PreserveTopology;
private:
  vtkExtractSelectionBase(const vtkExtractSelectionBase&); // Not implemented.
  void operator=(const vtkExtractSelectionBase&); // Not implemented.
//ETX
};

#endif


