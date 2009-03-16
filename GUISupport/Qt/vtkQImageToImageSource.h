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
// .NAME vtkQImageToImageSource - Create image data from a QImage.
// .SECTION Description
// vtkQImageToImageSource produces image data from a QImage.

#ifndef __vtkQImageToImageSource_h
#define __vtkQImageToImageSource_h

#include "vtkImageAlgorithm.h"
#include "QVTKWin32Header.h"

class QImage;

class QVTK_EXPORT vtkQImageToImageSource : public vtkImageAlgorithm
{
public:
  static vtkQImageToImageSource *New();
  vtkTypeRevisionMacro(vtkQImageToImageSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get QImage surface to be used.
  void SetQImage( QImage* image )
      {this->QtImage = image;}
  QImage* GetQImage(){return QtImage;}

protected:
  vtkQImageToImageSource();
  ~vtkQImageToImageSource() {};

  QImage* QtImage;
  int DataExtent[6];

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestInformation ( vtkInformation * vtkNotUsed(request),
                           vtkInformationVector ** vtkNotUsed( inputVector ),
                           vtkInformationVector *outputVector);
private:
  vtkQImageToImageSource(const vtkQImageToImageSource&);  // Not implemented.
  void operator=(const vtkQImageToImageSource&);  // Not implemented.
};


#endif
