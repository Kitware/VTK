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
/**
 * @class   vtkValuePass
 *
 * Renders geometry using the values of a field array as fragment colors. The
 * output can be used for deferred color mapping. It supports using arrays of
 * either point or cell data. The target array can be selected by setting an
 * array name/id and a component number. Only opaque geometry is supported.
 *
 * There are two rendering modes available:
 *
 * * INVERTIBLE_LUT  Encodes array values as RGB data and renders the result to
 * the default framebuffer.  It uses a texture as a color LUT to map the values
 * to RGB data. Texture size constraints limit its precision (currently 12-bit).
 * The implementation of this mode is in vtkInternalsInvertible.
 *
 * * FLOATING_POINT  Renders actual array values as floating point data to an
 * internal RGBA32F framebuffer.  This class binds and unbinds the framebuffer
 * on each render pass. Resources are allocated on demand. When rendering point
 * data values are uploaded to the GPU as vertex attributes. When rendering cell
 * data values are uploaded as a texture buffer. Custom vertex and fragment
 * shaders are defined in order to adjust its behavior for either type of data.
 *
 * @sa
 * vtkRenderPass vtkOpenGLRenderPass
*/
#ifndef vtkValuePass_h
#define vtkValuePass_h

#include "vtkOpenGLRenderPass.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h" //for ivar

class vtkAbstractArray;
class vtkActor;
class vtkDataArray;
class vtkDataObject;
class vtkFloatArray;
class vtkMapper;
class vtkOpenGLVertexArrayObject;
class vtkProperty;
class vtkRenderer;
class vtkRenderWindow;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkValuePass : public vtkOpenGLRenderPass
{
public:

  enum Mode
  {
    INVERTIBLE_LUT = 1,
    FLOATING_POINT = 2
  };

  static vtkValuePass *New();
  vtkTypeMacro(vtkValuePass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkSetMacro(RenderingMode, int);
  vtkGetMacro(RenderingMode, int);
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldId);
  void SetInputComponentToProcess(int component);
  void SetScalarRange(double min, double max);

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) VTK_OVERRIDE;

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Returns a
   * single component array containing the rendered values.
   * \warning The returned array is owned by this class.
   */
  vtkFloatArray* GetFloatImageDataArray(vtkRenderer* ren);

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Low level API,
   * a format for the internal glReadPixels call can be specified. 'data' is expected
   * to be allocated and cleaned-up by the caller.
   */
  void GetFloatImageData(int const format, int const width, int const height,
    void* data);

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Image extents of
   * the value array.
   */
  int* GetFloatImageExtents();

  /**
   * Check for extension support.
   */
  bool IsFloatingPointModeSupported();

  void ReleaseGraphicsResources(vtkWindow *win) VTK_OVERRIDE;

  /**
   * Convert an RGB triplet to a floating point value. This method is exposed
   * as a convenience function for testing (TestValuePass2).
   */
  void ColorToValue(unsigned char const* color, double const min, double const scale,
    double& value);

 protected:
  vtkValuePass();
  ~vtkValuePass() VTK_OVERRIDE;

  ///@{
  /**
   * \brief vtkOpenGLRenderPass API.
   */

  /**
   * Use vtkShaderProgram::Substitute to replace //VTK::XXX:YYY declarations in
   * the shader sources. Gets called after other mapper shader replacements.
   * Return false on error.
   */
  bool PostReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper,
                                   vtkProp *prop) VTK_OVERRIDE;
  /**
   * Update the uniforms of the shader program.
   * Return false on error.
   */
  bool SetShaderParameters(vtkShaderProgram* program,
                                   vtkAbstractMapper* mapper, vtkProp* prop,
                                   vtkOpenGLVertexArrayObject* VAO = NULL) VTK_OVERRIDE;
  /**
   * For multi-stage render passes that need to change shader code during a
   * single pass, use this method to notify a mapper that the shader needs to be
   * rebuilt (rather than reuse the last cached shader. This method should
   * return the last time that the shader stage changed, or 0 if the shader
   * is single-stage.
   */
  vtkMTimeType GetShaderStageMTime() VTK_OVERRIDE;
  ///@}

  /**
   * Manages graphics resources depending on the rendering mode.  Binds internal
   * FBO when FLOATING_POINT mode is enabled.
   */
  void BeginPass(vtkRenderer* ren);

  /**
   * Unbinds internal FBO when FLOATING_POINT mode is enabled.
   */
  void EndPass();

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  void RenderOpaqueGeometry(const vtkRenderState *s);

  /**
   * Unbind textures, etc.
   */
  void RenderPieceFinish();

  /**
   * Upload new data if necessary, bind textures, etc.
   */
  void RenderPieceStart(vtkDataArray* dataArr, vtkMapper *m);

  /**
   * Setup the mapper state, buffer objects or property variables necessary
   * to render the active rendering mode.
   */
  void BeginMapperRender(vtkMapper* mapper, vtkDataArray* dataArray,
    vtkProperty* property);

  /**
   * Revert any changes made in BeginMapperRender.
   */
  void EndMapperRender(vtkMapper* mapper, vtkProperty* property);

  void InitializeBuffers(vtkRenderer* ren);

  /**
   * Add necessary shader definitions.
   */
  bool UpdateShaders(std::string& VSSource, std::string& FSSource);

  /**
   * Bind shader variables.
   */
  void BindAttributes(vtkShaderProgram* prog, vtkOpenGLVertexArrayObject* VAO);
  void BindUniforms(vtkShaderProgram* prog);

  //@{
  /**
   * Methods managing graphics resources required during FLOATING_POINT mode.
   */
  bool HasWindowSizeChanged(vtkRenderer* ren);
  bool InitializeFBO(vtkRenderer* ren);
  void ReleaseFBO(vtkWindow* win);
  //@}

  class vtkInternalsFloat;
  vtkInternalsFloat* ImplFloat;

  class vtkInternalsInvertible;
  vtkInternalsInvertible* ImplInv;

  struct Parameters;
  Parameters* PassState;

  int RenderingMode;

 private:
  vtkDataArray* GetCurrentArray(vtkMapper* mapper, Parameters* arrayPar);

  vtkAbstractArray* GetArrayFromCompositeData(vtkMapper* mapper,
    Parameters* arrayPar);

  vtkSmartPointer<vtkAbstractArray> MultiBlocksArray;

  void PopulateCellCellMap(const vtkRenderState *s);

  vtkValuePass(const vtkValuePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePass&) VTK_DELETE_FUNCTION;
};

#endif
