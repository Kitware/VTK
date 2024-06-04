// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSurfaceLICMapper
 * @brief   mapper that performs LIC on the surface of
 *  arbitrary geometry.
 *
 *
 *  vtkSurfaceLICMapper performs LIC on the surface of arbitrary
 *  geometry. Point vectors are used as the vector field for generating the LIC.
 *  The implementation was originallu  based on "Image Space Based Visualization
 *  on Unsteady Flow on Surfaces" by Laramee, Jobard and Hauser appeared in
 *  proceedings of IEEE Visualization '03, pages 131-138.
 *
 *  Internal pipeline:
 * <pre>
 * noise
 *     |
 *     [ PROJ (GAT) (COMP) LIC2D (SCAT) SHADE (CCE) DEP]
 *     |                                               |
 * vectors                                         surface LIC
 * </pre>
 * PROj  - project vectors onto surface
 * GAT   - gather data for compositing and guard pixel generation  (parallel only)
 * COMP  - composite gathered data
 * LIC2D - line intengral convolution, see vtkLineIntegralConvolution2D.
 * SCAT  - scatter result (parallel only, not all compositors use it)
 * SHADE - combine LIC and scalar colors
 * CCE   - color contrast enhancement (optional)
 * DEP   - depth test and copy to back buffer
 *
 * The result of each stage is cached in a texture so that during interaction
 * a stage may be skipped if the user has not modified its parameters or input
 * data.
 *
 * The parallel parts of algorithm are implemented in vtkPSurfaceLICMapper.
 * Note that for MPI enabled builds this class will be automatically created
 * by the object factory.
 *
 * @sa
 * vtkLineIntegralConvolution2D
 */

#ifndef vtkSurfaceLICMapper_h
#define vtkSurfaceLICMapper_h

#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingLICOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkSurfaceLICInterface;
class vtkPainterCommunicator;

class VTKRENDERINGLICOPENGL2_EXPORT VTK_MARSHALAUTO vtkSurfaceLICMapper
  : public vtkOpenGLPolyDataMapper
{
public:
  static vtkSurfaceLICMapper* New();
  vtkTypeMacro(vtkSurfaceLICMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release. In this case, releases the display lists.
   */
  void ReleaseGraphicsResources(vtkWindow* win) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* ren, vtkActor* act) override;

  /**
   * Shallow copy of an actor.
   */
  void ShallowCopy(vtkAbstractMapper*) override;

  ///@{
  /**
   * Get the vtkSurfaceLICInterface used by this mapper
   */
  vtkGetObjectMacro(LICInterface, vtkSurfaceLICInterface);
  ///@}

protected:
  vtkSurfaceLICMapper();
  ~vtkSurfaceLICMapper() override;

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * vtkSurfaceLICMapperTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void StartTimerEvent(const char*) {}
  virtual void EndTimerEvent(const char*) {}

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void BuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

  vtkSurfaceLICInterface* LICInterface;

private:
  vtkSurfaceLICMapper(const vtkSurfaceLICMapper&) = delete;
  void operator=(const vtkSurfaceLICMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
