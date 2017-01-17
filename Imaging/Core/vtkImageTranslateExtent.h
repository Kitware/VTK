/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTranslateExtent.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageTranslateExtent
 * @brief   Changes extent, nothing else.
 *
 * vtkImageTranslateExtent  shift the whole extent, but does not
 * change the data.
*/

#ifndef vtkImageTranslateExtent_h
#define vtkImageTranslateExtent_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGCORE_EXPORT vtkImageTranslateExtent : public vtkImageAlgorithm
{
public:
  static vtkImageTranslateExtent *New();
  vtkTypeMacro(vtkImageTranslateExtent,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Delta to change "WholeExtent". -1 changes 0->10 to -1->9.
   */
  vtkSetVector3Macro(Translation, int);
  vtkGetVector3Macro(Translation, int);
  //@}

protected:
  vtkImageTranslateExtent();
  ~vtkImageTranslateExtent()VTK_OVERRIDE {}

  int Translation[3];

  int RequestUpdateExtent (vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkImageTranslateExtent(const vtkImageTranslateExtent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageTranslateExtent&) VTK_DELETE_FUNCTION;
};

#endif
