/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBProperty.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkRIBProperty - RIP Property
// .SECTION Description
// vtkRIBProperty is a subclass of vtkProperty that allows the user to
// specify named shaders for use with RenderMan. Both a surface shader
// and displacement shader can be specified. Parameters for the shaders
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
  static vtkRIBProperty *New();
  vtkTypeMacro(vtkRIBProperty,vtkProperty);
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
  vtkRIBProperty();
  ~vtkRIBProperty();
  vtkRIBProperty(const vtkRIBProperty&);
  void operator=(const vtkRIBProperty&);

  void Render(vtkActor *a, vtkRenderer *ren);
  vtkProperty *Property;
  char *SurfaceShader;
  char *DisplacementShader;
  char *Declarations;
  char *Parameters;
};

#endif
