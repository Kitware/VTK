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
/**
 * @class   vtkBoostExtractLargestComponent
 * @brief   Extract the largest connected
 * component of a graph
 *
 *
 * vtkBoostExtractLargestComponent finds the largest connected region of a
 * vtkGraph. For directed graphs, this returns the largest biconnected component.
 * See vtkBoostConnectedComponents for details.
*/

#ifndef vtkBoostExtractLargestComponent_h
#define vtkBoostExtractLargestComponent_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkGraph;

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostExtractLargestComponent : public vtkGraphAlgorithm
{
public:
  vtkTypeMacro(vtkBoostExtractLargestComponent, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct an instance of vtkBoostExtractLargestComponent with
   * InvertSelection set to false.
   */
  static vtkBoostExtractLargestComponent* New();

  //@{
  /**
   * Set the flag to determine if the selection should be inverted.
   */
  vtkSetMacro(InvertSelection, bool);
  vtkGetMacro(InvertSelection, bool);
  //@}

protected:
  vtkBoostExtractLargestComponent();
  ~vtkBoostExtractLargestComponent(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  /**
   * Store the choice of whether or not to invert the selection.
   */
  bool InvertSelection;

private:
  vtkBoostExtractLargestComponent(const vtkBoostExtractLargestComponent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoostExtractLargestComponent&) VTK_DELETE_FUNCTION;
};

#endif
