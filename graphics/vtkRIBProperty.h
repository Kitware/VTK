/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBProperty.h
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
// .NAME vtkRIBProperty - RIP Property
// .SECTION Description
// vtkRIBProperty is a subclass of vtkProperty that allows the user to
// specify named shaders for use with RenderMan. Both a surface shader
// and displacement shadr can be specified. Parameters for the shaders
// can be declared and set.
//
// .SECTION See Also
// vtkRIBExporter

#ifndef __vtkRIBProperty_h
#define __vtkRIBProperty_h

#include <stdio.h>
#include "vtkProperty.h"

class vtkRIBRenderer;

class VTK_EXPORT vtkRIBProperty : public vtkProperty
{
public:
  vtkRIBProperty();
  ~vtkRIBProperty();
  static vtkRIBProperty *New() {return new vtkRIBProperty;};
  const char *GetClassName() {return "vtkRIBProperty";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of a surface shader.
  vtkSetStringMacro(SurfaceShader);
  vtkGetStringMacro(SurfaceShader);
  
  // Description:
  // Specify the name of a displacement shader.
  vtkSetStringMacro(DisplacementShader);
  vtkGetStringMacro(DisplacementShader);
  
  // Description:
  // Specify declarations for variables..
  void SetVariable (char *variable, char *declaration);
  void AddVariable (char *variable, char *declaration);
  
  // Description:
  // Get variable declarations
  char *GetDeclarations ();
  
  // Description:
  // Specify parameter values for variables.
  void SetParameter (char *parameter, char *value);
  void AddParameter (char *parameter, char *value);
  
  // Description:
  // Get parameters.
  char *GetParameters ();
  
protected:
  void Render(vtkActor *a, vtkRenderer *ren);
  vtkProperty *Property;
  char *SurfaceShader;
  char *DisplacementShader;
  char *Declarations;
  char *Parameters;
};

#endif
