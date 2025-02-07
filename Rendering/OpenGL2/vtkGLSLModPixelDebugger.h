// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGLSLModPixelDebugger
 * @brief   Allow live pixel debugging by overwriting gl_FragData[0] output.
 *
 * This modification significantly simplifies the lives of VTK OpenGL developers, enabling them to
 * debug and adjust GLSL code in real-time while the application is running.
 *
 * This eliminates the need to recompile VTK for minor shader code adjustments. Developers can
 * conveniently keep the JSON and associated GLSL files open in a code editor, making changes while
 * a unit test or VTK application is actively running. Ultimately, you can just move your mouse in
 * the render window to witness your modifications taking effect!
 *
 * Shader substitutions will need to be defined in a json file. An example is provided in
 * Rendering/CellGrid/LiveGLSLDebugSample/sample.json file. If you've built VTK from source,
 * you may live edit that json file and glsl files under the LiveGLSLDebugSample directory.
 * Here is what it looks like:
 * {
 *   "Substitutions": [
 *     {
 *       "Target": "//VTK::Light::Impl",
 *       "ShaderType": "Fragment",
 *       "FileName": "normal-debug.glsl",
 *       "ReplaceAllOccurrences": false,
 *       "FileNameIsAbsolute": false,
 *       "Enabled": false
 *     },
 *     {
 *       "Target": "//VTK::Light::Impl",
 *       "ShaderType": "Fragment",
 *       "FileName": "parametric-debug.glsl",
 *       "ReplaceAllOccurrences": false,
 *       "FileNameIsAbsolute": false,
 *       "Enabled": false
 *     }
 *   ]
 * }
 * In the sample, both substitutions are disabled. Please enable either to view it.
 * Here is detailed information about the keys:
 *
 * \li \c Substitutions: This is a list of maps that contain information about a substitution.
 *
 * \li \c Target: This must be a string of type "//VTK::Feature::[Dec,Impl]".
 * These are found in the shader templates.
 *
 * \li \c ShaderType: This must be either "Fragment" or "Vertex" or "Geometry".
 *
 * \li \c FileName: Path to a file which has glsl code which will be pasted in place of the 'Target'
 * string.
 *
 * \li \c ReplaceAllOccurrences: Whether to replace all occurrences of 'Target' string with the
 * contents from the 'FileName' file.
 *
 * \li \c FileNameIsAbsolute: Whether 'FileName' is an absolute path or relative to the json file.
 *
 * \li \c Enabled: When enabled is true, the mod will perform the substitution, otherwise, the
 * substitution is not applied.
 */

#ifndef vtkGLSLModPixelDebugger_h
#define vtkGLSLModPixelDebugger_h

#include "vtkGLSLModifierBase.h"

#include "vtkOpenGLRenderer.h"         // for ivar
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkStringToken.h"            // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkInformationObjectBaseKey;
class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkGLSLModPixelDebugger : public vtkGLSLModifierBase
{
public:
  static vtkGLSLModPixelDebugger* New();
  vtkTypeMacro(vtkGLSLModPixelDebugger, vtkGLSLModifierBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set this to a json file on your file system. Look inside
  /// Rendering/CellGrid/LiveGLSLDebugSample/ for an example.
  void SetSubstitutionJSONFileName(const std::string& filename)
  {
    this->SubstitutionJSONFileName = filename;
  }

  // vtkGLSLModifierBase virtuals:
  bool ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
    std::string& tessControlShader, std::string& tessEvalShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkActor* actor) override;
  bool SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
    vtkAbstractMapper* mapper, vtkActor* actor, vtkOpenGLVertexArrayObject* VAO = nullptr) override;
  bool IsUpToDate(vtkOpenGLRenderer* vtkNotUsed(renderer), vtkAbstractMapper* vtkNotUsed(mapper),
    vtkActor* vtkNotUsed(actor)) override
  {
    return this->HashSubstitutionJSONFileContents() ==
      this->LastSubstitutionJSONFileContentsToken &&
      this->HashGLSLFilesContents() == this->LastGLSLFilesContentsToken;
  }

protected:
  vtkGLSLModPixelDebugger();
  ~vtkGLSLModPixelDebugger() override;

  std::string SubstitutionJSONFileName;

  vtkStringToken LastSubstitutionJSONFileContentsToken; // computed internally
  vtkStringToken LastGLSLFilesContentsToken;            // computed internally

  vtkStringToken HashSubstitutionJSONFileContents();
  vtkStringToken HashGLSLFilesContents();

private:
  vtkGLSLModPixelDebugger(const vtkGLSLModPixelDebugger&) = delete;
  void operator=(const vtkGLSLModPixelDebugger&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
