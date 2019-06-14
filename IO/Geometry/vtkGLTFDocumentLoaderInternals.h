/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFDocumentLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class vtkGLTFDocumentLoaderInternals
 * @brief Internal class for vtkGLTFDocumentLoader
 *
 * This class provides json-related methods for vtkGLTFDocumentLoader
 */

#ifndef vtkGLTFDocumentLoaderInternals_h
#define vtkGLTFDocumentLoaderInternals_h

#include "vtkGLTFDocumentLoader.h" // For vtkGLTFDocumentLoader
#include "vtk_jsoncpp_fwd.h"       // For Json forward declaration

#include <string> // For string
#include <vector> // For vector

class vtkGLTFDocumentLoaderInternals
{
public:
  vtkGLTFDocumentLoaderInternals() = default;

  /**
   * Reset internal Model struct, and serialize glTF metadata (all json information) into it.
   * Fill usedExtensions vector with the list of used and supported extensions in the glTF file.
   * To load buffers, use LoadModelData
   */
  bool LoadModelMetaDataFromFile(std::string& FileName, std::vector<std::string>& usedExtensions);
  vtkGLTFDocumentLoader* Self = nullptr;

  /**
   * Reads the model's buffer metadata, then uses it to load all buffers into the model.
   */
  bool LoadBuffers(bool firstBufferIsGLB);

  static const unsigned short GL_POINTS = 0x0000;
  static const unsigned short GL_LINES = 0x0001;
  static const unsigned short GL_LINE_LOOP = 0x0002;
  static const unsigned short GL_LINE_STRIP = 0x0003;
  static const unsigned short GL_TRIANGLES = 0x0004;
  static const unsigned short GL_TRIANGLE_STRIP = 0x0005;
  static const unsigned short GL_TRIANGLE_FAN = 0x0006;

private:
  /**
   * Load node-level extension metadata into the Node::Extensions struct.
   */
  bool LoadNodeExtensions(
    const Json::Value& root, vtkGLTFDocumentLoader::Node::Extensions& nodeExtensions);

  /**
   * Load root-level extension metadata into the Extensions struct.
   */
  bool LoadExtensions(const Json::Value& root, vtkGLTFDocumentLoader::Extensions& extensions);

  /**
   * Reads a Json value describing a glTF buffer object, then uses this information to load the
   * corresponding binary buffer into an std::vector<char> array.
   * Needs to know the .glTF file's location in order to interpret relative paths.
   */
  bool LoadBuffer(
    const Json::Value& root, std::vector<char>& buffer, const std::string& glTFFileName);

  /**
   * Load a glTF file and parse it into a Json value. File extension can be either .gltf
   * or .glb. In case of a binary glTF file, only the Json part will be read.
   */
  bool LoadFileMetaData(const std::string& fileName, Json::Value& gltfRoot);

  /**
   * Populate a Skin struct with data from a Json variable describing the object.
   * This method only loads metadata from the json file, it does not load the bind matrices from the
   * buffer.
   */
  bool LoadSkin(const Json::Value& root, vtkGLTFDocumentLoader::Skin& skin);

  /**
   * Populate a BufferView struct with data from a Json variable describing the object.
   */
  bool LoadBufferView(const Json::Value& root, vtkGLTFDocumentLoader::BufferView& bufferView);

  /**
   * Populate a Sparse struct with data from a Json variable describing the object.
   */
  bool LoadSparse(const Json::Value& root, vtkGLTFDocumentLoader::Accessor::Sparse& sparse);

  /**
   * Sets an Accessor's min and max fields with values from a Json variable.
   */
  bool LoadAccessorBounds(const Json::Value& root, vtkGLTFDocumentLoader::Accessor& accessor);

  /**
   * Populate a Camera struct with data from a Json variable describing the object.
   */
  bool LoadCamera(const Json::Value& root, vtkGLTFDocumentLoader::Camera& camera);

