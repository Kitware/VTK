/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitVolume.h
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
// .NAME vtkImplicitVolume - treat a volume as if it were an implicit function
// .SECTION Description
// vtkImplicitVolume treats a volume (e.g., structured point dataset)
// as if it were an implicit function. This means it computes a function
// value and gradient. vtkImplicitVolume is a concrete implementation of 
// vtkImplicitFunction.
//
// .SECTION See Also
// vtkImplicitFunction vtkClipper vtkCutter

#ifndef __vtkImplicitVolume_h
#define __vtkImplicitVolume_h

#include "vtkImplicitFunction.h"
#include "vtkStructuredPoints.h"

class vtkImplicitVolume : public vtkImplicitFunction
{
public:
  vtkImplicitVolume();
  char *GetClassName() {return "vtkImplicitVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long int GetMTime();

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify the volume for the implicit function.
  vtkSetObjectMacro(Volume,vtkStructuredPoints);
  vtkGetObjectMacro(Volume,vtkStructuredPoints);

protected:
  vtkStructuredPoints *Volume; // the structured points

};

#endif


