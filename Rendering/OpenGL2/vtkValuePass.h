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

  /// \brief Interface to get the result of a render pass in FLOATING_POINT mode.
  vtkFloatArray* GetFloatImageData(vtkRenderer* ren);
  std::vector<int> GetFloatImageExtents(vtkRenderer* ren);

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
  void InitializeFloatingPointMode(vtkRenderer* ren);
  void ReleaseFloatingPointMode(vtkRenderer* ren);

///////////////////////////////////////////////////////////////////////////////

  class vtkInternals;
  vtkInternals *Internals;
  int RenderingMode;

  /// \brief FLOATING_POINT mode resources. FBO, attachments and other
  /// control variables.
  vtkFrameBufferObject2* ValueFrameBO;
  vtkRenderbuffer* ValueRenderBO;
  int Size[2];
  bool ValuePassResourcesAllocated;

 private:
  vtkValuePass(const vtkValuePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkValuePass&) VTK_DELETE_FUNCTION;

};

#endif
