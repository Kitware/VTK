/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkProperty2D
// .SECTION Description
// vtkProperty2D contains properties used to render two dimensional images
// and annotations.

// .SECTION See Also
// vtkActor2D 

#ifndef __vtkProperty2D_h
#define __vtkProperty2D_h

#include "vtkObject.h"

class vtkViewport;

class VTK_EXPORT vtkProperty2D : public vtkObject
{
public:
  vtkProperty2D();
  ~vtkProperty2D();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a vtkProperty2D with the following default values:
  // Opacity 1, Color (1,1,1)
  static vtkProperty2D *New() {return new vtkProperty2D;};

  // Description:
  // Set/Get the RGB color of this property.
  vtkSetVector3Macro(Color, float);
  vtkGetVectorMacro(Color, float, 3);

  // Description:
  // Set/Get the Opacity of this property.
  vtkGetMacro(Opacity, float);
  vtkSetMacro(Opacity, float);

  // Description:
  // Have the device specific subclass rneder this property.
  void Render (vtkViewport* vtkNotUsed(viewport))  {}
  
protected:
  float Color[3];
  float   Opacity;
};
  
  
#endif


  
