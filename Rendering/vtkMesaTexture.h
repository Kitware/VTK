/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaTexture - Mesa texture map
// .SECTION Description
// vtkMesaTexture is a concrete implementation of the abstract class 
// vtkTexture. vtkMesaTexture interfaces to the Mesa rendering library.

#ifndef __vtkMesaTexture_h
#define __vtkMesaTexture_h

#include "vtkTexture.h"

class vtkWindow;
class vtkMesaRenderer;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkMesaTexture : public vtkTexture
{
public:
  static vtkMesaTexture *New();
  vtkTypeMacro(vtkMesaTexture,vtkTexture);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer *ren);
  
  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported. 
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkMesaTexture();
  ~vtkMesaTexture();

  unsigned char *ResampleToPowerOfTwo(int &xsize, int &ysize, 
                                      unsigned char *dptr, int bpp);

  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for previous render
private:
  vtkMesaTexture(const vtkMesaTexture&);  // Not implemented.
  void operator=(const vtkMesaTexture&);  // Not implemented.
};

#endif
