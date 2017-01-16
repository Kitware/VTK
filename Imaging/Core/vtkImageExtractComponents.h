/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageExtractComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageExtractComponents
 * @brief   Outputs a single component
 *
 * vtkImageExtractComponents takes an input with any number of components
 * and outputs some of them.  It does involve a copy of the data.
 *
 * @sa
 * vtkImageAppendComponents
*/

#ifndef vtkImageExtractComponents_h
#define vtkImageExtractComponents_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageExtractComponents : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageExtractComponents *New();
  vtkTypeMacro(vtkImageExtractComponents,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the components to extract.
   */
  void SetComponents(int c1);
  void SetComponents(int c1, int c2);
  void SetComponents(int c1, int c2, int c3);
  vtkGetVector3Macro(Components,int);
  //@}

  //@{
  /**
   * Get the number of components to extract. This is set implicitly by the
   * SetComponents() method.
   */
  vtkGetMacro(NumberOfComponents,int);
  //@}

protected:
  vtkImageExtractComponents();
  ~vtkImageExtractComponents() VTK_OVERRIDE {}

  int NumberOfComponents;
  int Components[3];

  int RequestInformation (vtkInformation *, vtkInformationVector**,
                                  vtkInformationVector *) VTK_OVERRIDE;

  void ThreadedExecute (vtkImageData *inData, vtkImageData *outData,
                       int ext[6], int id) VTK_OVERRIDE;
private:
  vtkImageExtractComponents(const vtkImageExtractComponents&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageExtractComponents&) VTK_DELETE_FUNCTION;
};

#endif