  /**
   * Populate an Accessor struct with data from a Json variable describing the object.
   */
  bool LoadAccessor(const Json::Value& root, vtkGLTFDocumentLoader::Accessor& accessor);

  /**
   * Populate a Primitive struct with data from a Json variable describing the object.
   * This method only loads integer indices to accessors, it does not extract any value from a
   * buffer.
   */
  bool LoadPrimitive(const Json::Value& root, vtkGLTFDocumentLoader::Primitive& primitive);

  /**
   * Populate a Mesh structure with data from a Json variable describing the object.
   */
  bool LoadMesh(const Json::Value& root, vtkGLTFDocumentLoader::Mesh& mesh);

  /**
   * Populate a TextureInfo struct with data from a Json variable describing the object.
   */
  bool LoadTextureInfo(const Json::Value& root, vtkGLTFDocumentLoader::TextureInfo& textureInfo);

  /**
   * Populate a Material struct with data from a Json variable describing the object.
   */
  bool LoadMaterial(const Json::Value& root, vtkGLTFDocumentLoader::Material& material);

  /**
   * Populate an Animation struct with data from a Json variable describing the object.
   * This function only loads indices to the keyframe accessors, not the data they contain.
   */
  bool LoadAnimation(const Json::Value& root, vtkGLTFDocumentLoader::Animation& animation);

  /**
   * Populate a Scene struct with data from a Json variable describing the object.
   * Does not check for node's existence.
   */
  bool LoadScene(const Json::Value& root, vtkGLTFDocumentLoader::Scene& scene);

  /**
   * Populate a Node struct with data from a Json variable describing the object.
   * Does not check for the node's children's existence.
   */
  bool LoadNode(const Json::Value& root, vtkGLTFDocumentLoader::Node& node);

  /**
   * Populate an Image struct with data from a Json variable describing the object.
   * This loads a glTF object, not an actual image file.
   */
  bool LoadImage(const Json::Value& root, vtkGLTFDocumentLoader::Image& image);

  /**
   * Populate a Texture struct with data from a Json variable describing the object.
   * This loads a glTF object from a Json value, no files are loaded by this function.
   * Discounting the 'name' field, glTF texture objects contain two indices: one to an image
   * object (the object that references to an actual image file), and one to a sampler
   * object (which specifies filter and wrapping options for a texture).
   */
  bool LoadTexture(const Json::Value& root, vtkGLTFDocumentLoader::Texture& texture);

  /**
   * Populate a Sampler struct with data from a Json variable describing the object.
   */
  bool LoadSampler(const Json::Value& root, vtkGLTFDocumentLoader::Sampler& sampler);

  /**
   * Associates an accessor type string to the corresponding enum value.
   */
  vtkGLTFDocumentLoader::AccessorType AccessorTypeStringToEnum(std::string typeName);

  /**
   * Associate a material's alphaMode string to the corresponding enum value.
   */
  vtkGLTFDocumentLoader::Material::AlphaModeType MaterialAlphaModeStringToEnum(
    std::string alphaModeString);

  /**
   * Load node-specific KHR_lights_punctual metadata into the Node::Extensions::KHRLightsPunctual
   * struct (load light indices).
   */
  bool LoadKHRLightsPunctualNodeExtension(const Json::Value& root,
    vtkGLTFDocumentLoader::Node::Extensions::KHRLightsPunctual& lightsExtension);

  /**
   * Load root-level KHR_lights_punctual metadata into the Extensions::KHRLightsPunctual struct
   * (load all lights).
   */
  bool LoadKHRLightsPunctualExtension(
    const Json::Value& root, vtkGLTFDocumentLoader::Extensions::KHRLightsPunctual& lights);

  /**
   * Load a KHR_lights_punctual light object into the Extensions::KHRLightsPunctual::Light struct.
   */
  bool LoadKHRLightsPunctualExtensionLight(
    const Json::Value& root, vtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light& light);
};

#endif

// VTK-HeaderTest-Exclude: vtkGLTFDocumentLoaderInternals.h
