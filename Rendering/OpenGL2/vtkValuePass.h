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

#include <vector>

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class vtkInformationDoubleVectorKey;
class vtkInformationIntegerKey;
class vtkInformationStringKey;
class vtkRenderer;
class vtkRenderWindow;
class vtkFrameBufferObject2;
class vtkRenderbuffer;
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

  // Description:
  // Passed down the rendering pipeline to control what data array to draw.
  static vtkInformationIntegerKey *SCALAR_MODE();
  static vtkInformationIntegerKey *ARRAY_MODE();
  static vtkInformationIntegerKey *ARRAY_ID();
  static vtkInformationStringKey *ARRAY_NAME();
  static vtkInformationIntegerKey *ARRAY_COMPONENT();
  static vtkInformationDoubleVectorKey *SCALAR_RANGE();

  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);

  /// @{
  /// @description Interface to get the result of a render pass in
  /// FLOATING_POINT mode.

  /// @brief Returns a single component array containing the rendered values.
  /// The returned array is owned by vtkValuePass so it is intended to be deep copied.
  vtkFloatArray* GetFloatImageDataArray(vtkRenderer* ren);

  /// @brief Image extents of the value array.
  std::vector<int> GetFloatImageExtents();

  /// @brief Low level API, a format for the internal glReadPixels call can be
  /// specified. 'data' is expected to be allocated and cleaned-up by the caller.
  void GetFloatImageData(int const format, int const width, int const height,
    void* data);
  /// @}

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

  // Description:
  // Manages graphics resources depending on the rendering mode.  Binds internal
  // FBO when FLOATING_POINT mode is enabled.
  void BeginPass(vtkRenderer* ren);

  // Description:
  // Unbinds internal FBO when FLOATING_POINT mode is enabled.
  void EndPass();

  /// \brief Member methods managing graphics resources required during FLOATING_POINT
  /// mode.
  bool IsFloatFBOSupported(vtkRenderWindow* renWin);
  bool HasWindowSizeChanged(vtkRenderer* ren);
  bool InitializeFloatingPointMode(vtkRenderer* ren);
  void ReleaseFloatingPointMode(vtkRenderer* ren);

///////////////////////////////////////////////////////////////////////////////

  /// \brief FLOATING_POINT mode resources. FBO, attachments and other
  /// control variables.
  vtkFrameBufferObject2* ValueFrameBO;
  vtkRenderbuffer* ValueRenderBO;
  vtkRenderbuffer* DepthRenderBO;
  bool ValuePassResourcesAllocated;
  int RenderingMode;

  class vtkInternals;
  vtkInternals *Internals;

 private:
  vtkValuePass(const vtkValuePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePass&) VTK_DELETE_FUNCTION;

};

#endif
