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

#define VTK_BLACK	        0   // BLACKNESS   // R2_BLACK
#define VTK_NOT_DEST		1   // DSTINVERT   // R2_NOT
#define VTK_SRC_AND_DEST	2   // SRCAND      // R2_MASKPEN
#define VTK_SRC_OR_DEST		3   // SRCPAINT    // R2_MERGEPEN
#define VTK_NOT_SRC		4   // NOTSRCCOPY  // R2_NOTCOPYPEN
#define VTK_SRC_XOR_DEST	5   // SRCINVERT   // R2_XORPEN
#define VTK_SRC_AND_notDEST	6   // SRCERASE    // R2_MERGEPENNOT
#define VTK_SRC			7   // SRCCOPY    // R2_COPYPEN
#define VTK_WHITE		8   // WHITENESS   // R2_WHITE

class vtkViewport;

class VTK_EXPORT vtkProperty2D : public vtkObject
{
public:

  vtkProperty2D();
  ~vtkProperty2D();
  static vtkProperty2D *New() {return new vtkProperty2D;};

  vtkSetVector3Macro(Color, float);
  vtkGetVectorMacro(Color, float, 3);

  vtkGetMacro(CompositingOperator, int);
  vtkSetMacro(CompositingOperator, int);

  vtkGetMacro(Opacity, float);
  vtkSetMacro(Opacity, float);

  void SetCompositingOperatorToBlack() {this->CompositingOperator = VTK_BLACK;};
  void SetCompositingOperatorToNotDest() {this->CompositingOperator = VTK_NOT_DEST;};
  void SetCompositingOperatorToSrcAndDest() {this->CompositingOperator = VTK_SRC_AND_DEST;};
  void SetCompositingOperatorToSrcOrDest() {this->CompositingOperator = VTK_SRC_OR_DEST;};
  void SetCompositingOperatorToNotSrc() {this->CompositingOperator = VTK_NOT_SRC;};
  void SetCompositingOperatorToSrcXorDest() {this->CompositingOperator = VTK_SRC_XOR_DEST;};
  void SetCompositingOperatorToSrcAndNotDest() {this->CompositingOperator = VTK_SRC_AND_notDEST;};
  void SetCompositingOperatorToSrc() {this->CompositingOperator = VTK_SRC;};
  void SetCompositingOperatorToWhite() {this->CompositingOperator = VTK_WHITE;};

  void Render (vtkViewport* viewport)  { viewport;}
  
protected:
  float Color[3];
  int   CompositingOperator;
  float   Opacity;
};
  
  
#endif


  
