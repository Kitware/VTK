/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGridWithImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBlankStructuredGridWithImage
 * @brief   blank a structured grid with an image
 *
 * This filter can be used to set the blanking in a structured grid with
 * an image. The filter takes two inputs: the structured grid to blank,
 * and the image used to set the blanking. Make sure that the dimensions of
 * both the image and the structured grid are identical.
 *
 * Note that the image is interpreted as follows: zero values indicate that
 * the structured grid point is blanked; non-zero values indicate that the
 * structured grid point is visible. The blanking data must be unsigned char.
 *
 * @sa
 * vtkStructuredGrid
*/

#ifndef vtkBlankStructuredGridWithImage_h
#define vtkBlankStructuredGridWithImage_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

class vtkImageData;

class VTKFILTERSGENERAL_EXPORT vtkBlankStructuredGridWithImage : public vtkStructuredGridAlgorithm
{
public:
  static vtkBlankStructuredGridWithImage *New();
  vtkTypeMacro(vtkBlankStructuredGridWithImage,vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set / get the input image used to perform the blanking.
   */
  void SetBlankingInputData(vtkImageData *input);
  vtkImageData *GetBlankingInput();
  //@}

protected:
  vtkBlankStructuredGridWithImage();
  ~vtkBlankStructuredGridWithImage() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkBlankStructuredGridWithImage(const vtkBlankStructuredGridWithImage&) = delete;
  void operator=(const vtkBlankStructuredGridWithImage&) = delete;
};

#endif
