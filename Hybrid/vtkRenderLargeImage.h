/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderLargeImage.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderLargeImage - Use tiling to generate a large rendering
// .SECTION Description
// vtkRenderLargeImage provides methods needed to read a region from a file.


#ifndef __vtkRenderLargeImage_h
#define __vtkRenderLargeImage_h

#include "vtkImageSource.h"

class vtkRenderer;

class VTK_HYBRID_EXPORT vtkRenderLargeImage : public vtkImageSource
{
public:
  static vtkRenderLargeImage *New();
  vtkTypeRevisionMacro(vtkRenderLargeImage,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // The magnification of the current render window
  vtkSetMacro(Magnification,int);
  vtkGetMacro(Magnification,int);

  // Description:
  // Indicates what renderer to get the pixel data from.
  virtual void SetInput(vtkRenderer*);

  // Description:
  // Returns which renderer is being used as the source for the pixel data.
  vtkGetObjectMacro(Input,vtkRenderer);

protected:
  vtkRenderLargeImage();
  ~vtkRenderLargeImage();

  int Magnification;
  vtkRenderer *Input;
  void ExecuteData(vtkDataObject *data);
  void ExecuteInformation();  
private:
  vtkRenderLargeImage(const vtkRenderLargeImage&);  // Not implemented.
  void operator=(const vtkRenderLargeImage&);  // Not implemented.
};

#endif
