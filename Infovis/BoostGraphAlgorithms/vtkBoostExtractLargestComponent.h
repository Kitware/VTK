/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostExtractLargestComponent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBoostExtractLargestComponent - Extract the largest connected
// component of a graph
//
// .SECTION Description
// vtkBoostExtractLargestComponent finds the largest connected region of a
// vtkGraph. For directed graphs, this returns the largest biconnected component.
// See vtkBoostConnectedComponents for details.

#ifndef __vtkBoostExtractLargestComponent_h
#define __vtkBoostExtractLargestComponent_h

#include "vtkGraphAlgorithm.h"

class vtkGraph;

class VTK_INFOVIS_EXPORT vtkBoostExtractLargestComponent : public vtkGraphAlgorithm
{
public:
  vtkTypeMacro(vtkBoostExtractLargestComponent, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct an instance of vtkBoostExtractLargestComponent with
  // InvertSelection set to false.
  static vtkBoostExtractLargestComponent* New();

  // Description:
  // Set the flag to determine if the selection should be inverted.
  vtkSetMacro(InvertSelection, bool);
  vtkGetMacro(InvertSelection, bool);

protected:
  vtkBoostExtractLargestComponent();
  ~vtkBoostExtractLargestComponent(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Store the choice of whether or not to invert the selection.
  bool InvertSelection;

private:
  vtkBoostExtractLargestComponent(const vtkBoostExtractLargestComponent&);  // Not implemented.
  void operator=(const vtkBoostExtractLargestComponent&);  // Not implemented.
};

#endif
