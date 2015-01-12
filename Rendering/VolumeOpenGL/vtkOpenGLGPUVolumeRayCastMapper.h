/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGPUVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLGPUVolumeRayCastMapper - OpenGL subclass that draws the
// image to the screen
// .SECTION Description
// This is the concrete implementation of a ray cast image display helper -
// a helper class responsible for drawing the image to the screen.

// .SECTION see also
// vtkGPUVolumeRayCastMapper
//
// .SECTION Thanks
// Thanks to Michael Granseier for helping to debug this class with respect
// to maximum memory issues (which must be specified as vtkIdType and not int).
#ifndef vtkOpenGLGPUVolumeRayCastMapper_h
#define vtkOpenGLGPUVolumeRayCastMapper_h

#include "vtkRenderingVolumeOpenGLModule.h" // For export macro
#include "vtkGPUVolumeRayCastMapper.h"

class vtkVolume;
class vtkRenderer;
class vtkOpenGLExtensionManager;
class vtkMatrix4x4;
class vtkUnsupportedRequiredExtensionsStringStream; // Pimpl
class vtkMapDataArrayTextureId; // Pimpl
class vtkMapMaskTextureId; // Pimpl
class vtkPolyData;
class vtkClipConvexPolyData;
class vtkClipPolyData;
class vtkTessellatedBoxSource;

class vtkOpacityTable; // internal class.
class vtkRGBTable; // internal class.
class vtkKWScalarField; // internal class.
class vtkKWMask; // internal class.

class vtkOpacityTables; // Pimpl
class vtkDensifyPolyData;
class vtkStdString;

class vtkShaderProgram2;
class vtkShader2;

