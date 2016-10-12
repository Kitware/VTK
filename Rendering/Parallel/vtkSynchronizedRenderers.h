/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedRenderers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSynchronizedRenderers
 * @brief   synchronizes renderers across processes.
 *
 * vtkSynchronizedRenderers is used to synchronize renderers (vtkRenderer and
 * subclasses) across processes for parallel rendering. It's designed to be used
 * in conjunction with vtkSynchronizedRenderWindows to synchronize the render
 * windows among those processes.
 * This class handles synchronization of certain render parameters among the
 * renderers such as viewport, camera parameters. It doesn't support compositing
 * of rendered images across processes on its own. You typically either subclass
 * to implement a compositing algorithm or use a renderer capable of compositing
 * eg. IceT based renderer.
*/

#ifndef vtkSynchronizedRenderers_h
#define vtkSynchronizedRenderers_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkUnsignedCharArray.h" // needed for vtkUnsignedCharArray.
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkFXAAOptions;
class vtkRenderer;
class vtkMultiProcessController;
class vtkMultiProcessStream;
class vtkOpenGLFXAAFilter;
class vtkOpenGLRenderer;

class VTKRENDERINGPARALLEL_EXPORT vtkSynchronizedRenderers : public vtkObject
{
public:
  static vtkSynchronizedRenderers* New();
  vtkTypeMacro(vtkSynchronizedRenderers, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the renderer to be synchronized by this instance. A
   * vtkSynchronizedRenderers instance can be used to synchronize exactly 1
   * renderer on each processes. You can create multiple instances on
   * vtkSynchronizedRenderers to synchronize multiple renderers.
   */
  virtual void SetRenderer(vtkRenderer*);
  virtual vtkRenderer* GetRenderer();
  //@}

  //@{
  /**
   * Set the parallel message communicator. This is used to communicate among
   * processes.
   */
  virtual void SetParallelController(vtkMultiProcessController*);
  vtkGetObjectMacro(ParallelController, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Enable/Disable parallel rendering. Unless Parallel rendering is on, the
   * cameras won't be synchronized across processes.
   */
  vtkSetMacro(ParallelRendering, bool);
  vtkGetMacro(ParallelRendering, bool);
  vtkBooleanMacro(ParallelRendering, bool);
  //@}

  //@{
  /**
   * Get/Set the image reduction factor.
   */
  vtkSetClampMacro(ImageReductionFactor, int, 1, 50);
  vtkGetMacro(ImageReductionFactor, int);
  //@}

  //@{
  /**
   * If on (default), the rendered images are pasted back on to the screen. You
   * should turn this flag off on processes that are not meant to be visible to
   * the user.
   */
  vtkSetMacro(WriteBackImages, bool);
  vtkGetMacro(WriteBackImages, bool);
  vtkBooleanMacro(WriteBackImages, bool);
  //@}

  //@{
  /**
   * Enable FXAA antialiasing. FXAA is applied after all rendering results are
   * composited into the final image.
   */
  vtkSetMacro(UseFXAA, bool)
  vtkGetMacro(UseFXAA, bool)
  vtkBooleanMacro(UseFXAA, bool)
  //@}

  //@{
  /**
   * The configuration options for FXAA antialiasing.
   */
  vtkGetObjectMacro(FXAAOptions, vtkFXAAOptions)
  virtual void SetFXAAOptions(vtkFXAAOptions*);
  //@}

  //@{
  /**
   * Get/Set the root-process id. This is required when the ParallelController
   * is a vtkSocketController. Set to 0 by default (which will not work when
   * using a vtkSocketController but will work for vtkMPIController).
   */
  vtkSetMacro(RootProcessId, int);
  vtkGetMacro(RootProcessId, int);
  //@}

  /**
   * Computes visible prob bounds. This must be called on all processes at the
   * same time. The collective result is made available on all processes once
   * this method returns.
   * Note that this method requires that bounds is initialized to some value.
   * This expands the bounds to include the prop bounds.
   */
  void CollectiveExpandForVisiblePropBounds(double bounds[6]);

  //@{
  /**
   * When set, this->CaptureRenderedImage() does not capture image from the
   * screen instead passes the call to the delegate.
   */
  virtual void SetCaptureDelegate(vtkSynchronizedRenderers*);
  vtkGetObjectMacro(CaptureDelegate, vtkSynchronizedRenderers);
  //@}

  //@{
  /**
   * When multiple groups of processes are synchronized together using different
   * controllers, one needs to specify the order in which the various
   * synchronizers execute. In such cases one starts with the outer most
   * vtkSynchronizedRenderers, sets the dependent one as a CaptureDelegate on it
   * and the turn off AutomaticEventHandling on the delegate.
   */
  vtkSetMacro(AutomaticEventHandling, bool);
  vtkGetMacro(AutomaticEventHandling, bool);
  vtkBooleanMacro(AutomaticEventHandling, bool);
  //@}

  enum
  {
    SYNC_RENDERER_TAG = 15101,
    RESET_CAMERA_TAG  = 15102,
    COMPUTE_BOUNDS_TAG = 15103
  };

  /// vtkRawImage can be used to make it easier to deal with images for
  /// compositing/communicating over client-server etc.
  struct VTKRENDERINGPARALLEL_EXPORT vtkRawImage
  {
  public:
    vtkRawImage()
    {
      this->Valid = false;
      this->Size[0] = this->Size[1] = 0;
      this->Data = vtkSmartPointer<vtkUnsignedCharArray>::New();
    }

    void Resize(int dx, int dy, int numcomps)
    {
      this->Valid = false;
      this->Allocate(dx, dy, numcomps);
    }

    /**
     * Create the buffer from an image data.
     */
    void Initialize(int dx, int dy, vtkUnsignedCharArray* data);

    void MarkValid() { this->Valid = true; }
    void MarkInValid() { this->Valid = false; }

    bool IsValid() { return this->Valid; }
    int GetWidth() { return this->Size[0];}
    int GetHeight() { return this->Size[1];}
    vtkUnsignedCharArray* GetRawPtr()
      { return this->Data; }

    // Pushes the image to the viewport.
    bool PushToViewport(vtkRenderer*);

    // This is a raw version of PushToViewport() that assumes that the
    // glViewport() has already been setup externally.
    // the argument is optional for backwards compat with old OpenGL
    bool PushToFrameBuffer(vtkRenderer *ren = NULL);

    // Captures the image from the viewport.
    // This doesn't trigger a render, just captures what's currently there in
    // the active buffer.
    bool Capture(vtkRenderer*);

    // Save the image as a png. Useful for debugging.
    void SaveAsPNG(const char* filename);

  private:
    bool Valid;
    int Size[2];
    vtkSmartPointer<vtkUnsignedCharArray> Data;

    void Allocate(int dx, int dy, int numcomps);
  };

protected:
  vtkSynchronizedRenderers();
  ~vtkSynchronizedRenderers();

  struct RendererInfo
  {
    int ImageReductionFactor;
    int Draw;
    int CameraParallelProjection;
    double Viewport[4];
    double CameraPosition[3];
    double CameraFocalPoint[3];
    double CameraViewUp[3];
    double CameraWindowCenter[2];
    double CameraClippingRange[2];
    double CameraViewAngle;
    double CameraParallelScale;
    double EyeTransformMatrix[16];
    double ModelTransformMatrix[16];

    // Save/restore the struct to/from a stream.
    void Save(vtkMultiProcessStream& stream);
    bool Restore(vtkMultiProcessStream& stream);

    void CopyFrom(vtkRenderer*);
    void CopyTo(vtkRenderer*);
  };

  bool UseFXAA;
  vtkFXAAOptions *FXAAOptions;
  vtkOpenGLFXAAFilter *FXAAFilter;

  // These methods are called on all processes as a consequence of corresponding
  // events being called on the renderer.
  virtual void HandleStartRender();
  virtual void HandleEndRender();
  virtual void HandleAbortRender() {}

  virtual void MasterStartRender();
  virtual void SlaveStartRender();

  virtual void MasterEndRender();
  virtual void SlaveEndRender();

  vtkMultiProcessController* ParallelController;
  vtkOpenGLRenderer* Renderer;

  /**
   * Can be used in HandleEndRender(), MasterEndRender() or SlaveEndRender()
   * calls to capture the rendered image. If this->ImageReductionFactor, then
   * the image will be capture in this->ReducedImage, otherwise it will be
   * captured in this->FullImage (this->ReducedImage will be pointing to the
   * same image).
   */
  virtual vtkRawImage& CaptureRenderedImage();

  /**
   * Can be used in HandleEndRender(), MasterEndRender() or SlaveEndRender()
   * calls to paste back the image from either this->ReducedImage or
   * this->FullImage info the viewport.
   */
  virtual void PushImageToScreen();

  vtkSynchronizedRenderers* CaptureDelegate;
  vtkRawImage ReducedImage;
  vtkRawImage FullImage;

  bool ParallelRendering;
  int ImageReductionFactor;
  bool WriteBackImages;
  int RootProcessId;
  bool AutomaticEventHandling;

private:
  vtkSynchronizedRenderers(const vtkSynchronizedRenderers&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSynchronizedRenderers&) VTK_DELETE_FUNCTION;

  class vtkObserver;
  vtkObserver* Observer;
  friend class vtkObserver;

  double LastViewport[4];
};

#endif

