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
 *
 * Renders geometry using the values of a field array as fragment colors. The
 * output can be used for deferred color mapping. It supports using arrays of
 * either point or cell data. The target array can be selected by setting an
 * array name/id and a component number. Only opaque geometry is supported.
 *
 * There are two rendering modes available:
 *
 * * INVERTIBLE_LUT  Encodes array values as RGB data and renders the result to
 * the default framebuffer.
 *
 * * FLOATING_POINT  Renders actual array values as floating point data to an
 * internal RGBA32F framebuffer.  This class binds and unbinds the framebuffer
 * on each render pass.
 *
 * @sa
 * vtkRenderPass vtkDefaultPass vtkValuePassHelper vtkMapper
*/

#ifndef vtkValuePass_h
#define vtkValuePass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationStringKey;
class vtkRenderer;
class vtkRenderWindow;
class vtkFloatArray;

class VTKRENDERINGOPENGL2_EXPORT vtkValuePass : public vtkDefaultPass
{
public:

  enum Mode {
    INVERTIBLE_LUT = 1,
    FLOATING_POINT = 2 };

  static vtkValuePass *New();
  vtkTypeMacro(vtkValuePass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkInformationIntegerKey *RENDER_VALUES();

  vtkSetMacro(RenderingMode, int);
  vtkGetMacro(RenderingMode, int);
  void SetInputArrayToProcess(int fieldAssociation, const char *name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldAttributeType);
  void SetInputComponentToProcess(int component);
  void SetScalarRange(double min, double max);

  //@{
  /**
   * Passed down the rendering pipeline to control what data array to draw.
   */
  static vtkInformationIntegerKey *SCALAR_MODE();
  static vtkInformationIntegerKey *ARRAY_MODE();
  static vtkInformationIntegerKey *ARRAY_ID();
  static vtkInformationStringKey *ARRAY_NAME();
  static vtkInformationIntegerKey *ARRAY_COMPONENT();
  static vtkInformationDoubleVectorKey *SCALAR_RANGE();
  static vtkInformationIntegerKey *RELOAD_DATA();
  //@}

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  virtual void Render(const vtkRenderState *s);

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Returns a
   * single component array containing the rendered values.  The returned array
   * is owned by vtkValuePass so it is intended to be deep copied.
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

  bool IsFloatingPointModeSupported(vtkRenderWindow* renWin);

 protected:
  /**
   * Default constructor.
   */
  vtkValuePass();

  /**
   * Destructor.
   */
  virtual ~vtkValuePass();

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOpaqueGeometry(const vtkRenderState *s);

  /**
   * Manages graphics resources depending on the rendering mode.  Binds internal
   * FBO when FLOATING_POINT mode is enabled.
   */
  void BeginPass(vtkRenderer* ren);

  /**
   * Unbinds internal FBO when FLOATING_POINT mode is enabled.
   */
  void EndPass();

  //@{
  /**
   * Methods managing graphics resources required during FLOATING_POINT mode.
   */
  bool HasWindowSizeChanged(vtkRenderer* ren);
  bool InitializeFloatingPointMode(vtkRenderer* ren);
  void ReleaseFloatingPointMode(vtkRenderer* ren);
  //@}


  class vtkInternals;
  vtkInternals *Internals;
  int RenderingMode;

 private:
  vtkValuePass(const vtkValuePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePass&) VTK_DELETE_FUNCTION;

};

#endif
