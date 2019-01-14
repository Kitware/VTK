/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCountVertices.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCountVertices
 * @brief   Add a cell data array containing the number of
 * vertices per cell.
 *
 *
 * This filter adds a cell data array containing the number of vertices per
 * cell.
*/

#ifndef vtkCountVertices_h
#define vtkCountVertices_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkCountVertices: public vtkPassInputTypeAlgorithm
{
public:
  static vtkCountVertices* New();
  vtkTypeMacro(vtkCountVertices, vtkPassInputTypeAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  //@{
  /**
   * The name of the new output array containing the vertex counts.
   */
  vtkSetStringMacro(OutputArrayName)
  vtkGetStringMacro(OutputArrayName)
  //@}

protected:
  vtkCountVertices();
  ~vtkCountVertices() override;

  int RequestData(vtkInformation* request, vtkInformationVector **inInfoVec,
                  vtkInformationVector *outInfoVec) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char *OutputArrayName;

private:
  vtkCountVertices(const vtkCountVertices&) = delete;
  void operator=(const vtkCountVertices&) = delete;
};

#endif // vtkCountVertices_h
