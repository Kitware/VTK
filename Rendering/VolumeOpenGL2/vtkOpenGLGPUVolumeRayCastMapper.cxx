/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGPUVolumeRayCastMapper.h"

#include <vtk_glew.h>

#include "vtkVolumeShaderComposer.h"
#include "vtkVolumeStateRAII.h"

// Include compiled shader code
#include <raycasterfs.h>
#include <raycastervs.h>

// VTK includes
#include "vtkInformation.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLUniforms.h"
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkClipConvexPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkContourFilter.h>
#include <vtkDataArray.h>
#include <vtkDensifyPolyData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkLightCollection.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkMultiVolume.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLBufferObject.h>
#include <vtkOpenGLCamera.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLRenderPass.h>
#include <vtkOpenGLRenderUtilities.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLShaderCache.h>
#include <vtkOpenGLShaderProperty.h>
#include <vtkOpenGLVertexArrayObject.h>
#include <vtkPixelBufferObject.h>
#include <vtkPixelExtent.h>
#include <vtkPixelTransfer.h>
#include <vtkPlaneCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkShader.h>
#include <vtkShaderProgram.h>
#include <vtkSmartPointer.h>
#include <vtkTessellatedBoxSource.h>
#include <vtkTextureObject.h>
#include <vtkTimerLog.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include <vtkVolumeInputHelper.h>

#include "vtkOpenGLVolumeGradientOpacityTable.h"
#include "vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h"
#include "vtkOpenGLVolumeMaskTransferFunction2D.h"
#include "vtkOpenGLVolumeOpacityTable.h"
#include "vtkOpenGLVolumeRGBTable.h"
#include "vtkOpenGLVolumeTransferFunction2D.h"

#include <vtkHardwareSelector.h>
#include <vtkVolumeMask.h>
#include <vtkVolumeProperty.h>
#include <vtkVolumeTexture.h>
#include <vtkWeakPointer.h>

// C/C++ includes
#include <cassert>
#include <limits>
#include <map>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkOpenGLGPUVolumeRayCastMapper);

//----------------------------------------------------------------------------
class vtkOpenGLGPUVolumeRayCastMapper::vtkInternal
{
public:
  // Constructor
  //--------------------------------------------------------------------------
  vtkInternal(vtkOpenGLGPUVolumeRayCastMapper* parent)
  {
    this->Parent = parent;
    this->ValidTransferFunction = false;
    this->LoadDepthTextureExtensionsSucceeded = false;
    this->CameraWasInsideInLastUpdate = false;
    this->CubeVBOId = 0;
    this->CubeVAOId = 0;
    this->CubeIndicesId = 0;
    this->DepthTextureObject = nullptr;
    this->SharedDepthTextureObject = false;
    this->TextureWidth = 1024;
    this->ActualSampleDistance = 1.0;
    this->CurrentMask = nullptr;
    this->TextureSize[0] = this->TextureSize[1] = this->TextureSize[2] = -1;
    this->WindowLowerLeft[0] = this->WindowLowerLeft[1] = 0;
    this->WindowSize[0] = this->WindowSize[1] = 0;
    this->LastDepthPassWindowSize[0] = this->LastDepthPassWindowSize[1] = 0;
    this->LastRenderToImageWindowSize[0] = 0;
    this->LastRenderToImageWindowSize[1] = 0;
    this->CurrentSelectionPass = vtkHardwareSelector::MIN_KNOWN_PASS - 1;

    this->NumberOfLights = 0;
    this->LightComplexity = 0;

    this->NeedToInitializeResources = false;
    this->ShaderCache = nullptr;

    this->FBO = nullptr;
    this->RTTDepthBufferTextureObject = nullptr;
    this->RTTDepthTextureObject = nullptr;
    this->RTTColorTextureObject = nullptr;
    this->RTTDepthTextureType = -1;

    this->DPFBO = nullptr;
    this->DPDepthBufferTextureObject = nullptr;
    this->DPColorTextureObject = nullptr;
    this->PreserveViewport = false;
    this->PreserveGLState = false;

    this->Partitions[0] = this->Partitions[1] = this->Partitions[2] = 1;
  }

  // Destructor
  //--------------------------------------------------------------------------
  ~vtkInternal()
  {
    if (this->DepthTextureObject)
    {
      this->DepthTextureObject->Delete();
      this->DepthTextureObject = nullptr;
    }

    if (this->FBO)
    {
      this->FBO->Delete();
      this->FBO = nullptr;
    }

    if (this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject->Delete();
      this->RTTDepthBufferTextureObject = nullptr;
    }

    if (this->RTTDepthTextureObject)
    {
      this->RTTDepthTextureObject->Delete();
      this->RTTDepthTextureObject = nullptr;
    }

    if (this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject->Delete();
      this->RTTColorTextureObject = nullptr;
    }

    if (this->ImageSampleFBO)
    {
      this->ImageSampleFBO->Delete();
      this->ImageSampleFBO = nullptr;
    }

    for (auto& tex : this->ImageSampleTexture)
    {
      tex = nullptr;
    }
    this->ImageSampleTexture.clear();
    this->ImageSampleTexNames.clear();

    if (this->ImageSampleVAO)
    {
      this->ImageSampleVAO->Delete();
      this->ImageSampleVAO = nullptr;
    }
    this->DeleteMaskTransfer();

    // Do not delete the shader programs - Let the cache clean them up.
    this->ImageSampleProg = nullptr;
  }

  // Helper methods
  //--------------------------------------------------------------------------
  template <typename T>
  static void ToFloat(const T& in1, const T& in2, float (&out)[2]);
  template <typename T>
  static void ToFloat(const T& in1, const T& in2, const T& in3, float (&out)[3]);
  template <typename T>
  static void ToFloat(T* in, float* out, int noOfComponents);
  template <typename T>
  static void ToFloat(T (&in)[3], float (&out)[3]);
  template <typename T>
  static void ToFloat(T (&in)[2], float (&out)[2]);
  template <typename T>
  static void ToFloat(T& in, float& out);
  template <typename T>
  static void ToFloat(T (&in)[4][2], float (&out)[4][2]);
  template <typename T, int SizeX, int SizeY>
  static void CopyMatrixToVector(T* matrix, float* matrixVec, int offset);
  template <typename T, int SizeSrc>
  static void CopyVector(T* srcVec, T* dstVec, int offset);

  ///@{
  /**
   * \brief Setup and clean-up transfer functions for each vtkVolumeInputHelper
   * and masks.
   */
  void UpdateTransferFunctions(vtkRenderer* ren);

  void RefreshMaskTransfer(vtkRenderer* ren, VolumeInput& input);
  int UpdateMaskTransfer(vtkRenderer* ren, vtkVolume* vol, unsigned int component);
  void SetupMaskTransfer(vtkRenderer* ren);
  void ReleaseGraphicsMaskTransfer(vtkWindow* window);
  void DeleteMaskTransfer();
  ///@}

  bool LoadMask(vtkRenderer* ren);

  // Update the depth sampler with the current state of the z-buffer. The
  // sampler is used for z-buffer compositing with opaque geometry during
  // ray-casting (rays are early-terminated if hidden begin opaque geometry).
  void CaptureDepthTexture(vtkRenderer* ren);

  // Test if camera is inside the volume geometry
  bool IsCameraInside(vtkRenderer* ren, vtkVolume* vol, double geometry[24]);

  //@{
  /**
   * Update volume's proxy-geometry and draw it
   */
  bool IsGeometryUpdateRequired(vtkRenderer* ren, vtkVolume* vol, double geometry[24]);
  void RenderVolumeGeometry(
    vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol, double geometry[24]);
  //@}

  // Update cropping params to shader
  void SetCroppingRegions(vtkShaderProgram* prog, double loadedBounds[6]);

  // Update clipping params to shader
  void SetClippingPlanes(vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol);

  // Update the ray sampling distance. Sampling distance should be updated
  // before updating opacity transfer functions.
  void UpdateSamplingDistance(vtkRenderer* ren);

  // Check if the mapper should enter picking mode.
  void CheckPickingState(vtkRenderer* ren);

  // Look for property keys used to control the mapper's state.
  // This is necessary for some render passes which need to ensure
  // a specific OpenGL state when rendering through this mapper.
  void CheckPropertyKeys(vtkVolume* vol);

  // Configure the vtkHardwareSelector to begin a picking pass. This call
  // changes GL_BLEND, so it needs to be called before constructing
  // vtkVolumeStateRAII.
  void BeginPicking(vtkRenderer* ren);

  // Update the prop Id if hardware selection is enabled.
  void SetPickingId(vtkRenderer* ren);

  // Configure the vtkHardwareSelector to end a picking pass.
  void EndPicking(vtkRenderer* ren);

  // Load OpenGL extensiosn required to grab depth sampler buffer
  void LoadRequireDepthTextureExtensions(vtkRenderWindow* renWin);

  // Create GL buffers
  void CreateBufferObjects();

  // Dispose / free GL buffers
  void DeleteBufferObjects();

  // Convert vtkTextureObject to vtkImageData
  void ConvertTextureToImageData(vtkTextureObject* texture, vtkImageData* output);

  // Render to texture for final rendering
  void SetupRenderToTexture(vtkRenderer* ren);
  void ExitRenderToTexture(vtkRenderer* ren);

  // Render to texture for depth pass
  void SetupDepthPass(vtkRenderer* ren);
  void RenderContourPass(vtkRenderer* ren);
  void ExitDepthPass(vtkRenderer* ren);
  void RenderWithDepthPass(vtkRenderer* ren, vtkOpenGLCamera* cam, vtkMTimeType renderPassTime);

  void RenderSingleInput(vtkRenderer* ren, vtkOpenGLCamera* cam, vtkShaderProgram* prog);

  void RenderMultipleInputs(vtkRenderer* ren, vtkOpenGLCamera* cam, vtkShaderProgram* prog);

  //@{
  /**
   * Update shader parameters.
   */
  void SetLightingShaderParameters(
    vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol, int numberOfSamplers);

  /**
   * Global parameters.
   */
  void SetMapperShaderParameters(
    vtkShaderProgram* prog, vtkRenderer* ren, int independent, int numComponents);

  /**
   * Per input data/ per component parameters.
   */
  void SetVolumeShaderParameters(
    vtkShaderProgram* prog, int independent, int numComponents, vtkMatrix4x4* modelViewMat);
  void BindTransformations(vtkShaderProgram* prog, vtkMatrix4x4* modelViewMat);

  /**
   * Transformation parameters.
   */
  void SetCameraShaderParameters(vtkShaderProgram* prog, vtkRenderer* ren, vtkOpenGLCamera* cam);

  /**
   * Feature specific.
   */
  void SetMaskShaderParameters(vtkShaderProgram* prog, vtkVolumeProperty* prop, int noOfComponents);
  void SetRenderToImageParameters(vtkShaderProgram* prog);
  void SetAdvancedShaderParameters(vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol,
    vtkVolumeTexture::VolumeBlock* block, int numComp);
  //@}

  void FinishRendering(int numComponents);

  inline bool ShaderRebuildNeeded(vtkCamera* cam, vtkVolume* vol, vtkMTimeType renderPassTime);
  bool VolumePropertyChanged = true;

  //@{
  /**
   * Image XY-Sampling
   * Render to an internal framebuffer with lower resolution than the currently
   * bound one (hence casting less rays and improving performance). The rendered
   * image is subsequently rendered as a texture-mapped quad (linearly
   * interpolated) to the default (or previously attached) framebuffer. If a
   * vtkOpenGLRenderPass is attached, a variable number of render targets are
   * supported (as specified by the RenderPass). The render targets are assumed
   * to be ordered from GL_COLOR_ATTACHMENT0 to GL_COLOR_ATTACHMENT$N$, where
   * $N$ is the number of targets specified (targets of the previously bound
   * framebuffer as activated through ActivateDrawBuffers(int)). Without a
   * RenderPass attached, it relies on FramebufferObject to re-activate the
   * appropriate previous DrawBuffer.
   *
   * \sa vtkOpenGLRenderPass vtkOpenGLFramebufferObject
   */
  void BeginImageSample(vtkRenderer* ren);
  bool InitializeImageSampleFBO(vtkRenderer* ren);
  void EndImageSample(vtkRenderer* ren);
  size_t GetNumImageSampleDrawBuffers(vtkVolume* vol);
  //@}

  //@{
  /**
   * Allocate and update input data. A list of active ports is maintained
   * by the parent class. This list is traversed to update internal structures
   * used during rendering.
   */
  bool UpdateInputs(vtkRenderer* ren, vtkVolume* vol);

  /**
   * Cleanup resources of inputs that have been removed.
   */
  void ClearRemovedInputs(vtkWindow* win);

  /**
   * Forces transfer functions in all of the active vtkVolumeInputHelpers to
   * re-initialize in the next update. This is essential if the order in
   * AssembledInputs changes (inputs are added or removed), given that variable
   * names cached in vtkVolumeInputHelper instances are indexed.
   */
  void ForceTransferInit();
  //@}

  vtkVolume* GetActiveVolume()
  {
    return this->MultiVolume ? this->MultiVolume : this->Parent->AssembledInputs[0].Volume;
  }
  int GetComponentMode(vtkVolumeProperty* prop, vtkDataArray* array) const;

  void ReleaseRenderToTextureGraphicsResources(vtkWindow* win);
  void ReleaseImageSampleGraphicsResources(vtkWindow* win);
  void ReleaseDepthPassGraphicsResources(vtkWindow* win);

  // Private member variables
  //--------------------------------------------------------------------------
  vtkOpenGLGPUVolumeRayCastMapper* Parent;

  bool ValidTransferFunction;
  bool LoadDepthTextureExtensionsSucceeded;
  bool CameraWasInsideInLastUpdate;

  GLuint CubeVBOId;
  GLuint CubeVAOId;
  GLuint CubeIndicesId;

  vtkTextureObject* DepthTextureObject;
  bool SharedDepthTextureObject;

  int TextureWidth;

  float ActualSampleDistance;

  int LastProjectionParallel;
  int TextureSize[3];
  int WindowLowerLeft[2];
  int WindowSize[2];
  int LastDepthPassWindowSize[2];
  int LastRenderToImageWindowSize[2];

  int NumberOfLights;
  int LightComplexity;

  std::ostringstream ExtensionsStringStream;

  vtkSmartPointer<vtkOpenGLVolumeMaskTransferFunction2D> LabelMapTransfer2D;
  vtkSmartPointer<vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D> LabelMapGradientOpacity;

  vtkTimeStamp ShaderBuildTime;

  vtkNew<vtkMatrix4x4> InverseProjectionMat;
  vtkNew<vtkMatrix4x4> InverseModelViewMat;
  vtkNew<vtkMatrix4x4> InverseVolumeMat;

  vtkSmartPointer<vtkPolyData> BBoxPolyData;
  vtkSmartPointer<vtkVolumeTexture> CurrentMask;

  vtkTimeStamp InitializationTime;
  vtkTimeStamp MaskUpdateTime;
  vtkTimeStamp ReleaseResourcesTime;
  vtkTimeStamp DepthPassTime;
  vtkTimeStamp DepthPassSetupTime;
  vtkTimeStamp SelectionStateTime;
  int CurrentSelectionPass;
  bool IsPicking;

  bool NeedToInitializeResources;
  bool PreserveViewport;
  bool PreserveGLState;

  vtkShaderProgram* ShaderProgram;
  vtkOpenGLShaderCache* ShaderCache;

  vtkOpenGLFramebufferObject* FBO;
  vtkTextureObject* RTTDepthBufferTextureObject;
  vtkTextureObject* RTTDepthTextureObject;
  vtkTextureObject* RTTColorTextureObject;
  int RTTDepthTextureType;

  vtkOpenGLFramebufferObject* DPFBO;
  vtkTextureObject* DPDepthBufferTextureObject;
  vtkTextureObject* DPColorTextureObject;

