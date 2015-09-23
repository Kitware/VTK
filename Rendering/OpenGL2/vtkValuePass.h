/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkValuePass - TO DO
// .SECTION Description
// TO DO
//
// .SECTION See Also
// vtkRenderPass vtkDefaultPass

#ifndef vtkValuePass_h
#define vtkValuePass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationStringKey;

class VTKRENDERINGOPENGL2_EXPORT vtkValuePass : public vtkDefaultPass
{
public:
  static vtkValuePass *New();
  vtkTypeMacro(vtkValuePass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkInformationIntegerKey *RENDER_VALUES();

  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);
  void SetInputComponentToProcess(int component);

  // Description:
  // Passed down the rendering pipeline to control what data array to draw.
  static vtkInformationIntegerKey *SCALAR_MODE();
  static vtkInformationIntegerKey *ARRAY_MODE();
  static vtkInformationIntegerKey *ARRAY_ID();
  static vtkInformationStringKey *ARRAY_NAME();
  static vtkInformationIntegerKey *ARRAY_COMPONENT();

  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);

 protected:
  // Description:
  // Default constructor.
  vtkValuePass();

  // Description:
  // Destructor.
  virtual ~vtkValuePass();

  // Description:
  // Opaque pass with key checking.
  // \pre s_exists: s!=0
  virtual void RenderOpaqueGeometry(const vtkRenderState *s);

  class vtkInternals;
  vtkInternals *Internals;

 private:
  vtkValuePass(const vtkValuePass&);  // Not implemented.
  void operator=(const vtkValuePass&);  // Not implemented.
};

#endif
