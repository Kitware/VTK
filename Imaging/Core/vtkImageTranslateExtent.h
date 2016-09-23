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
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Delta to change "WholeExtent". -1 changes 0->10 to -1->9.
   */
  vtkSetVector3Macro(Translation, int);
  vtkGetVector3Macro(Translation, int);
  //@}

protected:
  vtkImageTranslateExtent();
  ~vtkImageTranslateExtent() {}

  int Translation[3];

  virtual int RequestUpdateExtent (vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation (vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkImageTranslateExtent(const vtkImageTranslateExtent&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageTranslateExtent&) VTK_DELETE_FUNCTION;
};

#endif
