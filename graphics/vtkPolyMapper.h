/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPolyMapper - map vtkPolyData to graphics primitives
// .SECTION Description
// vtkPolyMapper is a mapper to map polygonal data (i.e., vtkPolyData) to 
// graphics primitives. It is possible to control which geometric 
// primitives are displayed using the boolean variables provided.

#ifndef __vtkPolyMapper_h
#define __vtkPolyMapper_h

#include "vtkMapper.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"

class vtkPolyMapperDevice;

class VTK_EXPORT vtkPolyMapper : public vtkMapper 
{
public:
  vtkPolyMapper();
  ~vtkPolyMapper();
  static vtkPolyMapper *New() {return new vtkPolyMapper;};
  char *GetClassName() {return "vtkPolyMapper";};

  void Render(vtkRenderer *ren, vtkActor *a);
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  void SetInput(vtkPolyData *in);
  void SetInput(vtkPolyData& in) {this->SetInput(&in);};

protected:
  vtkPolyMapperDevice  *Device;
};

#endif


