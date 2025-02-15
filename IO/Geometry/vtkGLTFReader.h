// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGLTFReader
 * @brief   Read a GLTF file.
 *
 * vtkGLTFReader is a concrete subclass of vtkMultiBlockDataSetAlgorithm that reads glTF 2.0 files.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * This reader currently outputs a vtkMultiBlockDataSet containing geometry information
 * for the current selected scene, with animations, skins and morph targets applied, unless
 * configured not to (see ApplyDeformationsToGeometry).
 *
 * It is possible to get information about available scenes and animations by using the
 * corresponding accessors.
 * To use animations, first call SetFramerate with a non-zero value,
 * then use EnableAnimation or DisableAnimation to configure which animations you would like to
 * apply to the geometry.
 * Finally, use UPDATE_TIME_STEPS to choose which frame to apply.
 * If ApplyDeformationsToGeometry is set to true, the reader will apply the deformations, otherwise,
 * animation transformation information will be saved to the dataset's FieldData.
 *
 * Materials are currently not supported in this reader. If you would like to display materials,
 * please try using vtkGLTFImporter.
 * You could also use vtkGLTFReader::GetTexture, to access the image data that was loaded from
 * the glTF 2.0 document.
 *
 * This reader only supports assets that use the 2.x version of the glTF specification.
 *
 * If Stream is not nullptr, it will have priority against FileName.
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 *
 * @sa
 * vtkMultiBlockDataSetAlgorithm
 * vtkGLTFImporter
 */

#ifndef vtkGLTFReader_h
#define vtkGLTFReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkResourceStream.h" // For vtkResourceStream
#include "vtkSmartPointer.h"   // For vtkSmartPointer
#include "vtkURILoader.h"      // For vtkURILoader

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkFieldData;
class vtkGLTFDocumentLoader;
class vtkImageData;
class vtkStringArray;
class vtkTexture;
class vtkGLTFTexture;

