// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGLTFImporter
 * @brief   Import a GLTF file.
 *
 * vtkGLTFImporter is a concrete subclass of vtkImporter that reads glTF 2.0
 * files.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * This importer supports all physically-based rendering material features, with the exception of
 * alpha masking and mirrored texture wrapping, which are not supported.
 *
 * This importer does not support materials that use multiple
 * sets of texture coordinates. Only the first set will be used in this case.
 *
 * This importer does not support animations, morphing and skinning. If you would like to use
 * animations, morphing or skinning, please consider using vtkGLTFReader.
 *
 * This importer only supports assets that use the 2.x version of the glTF specification.
 *
 * This importer support recovering scene hierarchy partially, only actors are available.
 *
 * This importer supports the collection API
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 *
 * Supported extensions:
 * - KHR_lights_punctual :
 *   The importer supports the KHR_lights_punctual extension except for this feature:
 *   - VTK does not support changing the falloff of the cone with innerConeAngle and outerConeAngle.
 *     The importer uses outerConeAngle and ignores innerConeAngle as specified for this situation.
 *
 * @sa
 * vtkImporter
 * vtkGLTFReader
 */

#ifndef vtkGLTFImporter_h
#define vtkGLTFImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"
#include "vtkResourceStream.h" // For Stream
#include "vtkSmartPointer.h"   // For SmartPointer
#include "vtkURILoader.h"      // For URILoader

#include <map>    // For map
#include <vector> // For vector

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkActor;
class vtkCamera;
class vtkGLTFDocumentLoader;
class vtkTexture;
class vtkURILoader;

class VTKIOIMPORT_EXPORT vtkGLTFImporter : public vtkImporter
{
public:
  static vtkGLTFImporter* New();

  vtkTypeMacro(vtkGLTFImporter, vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the file to read.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the glTF source stream to read from. When selecting the input method, `Stream` has a
   * higher priority than `FileName` i.e. if a stream is provided, the filename is ignored.
   *
   * \note If the stream contains non-data URIs, specifying a custom uri loader is crucial.
   * \sa SetStreamURILoader()
   *
   * \sa SetStreamIsBinary()
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

  ///@{
  /**
   * Specify a custom URI loader for non-data URIs in the input stream.
   * \sa SetStream(), SetStreamIsBinary()
   */
  vtkSetSmartPointerMacro(StreamURILoader, vtkURILoader);
  vtkGetSmartPointerMacro(StreamURILoader, vtkURILoader);
  ///@}

  ///@{
  /**
   * Set/Get whether the input stream is binary
   *
   * \sa SetStream()
   */
  vtkSetMacro(StreamIsBinary, bool);
  vtkGetMacro(StreamIsBinary, bool);
  vtkBooleanMacro(StreamIsBinary, bool);
  ///@}

  /**
   * glTF defines multiple camera objects, but no default behavior for which camera should be
   * used. The importer will by default apply the asset's first camera. This accessor lets you use
   * the asset's other cameras.
   */
  vtkSmartPointer<vtkCamera> GetCamera(unsigned int id);

  /**
   * Get a printable string describing all outputs
   */
  std::string GetOutputsDescription() override { return this->OutputsDescription; }

  /**
   * update timestep
   */
  bool UpdateAtTimeValue(double timeValue) override;

  /**
   * Get the level of animation support in this importer, which is always
   * AnimationSupportLevel::MULTI
   */
  AnimationSupportLevel GetAnimationSupportLevel() override { return AnimationSupportLevel::MULTI; }

  /**
   * Get the number of available animations.
   */
  vtkIdType GetNumberOfAnimations() override;

  /**
   * Return the name of the animation.
   */
  std::string GetAnimationName(vtkIdType animationIndex) override;

  ///@{
  /**
   * Enable/Disable/Get the status of specific animations
   */
  void EnableAnimation(vtkIdType animationIndex) override;
  void DisableAnimation(vtkIdType animationIndex) override;
  bool IsAnimationEnabled(vtkIdType animationIndex) override;
  ///@}

  /**
   * Get the number of available cameras.
   */
  vtkIdType GetNumberOfCameras() override;

  /**
   * Get the name of a camera.
   */
  std::string GetCameraName(vtkIdType camIndex) override;

  /**
   * Enable a specific camera.
   * If a negative index is provided, no camera from the importer is used.
   */
  void SetCamera(vtkIdType camIndex) override;

  /**
   * Get temporal information for the provided animationIndex and frameRate.
   * frameRate is used to define the number of frames for one second of simulation,
   * set to zero if timeSteps are not needed.
   * If animation is present in the dataset, timeRange will be set by this method, return true.
   * If animation is present and frameRate > 0, nbTimeSteps and timeSteps will also be set, return
   * true. If animation is not present, return false.
   */
  bool GetTemporalInformation(vtkIdType animationIndex, double frameRate, int& nbTimeSteps,
    double timeRange[2], vtkDoubleArray* timeSteps) override;

protected:
  vtkGLTFImporter() = default;
  ~vtkGLTFImporter() override;

  /**
   * Initialize the document loader.
   * Can be subclassed to instantiate a custom loader.
   */
  virtual void InitializeLoader();

  int ImportBegin() override;
  void ImportActors(vtkRenderer* renderer) override;
  void ImportCameras(vtkRenderer* renderer) override;
  void ImportLights(vtkRenderer* renderer) override;

  void ApplySkinningMorphing();

  /**
   * Apply properties on the armature actors.
   * By default, the armature is represented with spheres for joints
   * and tubes for bones.
   * Can be subclassed to change properties.
   */
  virtual void ApplyArmatureProperties(vtkActor* actor);

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;
  vtkSmartPointer<vtkURILoader> StreamURILoader;
  bool StreamIsBinary = false;

  std::map<int, vtkSmartPointer<vtkCamera>> Cameras;
  std::map<int, vtkSmartPointer<vtkTexture>> Textures;
  std::map<int, std::vector<vtkSmartPointer<vtkActor>>> Actors;
  vtkSmartPointer<vtkGLTFDocumentLoader> Loader;
  std::string OutputsDescription;
  std::vector<bool> EnabledAnimations;
  vtkIdType EnabledCamera = -1;

private:
  vtkGLTFImporter(const vtkGLTFImporter&) = delete;
  void operator=(const vtkGLTFImporter&) = delete;

  std::map<int, vtkSmartPointer<vtkActor>> ArmatureActors;
};

VTK_ABI_NAMESPACE_END
#endif
