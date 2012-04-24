/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkIOExportModule.h" // For export macro
#include "vtkProperty.h"

class vtkRIBRenderer;

class VTKIOEXPORT_EXPORT vtkRIBProperty : public vtkProperty
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

  void Render(vtkActor *a, vtkRenderer *ren);
  vtkProperty *Property;
  char *SurfaceShader;
  char *DisplacementShader;
  char *Declarations;
  char *Parameters;
private:
  vtkRIBProperty(const vtkRIBProperty&);  // Not implemented.
  void operator=(const vtkRIBProperty&);  // Not implemented.
};

#endif