  vtkOpenGLFramebufferObject* ImageSampleFBO = nullptr;
  std::vector<vtkSmartPointer<vtkTextureObject> > ImageSampleTexture;
  std::vector<std::string> ImageSampleTexNames;
  vtkShaderProgram* ImageSampleProg = nullptr;
  vtkOpenGLVertexArrayObject* ImageSampleVAO = nullptr;
  size_t NumImageSampleDrawBuffers = 0;
  bool RebuildImageSampleProg = false;
  bool RenderPassAttached = false;

  vtkNew<vtkContourFilter> ContourFilter;
  vtkNew<vtkPolyDataMapper> ContourMapper;
  vtkNew<vtkActor> ContourActor;

  unsigned short Partitions[3];
  vtkMultiVolume* MultiVolume = nullptr;

  std::vector<float> VolMatVec, InvMatVec, TexMatVec, InvTexMatVec, TexEyeMatVec, CellToPointVec,
    TexMinVec, TexMaxVec, ScaleVec, BiasVec, StepVec, SpacingVec, RangeVec;
};

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  const T& in1, const T& in2, float (&out)[2])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
}

template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(
  const T& in1, const T& in2, const T& in3, float (&out)[3])
{
  out[0] = static_cast<float>(in1);
  out[1] = static_cast<float>(in2);
  out[2] = static_cast<float>(in3);
}

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(T* in, float* out, int noOfComponents)
{
  for (int i = 0; i < noOfComponents; ++i)
  {
    out[i] = static_cast<float>(in[i]);
  }
}

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(T (&in)[3], float (&out)[3])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
  out[2] = static_cast<float>(in[2]);
}

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(T (&in)[2], float (&out)[2])
{
  out[0] = static_cast<float>(in[0]);
  out[1] = static_cast<float>(in[1]);
}

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(T& in, float& out)
{
  out = static_cast<float>(in);
}

//----------------------------------------------------------------------------
template <typename T>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ToFloat(T (&in)[4][2], float (&out)[4][2])
{
  out[0][0] = static_cast<float>(in[0][0]);
  out[0][1] = static_cast<float>(in[0][1]);
  out[1][0] = static_cast<float>(in[1][0]);
  out[1][1] = static_cast<float>(in[1][1]);
  out[2][0] = static_cast<float>(in[2][0]);
  out[2][1] = static_cast<float>(in[2][1]);
  out[3][0] = static_cast<float>(in[3][0]);
  out[3][1] = static_cast<float>(in[3][1]);
}

//----------------------------------------------------------------------------
template <typename T, int SizeX, int SizeY>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CopyMatrixToVector(
  T* matrix, float* matrixVec, int offset)
{
  const int MatSize = SizeX * SizeY;
  for (int j = 0; j < MatSize; j++)
  {
    matrixVec[offset + j] = matrix->Element[j / SizeX][j % SizeY];
  }
}

