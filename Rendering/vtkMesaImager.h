/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImager.h
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
// .NAME vtkImager - Renders into part of a ImageWindow
// .SECTION Description
// vtkImager is the 2D counterpart to vtkRenderer. An Imager renders
// 2D actors into a viewport of an image window. 

// .SECTION See Also
//  vtkImageWindow vtkViewport
   

#ifndef __vtkMesaImager_h
#define __vtkMesaImager_h

#include "vtkImager.h"

class VTK_RENDERING_EXPORT vtkMesaImager : public vtkImager
{ 
public:
  static vtkMesaImager *New();
  vtkTypeRevisionMacro(vtkMesaImager,vtkImager);

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  int RenderOpaqueGeometry();

  // Description:
  // Erase the contents of the imager in the window.
  void Erase();

protected:
  vtkMesaImager() {};
  ~vtkMesaImager() {};
private:
  vtkMesaImager(const vtkMesaImager&);  // Not implemented.
  void operator=(const vtkMesaImager&);  // Not implemented.
};


#endif