class VTKIOGEOMETRY_EXPORT vtkGLTFReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGLTFReader* New();
  vtkTypeMacro(vtkGLTFReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Materials are not directly applied to this reader's output.
   * Use GetGLTFTexture to access a specific texture's image data, and the indices present in the
   * output dataset's field data to create vtkTextures and apply them to the geometry.
   * Note that texture coordinates need to be flipped using a texture transform.
   */
  struct VTK_DEPRECATED_IN_9_4_0("Use vtkGLTFTexture instead.") GLTFTexture
  {
    vtkSmartPointer<vtkImageData> Image;
    unsigned short MinFilterValue;
    unsigned short MaxFilterValue;
    unsigned short WrapSValue;
    unsigned short WrapTValue;
  };

  vtkIdType GetNumberOfTextures();
  VTK_DEPRECATED_IN_9_4_0("Use GetTexture() instead.")
  GLTFTexture GetGLTFTexture(vtkIdType textureIndex);
  vtkSmartPointer<vtkGLTFTexture> GetTexture(vtkIdType textureIndex);
  ///@}

  ///@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/Get the stream from which to read the glTF.
   * If Stream is not nullptr, it will have priority against FileName
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

  ///@{
  /**
   * Set/Get the position in the Stream where the GLB starts. By default it is 0,
   * but can be different than 0 for file formats have a GLB embedded in it,
   * for instance 3D Tiles B3DM.
   */
  vtkSetMacro(GLBStart, vtkTypeInt64);
  vtkGetMacro(GLBStart, vtkTypeInt64);
  ///@}

  ///@{
  /**
   * Set/Get the URI loader to use when reading from a Stream.
   * `URILoader` will be used to locate and load other files referenced in the glTF file.
   * If no URI loader is set when reading through a stream, only single file glTF can be read.
   */
  vtkSetSmartPointerMacro(URILoader, vtkURILoader);
  vtkGetSmartPointerMacro(URILoader, vtkURILoader);
  ///@}

  ///@{
  /**
   * The model's skinning transforms are computed and added to the different vtkPolyData objects'
   * field data.
   * If this flag is set to true, the reader will apply those skinning transforms to the model's
   * geometry.
   */
  void SetApplyDeformationsToGeometry(bool flag);
  vtkGetMacro(ApplyDeformationsToGeometry, bool);
  vtkBooleanMacro(ApplyDeformationsToGeometry, bool);
  ///@}

  ///@{
  /**
   * glTF models can contain multiple animations, with various names and duration. glTF does not
   * specify however any runtime behavior (order of playing, auto-start, loops, mapping of
   * timelines, etc), which is why no animation is enabled by default.
   * These accessors expose metadata information about a model's available animations.
   */
  vtkGetMacro(NumberOfAnimations, vtkIdType);
  std::string GetAnimationName(vtkIdType animationIndex);
  float GetAnimationDuration(vtkIdType animationIndex);
  ///@}

  ///@{
  /**
   * Enable/Disable an animation. The reader will apply all enabled animations to the model's
   * transformations, at the specified time step. Use UPDATE_TIME_STEP to select which frame should
   * be applied.
   */
  void EnableAnimation(vtkIdType animationIndex);
  void DisableAnimation(vtkIdType animationIndex);
  bool IsAnimationEnabled(vtkIdType animationIndex);
  ///@}

  ///@{
  /**
   * glTF models can contain multiple scene descriptions.
   * These accessors expose metadata information about a model's available scenes.
   */
  std::string GetSceneName(vtkIdType sceneIndex);
  vtkGetMacro(NumberOfScenes, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/Set the scene to be used by the reader
   */
  vtkGetMacro(CurrentScene, vtkIdType);
  vtkSetMacro(CurrentScene, vtkIdType);
  void SetScene(const std::string& scene);
  ///@}

  ///@{
  /**
   * Get/Set the rate at which animations will be sampled:
   * the glTF format does not have the concept of static timesteps.
   * TimeSteps are generated, during the REQUEST_INFORMATION pass,
   * as linearly interpolated time values between 0s and the animations' maximum durations,
   * sampled at the specified frame rate.
   * Use the TIME_STEPS information key to obtain integer indices to each of these steps.
   * Set to 0 to not set any TIME_STEPS and only provide a TIME_RANGE.
   */
  vtkGetMacro(FrameRate, unsigned int);
  vtkSetMacro(FrameRate, unsigned int);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. The default is vtkAlgorithm::SINGLE_PRECISION but
   * a 'matrix' or other transforms in the GLTF json could require vtkAlgorithm::DOUBLE_PRECISION.
   * This feature is useful for the Cesium 3DTiles format.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  /**
   * Get a list all scenes names as a vtkStringArray, with duplicate names numbered and empty names
   * replaced by a generic name. All names are guaranteed to be unique, and their index in the array
   * matches the glTF document's scene indices.
   */
  vtkStringArray* GetAllSceneNames();

  /**
   * Get the vtkDataArraySelection object to enable/disable animations.
   */
  vtkDataArraySelection* GetAnimationSelection();

protected:
  vtkGLTFReader();
  ~vtkGLTFReader() override;

  vtkSmartPointer<vtkGLTFDocumentLoader> Loader;

  vtkSmartPointer<vtkMultiBlockDataSet> OutputDataSet;

  std::vector<vtkSmartPointer<vtkGLTFTexture>> Textures;

  /**
   * Create and store GLTFTexture struct for each image present in the model.
   */
  void StoreTextureData();

  /**
   * Initialize the document loader.
   * Can be subclassed to instantiate a custom loader.
   */
  virtual void InitializeLoader();

  char* FileName = nullptr;
  vtkSmartPointer<vtkResourceStream> Stream;
  vtkTypeInt64 GLBStart = 0;
  vtkMTimeType LastStreamTimeStamp = 0;
  vtkSmartPointer<vtkURILoader> URILoader;

  vtkIdType CurrentScene = 0;
  unsigned int FrameRate = 60;
  vtkIdType NumberOfAnimations = 0;
  vtkIdType NumberOfScenes = 0;

  bool IsModelLoaded = false;
  bool IsMetaDataLoaded = false;

  bool ApplyDeformationsToGeometry = true;
  int OutputPointsPrecision = vtkAlgorithm::SINGLE_PRECISION;

  vtkSmartPointer<vtkStringArray> SceneNames;

  vtkSmartPointer<vtkDataArraySelection> PreviousAnimationSelection;
  vtkSmartPointer<vtkDataArraySelection> AnimationSelection;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Create the SceneNames array, generate unique identifiers for each scene based on their glTF
   * name, then fill the SceneNames array with the generated identifiers.
   */
  void CreateSceneNamesArray();

  /**
   * Fill the AnimationSelection vtkDataArraySelection with animation names. Names are adapted from
   * the glTF document to ensure that they are unique and non-empty.
   */
  void CreateAnimationSelection();

private:
  vtkGLTFReader(const vtkGLTFReader&) = delete;
  void operator=(const vtkGLTFReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
