// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeFrustumCuller_h
#define vtkWebGPUComputeFrustumCuller_h

#include "vtkCuller.h"
#include "vtkNew.h"                   // for new macro
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkSmartPointer.h"          // for the pipeline smart pointer
#include "vtkWebGPUComputePipeline.h" // for the member compute pipeline

VTK_ABI_NAMESPACE_BEGIN

/**
 * This culler culls props to the camera view frustum using WebGPU compute shaders.
 *
 * To use this culler, simply instantiate it:
 *
 * vtkNew<vtkWebGPUComputeFrustumCuller> webgpuFrustumCuller;
 *
 * and add it to the cullers of your renderer. Note that by default, the renderer contains a
 * vtkFrustumCoverageCuller. You probably want to remove it from the renderer before adding the GPU
 * compute frustum culler as they are pretty much redundant with each other
 *
 * // Removing the default culler
 * renderer->GetCullers()->RemoveAllItems();
 *
 * // Adding the WebGPU compute shader frustum culler
 * renderer->GetCullers()->AddItem(webgpuFrustumCuller);
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeFrustumCuller : public vtkCuller
{
public:
  vtkTypeMacro(vtkWebGPUComputeFrustumCuller, vtkCuller);
  static vtkWebGPUComputeFrustumCuller* New();

  virtual double Cull(
    vtkRenderer* renderer, vtkProp** propList, int& listLength, int& initialized) override;

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkWebGPUComputeFrustumCuller();
  ~vtkWebGPUComputeFrustumCuller() override;

private:
  vtkWebGPUComputeFrustumCuller(const vtkWebGPUComputeFrustumCuller&) = delete;
  void operator=(const vtkWebGPUComputeFrustumCuller&) = delete;

  /**
   * Creates the input bounds WebGPU buffer and adds it to the pipeline
   */
  void CreateInputBoundsBuffer(vtkProp** propList, unsigned int nbProps);

  /**
   * Creates the buffer that will contain the indices of the objects that were not culled
   */
  void CreateOutputIndicesBuffer(int nbProps);

  /**
   * Creates the uniform buffer that contains the view-projection matrix data of the camera.
   *
   * @warning The given vector is expected to contain the data of the view-projection matrix in
   * column major order (so the matrices returned by a vtkCamera need to be transposed before going
   * to WebGPU)
   */
  void CreateViewProjMatrixBuffer(const std::vector<float>& matrixData);

  /**
   * Reconfigures the culler so that it can handle a new number of props.
   *
   * This encompasses the size of the WebGPU bounds buffer, the vector for the cached props
   * positions etc...
   */
  void ResizeCuller(vtkProp** propList, int propsCount);

  /**
   * Resizes the buffer that contains the bounds of the objects to be culled and uploads the new
   * bounds
   */
  void ResizeBoundsBuffer(vtkProp** propList, int newPropsCount);

  /**
   * Resizes the buffer that will contain the indices of the objects that were not culled
   */
  void ResizeOutputIndicesBuffer(int newPropsCount);

  /**
   * Resizes the scratch list used by the OutputObjectIndicesMapCallback and fills it with the
   * addresses of the props of the propList
   */
  void ResizeScratchList(vtkProp** propList, int listLength);

  /**
   * Forces the recomputation of the bounds of the propList given by calling GetBounds().
   *
   * Bounds are lazily recomputed in VTK (only when GetBounds() is called) and in particular, they
   * are not recomputed when an actor's position changed. This means that an actor that is out of
   * the view frustum and that is culled, if moved into the view frustum will still be culled
   * because its bounds will not have been recomputed (unless GetBounds() was called).
   */
  void TriggerBoundsRecomputation(vtkProp** propList, int listLength);

  /**
   * Reuploads the bounds of the actors
   */
  void UpdateBoundsBuffer(vtkProp** propList, int listLength);

  /**
   * Re-uploads the camera data to the GPU
   */
  void UpdateCamera(vtkRenderer* renderer);

  /**
   * Callback that reads the number of objects that passed the culling test and that stores the
   * result in the listLength parameter of the Cull() method (passed through userdata).
   */
  static void OutputObjectCountMapCallback(const void* mappedData, void* userdata);

  /**
   * Reads the indices of the objects that passed the culling test and stores those indices at the
   * front of the vtkPropList passed in the OutputIndicesCallbackData structure passed through
   * userdata.
   */
  static void OutputObjectIndicesMapCallback(const void* mappedData, void* userdata);

  /**
   * Callback data passed to the OutputObjectIndicesMapCallback callback function.
   * indicesCount indicates how many objects passed the culling test. This value was retrieved
   * earlier by mapping the OutputObjectCountBuffer.
   * Because we're reading the props from the prop list and writing the results directly to the prop
   * list, we have a risk of overwriting the prop list before having the chance to read it. For
   * example:
   * - If the indices to copy are [0, 2, 1] and the prop list (list of addresses) is
   * [0x10, 0x20, 0x30], the final prop list is going to be [0x10, 0x30, 0x30] because the index '1'
   * at the end of the indices to copy now refers to 0x30 whereas it should have been referring to
   * 0x20.
   *
   * The scratch list helps us prevent this issue by keeping a "sane" list of props to read from
   */
  struct OutputIndicesCallbackData
  {
    // The list of props. This should be the same list as passed in parameter to the Cull() method
    vtkProp** propList = nullptr;
    // How many props passed the culling test? This should be the value contained in the OutputCount
    // WebGPU buffer
    int* indicesCount = nullptr;

    // The scratch list is a pointer on the pre-allocated vector stored in this compute culler
    std::vector<vtkProp*>* scratchList = nullptr;
  };

  // How many props was the culler last configured for. This variable is used to determine if the
  // number of props to be culled has changed since last time, meaning that we have to recreate a
  // new bounds buffer, a new PropsBounds cache std::vector etc...
  int PreviousPropsCount = -1;

  // Compute pipeline used for the frustum culling compute shader
  vtkSmartPointer<vtkWebGPUComputePipeline> Pipeline;

  // Frustum culling compute shader pass
  vtkSmartPointer<vtkWebGPUComputePass> FrustumCullingPass;

  // Scratch list used by the OutputObjectIndicesMapCallback
  std::vector<vtkProp*> CallbackScratchList;

  // Index of the input bounds buffer in the compute pipeline
  int InputBoundsBufferIndex = -1;
  // Index of the buffer that contains the view-projection matrix of the camera
  int CameraViewProjMatrixBufferIndex = -1;
  // Buffer that will contain the indices of the objects that were not culled.
  // This buffer will need to be initialized when the cull method is called because we don't know
  // how many actor we're going to cull yet (and thus we cannot initialize the size of this buffer)
  int OutputIndicesBufferIndex = -1;
  // Index of the buffer that contains the number of actors that were not culled
  int OutputObjectCountBufferIndex = -1;
};

VTK_ABI_NAMESPACE_END

#endif
