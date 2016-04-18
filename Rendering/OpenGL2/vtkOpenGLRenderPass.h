/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGLRenderPass - Abstract render pass with shader modifications.
//
// .SECTION Description
// Allows a render pass to update shader code using a new virtual API.

#ifndef vtkOpenGLRenderPass_h
#define vtkOpenGLRenderPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

#include <string> // For std::string

class vtkAbstractMapper;
class vtkInformationObjectBaseVectorKey;
class vtkProp;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderPass: public vtkRenderPass
{
public:
  vtkTypeMacro(vtkOpenGLRenderPass, vtkRenderPass)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Use vtkShaderProgram::Substitute to replace //VTK::XXX:YYY declarations in
  // the shader sources.
  // Return false on error.
  virtual bool ReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper, vtkProp *prop);

  // Description:
  // Update the uniforms of the shader program.
  // Return false on error.
  virtual bool SetShaderParameters(vtkShaderProgram *program,
                                   vtkAbstractMapper *mapper, vtkProp *prop);

  // Description:
  // For multi-stage render passes that need to change shader code during a
  // single pass, use this method to notify a mapper that the shader needs to be
  // rebuilt (rather than reuse the last cached shader. This method should
  // return the last time that the shader stage changed, or 0 if the shader
  // is single-stage.
  virtual unsigned long int GetShaderStageMTime();

  // Description:
  // Key containing information about the current pass.
  static vtkInformationObjectBaseVectorKey *RenderPasses();

protected:
  vtkOpenGLRenderPass();
  ~vtkOpenGLRenderPass();

  // Description:
  // Call before rendering to update the actors' information keys.
  void PreRender(const vtkRenderState *s);

  // Description:
  // Call after rendering to clean up the actors' information keys.
  void PostRender(const vtkRenderState *s);

private:
  vtkOpenGLRenderPass(const vtkOpenGLRenderPass&); // Not implemented
  void operator=(const vtkOpenGLRenderPass&); // Not implemented
};

#endif // vtkOpenGLRenderPass_h