//----------------------------------------------------------------------------
template <typename T, int SizeSrc>
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CopyVector(T* srcVec, T* dstVec, int offset)
{
  for (int j = 0; j < SizeSrc; j++)
  {
    dstVec[offset + j] = srcVec[j];
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetupMaskTransfer(vtkRenderer* ren)
{
  this->ReleaseGraphicsMaskTransfer(ren->GetRenderWindow());
  this->DeleteMaskTransfer();

  if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType &&
    !this->LabelMapTransfer2D)
  {
    this->LabelMapTransfer2D = vtkSmartPointer<vtkOpenGLVolumeMaskTransferFunction2D>::New();
    this->LabelMapGradientOpacity =
      vtkSmartPointer<vtkOpenGLVolumeMaskGradientOpacityTransferFunction2D>::New();
  }

  this->InitializationTime.Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RefreshMaskTransfer(
  vtkRenderer* ren, VolumeInput& input)
{
  auto vol = input.Volume;
  if (this->NeedToInitializeResources ||
    input.Volume->GetProperty()->GetMTime() > this->InitializationTime.GetMTime())
  {
    this->SetupMaskTransfer(ren);
  }
  this->UpdateMaskTransfer(ren, vol, 0);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateTransferFunctions(vtkRenderer* ren)
{
  int uniformIndex = 0;
  for (const auto& port : this->Parent->Ports)
  {
    auto& input = this->Parent->AssembledInputs[port];
    input.ColorRangeType = this->Parent->GetColorRangeType();
    input.ScalarOpacityRangeType = this->Parent->GetScalarOpacityRangeType();
    input.GradientOpacityRangeType = this->Parent->GetGradientOpacityRangeType();
    input.RefreshTransferFunction(
      ren, uniformIndex, this->Parent->BlendMode, this->ActualSampleDistance);

    uniformIndex++;
  }

  if (!this->MultiVolume)
  {
    this->RefreshMaskTransfer(ren, this->Parent->AssembledInputs[0]);
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadMask(vtkRenderer* ren)
{
  bool result = true;
  auto maskInput = this->Parent->MaskInput;
  if (maskInput)
  {
    if (!this->CurrentMask)
    {
      this->CurrentMask = vtkSmartPointer<vtkVolumeTexture>::New();

      const auto part = this->Partitions;
      this->CurrentMask->SetPartitions(part[0], part[1], part[2]);
    }

    int isCellData;
    vtkDataArray* arr = this->Parent->GetScalars(maskInput, this->Parent->ScalarMode,
      this->Parent->ArrayAccessMode, this->Parent->ArrayId, this->Parent->ArrayName, isCellData);
    if (maskInput->GetMTime() > this->MaskUpdateTime ||
      this->CurrentMask->GetLoadedScalars() != arr ||
      (arr && arr->GetMTime() > this->MaskUpdateTime))
    {
      result =
        this->CurrentMask->LoadVolume(ren, maskInput, arr, isCellData, VTK_NEAREST_INTERPOLATION);

      this->MaskUpdateTime.Modified();
    }
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ReleaseGraphicsMaskTransfer(vtkWindow* window)
{
  if (this->LabelMapTransfer2D)
  {
    this->LabelMapTransfer2D->ReleaseGraphicsResources(window);
  }
  if (this->LabelMapGradientOpacity)
  {
    this->LabelMapGradientOpacity->ReleaseGraphicsResources(window);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::DeleteMaskTransfer()
{
  this->LabelMapTransfer2D = nullptr;
  this->LabelMapGradientOpacity = nullptr;
}

//----------------------------------------------------------------------------
int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateMaskTransfer(
  vtkRenderer* ren, vtkVolume* vol, unsigned int component)
{
  vtkVolumeProperty* volumeProperty = vol->GetProperty();

  auto volumeTex = this->Parent->AssembledInputs[0].Texture.GetPointer();
  double componentRange[2];
  for (int i = 0; i < 2; ++i)
  {
    componentRange[i] = volumeTex->ScalarRange[component][i];
  }

  if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
  {
    this->LabelMapTransfer2D->Update(volumeProperty, componentRange, 0, 0, 0,
      vtkTextureObject::Nearest, vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

    if (volumeProperty->HasLabelGradientOpacity())
    {
      this->LabelMapGradientOpacity->Update(volumeProperty, componentRange, 0, 0, 0,
        vtkTextureObject::Nearest, vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CaptureDepthTexture(vtkRenderer* ren)
{
  // Make sure our render window is the current OpenGL context
  ren->GetRenderWindow()->MakeCurrent();

  // Load required extensions for grabbing depth sampler buffer
  if (!this->LoadDepthTextureExtensionsSucceeded)
  {
    this->LoadRequireDepthTextureExtensions(ren->GetRenderWindow());
  }

  // If we can't load the necessary extensions, provide
  // feedback on why it failed.
  if (!this->LoadDepthTextureExtensionsSucceeded)
  {
    std::cerr << this->ExtensionsStringStream.str() << std::endl;
    return;
  }

  if (!this->DepthTextureObject)
  {
    this->DepthTextureObject = vtkTextureObject::New();
  }

  this->DepthTextureObject->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

  //  this->DepthTextureObject->Activate();
  if (!this->DepthTextureObject->GetHandle())
  {
    // First set the parameters
    this->DepthTextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetWrapT(vtkTextureObject::ClampToEdge);
    this->DepthTextureObject->SetMagnificationFilter(vtkTextureObject::Linear);
    this->DepthTextureObject->SetMinificationFilter(vtkTextureObject::Linear);
    this->DepthTextureObject->AllocateDepth(this->WindowSize[0], this->WindowSize[1], 4);
  }

#ifndef GL_ES_VERSION_3_0
  // currently broken on ES
  this->DepthTextureObject->CopyFromFrameBuffer(this->WindowLowerLeft[0], this->WindowLowerLeft[1],
    0, 0, this->WindowSize[0], this->WindowSize[1]);
#endif
  //  this->DepthTextureObject->Deactivate();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetLightingShaderParameters(
  vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol, int numberOfSamplers)
{
  // Set basic lighting parameters (per component)
  if (!ren || !prog || !vol)
  {
    return;
  }

  auto volumeProperty = vol->GetProperty();
  float ambient[4][3];
  float diffuse[4][3];
  float specular[4][3];
  float specularPower[4];

  for (int i = 0; i < numberOfSamplers; ++i)
  {
    ambient[i][0] = ambient[i][1] = ambient[i][2] = volumeProperty->GetAmbient(i);
    diffuse[i][0] = diffuse[i][1] = diffuse[i][2] = volumeProperty->GetDiffuse(i);
    specular[i][0] = specular[i][1] = specular[i][2] = volumeProperty->GetSpecular(i);
    specularPower[i] = volumeProperty->GetSpecularPower(i);
  }

  prog->SetUniform3fv("in_ambient", numberOfSamplers, ambient);
  prog->SetUniform3fv("in_diffuse", numberOfSamplers, diffuse);
  prog->SetUniform3fv("in_specular", numberOfSamplers, specular);
  prog->SetUniform1fv("in_shininess", numberOfSamplers, specularPower);

  // Set advanced lighting features
  if (vol && !vol->GetProperty()->GetShade())
  {
    return;
  }

  prog->SetUniformi("in_twoSidedLighting", ren->GetTwoSidedLighting());

  // for lightkit case there are some parameters to set
  vtkCamera* cam = ren->GetActiveCamera();
  vtkTransform* viewTF = cam->GetModelViewTransformObject();

  // Bind some light settings
  int numberOfLights = 0;
  vtkLightCollection* lc = ren->GetLights();
  vtkLight* light;

  vtkCollectionSimpleIterator sit;
  float lightAmbientColor[6][3];
  float lightDiffuseColor[6][3];
  float lightSpecularColor[6][3];
  float lightDirection[6][3];
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    float status = light->GetSwitch();
    if (status > 0.0)
    {
      double* aColor = light->GetAmbientColor();
      double* dColor = light->GetDiffuseColor();
      double* sColor = light->GetDiffuseColor();
      double intensity = light->GetIntensity();
      lightAmbientColor[numberOfLights][0] = aColor[0] * intensity;
      lightAmbientColor[numberOfLights][1] = aColor[1] * intensity;
      lightAmbientColor[numberOfLights][2] = aColor[2] * intensity;
      lightDiffuseColor[numberOfLights][0] = dColor[0] * intensity;
      lightDiffuseColor[numberOfLights][1] = dColor[1] * intensity;
      lightDiffuseColor[numberOfLights][2] = dColor[2] * intensity;
      lightSpecularColor[numberOfLights][0] = sColor[0] * intensity;
      lightSpecularColor[numberOfLights][1] = sColor[1] * intensity;
      lightSpecularColor[numberOfLights][2] = sColor[2] * intensity;
      // Get required info from light
      double* lfp = light->GetTransformedFocalPoint();
      double* lp = light->GetTransformedPosition();
      double lightDir[3];
      vtkMath::Subtract(lfp, lp, lightDir);
      vtkMath::Normalize(lightDir);
      double* tDir = viewTF->TransformNormal(lightDir);
      lightDirection[numberOfLights][0] = tDir[0];
      lightDirection[numberOfLights][1] = tDir[1];
      lightDirection[numberOfLights][2] = tDir[2];
      numberOfLights++;
    }
  }

  prog->SetUniform3fv("in_lightAmbientColor", numberOfLights, lightAmbientColor);
  prog->SetUniform3fv("in_lightDiffuseColor", numberOfLights, lightDiffuseColor);
  prog->SetUniform3fv("in_lightSpecularColor", numberOfLights, lightSpecularColor);
  prog->SetUniform3fv("in_lightDirection", numberOfLights, lightDirection);
  prog->SetUniformi("in_numberOfLights", numberOfLights);

  // we are done unless we have positional lights
  if (this->LightComplexity < 3)
  {
    return;
  }

  // if positional lights pass down more parameters
  float lightAttenuation[6][3];
  float lightPosition[6][3];
  float lightConeAngle[6];
  float lightExponent[6];
  int lightPositional[6];
  numberOfLights = 0;
  for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
  {
    float status = light->GetSwitch();
    if (status > 0.0)
    {
      double* attn = light->GetAttenuationValues();
      lightAttenuation[numberOfLights][0] = attn[0];
      lightAttenuation[numberOfLights][1] = attn[1];
      lightAttenuation[numberOfLights][2] = attn[2];
      lightExponent[numberOfLights] = light->GetExponent();
      lightConeAngle[numberOfLights] = light->GetConeAngle();
      double* lp = light->GetTransformedPosition();
      double* tlp = viewTF->TransformPoint(lp);
      lightPosition[numberOfLights][0] = tlp[0];
      lightPosition[numberOfLights][1] = tlp[1];
      lightPosition[numberOfLights][2] = tlp[2];
      lightPositional[numberOfLights] = light->GetPositional();
      numberOfLights++;
    }
  }
  prog->SetUniform3fv("in_lightAttenuation", numberOfLights, lightAttenuation);
  prog->SetUniform1iv("in_lightPositional", numberOfLights, lightPositional);
  prog->SetUniform3fv("in_lightPosition", numberOfLights, lightPosition);
  prog->SetUniform1fv("in_lightExponent", numberOfLights, lightExponent);
  prog->SetUniform1fv("in_lightConeAngle", numberOfLights, lightConeAngle);
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsCameraInside(
  vtkRenderer* ren, vtkVolume* vol, double geometry[24])
{
  vtkNew<vtkMatrix4x4> dataToWorld;
  dataToWorld->DeepCopy(vol->GetMatrix());

  vtkCamera* cam = ren->GetActiveCamera();

  double planes[24];
  cam->GetFrustumPlanes(ren->GetTiledAspectRatio(), planes);

  // convert geometry to world then compare to frustum planes
  double in[4];
  in[3] = 1.0;
  double out[4];
  double worldGeometry[24];
  for (int i = 0; i < 8; ++i)
  {
    in[0] = geometry[i * 3];
    in[1] = geometry[i * 3 + 1];
    in[2] = geometry[i * 3 + 2];
    dataToWorld->MultiplyPoint(in, out);
    worldGeometry[i * 3] = out[0] / out[3];
    worldGeometry[i * 3 + 1] = out[1] / out[3];
    worldGeometry[i * 3 + 2] = out[2] / out[3];
  }

  // does the front clipping plane intersect the volume?
  // true if points are on both sides of the plane
  bool hasPositive = false;
  bool hasNegative = false;
  bool hasZero = false;
  for (int i = 0; i < 8; ++i)
  {
    double val = planes[4 * 4] * worldGeometry[i * 3] +
      planes[4 * 4 + 1] * worldGeometry[i * 3 + 1] + planes[4 * 4 + 2] * worldGeometry[i * 3 + 2] +
      planes[4 * 4 + 3];
    if (val < 0)
    {
      hasNegative = true;
    }
    else if (val > 0)
    {
      hasPositive = true;
    }
    else
    {
      hasZero = true;
    }
  }

  return hasZero || (hasNegative && hasPositive);
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::IsGeometryUpdateRequired(
  vtkRenderer* ren, vtkVolume* vol, double geometry[24])
{
  if (!this->BBoxPolyData)
  {
    return true;
  }

  using namespace std;
  const auto GeomTime = this->BBoxPolyData->GetMTime();
  const bool uploadTimeChanged = any_of(this->Parent->AssembledInputs.begin(),
    this->Parent->AssembledInputs.end(), [&GeomTime](const pair<int, vtkVolumeInputHelper>& item) {
      return item.second.Texture->UploadTime > GeomTime;
    });

  return (this->NeedToInitializeResources || uploadTimeChanged ||
    this->IsCameraInside(ren, vol, geometry) || this->CameraWasInsideInLastUpdate ||
    (this->MultiVolume && this->MultiVolume->GetBoundsTime() > this->BBoxPolyData->GetMTime()));
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RenderVolumeGeometry(
  vtkRenderer* ren, vtkShaderProgram* prog, vtkVolume* vol, double geometry[24])
{
  if (this->IsGeometryUpdateRequired(ren, vol, geometry))
  {
    vtkNew<vtkPolyData> boxSource;

    {
      vtkNew<vtkCellArray> cells;
      vtkNew<vtkPoints> points;
      points->SetDataTypeToDouble();
      for (int i = 0; i < 8; ++i)
      {
        points->InsertNextPoint(geometry + i * 3);
      }
      // 6 faces 12 triangles
      int tris[36] = {
        0, 1, 2, //
        1, 3, 2, //
        1, 5, 3, //
        5, 7, 3, //
        5, 4, 7, //
        4, 6, 7, //
        4, 0, 6, //
        0, 2, 6, //
        2, 3, 6, //
        3, 7, 6, //
        0, 4, 1, //
        1, 4, 5  //
      };
      for (int i = 0; i < 12; ++i)
      {
        cells->InsertNextCell(3);
        // this code uses a clockwise convention for some reason
        // no clue why but the ClipConvexPolyData assumes the same
        // so we add verts as 0 2 1 instead of 0 1 2
        cells->InsertCellPoint(tris[i * 3]);
        cells->InsertCellPoint(tris[i * 3 + 2]);
        cells->InsertCellPoint(tris[i * 3 + 1]);
      }
      boxSource->SetPoints(points);
      boxSource->SetPolys(cells);
    }

    vtkNew<vtkDensifyPolyData> densifyPolyData;
    if (this->IsCameraInside(ren, vol, geometry))
    {
      vtkNew<vtkMatrix4x4> dataToWorld;
      dataToWorld->DeepCopy(vol->GetMatrix());

      vtkCamera* cam = ren->GetActiveCamera();

      double fplanes[24];
      cam->GetFrustumPlanes(ren->GetTiledAspectRatio(), fplanes);

      // have to convert the 5th plane to volume coordinates
      double pOrigin[4];
      pOrigin[3] = 1.0;
      double pNormal[3];
      for (int i = 0; i < 3; ++i)
      {
        pNormal[i] = fplanes[16 + i];
        pOrigin[i] = -fplanes[16 + 3] * fplanes[16 + i];
      }

      // convert the normal
      double* dmat = dataToWorld->GetData();
      dataToWorld->Transpose();
      double pNormalV[3];
      pNormalV[0] = pNormal[0] * dmat[0] + pNormal[1] * dmat[1] + pNormal[2] * dmat[2];
      pNormalV[1] = pNormal[0] * dmat[4] + pNormal[1] * dmat[5] + pNormal[2] * dmat[6];
      pNormalV[2] = pNormal[0] * dmat[8] + pNormal[1] * dmat[9] + pNormal[2] * dmat[10];
      vtkMath::Normalize(pNormalV);

      // convert the point
      dataToWorld->Transpose();
      dataToWorld->Invert();
      dataToWorld->MultiplyPoint(pOrigin, pOrigin);

      vtkNew<vtkPlane> nearPlane;

      // We add an offset to the near plane to avoid hardware clipping of the
      // near plane due to floating-point precision.
      // camPlaneNormal is a unit vector, if the offset is larger than the
      // distance between near and far point, it will not work. Hence, we choose
      // a fraction of the near-far distance. However, care should be taken
      // to avoid hardware clipping in volumes with very small spacing where the
      // distance between near and far plane is also very small. In that case,
      // a minimum offset is chosen. This is chosen based on the typical
      // epsilon values on x86 systems.
      double offset = (cam->GetClippingRange()[1] - cam->GetClippingRange()[0]) * 0.001;
      // Minimum offset to avoid floating point precision issues for volumes
      // with very small spacing
      double minOffset = static_cast<double>(std::numeric_limits<float>::epsilon()) * 1000.0;
      offset = offset < minOffset ? minOffset : offset;

      for (int i = 0; i < 3; ++i)
      {
        pOrigin[i] += (pNormalV[i] * offset);
      }

      nearPlane->SetOrigin(pOrigin);
      nearPlane->SetNormal(pNormalV);

      vtkNew<vtkPlaneCollection> planes;
      planes->RemoveAllItems();
      planes->AddItem(nearPlane);

      vtkNew<vtkClipConvexPolyData> clip;
      clip->SetInputData(boxSource);
      clip->SetPlanes(planes);

      densifyPolyData->SetInputConnection(clip->GetOutputPort());

      this->CameraWasInsideInLastUpdate = true;
    }
    else
    {
      densifyPolyData->SetInputData(boxSource);
      this->CameraWasInsideInLastUpdate = false;
    }

    densifyPolyData->SetNumberOfSubdivisions(2);
    densifyPolyData->Update();

    this->BBoxPolyData = vtkSmartPointer<vtkPolyData>::New();
    this->BBoxPolyData->ShallowCopy(densifyPolyData->GetOutput());
    vtkPoints* points = this->BBoxPolyData->GetPoints();
    vtkCellArray* cells = this->BBoxPolyData->GetPolys();

    vtkNew<vtkUnsignedIntArray> polys;
    polys->SetNumberOfComponents(3);
    vtkIdType npts;
    const vtkIdType* pts;

    // See if the volume transform is orientation-preserving
    // and orient polygons accordingly
    vtkMatrix4x4* volMat = vol->GetMatrix();
    double det = vtkMath::Determinant3x3(volMat->GetElement(0, 0), volMat->GetElement(0, 1),
      volMat->GetElement(0, 2), volMat->GetElement(1, 0), volMat->GetElement(1, 1),
      volMat->GetElement(1, 2), volMat->GetElement(2, 0), volMat->GetElement(2, 1),
      volMat->GetElement(2, 2));
    bool preservesOrientation = det > 0.0;

    const vtkIdType indexMap[3] = { preservesOrientation ? 0 : 2, 1, preservesOrientation ? 2 : 0 };

    while (cells->GetNextCell(npts, pts))
    {
      polys->InsertNextTuple3(pts[indexMap[0]], pts[indexMap[1]], pts[indexMap[2]]);
    }

    // Dispose any previously created buffers
    this->DeleteBufferObjects();

    // Now create new ones
    this->CreateBufferObjects();

    // TODO: should really use the built in VAO class
    glBindVertexArray(this->CubeVAOId);

    // Pass cube vertices to buffer object memory
    glBindBuffer(GL_ARRAY_BUFFER, this->CubeVBOId);
    glBufferData(GL_ARRAY_BUFFER,
      points->GetData()->GetDataSize() * points->GetData()->GetDataTypeSize(),
      points->GetData()->GetVoidPointer(0), GL_STATIC_DRAW);

    prog->EnableAttributeArray("in_vertexPos");
    prog->UseAttributeArray("in_vertexPos", 0, 0, VTK_FLOAT, 3, vtkShaderProgram::NoNormalize);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, polys->GetDataSize() * polys->GetDataTypeSize(),
      polys->GetVoidPointer(0), GL_STATIC_DRAW);
  }
  else
  {
    glBindVertexArray(this->CubeVAOId);
  }

  glDrawElements(
    GL_TRIANGLES, this->BBoxPolyData->GetNumberOfCells() * 3, GL_UNSIGNED_INT, nullptr);

  vtkOpenGLStaticCheckErrorMacro("Error after glDrawElements in"
                                 " RenderVolumeGeometry!");
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetCroppingRegions(
  vtkShaderProgram* prog, double loadedBounds[6])
{
  if (this->Parent->GetCropping())
  {
    int cropFlags = this->Parent->GetCroppingRegionFlags();
    double croppingRegionPlanes[6];
    this->Parent->GetCroppingRegionPlanes(croppingRegionPlanes);

    // Clamp it
    croppingRegionPlanes[0] =
      croppingRegionPlanes[0] < loadedBounds[0] ? loadedBounds[0] : croppingRegionPlanes[0];
    croppingRegionPlanes[0] =
      croppingRegionPlanes[0] > loadedBounds[1] ? loadedBounds[1] : croppingRegionPlanes[0];
    croppingRegionPlanes[1] =
      croppingRegionPlanes[1] < loadedBounds[0] ? loadedBounds[0] : croppingRegionPlanes[1];
    croppingRegionPlanes[1] =
      croppingRegionPlanes[1] > loadedBounds[1] ? loadedBounds[1] : croppingRegionPlanes[1];

    croppingRegionPlanes[2] =
      croppingRegionPlanes[2] < loadedBounds[2] ? loadedBounds[2] : croppingRegionPlanes[2];
    croppingRegionPlanes[2] =
      croppingRegionPlanes[2] > loadedBounds[3] ? loadedBounds[3] : croppingRegionPlanes[2];
    croppingRegionPlanes[3] =
      croppingRegionPlanes[3] < loadedBounds[2] ? loadedBounds[2] : croppingRegionPlanes[3];
    croppingRegionPlanes[3] =
      croppingRegionPlanes[3] > loadedBounds[3] ? loadedBounds[3] : croppingRegionPlanes[3];

    croppingRegionPlanes[4] =
      croppingRegionPlanes[4] < loadedBounds[4] ? loadedBounds[4] : croppingRegionPlanes[4];
    croppingRegionPlanes[4] =
      croppingRegionPlanes[4] > loadedBounds[5] ? loadedBounds[5] : croppingRegionPlanes[4];
    croppingRegionPlanes[5] =
      croppingRegionPlanes[5] < loadedBounds[4] ? loadedBounds[4] : croppingRegionPlanes[5];
    croppingRegionPlanes[5] =
      croppingRegionPlanes[5] > loadedBounds[5] ? loadedBounds[5] : croppingRegionPlanes[5];

    float cropPlanes[6] = { static_cast<float>(croppingRegionPlanes[0]),
      static_cast<float>(croppingRegionPlanes[1]), static_cast<float>(croppingRegionPlanes[2]),
      static_cast<float>(croppingRegionPlanes[3]), static_cast<float>(croppingRegionPlanes[4]),
      static_cast<float>(croppingRegionPlanes[5]) };

    prog->SetUniform1fv("in_croppingPlanes", 6, cropPlanes);
    const int numberOfRegions = 32;
    int cropFlagsArray[numberOfRegions];
    cropFlagsArray[0] = 0;
    int i = 1;
    while (cropFlags && i < 32)
    {
      cropFlagsArray[i] = cropFlags & 1;
      cropFlags = cropFlags >> 1;
      ++i;
    }
    for (; i < 32; ++i)
    {
      cropFlagsArray[i] = 0;
    }

    prog->SetUniform1iv("in_croppingFlags", numberOfRegions, cropFlagsArray);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetClippingPlanes(
  vtkRenderer* vtkNotUsed(ren), vtkShaderProgram* prog, vtkVolume* vol)
{
  if (this->Parent->GetClippingPlanes())
  {
    std::vector<float> clippingPlanes;
    // Currently we don't have any clipping plane
    clippingPlanes.push_back(0);

    this->Parent->ClippingPlanes->InitTraversal();
    vtkPlane* plane;
    while ((plane = this->Parent->ClippingPlanes->GetNextItem()))
    {
      // Planes are in world coordinates
      double planeOrigin[3], planeNormal[3];
      plane->GetOrigin(planeOrigin);
      plane->GetNormal(planeNormal);

      clippingPlanes.push_back(planeOrigin[0]);
      clippingPlanes.push_back(planeOrigin[1]);
      clippingPlanes.push_back(planeOrigin[2]);
      clippingPlanes.push_back(planeNormal[0]);
      clippingPlanes.push_back(planeNormal[1]);
      clippingPlanes.push_back(planeNormal[2]);
    }

    clippingPlanes[0] = clippingPlanes.size() > 1 ? static_cast<int>(clippingPlanes.size() - 1) : 0;

    prog->SetUniform1fv(
      "in_clippingPlanes", static_cast<int>(clippingPlanes.size()), &clippingPlanes[0]);
    float clippedVoxelIntensity =
      static_cast<float>(vol->GetProperty()->GetClippedVoxelIntensity());
    prog->SetUniformf("in_clippedVoxelIntensity", clippedVoxelIntensity);
  }
}

// -----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CheckPropertyKeys(vtkVolume* vol)
{
  // Check the property keys to see if we should modify the blend/etc state:
  // Otherwise this breaks volume/translucent geo depth peeling.
  vtkInformation* volumeKeys = vol->GetPropertyKeys();
  this->PreserveGLState = false;
  if (volumeKeys && volumeKeys->Has(vtkOpenGLActor::GLDepthMaskOverride()))
  {
    int override = volumeKeys->Get(vtkOpenGLActor::GLDepthMaskOverride());
    if (override != 0 && override != 1)
    {
      this->PreserveGLState = true;
    }
  }

  // Some render passes (e.g. DualDepthPeeling) adjust the viewport for
  // intermediate passes so it is necessary to preserve it. This is a
  // temporary fix for vtkDualDepthPeelingPass to work when various viewports
  // are defined.  The correct way of fixing this would be to avoid setting the
  // viewport within the mapper.  It is enough for now to check for the
  // RenderPasses() vtkInfo given that vtkDualDepthPeelingPass is the only pass
  // currently supported by this mapper, the viewport will have to be adjusted
  // externally before adding support for other passes.
  vtkInformation* info = vol->GetPropertyKeys();
  this->PreserveViewport = info && info->Has(vtkOpenGLRenderPass::RenderPasses());
}

// -----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CheckPickingState(vtkRenderer* ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  bool selectorPicking = selector != nullptr;
  if (selector)
  {
    // this mapper currently only supports cell picking
    selectorPicking &= selector->GetFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS;
  }

  this->IsPicking = selectorPicking;
  if (this->IsPicking)
  {
    // rebuild the shader on every pass
    this->SelectionStateTime.Modified();
    this->CurrentSelectionPass =
      selector ? selector->GetCurrentPass() : vtkHardwareSelector::ACTOR_PASS;
  }
  else if (this->CurrentSelectionPass != vtkHardwareSelector::MIN_KNOWN_PASS - 1)
  {
    // return to the regular rendering state
    this->SelectionStateTime.Modified();
    this->CurrentSelectionPass = vtkHardwareSelector::MIN_KNOWN_PASS - 1;
  }
}

// -----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::BeginPicking(vtkRenderer* ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->IsPicking)
  {
    selector->BeginRenderProp();
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetPickingId(vtkRenderer* ren)
{
  float propIdColor[3] = { 0.0, 0.0, 0.0 };
  vtkHardwareSelector* selector = ren->GetSelector();

  if (selector && this->IsPicking)
  {
    // query the selector for the appropriate id
    selector->GetPropColorValue(propIdColor);
  }

  this->ShaderProgram->SetUniform3f("in_propId", propIdColor);
}

// ---------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::EndPicking(vtkRenderer* ren)
{
  vtkHardwareSelector* selector = ren->GetSelector();
  if (selector && this->IsPicking)
  {
    if (this->CurrentSelectionPass >= vtkHardwareSelector::POINT_ID_LOW24)
    {
      // Only supported on single-input
      int extents[6];
      this->Parent->GetTransformedInput(0)->GetExtent(extents);

      // Tell the selector the maximum number of cells that the mapper could render
      unsigned int const numVoxels = (extents[1] - extents[0] + 1) * (extents[3] - extents[2] + 1) *
        (extents[5] - extents[4] + 1);
      selector->UpdateMaximumPointId(numVoxels);
      selector->UpdateMaximumCellId(numVoxels);
    }
    selector->EndRenderProp();
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateSamplingDistance(
  vtkRenderer* vtkNotUsed(ren))
{
  auto input = this->Parent->GetTransformedInput(0);
  auto vol = this->Parent->AssembledInputs[0].Volume;
  double cellSpacing[3];
  input->GetSpacing(cellSpacing);

  if (!this->Parent->AutoAdjustSampleDistances)
  {
    if (this->Parent->LockSampleDistanceToInputSpacing)
    {
      int extents[6];
      input->GetExtent(extents);

      float const d =
        static_cast<float>(this->Parent->SpacingAdjustedSampleDistance(cellSpacing, extents));
      float const sample = this->Parent->SampleDistance;

      // ActualSampleDistance will grow proportionally to numVoxels^(1/3) (see
      // vtkVolumeMapper.cxx). Until it reaches 1/2 average voxel size when number of
      // voxels is 1E6.
      this->ActualSampleDistance =
        (sample / d < 0.999f || sample / d > 1.001f) ? d : this->Parent->SampleDistance;

      return;
    }

    this->ActualSampleDistance = this->Parent->SampleDistance;
  }
  else
  {
    input->GetSpacing(cellSpacing);
    vtkMatrix4x4* worldToDataset = vol->GetMatrix();
    double minWorldSpacing = VTK_DOUBLE_MAX;
    int i = 0;
    while (i < 3)
    {
      double tmp = worldToDataset->GetElement(0, i);
      double tmp2 = tmp * tmp;
      tmp = worldToDataset->GetElement(1, i);
      tmp2 += tmp * tmp;
      tmp = worldToDataset->GetElement(2, i);
      tmp2 += tmp * tmp;

      // We use fabs() in case the spacing is negative.
      double worldSpacing = fabs(cellSpacing[i] * sqrt(tmp2));
      if (worldSpacing < minWorldSpacing)
      {
        minWorldSpacing = worldSpacing;
      }
      ++i;
    }

    // minWorldSpacing is the optimal sample distance in world space.
    // To go faster (reduceFactor<1.0), we multiply this distance
    // by 1/reduceFactor.
    this->ActualSampleDistance = static_cast<float>(minWorldSpacing);

    if (this->Parent->ReductionFactor < 1.0 && this->Parent->ReductionFactor != 0.0)
    {
      this->ActualSampleDistance /= static_cast<GLfloat>(this->Parent->ReductionFactor);
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::LoadRequireDepthTextureExtensions(
  vtkRenderWindow* vtkNotUsed(renWin))
{
  // Reset the message stream for extensions
  this->LoadDepthTextureExtensionsSucceeded = true;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::CreateBufferObjects()
{
  glGenVertexArrays(1, &this->CubeVAOId);
  glGenBuffers(1, &this->CubeVBOId);
  glGenBuffers(1, &this->CubeIndicesId);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::DeleteBufferObjects()
{
  if (this->CubeVBOId)
  {
    glBindBuffer(GL_ARRAY_BUFFER, this->CubeVBOId);
    glDeleteBuffers(1, &this->CubeVBOId);
    this->CubeVBOId = 0;
  }

  if (this->CubeIndicesId)
  {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->CubeIndicesId);
    glDeleteBuffers(1, &this->CubeIndicesId);
    this->CubeIndicesId = 0;
  }

  if (this->CubeVAOId)
  {
    glDeleteVertexArrays(1, &this->CubeVAOId);
    this->CubeVAOId = 0;
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ConvertTextureToImageData(
  vtkTextureObject* texture, vtkImageData* output)
{
  if (!texture)
  {
    return;
  }
  unsigned int tw = texture->GetWidth();
  unsigned int th = texture->GetHeight();
  unsigned int tnc = texture->GetComponents();
  int tt = texture->GetVTKDataType();

  vtkPixelExtent texExt(0U, tw - 1U, 0U, th - 1U);

  int dataExt[6] = { 0, 0, 0, 0, 0, 0 };
  texExt.GetData(dataExt);

  double dataOrigin[6] = { 0, 0, 0, 0, 0, 0 };

  vtkImageData* id = vtkImageData::New();
  id->SetOrigin(dataOrigin);
  id->SetDimensions(tw, th, 1);
  id->SetExtent(dataExt);
  id->AllocateScalars(tt, tnc);

  vtkPixelBufferObject* pbo = texture->Download();

  vtkPixelTransfer::Blit(texExt, texExt, texExt, texExt, tnc, tt, pbo->MapPackedBuffer(), tnc, tt,
    id->GetScalarPointer(0, 0, 0));

  pbo->UnmapPackedBuffer();
  pbo->Delete();

  if (!output)
  {
    output = vtkImageData::New();
  }
  output->DeepCopy(id);
  id->Delete();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::BeginImageSample(vtkRenderer* ren)
{
  auto vol = this->GetActiveVolume();
  const auto numBuffers = this->GetNumImageSampleDrawBuffers(vol);
  if (numBuffers != this->NumImageSampleDrawBuffers)
  {
    if (numBuffers > this->NumImageSampleDrawBuffers)
    {
      this->ReleaseImageSampleGraphicsResources(ren->GetRenderWindow());
    }

    this->NumImageSampleDrawBuffers = numBuffers;
    this->RebuildImageSampleProg = true;
  }

  float const xySampleDist = this->Parent->ImageSampleDistance;
  if (xySampleDist != 1.f && this->InitializeImageSampleFBO(ren))
  {
    this->ImageSampleFBO->GetContext()->GetState()->PushDrawFramebufferBinding();
    this->ImageSampleFBO->Bind(GL_DRAW_FRAMEBUFFER);
    this->ImageSampleFBO->ActivateDrawBuffers(
      static_cast<unsigned int>(this->NumImageSampleDrawBuffers));

    this->ImageSampleFBO->GetContext()->GetState()->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
    this->ImageSampleFBO->GetContext()->GetState()->vtkglClear(GL_COLOR_BUFFER_BIT);
  }
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::InitializeImageSampleFBO(vtkRenderer* ren)
{
  // Set the FBO viewport size. These are used in the shader to normalize the
  // fragment coordinate, the normalized coordinate is used to fetch the depth
  // buffer.
  this->WindowSize[0] /= this->Parent->ImageSampleDistance;
  this->WindowSize[1] /= this->Parent->ImageSampleDistance;
  this->WindowLowerLeft[0] = 0;
  this->WindowLowerLeft[1] = 0;

  vtkOpenGLRenderWindow* win = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());

  // Set FBO viewport
  win->GetState()->vtkglViewport(
    this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);

  if (!this->ImageSampleFBO)
  {
    this->ImageSampleTexture.reserve(this->NumImageSampleDrawBuffers);
    this->ImageSampleTexNames.reserve(this->NumImageSampleDrawBuffers);
    for (size_t i = 0; i < this->NumImageSampleDrawBuffers; i++)
    {
      auto tex = vtkSmartPointer<vtkTextureObject>::New();
      tex->SetContext(win);
      tex->Create2D(this->WindowSize[0], this->WindowSize[1], 4, VTK_UNSIGNED_CHAR, false);
      tex->Activate();
      tex->SetMinificationFilter(vtkTextureObject::Linear);
      tex->SetMagnificationFilter(vtkTextureObject::Linear);
      tex->SetWrapS(vtkTextureObject::ClampToEdge);
      tex->SetWrapT(vtkTextureObject::ClampToEdge);
      this->ImageSampleTexture.push_back(tex);

      std::stringstream ss;
      ss << i;
      const std::string name = "renderedTex_" + ss.str();
      this->ImageSampleTexNames.push_back(name);
    }

    this->ImageSampleFBO = vtkOpenGLFramebufferObject::New();
    this->ImageSampleFBO->SetContext(win);
    win->GetState()->PushFramebufferBindings();
    this->ImageSampleFBO->Bind();
    this->ImageSampleFBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

    auto num = static_cast<unsigned int>(this->NumImageSampleDrawBuffers);
    for (unsigned int i = 0; i < num; i++)
    {
      this->ImageSampleFBO->AddColorAttachment(i, this->ImageSampleTexture[i]);
    }

    // Verify completeness
    const int complete = this->ImageSampleFBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    for (auto& tex : this->ImageSampleTexture)
    {
      tex->Deactivate();
    }
    win->GetState()->PopFramebufferBindings();

    if (!complete)
    {
      vtkGenericWarningMacro(<< "Failed to attach ImageSampleFBO!");
      this->ReleaseImageSampleGraphicsResources(win);
      return false;
    }

    this->RebuildImageSampleProg = true;
    return true;
  }

  // Resize if necessary
  int lastSize[2];
  this->ImageSampleFBO->GetLastSize(lastSize);
  if (lastSize[0] != this->WindowSize[0] || lastSize[1] != this->WindowSize[1])
  {
    this->ImageSampleFBO->Resize(this->WindowSize[0], this->WindowSize[1]);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::EndImageSample(vtkRenderer* ren)
{
  if (this->Parent->ImageSampleDistance != 1.f)
  {
    this->ImageSampleFBO->DeactivateDrawBuffers();
    if (this->RenderPassAttached)
    {
      this->ImageSampleFBO->ActivateDrawBuffers(
        static_cast<unsigned int>(this->NumImageSampleDrawBuffers));
    }
    vtkOpenGLRenderWindow* win = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
    win->GetState()->PopDrawFramebufferBinding();

    // Render the contents of ImageSampleFBO as a quad to intermix with the
    // rest of the scene.
    typedef vtkOpenGLRenderUtilities GLUtil;

    if (this->RebuildImageSampleProg)
    {
      std::string frag = GLUtil::GetFullScreenQuadFragmentShaderTemplate();

      vtkShaderProgram::Substitute(frag, "//VTK::FSQ::Decl",
        vtkvolume::ImageSampleDeclarationFrag(
          this->ImageSampleTexNames, this->NumImageSampleDrawBuffers));
      vtkShaderProgram::Substitute(frag, "//VTK::FSQ::Impl",
        vtkvolume::ImageSampleImplementationFrag(
          this->ImageSampleTexNames, this->NumImageSampleDrawBuffers));

      this->ImageSampleProg =
        win->GetShaderCache()->ReadyShaderProgram(GLUtil::GetFullScreenQuadVertexShader().c_str(),
          frag.c_str(), GLUtil::GetFullScreenQuadGeometryShader().c_str());
    }
    else
    {
      win->GetShaderCache()->ReadyShaderProgram(this->ImageSampleProg);
    }

    if (!this->ImageSampleProg)
    {
      vtkGenericWarningMacro(<< "Failed to initialize ImageSampleProgram!");
      return;
    }

    if (!this->ImageSampleVAO)
    {
      this->ImageSampleVAO = vtkOpenGLVertexArrayObject::New();
      GLUtil::PrepFullScreenVAO(win, this->ImageSampleVAO, this->ImageSampleProg);
    }

    vtkOpenGLState* ostate = win->GetState();

    // Adjust the GL viewport to VTK's defined viewport
    ren->GetTiledSizeAndOrigin(
      this->WindowSize, this->WindowSize + 1, this->WindowLowerLeft, this->WindowLowerLeft + 1);
    ostate->vtkglViewport(
      this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);

    // Bind objects and draw
    ostate->vtkglEnable(GL_BLEND);
    ostate->vtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ostate->vtkglDisable(GL_DEPTH_TEST);

    for (size_t i = 0; i < this->NumImageSampleDrawBuffers; i++)
    {
      this->ImageSampleTexture[i]->Activate();
      this->ImageSampleProg->SetUniformi(
        this->ImageSampleTexNames[i].c_str(), this->ImageSampleTexture[i]->GetTextureUnit());
    }

    this->ImageSampleVAO->Bind();
    GLUtil::DrawFullScreenQuad();
    this->ImageSampleVAO->Release();
    vtkOpenGLStaticCheckErrorMacro("Error after DrawFullScreenQuad()!");

    for (auto& tex : this->ImageSampleTexture)
    {
      tex->Deactivate();
    }
  }
}

//------------------------------------------------------------------------------
size_t vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::GetNumImageSampleDrawBuffers(vtkVolume* vol)
{
  if (this->RenderPassAttached)
  {
    vtkInformation* info = vol->GetPropertyKeys();
    const int num = info->Length(vtkOpenGLRenderPass::RenderPasses());
    vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), num - 1);
    vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
    return static_cast<size_t>(rp->GetActiveDrawBuffers());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetupRenderToTexture(vtkRenderer* ren)
{
  if (this->Parent->RenderToImage && this->Parent->CurrentPass == RenderPass)
  {
    if (this->Parent->ImageSampleDistance != 1.f)
    {
      this->WindowSize[0] /= this->Parent->ImageSampleDistance;
      this->WindowSize[1] /= this->Parent->ImageSampleDistance;
    }

    if ((this->LastRenderToImageWindowSize[0] != this->WindowSize[0]) ||
      (this->LastRenderToImageWindowSize[1] != this->WindowSize[1]))
    {
      this->LastRenderToImageWindowSize[0] = this->WindowSize[0];
      this->LastRenderToImageWindowSize[1] = this->WindowSize[1];
      this->ReleaseRenderToTextureGraphicsResources(ren->GetRenderWindow());
    }

    if (!this->FBO)
    {
      this->FBO = vtkOpenGLFramebufferObject::New();
    }

    vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
    this->FBO->SetContext(renWin);

    renWin->GetState()->PushFramebufferBindings();
    this->FBO->Bind();
    this->FBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

    int depthImageScalarType = this->Parent->GetDepthImageScalarType();
    bool initDepthTexture = true;
    // Re-instantiate the depth texture object if the scalar type requested has
    // changed from the last frame
    if (this->RTTDepthTextureObject && this->RTTDepthTextureType == depthImageScalarType)
    {
      initDepthTexture = false;
    }

    if (initDepthTexture)
    {
      if (this->RTTDepthTextureObject)
      {
        this->RTTDepthTextureObject->Delete();
        this->RTTDepthTextureObject = nullptr;
      }
      this->RTTDepthTextureObject = vtkTextureObject::New();
      this->RTTDepthTextureObject->SetContext(renWin);
      this->RTTDepthTextureObject->Create2D(
        this->WindowSize[0], this->WindowSize[1], 1, depthImageScalarType, false);
      this->RTTDepthTextureObject->Activate();
      this->RTTDepthTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
      this->RTTDepthTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
      this->RTTDepthTextureObject->SetAutoParameters(0);

      // Cache the value of the scalar type
      this->RTTDepthTextureType = depthImageScalarType;
    }

    if (!this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject = vtkTextureObject::New();

      this->RTTColorTextureObject->SetContext(
        vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
      this->RTTColorTextureObject->Create2D(
        this->WindowSize[0], this->WindowSize[1], 4, VTK_UNSIGNED_CHAR, false);
      this->RTTColorTextureObject->Activate();
      this->RTTColorTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
      this->RTTColorTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
      this->RTTColorTextureObject->SetAutoParameters(0);
    }

    if (!this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject = vtkTextureObject::New();
      this->RTTDepthBufferTextureObject->SetContext(renWin);
      this->RTTDepthBufferTextureObject->AllocateDepth(
        this->WindowSize[0], this->WindowSize[1], vtkTextureObject::Float32);
      this->RTTDepthBufferTextureObject->Activate();
      this->RTTDepthBufferTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
      this->RTTDepthBufferTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
      this->RTTDepthBufferTextureObject->SetAutoParameters(0);
    }

    this->FBO->Bind(GL_FRAMEBUFFER);
    this->FBO->AddDepthAttachment(this->RTTDepthBufferTextureObject);
    this->FBO->AddColorAttachment(0U, this->RTTColorTextureObject);
    this->FBO->AddColorAttachment(1U, this->RTTDepthTextureObject);
    this->FBO->ActivateDrawBuffers(2);

    this->FBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);

    this->FBO->GetContext()->GetState()->vtkglClearColor(1.0, 1.0, 1.0, 0.0);
    this->FBO->GetContext()->GetState()->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ExitRenderToTexture(vtkRenderer* vtkNotUsed(ren))
{
  if (this->Parent->RenderToImage && this->Parent->CurrentPass == RenderPass)
  {
    this->FBO->RemoveDepthAttachment();
    this->FBO->RemoveColorAttachment(0U);
    this->FBO->RemoveColorAttachment(1U);
    this->FBO->DeactivateDrawBuffers();
    this->FBO->GetContext()->GetState()->PopFramebufferBindings();

    this->RTTDepthBufferTextureObject->Deactivate();
    this->RTTColorTextureObject->Deactivate();
    this->RTTDepthTextureObject->Deactivate();
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetupDepthPass(vtkRenderer* ren)
{
  if (this->Parent->ImageSampleDistance != 1.f)
  {
    this->WindowSize[0] /= this->Parent->ImageSampleDistance;
    this->WindowSize[1] /= this->Parent->ImageSampleDistance;
  }

  if ((this->LastDepthPassWindowSize[0] != this->WindowSize[0]) ||
    (this->LastDepthPassWindowSize[1] != this->WindowSize[1]))
  {
    this->LastDepthPassWindowSize[0] = this->WindowSize[0];
    this->LastDepthPassWindowSize[1] = this->WindowSize[1];
    this->ReleaseDepthPassGraphicsResources(ren->GetRenderWindow());
  }

  if (!this->DPFBO)
  {
    this->DPFBO = vtkOpenGLFramebufferObject::New();
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->DPFBO->SetContext(renWin);

  renWin->GetState()->PushFramebufferBindings();
  this->DPFBO->Bind();
  this->DPFBO->InitializeViewport(this->WindowSize[0], this->WindowSize[1]);

  if (!this->DPDepthBufferTextureObject || !this->DPColorTextureObject)
  {
    this->DPDepthBufferTextureObject = vtkTextureObject::New();
    this->DPDepthBufferTextureObject->SetContext(renWin);
    this->DPDepthBufferTextureObject->AllocateDepth(
      this->WindowSize[0], this->WindowSize[1], vtkTextureObject::Native);
    this->DPDepthBufferTextureObject->Activate();
    this->DPDepthBufferTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
    this->DPDepthBufferTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->DPDepthBufferTextureObject->SetAutoParameters(0);
    this->DPDepthBufferTextureObject->Bind();

    this->DPColorTextureObject = vtkTextureObject::New();

    this->DPColorTextureObject->SetContext(renWin);
    this->DPColorTextureObject->Create2D(
      this->WindowSize[0], this->WindowSize[1], 4, VTK_UNSIGNED_CHAR, false);
    this->DPColorTextureObject->Activate();
    this->DPColorTextureObject->SetMinificationFilter(vtkTextureObject::Nearest);
    this->DPColorTextureObject->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->DPColorTextureObject->SetAutoParameters(0);

    this->DPFBO->AddDepthAttachment(this->DPDepthBufferTextureObject);

    this->DPFBO->AddColorAttachment(0U, this->DPColorTextureObject);
  }

  this->DPFBO->ActivateDrawBuffers(1);
  this->DPFBO->CheckFrameBufferStatus(GL_FRAMEBUFFER);

  // Setup the contour polydata mapper to render to DPFBO
  this->ContourMapper->SetInputConnection(this->ContourFilter->GetOutputPort());

  vtkOpenGLState* ostate = this->DPFBO->GetContext()->GetState();
  ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ostate->vtkglEnable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RenderContourPass(vtkRenderer* ren)
{
  this->SetupDepthPass(ren);
  this->ContourActor->Render(ren, this->ContourMapper.GetPointer());
  this->ExitDepthPass(ren);
  this->DepthPassTime.Modified();
  this->Parent->CurrentPass = this->Parent->RenderPass;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ExitDepthPass(vtkRenderer* vtkNotUsed(ren))
{
  this->DPFBO->DeactivateDrawBuffers();
  vtkOpenGLState* ostate = this->DPFBO->GetContext()->GetState();
  ostate->PopFramebufferBindings();

  this->DPDepthBufferTextureObject->Deactivate();
  this->DPColorTextureObject->Deactivate();
  ostate->vtkglDisable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal ::ReleaseRenderToTextureGraphicsResources(
  vtkWindow* win)
{
  vtkOpenGLRenderWindow* rwin = vtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->FBO)
    {
      this->FBO->Delete();
      this->FBO = nullptr;
    }

    if (this->RTTDepthBufferTextureObject)
    {
      this->RTTDepthBufferTextureObject->ReleaseGraphicsResources(win);
      this->RTTDepthBufferTextureObject->Delete();
      this->RTTDepthBufferTextureObject = nullptr;
    }

    if (this->RTTDepthTextureObject)
    {
      this->RTTDepthTextureObject->ReleaseGraphicsResources(win);
      this->RTTDepthTextureObject->Delete();
      this->RTTDepthTextureObject = nullptr;
    }

    if (this->RTTColorTextureObject)
    {
      this->RTTColorTextureObject->ReleaseGraphicsResources(win);
      this->RTTColorTextureObject->Delete();
      this->RTTColorTextureObject = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal ::ReleaseDepthPassGraphicsResources(
  vtkWindow* win)
{
  vtkOpenGLRenderWindow* rwin = vtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->DPFBO)
    {
      this->DPFBO->Delete();
      this->DPFBO = nullptr;
    }

    if (this->DPDepthBufferTextureObject)
    {
      this->DPDepthBufferTextureObject->ReleaseGraphicsResources(win);
      this->DPDepthBufferTextureObject->Delete();
      this->DPDepthBufferTextureObject = nullptr;
    }

    if (this->DPColorTextureObject)
    {
      this->DPColorTextureObject->ReleaseGraphicsResources(win);
      this->DPColorTextureObject->Delete();
      this->DPColorTextureObject = nullptr;
    }

    this->ContourMapper->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal ::ReleaseImageSampleGraphicsResources(
  vtkWindow* win)
{
  vtkOpenGLRenderWindow* rwin = vtkOpenGLRenderWindow::SafeDownCast(win);

  if (rwin)
  {
    if (this->ImageSampleFBO)
    {
      this->ImageSampleFBO->Delete();
      this->ImageSampleFBO = nullptr;
    }

    for (auto& tex : this->ImageSampleTexture)
    {
      tex->ReleaseGraphicsResources(win);
      tex = nullptr;
    }
    this->ImageSampleTexture.clear();
    this->ImageSampleTexNames.clear();

    if (this->ImageSampleVAO)
    {
      this->ImageSampleVAO->Delete();
      this->ImageSampleVAO = nullptr;
    }

    // Do not delete the shader program - Let the cache clean it up.
    this->ImageSampleProg = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::vtkOpenGLGPUVolumeRayCastMapper()
  : vtkGPUVolumeRayCastMapper()
{
  this->Impl = new vtkInternal(this);
  this->ReductionFactor = 1.0;
  this->CurrentPass = RenderPass;

  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLGPUVolumeRayCastMapper>(
    this, &vtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources);

  //  this->VolumeTexture = vtkVolumeTexture::New();
  //  this->VolumeTexture->SetMapper(this);
}

//----------------------------------------------------------------------------
vtkOpenGLGPUVolumeRayCastMapper::~vtkOpenGLGPUVolumeRayCastMapper()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }

  delete this->Impl;
  this->Impl = nullptr;

  this->AssembledInputs.clear();
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ReductionFactor: " << this->ReductionFactor << "\n";
  os << indent << "CurrentPass: " << this->CurrentPass << "\n";
}

void vtkOpenGLGPUVolumeRayCastMapper::SetSharedDepthTexture(vtkTextureObject* nt)
{
  if (this->Impl->DepthTextureObject == nt)
  {
    return;
  }
  if (this->Impl->DepthTextureObject)
  {
    this->Impl->DepthTextureObject->Delete();
  }
  this->Impl->DepthTextureObject = nt;

  if (nt)
  {
    nt->Register(this); // as it will get deleted later on
    this->Impl->SharedDepthTextureObject = true;
  }
  else
  {
    this->Impl->SharedDepthTextureObject = false;
  }
}

//----------------------------------------------------------------------------
vtkTextureObject* vtkOpenGLGPUVolumeRayCastMapper::GetDepthTexture()
{
  return this->Impl->RTTDepthTextureObject;
}

//----------------------------------------------------------------------------
vtkTextureObject* vtkOpenGLGPUVolumeRayCastMapper::GetColorTexture()
{
  return this->Impl->RTTColorTextureObject;
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetDepthImage(vtkImageData* output)
{
  return this->Impl->ConvertTextureToImageData(this->Impl->RTTDepthTextureObject, output);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetColorImage(vtkImageData* output)
{
  return this->Impl->ConvertTextureToImageData(this->Impl->RTTColorTextureObject, output);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow* window)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->Impl->DeleteBufferObjects();

  for (auto& input : this->AssembledInputs)
  {
    input.second.ReleaseGraphicsResources(window);
  }

  if (this->Impl->DepthTextureObject && !this->Impl->SharedDepthTextureObject)
  {
    this->Impl->DepthTextureObject->ReleaseGraphicsResources(window);
    this->Impl->DepthTextureObject->Delete();
    this->Impl->DepthTextureObject = nullptr;
  }

  this->Impl->ReleaseRenderToTextureGraphicsResources(window);
  this->Impl->ReleaseDepthPassGraphicsResources(window);
  this->Impl->ReleaseImageSampleGraphicsResources(window);

  if (this->Impl->CurrentMask)
  {
    this->Impl->CurrentMask->ReleaseGraphicsResources(window);
    this->Impl->CurrentMask = nullptr;
  }

  this->Impl->ReleaseGraphicsMaskTransfer(window);
  this->Impl->DeleteMaskTransfer();

  this->Impl->ReleaseResourcesTime.Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GetShaderTemplate(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkOpenGLShaderProperty* p)
{
  if (shaders[vtkShader::Vertex])
  {
    if (p->HasVertexShaderCode())
    {
      shaders[vtkShader::Vertex]->SetSource(p->GetVertexShaderCode());
    }
    else
    {
      shaders[vtkShader::Vertex]->SetSource(raycastervs);
    }
  }

  if (shaders[vtkShader::Fragment])
  {
    if (p->HasFragmentShaderCode())
    {
      shaders[vtkShader::Fragment]->SetSource(p->GetFragmentShaderCode());
    }
    else
    {
      shaders[vtkShader::Fragment]->SetSource(raycasterfs);
    }
  }

  if (shaders[vtkShader::Geometry])
  {
    shaders[vtkShader::Geometry]->SetSource("");
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCustomUniforms(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkOpenGLShaderProperty* p)
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkOpenGLUniforms* vu = static_cast<vtkOpenGLUniforms*>(p->GetVertexCustomUniforms());
  vtkShaderProgram::Substitute(vertexShader, "//VTK::CustomUniforms::Dec", vu->GetDeclarations());

  vtkShader* fragmentShader = shaders[vtkShader::Fragment];
  vtkOpenGLUniforms* fu = static_cast<vtkOpenGLUniforms*>(p->GetFragmentCustomUniforms());
  vtkShaderProgram::Substitute(fragmentShader, "//VTK::CustomUniforms::Dec", fu->GetDeclarations());

  vtkShader* geometryShader = shaders[vtkShader::Geometry];
  vtkOpenGLUniforms* gu = static_cast<vtkOpenGLUniforms*>(p->GetGeometryCustomUniforms());
  vtkShaderProgram::Substitute(geometryShader, "//VTK::CustomUniforms::Dec", gu->GetDeclarations());
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderBase(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps)
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  vtkShaderProgram::Substitute(vertexShader, "//VTK::ComputeClipPos::Impl",
    vtkvolume::ComputeClipPositionImplementation(ren, this, vol));

  vtkShaderProgram::Substitute(vertexShader, "//VTK::ComputeTextureCoords::Impl",
    vtkvolume::ComputeTextureCoordinates(ren, this, vol));

  vtkShaderProgram::Substitute(vertexShader, "//VTK::Base::Dec",
    vtkvolume::BaseDeclarationVertex(ren, this, vol, this->Impl->MultiVolume != nullptr));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::CallWorker::Impl", vtkvolume::WorkerImplementation(ren, this, vol));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Base::Dec",
    vtkvolume::BaseDeclarationFragment(ren, this, this->AssembledInputs, this->Impl->NumberOfLights,
      this->Impl->LightComplexity, numComps, independentComponents));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Base::Init",
    vtkvolume::BaseInit(ren, this, this->AssembledInputs, this->Impl->LightComplexity));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Base::Impl", vtkvolume::BaseImplementation(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Base::Exit", vtkvolume::BaseExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderTermination(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int vtkNotUsed(numComps))
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  vtkShaderProgram::Substitute(vertexShader, "//VTK::Termination::Dec",
    vtkvolume::TerminationDeclarationVertex(ren, this, vol));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Termination::Dec",
    vtkvolume::TerminationDeclarationFragment(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Terminate::Init", vtkvolume::TerminationInit(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Terminate::Impl", vtkvolume::TerminationImplementation(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Terminate::Exit", vtkvolume::TerminationExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderShading(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps)
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  vtkShaderProgram::Substitute(
    vertexShader, "//VTK::Shading::Dec", vtkvolume::ShadingDeclarationVertex(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Shading::Dec", vtkvolume::ShadingDeclarationFragment(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Shading::Init", vtkvolume::ShadingInit(ren, this, vol));

  if (this->Impl->MultiVolume)
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Shading::Impl",
      vtkvolume::ShadingMultipleInputs(this, this->AssembledInputs));
  }
  else
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Shading::Impl",
      vtkvolume::ShadingSingleInput(ren, this, vol, this->MaskInput, this->Impl->CurrentMask,
        this->MaskType, numComps, independentComponents));
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Shading::Exit",
    vtkvolume::ShadingExit(ren, this, vol, numComps, independentComponents));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCompute(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps)
{
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  // Every volume should have a property (cannot be nullptr);
  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  int independentComponents = volumeProperty->GetIndependentComponents();

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeGradient::Dec",
    vtkvolume::ComputeGradientDeclaration(this, this->AssembledInputs));

  if (this->Impl->MultiVolume)
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::GradientCache::Dec",
      vtkvolume::GradientCacheDec(ren, vol, this->AssembledInputs, independentComponents));

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Transfer2D::Dec",
      vtkvolume::Transfer2DDeclaration(this->AssembledInputs));

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeOpacity::Dec",
      vtkvolume::ComputeOpacityMultiDeclaration(this->AssembledInputs));

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeGradientOpacity1D::Dec",
      vtkvolume::ComputeGradientOpacityMulti1DDecl(this->AssembledInputs));

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeColor::Dec",
      vtkvolume::ComputeColorMultiDeclaration(this->AssembledInputs));
  }
  else
  {
    // Single input
    switch (volumeProperty->GetTransferFunctionMode())
    {
      case vtkVolumeProperty::TF_1D:
      {
        auto& input = this->AssembledInputs[0];

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeOpacity::Dec",
          vtkvolume::ComputeOpacityDeclaration(
            ren, this, vol, numComps, independentComponents, input.OpacityTablesMap));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeGradientOpacity1D::Dec",
          vtkvolume::ComputeGradientOpacity1DDecl(
            vol, numComps, independentComponents, input.GradientOpacityTablesMap));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeColor::Dec",
          vtkvolume::ComputeColorDeclaration(
            ren, this, vol, numComps, independentComponents, input.RGBTablesMap));
      }
      break;
      case vtkVolumeProperty::TF_2D:
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeOpacity::Dec",
          vtkvolume::ComputeOpacity2DDeclaration(ren, this, vol, numComps, independentComponents,
            this->AssembledInputs[0].TransferFunctions2DMap));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeColor::Dec",
          vtkvolume::ComputeColor2DDeclaration(ren, this, vol, numComps, independentComponents,
            this->AssembledInputs[0].TransferFunctions2DMap));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::GradientCache::Dec",
          vtkvolume::GradientCacheDec(ren, vol, this->AssembledInputs, independentComponents));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::PreComputeGradients::Impl",
          vtkvolume::PreComputeGradientsImpl(ren, vol, numComps, independentComponents));

        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Transfer2D::Dec",
          vtkvolume::Transfer2DDeclaration(this->AssembledInputs));
        break;
    }
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeLighting::Dec",
    vtkvolume::ComputeLightingDeclaration(ren, this, vol, numComps, independentComponents,
      this->Impl->NumberOfLights, this->Impl->LightComplexity));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::ComputeRayDirection::Dec",
    vtkvolume::ComputeRayDirectionDeclaration(ren, this, vol, numComps));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderCropping(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int vtkNotUsed(numComps))
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  vtkShaderProgram::Substitute(
    vertexShader, "//VTK::Cropping::Dec", vtkvolume::CroppingDeclarationVertex(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Cropping::Dec", vtkvolume::CroppingDeclarationFragment(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Cropping::Init", vtkvolume::CroppingInit(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Cropping::Impl", vtkvolume::CroppingImplementation(ren, this, vol));
  // true);

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Cropping::Exit", vtkvolume::CroppingExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderClipping(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int vtkNotUsed(numComps))
{
  vtkShader* vertexShader = shaders[vtkShader::Vertex];
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  vtkShaderProgram::Substitute(
    vertexShader, "//VTK::Clipping::Dec", vtkvolume::ClippingDeclarationVertex(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Clipping::Dec", vtkvolume::ClippingDeclarationFragment(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Clipping::Init", vtkvolume::ClippingInit(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Clipping::Impl", vtkvolume::ClippingImplementation(ren, this, vol));

  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Clipping::Exit", vtkvolume::ClippingExit(ren, this, vol));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderMasking(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol, int numComps)
{
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::BinaryMask::Dec",
    vtkvolume::BinaryMaskDeclaration(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::BinaryMask::Impl",
    vtkvolume::BinaryMaskImplementation(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::CompositeMask::Dec",
    vtkvolume::CompositeMaskDeclarationFragment(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType));

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::CompositeMask::Impl",
    vtkvolume::CompositeMaskImplementation(
      ren, this, vol, this->MaskInput, this->Impl->CurrentMask, this->MaskType, numComps));
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderPicking(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int vtkNotUsed(numComps))
{
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  if (this->Impl->CurrentSelectionPass != (vtkHardwareSelector::MIN_KNOWN_PASS - 1))
  {
    switch (this->Impl->CurrentSelectionPass)
    {
      case vtkHardwareSelector::CELL_ID_LOW24:
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Picking::Exit",
          vtkvolume::PickingIdLow24PassExit(ren, this, vol));
        break;
      case vtkHardwareSelector::CELL_ID_HIGH24:
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Picking::Exit",
          vtkvolume::PickingIdHigh24PassExit(ren, this, vol));
        break;
      default: // ACTOR_PASS, PROCESS_PASS
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Picking::Dec",
          vtkvolume::PickingActorPassDeclaration(ren, this, vol));

        vtkShaderProgram::Substitute(
          fragmentShader, "//VTK::Picking::Exit", vtkvolume::PickingActorPassExit(ren, this, vol));
        break;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderRTT(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int vtkNotUsed(numComps))
{
  vtkShader* fragmentShader = shaders[vtkShader::Fragment];

  if (this->RenderToImage)
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::RenderToImage::Dec",
      vtkvolume::RenderToImageDeclarationFragment(ren, this, vol));

    vtkShaderProgram::Substitute(
      fragmentShader, "//VTK::RenderToImage::Init", vtkvolume::RenderToImageInit(ren, this, vol));

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::RenderToImage::Impl",
      vtkvolume::RenderToImageImplementation(ren, this, vol));

    vtkShaderProgram::Substitute(
      fragmentShader, "//VTK::RenderToImage::Exit", vtkvolume::RenderToImageExit(ren, this, vol));
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkRenderer* ren, vtkVolume* vol,
  int noOfComponents)
{
  // Every volume should have a property (cannot be nullptr);
  vtkVolumeProperty* volumeProperty = vol->GetProperty();
  auto shaderProperty = vtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());

  if (volumeProperty->GetShade())
  {
    vtkLightCollection* lc = ren->GetLights();
    vtkLight* light;
    this->Impl->NumberOfLights = 0;

    // Compute light complexity.
    vtkCollectionSimpleIterator sit;
    for (lc->InitTraversal(sit); (light = lc->GetNextLight(sit));)
    {
      float status = light->GetSwitch();
      if (status > 0.0)
      {
        this->Impl->NumberOfLights++;
        if (this->Impl->LightComplexity == 0)
        {
          this->Impl->LightComplexity = 1;
        }
      }

      if (this->Impl->LightComplexity == 1 &&
        (this->Impl->NumberOfLights > 1 || light->GetIntensity() != 1.0 ||
          light->GetLightType() != VTK_LIGHT_TYPE_HEADLIGHT))
      {
        this->Impl->LightComplexity = 2;
      }

      if (this->Impl->LightComplexity < 3 && (light->GetPositional()))
      {
        this->Impl->LightComplexity = 3;
        break;
      }
    }
  }

  // Render pass pre replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderRenderPass(shaders, vol, true);

  // Custom uniform variables replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCustomUniforms(shaders, shaderProperty);

  // Base methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderBase(shaders, ren, vol, noOfComponents);

  // Termination methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderTermination(shaders, ren, vol, noOfComponents);

  // Shading methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderShading(shaders, ren, vol, noOfComponents);

  // Compute methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCompute(shaders, ren, vol, noOfComponents);

  // Cropping methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderCropping(shaders, ren, vol, noOfComponents);

  // Clipping methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderClipping(shaders, ren, vol, noOfComponents);

  // Masking methods replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderMasking(shaders, ren, vol, noOfComponents);

  // Picking replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderPicking(shaders, ren, vol, noOfComponents);

  // Render to texture
  //---------------------------------------------------------------------------
  this->ReplaceShaderRTT(shaders, ren, vol, noOfComponents);

  // Set number of isosurfaces
  if (this->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    std::ostringstream ss;
    ss << volumeProperty->GetIsoSurfaceValues()->GetNumberOfContours();
    vtkShaderProgram::Substitute(shaders[vtkShader::Fragment], "NUMBER_OF_CONTOURS", ss.str());
  }

  // Render pass post replacements
  //---------------------------------------------------------------------------
  this->ReplaceShaderRenderPass(shaders, vol, false);
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::BuildShader(vtkRenderer* ren)
{
  std::map<vtkShader::Type, vtkShader*> shaders;
  vtkShader* vertexShader = vtkShader::New();
  vertexShader->SetType(vtkShader::Vertex);
  shaders[vtkShader::Vertex] = vertexShader;
  vtkShader* fragmentShader = vtkShader::New();
  fragmentShader->SetType(vtkShader::Fragment);
  shaders[vtkShader::Fragment] = fragmentShader;
  vtkShader* geometryShader = vtkShader::New();
  geometryShader->SetType(vtkShader::Geometry);
  shaders[vtkShader::Geometry] = geometryShader;

  auto vol = this->Impl->GetActiveVolume();

  vtkOpenGLShaderProperty* sp = vtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());
  this->GetShaderTemplate(shaders, sp);

  // user specified pre replacements
  vtkOpenGLShaderProperty::ReplacementMap repMap = sp->GetAllShaderReplacements();
  for (auto i : repMap)
  {
    if (i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }

  auto numComp = this->AssembledInputs[0].Texture->GetLoadedScalars()->GetNumberOfComponents();
  this->ReplaceShaderValues(shaders, ren, vol, numComp);

  // user specified post replacements
  for (auto i : repMap)
  {
    if (!i.first.ReplaceFirst)
    {
      std::string ssrc = shaders[i.first.ShaderType]->GetSource();
      vtkShaderProgram::Substitute(
        ssrc, i.first.OriginalValue, i.second.Replacement, i.second.ReplaceAll);
      shaders[i.first.ShaderType]->SetSource(ssrc);
    }
  }

  // Now compile the shader
  //--------------------------------------------------------------------------
  this->Impl->ShaderProgram = this->Impl->ShaderCache->ReadyShaderProgram(shaders);
  if (!this->Impl->ShaderProgram || !this->Impl->ShaderProgram->GetCompiled())
  {
    vtkErrorMacro("Shader failed to compile");
  }

  vertexShader->Delete();
  fragmentShader->Delete();
  geometryShader->Delete();

  this->Impl->ShaderBuildTime.Modified();
}

//-----------------------------------------------------------------------------
// Update the reduction factor of the render viewport (this->ReductionFactor)
// according to the time spent in seconds to render the previous frame
// (this->TimeToDraw) and a time in seconds allocated to render the next
// frame (allocatedTime).
// \pre valid_current_reduction_range: this->ReductionFactor>0.0 &&
// this->ReductionFactor<=1.0 \pre positive_TimeToDraw: this->TimeToDraw>=0.0
// \pre positive_time: allocatedTime>0.0
// \post valid_new_reduction_range: this->ReductionFactor>0.0 &&
// this->ReductionFactor<=1.0
//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ComputeReductionFactor(double allocatedTime)
{
  if (!this->AutoAdjustSampleDistances)
  {
    this->ReductionFactor = 1.0 / this->ImageSampleDistance;
    return;
  }

  if (this->TimeToDraw)
  {
    double oldFactor = this->ReductionFactor;

    double timeToDraw;
    if (allocatedTime < 1.0)
    {
      timeToDraw = this->SmallTimeToDraw;
      if (timeToDraw == 0.0)
      {
        timeToDraw = this->BigTimeToDraw / 3.0;
      }
    }
    else
    {
      timeToDraw = this->BigTimeToDraw;
    }

    // This should be the case when rendering the volume very first time
    // 10.0 is an arbitrary value chosen which happen to a large number
    // in this context
    if (timeToDraw == 0.0)
    {
      timeToDraw = 10.0;
    }

    double fullTime = timeToDraw / this->ReductionFactor;
    double newFactor = allocatedTime / fullTime;

    // Compute average factor
    this->ReductionFactor = (newFactor + oldFactor) / 2.0;

    // Discretize reduction factor so that it doesn't cause
    // visual artifacts when used to reduce the sample distance
    this->ReductionFactor = (this->ReductionFactor > 1.0) ? 1.0 : (this->ReductionFactor);

    if (this->ReductionFactor < 0.20)
    {
      this->ReductionFactor = 0.10;
    }
    else if (this->ReductionFactor < 0.50)
    {
      this->ReductionFactor = 0.20;
    }
    else if (this->ReductionFactor < 1.0)
    {
      this->ReductionFactor = 0.50;
    }

    // Clamp it
    if (1.0 / this->ReductionFactor > this->MaximumImageSampleDistance)
    {
      this->ReductionFactor = 1.0 / this->MaximumImageSampleDistance;
    }
    if (1.0 / this->ReductionFactor < this->MinimumImageSampleDistance)
    {
      this->ReductionFactor = 1.0 / this->MinimumImageSampleDistance;
    }
  }
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::PreLoadData(vtkRenderer* ren, vtkVolume* vol)
{
  if (!this->ValidateRender(ren, vol))
  {
    return false;
  }

  // have to register if we preload
  this->ResourceCallback->RegisterGraphicsResources(
    static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

  this->Impl->ClearRemovedInputs(ren->GetRenderWindow());
  return this->Impl->UpdateInputs(ren, vol);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ForceTransferInit()
{
  auto& inputs = this->Parent->AssembledInputs;
  auto fu = [](std::pair<const int, vtkVolumeInputHelper>& p) { p.second.ForceTransferInit(); };
  std::for_each(inputs.begin(), inputs.end(), fu);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ClearRemovedInputs(vtkWindow* win)
{
  bool orderChanged = false;
  for (const int& port : this->Parent->RemovedPorts)
  {
    auto it = this->Parent->AssembledInputs.find(port);
    if (it == this->Parent->AssembledInputs.cend())
    {
      continue;
    }

    auto input = (*it).second;
    input.Texture->ReleaseGraphicsResources(win);
    input.GradientOpacityTables->ReleaseGraphicsResources(win);
    input.OpacityTables->ReleaseGraphicsResources(win);
    input.RGBTables->ReleaseGraphicsResources(win);
    this->Parent->AssembledInputs.erase(it);
    orderChanged = true;
  }
  this->Parent->RemovedPorts.clear();

  if (orderChanged)
  {
    this->ForceTransferInit();
  }
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::UpdateInputs(vtkRenderer* ren, vtkVolume* vol)
{
  this->VolumePropertyChanged = false;
  bool orderChanged = false;
  bool success = true;
  for (const auto& port : this->Parent->Ports)
  {
    if (this->MultiVolume)
    {
      vol = this->MultiVolume->GetVolume(port);
    }
    auto property = vol->GetProperty();
    auto input = this->Parent->GetTransformedInput(port);

    // Check for property changes
    this->VolumePropertyChanged |= property->GetMTime() > this->ShaderBuildTime.GetMTime();

    auto it = this->Parent->AssembledInputs.find(port);
    if (it == this->Parent->AssembledInputs.cend())
    {
      // Create new input structure
      auto texture = vtkSmartPointer<vtkVolumeTexture>::New();

      VolumeInput currentInput(texture, vol);
      this->Parent->AssembledInputs[port] = std::move(currentInput);
      orderChanged = true;

      it = this->Parent->AssembledInputs.find(port);
    }
    assert(it != this->Parent->AssembledInputs.cend());

    /// TODO Currently, only input arrays with the same name/id/mode can be
    // (across input objects) can be rendered. This could be addressed by
    // overriding the mapper's settings with array settings defined in the
    // vtkMultiVolume instance.
    vtkDataArray* scalars =
      this->Parent->GetScalars(input, this->Parent->ScalarMode, this->Parent->ArrayAccessMode,
        this->Parent->ArrayId, this->Parent->ArrayName, this->Parent->CellFlag);

    if (this->NeedToInitializeResources || (input->GetMTime() > it->second.Texture->UploadTime) ||
      (scalars != it->second.Texture->GetLoadedScalars()) ||
      (scalars != nullptr && scalars->GetMTime() > it->second.Texture->UploadTime))
    {
      auto& volInput = this->Parent->AssembledInputs[port];
      auto volumeTex = volInput.Texture.GetPointer();
      volumeTex->SetPartitions(this->Partitions[0], this->Partitions[1], this->Partitions[2]);
      success &= volumeTex->LoadVolume(
        ren, input, scalars, this->Parent->CellFlag, property->GetInterpolationType());
      volInput.ComponentMode = this->GetComponentMode(property, scalars);
    }
    else
    {
      // Update vtkVolumeTexture
      it->second.Texture->UpdateVolume(property);
    }
  }

  if (orderChanged)
  {
    this->ForceTransferInit();
  }

  return success;
}

int vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::GetComponentMode(
  vtkVolumeProperty* prop, vtkDataArray* array) const
{
  if (prop->GetIndependentComponents())
  {
    return VolumeInput::INDEPENDENT;
  }
  else
  {
    const auto numComp = array->GetNumberOfComponents();
    if (numComp == 1 || numComp == 2)
      return VolumeInput::LA;
    else if (numComp == 4)
      return VolumeInput::RGBA;
    else if (numComp == 3)
    {
      vtkGenericWarningMacro(<< "3 dependent components (e.g. RGB) are not supported."
                                "Only 2 (LA) and 4 (RGBA) supported.");
      return VolumeInput::INVALID;
    }
    else
      return VolumeInput::INVALID;
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::GPURender(vtkRenderer* ren, vtkVolume* vol)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLCamera* cam = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  if (this->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND &&
    vol->GetProperty()->GetIsoSurfaceValues()->GetNumberOfContours() == 0)
  {
    // Early exit: nothing to render.
    return;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->ResourceCallback->RegisterGraphicsResources(static_cast<vtkOpenGLRenderWindow*>(renWin));
  // Make sure the context is current
  renWin->MakeCurrent();

  // Get window size and corners
  this->Impl->CheckPropertyKeys(vol);
  if (!this->Impl->PreserveViewport)
  {
    ren->GetTiledSizeAndOrigin(this->Impl->WindowSize, this->Impl->WindowSize + 1,
      this->Impl->WindowLowerLeft, this->Impl->WindowLowerLeft + 1);
  }
  else
  {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    this->Impl->WindowLowerLeft[0] = vp[0];
    this->Impl->WindowLowerLeft[1] = vp[1];
    this->Impl->WindowSize[0] = vp[2];
    this->Impl->WindowSize[1] = vp[3];
  }

  this->Impl->NeedToInitializeResources =
    (this->Impl->ReleaseResourcesTime.GetMTime() > this->Impl->InitializationTime.GetMTime());

  this->ComputeReductionFactor(vol->GetAllocatedRenderTime());
  if (!this->Impl->SharedDepthTextureObject)
  {
    this->Impl->CaptureDepthTexture(ren);
  }

  vtkMTimeType renderPassTime = this->GetRenderPassStageMTime(vol);

  const auto multiVol = vtkMultiVolume::SafeDownCast(vol);
  this->Impl->MultiVolume = multiVol && this->GetInputCount() > 1 ? multiVol : nullptr;

  this->Impl->ClearRemovedInputs(renWin);
  this->Impl->UpdateInputs(ren, vol);
  this->Impl->UpdateSamplingDistance(ren);
  this->Impl->UpdateTransferFunctions(ren);

  // Masks are only supported on single-input rendring.
  if (!this->Impl->MultiVolume)
  {
    this->Impl->LoadMask(ren);
  }

  // Get the shader cache. This is important to make sure that shader cache
  // knows the state of various shader programs in use.
  this->Impl->ShaderCache =
    vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow())->GetShaderCache();

  this->Impl->CheckPickingState(ren);

  if (this->UseDepthPass && this->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND)
  {
    this->Impl->RenderWithDepthPass(ren, cam, renderPassTime);
  }
  else
  {
    if (this->Impl->IsPicking && !this->Impl->MultiVolume)
    {
      this->Impl->BeginPicking(ren);
    }
    vtkVolumeStateRAII glState(renWin->GetState(), this->Impl->PreserveGLState);

    if (this->Impl->ShaderRebuildNeeded(cam, vol, renderPassTime))
    {
      this->Impl->LastProjectionParallel = cam->GetParallelProjection();
      this->BuildShader(ren);
    }
    else
    {
      // Bind the shader
      this->Impl->ShaderCache->ReadyShaderProgram(this->Impl->ShaderProgram);
      this->InvokeEvent(vtkCommand::UpdateShaderEvent, this->Impl->ShaderProgram);
    }

    vtkOpenGLShaderProperty* shaderProperty =
      vtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());
    if (this->RenderToImage)
    {
      this->Impl->SetupRenderToTexture(ren);
      this->Impl->SetRenderToImageParameters(this->Impl->ShaderProgram);
      this->DoGPURender(ren, cam, this->Impl->ShaderProgram, shaderProperty);
      this->Impl->ExitRenderToTexture(ren);
    }
    else
    {
      this->Impl->BeginImageSample(ren);
      this->DoGPURender(ren, cam, this->Impl->ShaderProgram, shaderProperty);
      this->Impl->EndImageSample(ren);
    }

    if (this->Impl->IsPicking && !this->Impl->MultiVolume)
    {
      this->Impl->EndPicking(ren);
    }
  }

  glFinish();
}

//----------------------------------------------------------------------------
bool vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::ShaderRebuildNeeded(
  vtkCamera* cam, vtkVolume* vol, vtkMTimeType renderPassTime)
{
  return (this->NeedToInitializeResources || this->VolumePropertyChanged ||
    vol->GetShaderProperty()->GetShaderMTime() > this->ShaderBuildTime.GetMTime() ||
    this->Parent->GetMTime() > this->ShaderBuildTime.GetMTime() ||
    cam->GetParallelProjection() != this->LastProjectionParallel ||
    this->SelectionStateTime.GetMTime() > this->ShaderBuildTime.GetMTime() ||
    renderPassTime > this->ShaderBuildTime);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RenderWithDepthPass(
  vtkRenderer* ren, vtkOpenGLCamera* cam, vtkMTimeType renderPassTime)
{
  this->Parent->CurrentPass = DepthPass;
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeProperty = vol->GetProperty();
  auto shaderProperty = vtkOpenGLShaderProperty::SafeDownCast(vol->GetShaderProperty());

  if (this->NeedToInitializeResources ||
    volumeProperty->GetMTime() > this->DepthPassSetupTime.GetMTime() ||
    this->Parent->GetMTime() > this->DepthPassSetupTime.GetMTime() ||
    cam->GetParallelProjection() != this->LastProjectionParallel ||
    this->SelectionStateTime.GetMTime() > this->ShaderBuildTime.GetMTime() ||
    renderPassTime > this->ShaderBuildTime ||
    shaderProperty->GetShaderMTime() > this->ShaderBuildTime)
  {
    this->LastProjectionParallel = cam->GetParallelProjection();

    this->ContourFilter->SetInputData(this->Parent->GetTransformedInput(0));
    for (vtkIdType i = 0; i < this->Parent->GetDepthPassContourValues()->GetNumberOfContours(); ++i)
    {
      this->ContourFilter->SetValue(i, this->Parent->DepthPassContourValues->GetValue(i));
    }

    this->RenderContourPass(ren);
    this->DepthPassSetupTime.Modified();
    this->Parent->BuildShader(ren);
  }
  else if (cam->GetMTime() > this->DepthPassTime.GetMTime())
  {
    this->RenderContourPass(ren);
  }

  if (this->IsPicking)
  {
    this->BeginPicking(ren);
  }

  // Set OpenGL states
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkVolumeStateRAII glState(renWin->GetState(), this->PreserveGLState);

  if (this->Parent->RenderToImage)
  {
    this->SetupRenderToTexture(ren);
  }

  if (!this->PreserveViewport)
  {
    // NOTE: This is a must call or else, multiple viewport rendering would
    // not work. The glViewport could have been changed by any of the internal
    // FBOs (RenderToTexure, etc.).  The viewport should (ideally) not be set
    // within the mapper, because it could cause issues when vtkOpenGLRenderPass
    // instances modify it too (this is a workaround for that).
    renWin->GetState()->vtkglViewport(
      this->WindowLowerLeft[0], this->WindowLowerLeft[1], this->WindowSize[0], this->WindowSize[1]);
  }

  renWin->GetShaderCache()->ReadyShaderProgram(this->ShaderProgram);
  this->Parent->InvokeEvent(vtkCommand::UpdateShaderEvent, this->ShaderProgram);

  this->DPDepthBufferTextureObject->Activate();
  this->ShaderProgram->SetUniformi(
    "in_depthPassSampler", this->DPDepthBufferTextureObject->GetTextureUnit());
  this->Parent->DoGPURender(ren, cam, this->ShaderProgram, shaderProperty);
  this->DPDepthBufferTextureObject->Deactivate();

  if (this->IsPicking)
  {
    this->EndPicking(ren);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::BindTransformations(
  vtkShaderProgram* prog, vtkMatrix4x4* modelViewMat)
{
  // Bind transformations. Because the bounding box has its own transformations,
  // it is considered here as an actual volume (numInputs + 1).
  const int numInputs = static_cast<int>(this->Parent->AssembledInputs.size());
  const int numVolumes = this->MultiVolume ? numInputs + 1 : numInputs;

  this->VolMatVec.resize(numVolumes * 16, 0);
  this->InvMatVec.resize(numVolumes * 16, 0);
  this->TexMatVec.resize(numVolumes * 16, 0);
  this->InvTexMatVec.resize(numVolumes * 16, 0);
  this->TexEyeMatVec.resize(numVolumes * 16, 0);
  this->CellToPointVec.resize(numVolumes * 16, 0);
  this->TexMinVec.resize(numVolumes * 3, 0);
  this->TexMaxVec.resize(numVolumes * 3, 0);

  vtkNew<vtkMatrix4x4> dataToWorld, texToDataMat, texToViewMat, cellToPointMat;
  float defaultTexMin[3] = { 0.f, 0.f, 0.f };
  float defaultTexMax[3] = { 1.f, 1.f, 1.f };

  auto it = this->Parent->AssembledInputs.begin();
  for (int i = 0; i < numVolumes; i++)
  {
    const int vecOffset = i * 16;
    float *texMin, *texMax;

    if (this->MultiVolume && i == 0)
    {
      // Bounding box
      auto bBoxToWorld = this->MultiVolume->GetMatrix();
      dataToWorld->DeepCopy(bBoxToWorld);

      auto texToBBox = this->MultiVolume->GetTextureMatrix();
      texToDataMat->DeepCopy(texToBBox);

      cellToPointMat->Identity();
      texMin = defaultTexMin;
      texMax = defaultTexMax;
    }
    else
    {
      // Volume inputs
      auto& inputData = (*it).second;
      it++;
      auto volTex = inputData.Texture;
      auto volMatrix = inputData.Volume->GetMatrix();
      dataToWorld->DeepCopy(volMatrix);
      texToDataMat->DeepCopy(volTex->GetCurrentBlock()->TextureToDataset.GetPointer());

      // Texture matrices (texture to view)
      vtkMatrix4x4::Multiply4x4(volMatrix, texToDataMat.GetPointer(), texToViewMat.GetPointer());
      vtkMatrix4x4::Multiply4x4(modelViewMat, texToViewMat.GetPointer(), texToViewMat.GetPointer());

      // texToViewMat->Transpose();
      vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
        texToViewMat.GetPointer(), this->TexEyeMatVec.data(), vecOffset);

      // Cell to Point (texture-cells to texture-points)
      cellToPointMat->DeepCopy(volTex->CellToPointMatrix.GetPointer());
      texMin = volTex->AdjustedTexMin;
      texMax = volTex->AdjustedTexMax;
    }

    // Volume matrices (dataset to world)
    dataToWorld->Transpose();
    vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
      dataToWorld.GetPointer(), this->VolMatVec.data(), vecOffset);

    this->InverseVolumeMat->DeepCopy(dataToWorld.GetPointer());
    this->InverseVolumeMat->Invert();
    vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
      this->InverseVolumeMat.GetPointer(), this->InvMatVec.data(), vecOffset);

    // Texture matrices (texture to dataset)
    texToDataMat->Transpose();
    vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
      texToDataMat.GetPointer(), this->TexMatVec.data(), vecOffset);

    texToDataMat->Invert();
    vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
      texToDataMat.GetPointer(), this->InvTexMatVec.data(), vecOffset);

    // Cell to Point (texture adjustment)
    cellToPointMat->Transpose();
    vtkInternal::CopyMatrixToVector<vtkMatrix4x4, 4, 4>(
      cellToPointMat.GetPointer(), this->CellToPointVec.data(), vecOffset);
    vtkInternal::CopyVector<float, 3>(texMin, this->TexMinVec.data(), i * 3);
    vtkInternal::CopyVector<float, 3>(texMax, this->TexMaxVec.data(), i * 3);
  }

  // the matrix from data to world
  prog->SetUniformMatrix4x4v("in_volumeMatrix", numVolumes, this->VolMatVec.data());
  prog->SetUniformMatrix4x4v("in_inverseVolumeMatrix", numVolumes, this->InvMatVec.data());

  // the matrix from tcoords to data
  prog->SetUniformMatrix4x4v("in_textureDatasetMatrix", numVolumes, this->TexMatVec.data());
  prog->SetUniformMatrix4x4v(
    "in_inverseTextureDatasetMatrix", numVolumes, this->InvTexMatVec.data());

  // matrix from texture to view coordinates
  prog->SetUniformMatrix4x4v("in_textureToEye", numVolumes, this->TexEyeMatVec.data());

  // handle cell/point differences in tcoords
  prog->SetUniformMatrix4x4v("in_cellToPoint", numVolumes, this->CellToPointVec.data());

  prog->SetUniform3fv(
    "in_texMin", numVolumes, reinterpret_cast<const float(*)[3]>(this->TexMinVec.data()));
  prog->SetUniform3fv(
    "in_texMax", numVolumes, reinterpret_cast<const float(*)[3]>(this->TexMaxVec.data()));
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetVolumeShaderParameters(
  vtkShaderProgram* prog, int independentComponents, int noOfComponents, vtkMatrix4x4* modelViewMat)
{
  this->BindTransformations(prog, modelViewMat);

  // Bind other properties (per-input)
  const int numInputs = static_cast<int>(this->Parent->AssembledInputs.size());
  this->ScaleVec.resize(numInputs * 4, 0);
  this->BiasVec.resize(numInputs * 4, 0);
  this->StepVec.resize(numInputs * 3, 0);
  this->SpacingVec.resize(numInputs * 3, 0);
  this->RangeVec.resize(numInputs * 8, 0);

  int index = 0;
  for (auto& input : this->Parent->AssembledInputs)
  {
    // Bind volume textures
    auto block = input.second.Texture->GetCurrentBlock();
    std::stringstream ss;
    ss << "in_volume[" << index << "]";
    block->TextureObject->Activate();
    prog->SetUniformi(ss.str().c_str(), block->TextureObject->GetTextureUnit());

    // LargeDataTypes have been already biased and scaled so in those cases 0s
    // and 1s are passed respectively.
    float tscale[4] = { 1.0, 1.0, 1.0, 1.0 };
    float tbias[4] = { 0.0, 0.0, 0.0, 0.0 };
    float(*scalePtr)[4] = &tscale;
    float(*biasPtr)[4] = &tbias;
    auto volTex = input.second.Texture.GetPointer();
    if (!volTex->HandleLargeDataTypes &&
      (noOfComponents == 1 || noOfComponents == 2 || independentComponents))
    {
      scalePtr = &volTex->Scale;
      biasPtr = &volTex->Bias;
    }
    vtkInternal::CopyVector<float, 4>(*scalePtr, this->ScaleVec.data(), index * 4);
    vtkInternal::CopyVector<float, 4>(*biasPtr, this->BiasVec.data(), index * 4);
    vtkInternal::CopyVector<float, 3>(block->CellStep, this->StepVec.data(), index * 3);
    vtkInternal::CopyVector<float, 3>(volTex->CellSpacing, this->SpacingVec.data(), index * 3);

    // 8 elements stands for [min, max] per 4-components
    vtkInternal::CopyVector<float, 8>(
      reinterpret_cast<float*>(volTex->ScalarRange), this->RangeVec.data(), index * 8);

    input.second.ActivateTransferFunction(prog, this->Parent->BlendMode);
    index++;
  }
  prog->SetUniform4fv(
    "in_volume_scale", numInputs, reinterpret_cast<const float(*)[4]>(this->ScaleVec.data()));
  prog->SetUniform4fv(
    "in_volume_bias", numInputs, reinterpret_cast<const float(*)[4]>(this->BiasVec.data()));
  prog->SetUniform2fv(
    "in_scalarsRange", 4 * numInputs, reinterpret_cast<const float(*)[2]>(this->RangeVec.data()));
  prog->SetUniform3fv(
    "in_cellStep", numInputs, reinterpret_cast<const float(*)[3]>(this->StepVec.data()));
  prog->SetUniform3fv(
    "in_cellSpacing", numInputs, reinterpret_cast<const float(*)[3]>(this->SpacingVec.data()));
}

////----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetMapperShaderParameters(
  vtkShaderProgram* prog, vtkRenderer* ren, int independent, int numComp)
{
#ifndef GL_ES_VERSION_3_0
  // currently broken on ES
  if (!this->SharedDepthTextureObject)
  {
    this->DepthTextureObject->Activate();
  }
  prog->SetUniformi("in_depthSampler", this->DepthTextureObject->GetTextureUnit());
#endif

  if (this->Parent->GetUseJittering())
  {
    vtkOpenGLRenderWindow* win = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
    prog->SetUniformi("in_noiseSampler", win->GetNoiseTextureUnit());
  }
  else
  {
    prog->SetUniformi("in_noiseSampler", 0);
  }

  prog->SetUniformi("in_useJittering", this->Parent->UseJittering);
  prog->SetUniformi("in_noOfComponents", numComp);
  prog->SetUniformi("in_independentComponents", independent);
  prog->SetUniformf("in_sampleDistance", this->ActualSampleDistance);

  // Set the scale and bias for color correction
  prog->SetUniformf("in_scale", 1.0 / this->Parent->FinalColorWindow);
  prog->SetUniformf(
    "in_bias", (0.5 - (this->Parent->FinalColorLevel / this->Parent->FinalColorWindow)));
}

////----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetCameraShaderParameters(
  vtkShaderProgram* prog, vtkRenderer* ren, vtkOpenGLCamera* cam)
{
  vtkMatrix4x4* glTransformMatrix;
  vtkMatrix4x4* modelViewMatrix;
  vtkMatrix3x3* normalMatrix;
  vtkMatrix4x4* projectionMatrix;
  cam->GetKeyMatrices(ren, modelViewMatrix, normalMatrix, projectionMatrix, glTransformMatrix);

  this->InverseProjectionMat->DeepCopy(projectionMatrix);
  this->InverseProjectionMat->Invert();
  prog->SetUniformMatrix("in_projectionMatrix", projectionMatrix);
  prog->SetUniformMatrix("in_inverseProjectionMatrix", this->InverseProjectionMat.GetPointer());

  this->InverseModelViewMat->DeepCopy(modelViewMatrix);
  this->InverseModelViewMat->Invert();
  prog->SetUniformMatrix("in_modelViewMatrix", modelViewMatrix);
  prog->SetUniformMatrix("in_inverseModelViewMatrix", this->InverseModelViewMat.GetPointer());

  float fvalue3[3];
  if (cam->GetParallelProjection())
  {
    double dir[4];
    cam->GetDirectionOfProjection(dir);
    vtkInternal::ToFloat(dir[0], dir[1], dir[2], fvalue3);
    prog->SetUniform3fv("in_projectionDirection", 1, &fvalue3);
  }

  vtkInternal::ToFloat(cam->GetPosition(), fvalue3, 3);
  prog->SetUniform3fv("in_cameraPos", 1, &fvalue3);

  // TODO Take consideration of reduction factor
  float fvalue2[2];
  vtkInternal::ToFloat(this->WindowLowerLeft, fvalue2);
  prog->SetUniform2fv("in_windowLowerLeftCorner", 1, &fvalue2);

  vtkInternal::ToFloat(1.0 / this->WindowSize[0], 1.0 / this->WindowSize[1], fvalue2);
  prog->SetUniform2fv("in_inverseOriginalWindowSize", 1, &fvalue2);

  vtkInternal::ToFloat(1.0 / this->WindowSize[0], 1.0 / this->WindowSize[1], fvalue2);
  prog->SetUniform2fv("in_inverseWindowSize", 1, &fvalue2);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetMaskShaderParameters(
  vtkShaderProgram* prog, vtkVolumeProperty* prop, int noOfComponents)
{
  if (this->CurrentMask)
  {
    auto maskTex = this->CurrentMask->GetCurrentBlock()->TextureObject;
    maskTex->Activate();
    prog->SetUniformi("in_mask", maskTex->GetTextureUnit());
  }

  if (noOfComponents == 1 && this->Parent->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
  {
    if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
    {
      this->LabelMapTransfer2D->Activate();
      prog->SetUniformi("in_labelMapTransfer", this->LabelMapTransfer2D->GetTextureUnit());
      if (prop->HasLabelGradientOpacity())
      {
        this->LabelMapGradientOpacity->Activate();
        prog->SetUniformi(
          "in_labelMapGradientOpacity", this->LabelMapGradientOpacity->GetTextureUnit());
      }
      prog->SetUniformf("in_maskBlendFactor", this->Parent->MaskBlendFactor);
      prog->SetUniformf("in_mask_scale", this->CurrentMask->Scale[0]);
      prog->SetUniformf("in_mask_bias", this->CurrentMask->Bias[0]);
      prog->SetUniformi("in_labelMapNumLabels", this->LabelMapTransfer2D->GetTextureHeight() - 1);
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetRenderToImageParameters(
  vtkShaderProgram* prog)
{
  prog->SetUniformi("in_clampDepthToBackface", this->Parent->GetClampDepthToBackface());
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::SetAdvancedShaderParameters(vtkRenderer* ren,
  vtkShaderProgram* prog, vtkVolume* vol, vtkVolumeTexture::VolumeBlock* block, int numComp)
{
  // Cropping and clipping
  auto bounds = block->LoadedBoundsAA;
  this->SetCroppingRegions(prog, bounds);
  this->SetClippingPlanes(ren, prog, vol);

  // Picking
  if (this->CurrentSelectionPass < vtkHardwareSelector::POINT_ID_LOW24)
  {
    this->SetPickingId(ren);
  }

  auto blockExt = block->Extents;
  float fvalue3[3];
  vtkInternal::ToFloat(blockExt[0], blockExt[2], blockExt[4], fvalue3);
  prog->SetUniform3fv("in_textureExtentsMin", 1, &fvalue3);

  vtkInternal::ToFloat(blockExt[1], blockExt[3], blockExt[5], fvalue3);
  prog->SetUniform3fv("in_textureExtentsMax", 1, &fvalue3);

  // Component weights (independent components)
  auto volProperty = vol->GetProperty();
  float fvalue4[4];
  if (numComp > 1 && volProperty->GetIndependentComponents())
  {
    for (int i = 0; i < numComp; ++i)
    {
      fvalue4[i] = static_cast<float>(volProperty->GetComponentWeight(i));
    }
    prog->SetUniform4fv("in_componentWeight", 1, &fvalue4);
  }

  // Set the scalar range to be considered for average ip blend
  double avgRange[2];
  float fvalue2[2];
  this->Parent->GetAverageIPScalarRange(avgRange);
  if (avgRange[1] < avgRange[0])
  {
    double tmp = avgRange[1];
    avgRange[1] = avgRange[0];
    avgRange[0] = tmp;
  }
  vtkInternal::ToFloat(avgRange[0], avgRange[1], fvalue2);
  prog->SetUniform2fv("in_averageIPRange", 1, &fvalue2);

  // Set contour values for isosurface blend mode
  //--------------------------------------------------------------------------
  if (this->Parent->BlendMode == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    vtkIdType nbContours = volProperty->GetIsoSurfaceValues()->GetNumberOfContours();

    std::vector<float> values(nbContours);
    for (int i = 0; i < nbContours; i++)
    {
      values[i] = static_cast<float>(volProperty->GetIsoSurfaceValues()->GetValue(i));
    }

    // The shader expect (for efficiency purposes) the isovalues to be sorted.
    std::sort(values.begin(), values.end());

    prog->SetUniform1fv("in_isosurfacesValues", nbContours, values.data());
  }

  // Set function attributes for slice blend mode
  //--------------------------------------------------------------------------
  if (this->Parent->BlendMode == vtkVolumeMapper::SLICE_BLEND)
  {
    vtkPlane* plane = vtkPlane::SafeDownCast(volProperty->GetSliceFunction());

    if (plane)
    {
      double planeOrigin[3];
      double planeNormal[3];

      plane->GetOrigin(planeOrigin);
      plane->GetNormal(planeNormal);

      prog->SetUniform3f("in_slicePlaneOrigin", planeOrigin);
      prog->SetUniform3f("in_slicePlaneNormal", planeNormal);
    }
  }
}

void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::FinishRendering(const int numComp)
{
  for (auto& item : this->Parent->AssembledInputs)
  {
    auto& input = item.second;
    input.Texture->GetCurrentBlock()->TextureObject->Deactivate();
    input.DeactivateTransferFunction(this->Parent->BlendMode);
  }

#ifndef GL_ES_VERSION_3_0
  if (this->DepthTextureObject && !this->SharedDepthTextureObject)
  {
    this->DepthTextureObject->Deactivate();
  }
#endif

  if (this->CurrentMask)
  {
    this->CurrentMask->GetCurrentBlock()->TextureObject->Deactivate();
  }

  if (numComp == 1 && this->Parent->BlendMode != vtkGPUVolumeRayCastMapper::ADDITIVE_BLEND)
  {
    if (this->Parent->MaskInput != nullptr && this->Parent->MaskType == LabelMapMaskType)
    {
      this->LabelMapTransfer2D->Deactivate();
      this->LabelMapGradientOpacity->Deactivate();
    }
  }

  vtkOpenGLStaticCheckErrorMacro("Failed after FinishRendering!");
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::DoGPURender(vtkRenderer* ren, vtkOpenGLCamera* cam,
  vtkShaderProgram* prog, vtkOpenGLShaderProperty* shaderProperty)
{
  if (!prog)
  {
    return;
  }

  // Upload the value of user-defined uniforms in the program
  auto vu = static_cast<vtkOpenGLUniforms*>(shaderProperty->GetVertexCustomUniforms());
  vu->SetUniforms(prog);
  auto fu = static_cast<vtkOpenGLUniforms*>(shaderProperty->GetFragmentCustomUniforms());
  fu->SetUniforms(prog);
  auto gu = static_cast<vtkOpenGLUniforms*>(shaderProperty->GetGeometryCustomUniforms());
  gu->SetUniforms(prog);

  this->SetShaderParametersRenderPass();
  if (!this->Impl->MultiVolume)
  {
    this->Impl->RenderSingleInput(ren, cam, prog);
  }
  else
  {
    this->Impl->RenderMultipleInputs(ren, cam, prog);
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RenderMultipleInputs(
  vtkRenderer* ren, vtkOpenGLCamera* cam, vtkShaderProgram* prog)
{
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeTex = input.Texture.GetPointer();
  const int independent = vol->GetProperty()->GetIndependentComponents();
  const int numComp = volumeTex->GetLoadedScalars()->GetNumberOfComponents();
  int const numSamplers = (independent ? numComp : 1);
  auto geometry = this->MultiVolume->GetDataGeometry();

  vtkMatrix4x4 *wcvc, *vcdc, *wcdc;
  vtkMatrix3x3* norm;
  cam->GetKeyMatrices(ren, wcvc, norm, vcdc, wcdc);

  this->SetMapperShaderParameters(prog, ren, independent, numComp);
  this->SetVolumeShaderParameters(prog, independent, numComp, wcvc);
  this->SetLightingShaderParameters(ren, prog, this->MultiVolume, numSamplers);
  this->SetCameraShaderParameters(prog, ren, cam);
  this->RenderVolumeGeometry(ren, prog, this->MultiVolume, geometry);
  this->FinishRendering(numComp);
}

//----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::vtkInternal::RenderSingleInput(
  vtkRenderer* ren, vtkOpenGLCamera* cam, vtkShaderProgram* prog)
{
  auto& input = this->Parent->AssembledInputs[0];
  auto vol = input.Volume;
  auto volumeTex = input.Texture.GetPointer();

  // Sort blocks in case the viewpoint changed, it immediately returns if there
  // is a single block.
  volumeTex->SortBlocksBackToFront(ren, vol->GetMatrix());
  vtkVolumeTexture::VolumeBlock* block = volumeTex->GetCurrentBlock();

  if (this->CurrentMask)
  {
    this->CurrentMask->SortBlocksBackToFront(ren, vol->GetMatrix());
  }

  const int independent = vol->GetProperty()->GetIndependentComponents();
  const int numComp = volumeTex->GetLoadedScalars()->GetNumberOfComponents();
  while (block != nullptr)
  {
    const int numSamplers = (independent ? numComp : 1);
    this->SetMapperShaderParameters(prog, ren, independent, numComp);

    vtkMatrix4x4 *wcvc, *vcdc, *wcdc;
    vtkMatrix3x3* norm;
    cam->GetKeyMatrices(ren, wcvc, norm, vcdc, wcdc);
    this->SetVolumeShaderParameters(prog, independent, numComp, wcvc);

    this->SetMaskShaderParameters(prog, vol->GetProperty(), numComp);
    this->SetLightingShaderParameters(ren, prog, vol, numSamplers);
    this->SetCameraShaderParameters(prog, ren, cam);
    this->SetAdvancedShaderParameters(ren, prog, vol, block, numComp);

    this->RenderVolumeGeometry(ren, prog, vol, block->VolumeGeometry);

    this->FinishRendering(numComp);
    block = volumeTex->GetNextBlock();
    if (this->CurrentMask)
    {
      this->CurrentMask->GetNextBlock();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::SetPartitions(
  unsigned short x, unsigned short y, unsigned short z)
{
  this->Impl->Partitions[0] = x;
  this->Impl->Partitions[1] = y;
  this->Impl->Partitions[2] = z;
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLGPUVolumeRayCastMapper::GetRenderPassStageMTime(vtkVolume* vol)
{
  vtkInformation* info = vol->GetPropertyKeys();
  vtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  this->Impl->RenderPassAttached = false;
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    this->Impl->RenderPassAttached = true;
  }

  int lastRenderPasses = 0;
  if (this->LastRenderPassInfo->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses = this->LastRenderPassInfo->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = VTK_MTIME_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      vtkObjectBase* curRP = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkObjectBase* lastRP = this->LastRenderPassInfo->Get(vtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = VTK_MTIME_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, rp->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    this->LastRenderPassInfo->CopyEntry(info, vtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    this->LastRenderPassInfo->Clear();
  }

  return renderPassMTime;
}

//-----------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::ReplaceShaderRenderPass(
  std::map<vtkShader::Type, vtkShader*>& shaders, vtkVolume* vol, bool prePass)
{
  std::string vertShader = shaders[vtkShader::Vertex]->GetSource();
  std::string geomShader = shaders[vtkShader::Geometry]->GetSource();
  std::string fragShader = shaders[vtkShader::Fragment]->GetSource();
  vtkInformation* info = vol->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!rp->PreReplaceShaderValues(vertShader, geomShader, fragShader, this, vol))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::PreReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
      else
      {
        if (!rp->PostReplaceShaderValues(vertShader, geomShader, fragShader, this, vol))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::PostReplaceShaderValues failed for " << rp->GetClassName());
        }
      }
    }
  }
  shaders[vtkShader::Vertex]->SetSource(vertShader);
  shaders[vtkShader::Geometry]->SetSource(geomShader);
  shaders[vtkShader::Fragment]->SetSource(fragShader);
}

//------------------------------------------------------------------------------
void vtkOpenGLGPUVolumeRayCastMapper::SetShaderParametersRenderPass()
{
  auto vol = this->Impl->GetActiveVolume();
  vtkInformation* info = vol->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* rp = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (!rp->SetShaderParameters(this->Impl->ShaderProgram, this, vol))
      {
        vtkErrorMacro(
          "RenderPass::SetShaderParameters failed for renderpass: " << rp->GetClassName());
      }
    }
  }
}