class VTKRENDERINGVOLUMEOPENGL_EXPORT vtkOpenGLGPUVolumeRayCastMapper
  : public vtkGPUVolumeRayCastMapper
{
public:
  static vtkOpenGLGPUVolumeRayCastMapper *New();
  vtkTypeMacro(vtkOpenGLGPUVolumeRayCastMapper,vtkGPUVolumeRayCastMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Based on hardware and properties, we may or may not be able to
  // render using 3D texture mapping. This indicates if 3D texture
  // mapping is supported by the hardware, and if the other extensions
  // necessary to support the specific properties are available.
  virtual int IsRenderSupported(vtkRenderWindow *window,
                                vtkVolumeProperty *property);

  // Description:
  // Delete OpenGL objects.
  // \post done: this->OpenGLObjectsCreated==0
  virtual void ReleaseGraphicsResources(vtkWindow *window);

  // Description:
  // Return a string matching the OpenGL errorCode.
  // \post result_exists: result!=0
  static const char *OpenGLErrorMessage(unsigned int errorCode);

  // Description:
  // Display headerMessage on the standard output and the last OpenGL error
  // message if any.
  // \pre headerMessage_exists: headerMessage!=0
  static void PrintError(const char *headerMessage);

protected:
  vtkOpenGLGPUVolumeRayCastMapper();
  ~vtkOpenGLGPUVolumeRayCastMapper();

  // The render method called by the superclass
  virtual void GPURender(vtkRenderer *ren,
                         vtkVolume *vol);

  // Methods called by the AMR Volume Mapper.
  virtual void PreRender(vtkRenderer *ren,
                         vtkVolume *vol,
                         double datasetBounds[6],
                         double scalarRange[2],
                         int numberOfScalarComponents,
                         unsigned int numberOfLevels);

  // \pre input is up-to-date
  virtual void RenderBlock(vtkRenderer *ren,
                           vtkVolume *vol,
                           unsigned int level);

  virtual void PostRender(vtkRenderer *ren,
                          int numberOfScalarComponents);

  // Description:
  // Return if the required OpenGL extension `extensionName' is supported.
  // If not, its name is added to the string of unsupported but required
  // extensions.
  // \pre extensions_exist: extensions!=0
  // \pre extensionName_exists: extensionName!=0
  int TestRequiredExtension(vtkOpenGLExtensionManager *extensions,
                            const char *extensionName);

  // Description:
  // Attempt to load required and optional OpenGL extensions for the current
  // context window. Variable LoadExtensionsSucceeded is set if all required
  // extensions has been loaded. In addition, variable
  // Supports_GL_ARB_texture_float is set if this extension has been loaded.
  // \pre: window_exists: window!=0
  void LoadExtensions(vtkRenderWindow *window);

  // Description:
  // Create OpenGL objects such as textures, buffers and fragment program Ids.
  // It only registers Ids, there is no actual initialization of textures or
  // fragment program.
  // \pre extensions_loaded: this->LoadExtensionsSucceeded
  // \post done: this->OpenGLObjectsCreated==1
  void CreateOpenGLObjects(vtkRenderer *ren);

  // Description:
  // Allocate memory on the GPU for the framebuffers according to the size of
  // the window or reallocate if the size has changed. Return true if
  // allocation succeeded.
  // \pre ren_exists: ren!=0
  // \pre opengl_objects_created: this->OpenGLObjectsCreated
  // \post right_size: LastSize[]=window size.
  int AllocateFrameBuffers(vtkRenderer *ren);

  // Description
  // Load the scalar field (one or four component scalar field), cell or point
  // based for a given subextent of the whole extent (can be the whole extent)
  // as a 3D texture on the GPU.
  // Extents are expressed in point if the cell flag is false or in cells of
  // the cell flag is true.
  // It returns true if it succeeded, false if there is not enough memory on
  // the GPU.
  // If succeeded, it updates the LoadedExtent, LoadedBounds, LoadedCellFlag
  // and LoadedTime. It also succeed if the scalar field is already loaded
  // (ie since last load, input has not changed and cell flag has not changed
  // and requested texture extents are enclosed in the loaded extent).
  // \pre input_exists: input!=0
  // \pre valid_point_extent: (this->CellFlag ||
  //                           (textureExtent[0]<textureExtent[1] &&
  //                            textureExtent[2]<textureExtent[3] &&
  //                            textureExtent[4]<textureExtent[5])))
  // \pre valid_cell_extent: (!this->CellFlag ||
  //                          (textureExtent[0]<=textureExtent[1] &&
  //                           textureExtent[2]<=textureExtent[3] &&
  //                           textureExtent[4]<=textureExtent[5])))
  int LoadScalarField(vtkImageData *input,
                      vtkImageData *maskInput,
                      int textureExtent[6],
                      vtkVolume *volume);

  // Description:
  // Allocate memory and load color table on the GPU or
  // reload it if the transfer function changed.
  // \pre vol_exists: vol!=0
  // \pre valid_numberOfScalarComponents: numberOfScalarComponents==1 || numberOfScalarComponents==4
  int UpdateColorTransferFunction(vtkVolume *vol,
                                  int numberOfScalarComponents);
  // Description:
  // Allocate memory and load opacity table on the GPU or
  // reload it if the transfer functions changed.
  // \pre vol_exists: vol!=0
  // \pre valid_numberOfScalarComponents: numberOfScalarComponents==1 || numberOfScalarComponents==4
  int UpdateOpacityTransferFunction(vtkVolume *vol,
                                    int numberOfScalarComponents,
                                    unsigned int level);

  // Description:
  // Prepare rendering in the offscreen framebuffer.
  // \pre ren_exists: ren!=0
  // \pre vol_exists: vol!=0
  void SetupRender(vtkRenderer *ren, vtkVolume *vol);

  // Description:
  // Clip the bounding box with all clipping planes
  // and the near and far plane
  void ClipBoundingBox(vtkRenderer *ren,
                       double worldBounds[6],
                       vtkVolume *vol);

  // Description:
  // Render the bounding box. The flag indicates whether
  // or not tcoords are rendered too. Return abort status (true==abort).
  // \pre valid_currentBlock: currentBlock>=0 && currentBlock<numberOfBlocks
  int RenderClippedBoundingBox(int tcoordFlag,
                               size_t currentBlock,
                               size_t numberOfBlocks,
                               vtkRenderWindow *renWin);

  // Description:
  // Method used to copy the state of the color buffer (which is in
  // a frame buffer object) to a texture.
  void CopyFBOToTexture();

  // Description:
  // Restore OpenGL state after rendering of the dataset.
  void CleanupRender();

  // Description:
  // Render the offscreen buffer to the screen.
  // \pre ren_exists: ren!=0
  void RenderTextureToScreen(vtkRenderer *ren);

  // Description:
  // Compute y=2^n such that x<=y.
  // \pre positive_x: x>=0
  // \post valid_result: result>=x
  int PowerOfTwoGreaterOrEqual(int x);

  // Description:
  // Display the status of the current framebuffer on the standard output.
  void CheckFrameBufferStatus();

  // Description:
  // Create a string from a buffer id. The result has to be free by the caller.
  vtkStdString BufferToString(int buffer);

  // Description:
  // Display the buffers assigned for drawing and reading operations.
  void DisplayReadAndDrawBuffers();

  // Description:
  // Display all the attachments of the current framebuffer object.
  void DisplayFrameBufferAttachments();

  // Description:
  // Display a given attachment for the current framebuffer object.
  void DisplayFrameBufferAttachment(unsigned int uattachment);

  // Description:
  // Concatenate the header string, projection type code and method to the
  // final fragment code in this->FragmentCode.
  // \pre valid_raycastMethod: raycastMethod>= vtkOpenGLGPUVolumeRayCastMapperMethodMaximumIntensityProjection && raycastMethod<=vtkOpenGLGPUVolumeRayCastMapperMethodMinIPFourDependent
  void BuildProgram(vtkRenderWindow *w,
                    int parallelProjection,
                    int raycastMethod,
                    int shadeMethod,
                    int componentMethod);

  // Description:
  // Return the current OpenGL state about lighting.
  void GetLightingStatus();

  // Description:
  // Update the reduction factor of the render viewport (this->ReductionFactor)
  // according to the time spent in seconds to render the previous frame
  // (this->TimeToDraw) and a time in seconds allocated to render the next
  // frame (allocatedTime).
  // \pre valid_current_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  // \pre positive_TimeToDraw: this->TimeToDraw>=0.0
  // \pre positive_time: allocatedTime>0
  // \post valid_new_reduction_range: this->ReductionFactor>0.0 && this->ReductionFactor<=1.0
  void ComputeReductionFactor(double allocatedTime);

  // Description:
  // Render a subvolume.
  // \pre this->ProgramShader!=0 and is linked.
  void RenderWholeVolume(vtkRenderer *ren,
                         vtkVolume *vol);

  // Description:
  // Render a subvolume.
  // \pre this->ProgramShader!=0 and is linked.
  void RenderRegions(vtkRenderer *ren,
                     vtkVolume *vol);

  // Return abort status (true==abort)
  int RenderSubVolume(vtkRenderer *ren,
                      double bounds[6],
                      vtkVolume *vol);

  void LoadProjectionParameters(vtkRenderer *ren,
                                vtkVolume *vol);

  // Description:
  // Compute and return the number of cropping regions
  void ComputeNumberOfCroppingRegions();

  void GetTextureFormat(vtkImageData *input,
                        unsigned int *internalFormat,
                        unsigned int *format,
                        unsigned int *type,
                        int *componentSize);

  bool TestLoadingScalar(unsigned int internalFormat,
                         unsigned int format,
                         unsigned int type,
                         int textureSize[3],
                         int componentSize);

  void SlabsFromDatasetToIndex(double slabsDataSet[6],
                               double slabsPoints[6]);

  void SlabsFromIndexToDataset(double slabsPoints[6],
                               double slabsDataSet[6]);

  const char *GetEnabledString(unsigned char value);
  void GetOpenGLState();

  void DebugDisplayBox(vtkPolyData *box);

  void UpdateNoiseTexture();

  // Description:
  // Compute how each axis of a cell is projected on the viewport in pixel.
  // This requires to have information about the camera and about the volume.
  // It set the value of IgnoreSampleDistancePerPixel to true in case of
  // degenerated case (axes aligned with the view).
  double ComputeMinimalSampleDistancePerPixel(vtkRenderer *renderer,
                                              vtkVolume *volume);

  // Description:
  // Return how much the dataset has to be reduced in each dimension to
  // fit on the GPU. If the value is 1.0, there is no need to reduce the
  // dataset.
  // \pre the calling thread has a current OpenGL context.
  // \pre mapper_supported: IsRenderSupported(renderer->GetRenderWindow(),0)
  // The computation is based on hardware limits (3D texture indexable size)
  // and MaxMemoryInBytes.
  // \post valid_i_ratio: ratio[0]>0 && ratio[0]<=1.0
  // \post valid_j_ratio: ratio[1]>0 && ratio[1]<=1.0
  // \post valid_k_ratio: ratio[2]>0 && ratio[2]<=1.0
  virtual void GetReductionRatio(double ratio[3]);

  int NumberOfCroppingRegions;

  // World coordinates of each corner of the dataset.
  double BoundingBox[8][3];

  // Used during the clipping process.
  vtkPolyData *PolyDataBoundingBox;
  vtkPlaneCollection *Planes;
  vtkPlane *NearPlane;

  vtkClipConvexPolyData *Clip;
  vtkMatrix4x4 *InvVolumeMatrix;

  vtkDensifyPolyData *Densify;

  int OpenGLObjectsCreated;
  int NumberOfFrameBuffers;

  unsigned int FrameBufferObject;
  unsigned int DepthRenderBufferObject;

  // 3D scalar texture +1D color+1D opacity+2D grabbed depth buffer
  // +1 2D colorbuffer.
  unsigned int TextureObjects[5];
  // used in MIP Mode (2 needed for ping-pong technique)
  unsigned int MaxValueFrameBuffer;
  unsigned int MaxValueFrameBuffer2;
  int ReducedSize[2];

  vtkPolyData *ClippedBoundingBox;

  int LastSize[2];

  double ReductionFactor;

  // Supported extensions
  // List of unsupported required extensions. Pimpl.
  vtkUnsupportedRequiredExtensionsStringStream *UnsupportedRequiredExtensions;
  int LoadExtensionsSucceeded;

  int Supports_GL_ARB_texture_float;
  int SupportsPixelBufferObjects;

  vtkTimeStamp DataBufferTime;

  // Matrices used in internal computation. As a member variable,
  // only one memory allocation is performed.
  vtkMatrix4x4 *TempMatrix[3];

  double TableRange[2];

  // Final string to send to the GPU as the fragment program source code.
//  char *FragmentCode;
//  int FragmentCodeCapacity;

  int ErrorLine;
  int ErrorColumn;
  char *ErrorString;

  // Store the last projection an raycast method in order to not rebuild
  // the fragment code at every call.
  int LastParallelProjection;
  int LastRayCastMethod;
  int LastCroppingMode;
  int LastComponent;
  int LastShade;

  vtkImageData *SmallInput;
  vtkTimeStamp SmallInputBuildTime;

  // Description:
  // Build the fragment shader program that scale and bias a texture
  // for window/level purpose.
  void BuildScaleBiasProgram(vtkRenderWindow *w);

#if 0
  vtkIdType LoadedExtent[6];
  double LoadedBounds[6];
  vtkTimeStamp LoadedScalarTime;
  int LoadedCellFlag; // point data or cell data (or field data, not handled) ?
#endif

  unsigned int SavedFrameBuffer; // some offscreen mode use a framebuffer too.

  vtkTessellatedBoxSource *BoxSource;

  float *NoiseTexture;
  int NoiseTextureSize; // size of one dimension.
  unsigned int NoiseTextureId; // GLuint

  bool IgnoreSampleDistancePerPixel;

  vtkMapDataArrayTextureId *ScalarsTextures; // need a list for AMR mode.
  vtkMapMaskTextureId *MaskTextures; // need a list for AMR mode.

  vtkRGBTable *RGBTable;
  vtkRGBTable *Mask1RGBTable;
  vtkRGBTable *Mask2RGBTable;

  vtkOpacityTables *OpacityTables;

  vtkKWScalarField *CurrentScalar;
  vtkKWMask *CurrentMask;

  float ActualSampleDistance;

  double LastProgressEventTime; // initial value is 0.0. Expressed in seconds.

  bool PreserveOrientation;

  vtkShaderProgram2 *Program;
  vtkShader2 *Main;
  vtkShader2 *Projection;
  vtkShader2 *Trace;
  vtkShader2 *CroppingShader;
  vtkShader2 *Component;
  vtkShader2 *Shade;

  // Internal Variable used to keep track of whether or render window's size
  // changed and therefore we need re-allocation.
  bool        SizeChanged;

  vtkShaderProgram2 *ScaleBiasProgram;

private:
  vtkOpenGLGPUVolumeRayCastMapper(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
  void operator=(const vtkOpenGLGPUVolumeRayCastMapper&);  // Not implemented.
};

#endif
