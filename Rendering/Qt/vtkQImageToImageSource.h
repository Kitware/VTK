/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkQImageToImageSource.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQImageToImageSource
 * @brief   Create image data from a QImage.
 *
 * vtkQImageToImageSource produces image data from a QImage.
*/

#ifndef vtkQImageToImageSource_h
#define vtkQImageToImageSource_h

#include "vtkRenderingQtModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class QImage;

class VTKRENDERINGQT_EXPORT vtkQImageToImageSource : public vtkImageAlgorithm
{
public:
  static vtkQImageToImageSource *New();
  vtkTypeMacro(vtkQImageToImageSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Set/Get QImage surface to be used.
   */
  void SetQImage( QImage* image )
      {this->QtImage = image; this->Modified();}
  const QImage* GetQImage(){return QtImage;}

protected:
  vtkQImageToImageSource();
  ~vtkQImageToImageSource() VTK_OVERRIDE {}

  const QImage* QtImage;
  int DataExtent[6];

  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int RequestInformation ( vtkInformation * vtkNotUsed(request),
                           vtkInformationVector ** vtkNotUsed( inputVector ),
                           vtkInformationVector *outputVector) VTK_OVERRIDE;
private:
  vtkQImageToImageSource(const vtkQImageToImageSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQImageToImageSource&) VTK_DELETE_FUNCTION;
};


#endif
