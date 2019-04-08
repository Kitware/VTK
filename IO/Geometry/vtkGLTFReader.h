/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGLTFReader
 * @brief   Read a GLTF file.
 *
 * vtkGLTFReader is a source object that reads a glTF file.
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
 *
 * This reader only supports assets that use the 2.x version of the glTF specification.
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 */

#ifndef vtkGLTFReader_h
#define vtkGLTFReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // For SmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

class vtkGLTFDocumentLoader;
class vtkFieldData;
class vtkImageData;

class VTKIOGEOMETRY_EXPORT vtkGLTFReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGLTFReader* New();
  vtkTypeMacro(vtkGLTFReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Materials are not directly applied to this reader's output.
   * Use GetGLTFTexture to access a specific texture's image data, and the indices present in the
   * output dataset's field data to create vtkTextures and apply them to the geometry.
   */
  struct GLTFTexture
  {
    vtkSmartPointer<vtkImageData> Image;
    unsigned short MinFilterValue;
    unsigned short MaxFilterValue;
    unsigned short WrapSValue;
    unsigned short WrapTValue;
  };

  size_t GetNumberOfTextures();
  GLTFTexture GetGLTFTexture(unsigned int textureIndex);
  //@}

  //@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * The model's skinning transforms are computed and added to the different vtkPolyData objects'
   * field data.
   * If this flag is set to true, the reader will apply those skinning transforms to the model's
   * geometry.
   */
  vtkSetMacro(ApplyDeformationsToGeometry, bool);
  vtkGetMacro(ApplyDeformationsToGeometry, bool);
  vtkBooleanMacro(ApplyDeformationsToGeometry, bool);
  //@}

  //@{
  /**
   * glTF models can contain multiple animations, with various names and duration. glTF does not
   * specify however any runtime behavior (order of playing, auto-start, loops, mapping of
   * timelines, etc), which is why no animation is enabled by default.
   * These accessors expose metadata information about a model's available animations.
   */
  vtkGetMacro(NumberOfAnimations, size_t);
  std::string GetAnimationName(unsigned int animationIndex);
  float GetAnimationDuration(unsigned int animationIndex);
  //@}

  //@{
  /**
   * Enable/Disable an animation. The reader will apply all enabled animations to the model's
   * transformations, at the specified time step. Use UPDATE_TIME_STEP to select which frame should
   * be applied.
   */
  void EnableAnimation(unsigned int animationIndex);
  void DisableAnimation(unsigned int animationIndex);
  bool IsAnimationEnabled(unsigned int animationIndex);
  //@}

  //@{
  /**
   * glTF models can contain multiple scene descriptions.
   * These accessors expose metadata information about a model's available scenes.
   */
  std::string GetSceneName(unsigned int sceneIndex);
  vtkGetMacro(NumberOfScenes, size_t);
  //@}

  //@{
  /**
   * Get/Set the scene to be used by the reader
   */
  vtkGetMacro(CurrentScene, unsigned int);
  vtkSetMacro(CurrentScene, unsigned int);
  //@}

  //@{
  /**
   * Get/Set the rate at which animations will be sampled:
   * the glTF format does not have the concept of static timesteps.
   * TimeSteps are generated, during the REQUEST_INFORMATION pass,
   * as linearly interpolated time values between 0s and the animations' maximum durations,
   * sampled at the specified frame rate.
   * Use the TIME_STEPS information key to obtain integer indices to each of these steps.
   */
  vtkGetMacro(FrameRate, unsigned int);
  vtkSetMacro(FrameRate, unsigned int);
  //@}

protected:
  vtkGLTFReader();
  ~vtkGLTFReader() override;

  vtkSmartPointer<vtkGLTFDocumentLoader> Loader;

  vtkSmartPointer<vtkMultiBlockDataSet> OutputDataSet;

  std::vector<GLTFTexture> Textures;

  /**
   * Create and store GLTFTexture struct for each image present in the model.
   */
  void StoreTextureData();

  char* FileName;

  unsigned int CurrentScene;
  unsigned int FrameRate;
  size_t NumberOfAnimations;
  size_t NumberOfScenes;

  std::vector<bool> AnimationEnabledStates;

  bool IsModelLoaded;
  bool IsMetaDataLoaded;
  bool ApplyDeformationsToGeometry;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGLTFReader(const vtkGLTFReader&) = delete;
  void operator=(const vtkGLTFReader&) = delete;
};

#endif
