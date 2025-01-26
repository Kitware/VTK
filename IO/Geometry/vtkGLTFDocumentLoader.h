// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGLTFDocumentLoader
 * @brief   Deserialize a GLTF model file.
 *
 *
 * vtkGLTFDocument loader is an internal utility class which defines data structures and functions
 * with the purpose of deserializing a glTF model from a glTF file, loading its data from binary
 * buffers and creating vtk objects with the extracted geometry.
 * It contains an internal Model structure into which all loading is performed.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * For the full specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 */

#ifndef vtkGLTFDocumentLoader_h
#define vtkGLTFDocumentLoader_h

#include "GLTFSampler.h"         // For "Sampler"
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkObject.h"
#include "vtkResourceStream.h" // For "vtkResourceStream"
#include "vtkSmartPointer.h"   // For "vtkSmartPointer"
#include "vtkURILoader.h"      // For "vtkURILoader"

#include <map>    // For std::map
#include <memory> // For std::shared_ptr
#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkFloatArray;
class vtkImageData;
class vtkMatrix4x4;
class vtkPoints;
class vtkPolyData;
class vtkUnsignedShortArray;

class VTKIOGEOMETRY_EXPORT vtkGLTFDocumentLoader : public vtkObject
{
public:
  static vtkGLTFDocumentLoader* New();
  vtkTypeMacro(vtkGLTFDocumentLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Define an openGL draw target.
   */
  enum class Target : unsigned short
  {
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963
  };

  /**
   * Defines an accessor's type. These are defined as strings in the glTF specification.
   * Each type implies a specific number of components.
   */
  enum class AccessorType : unsigned char
  {
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
    INVALID
  };

  /**
   * Define a type for different data components. Values match with the corresponding GLenum, as
   * they are used in the glTF specification.
   */
  enum class ComponentType : unsigned short
  {
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
  };

  /* The following structs help deserialize a glTF document, representing each object. As such,
   * their members mostly match with the specification. Default values and boundaries are set
   * according to the specification.
   * Most of these structs contain a name property, which is optional, and, while being loaded, is
   * not currently exploited by the loader.
   * They are mostly root-level properties, and once created, are stored into vectors in the Model
   * structure.
   */

  /**
   * This struct describes a glTF bufferView object.
   * A bufferView represents a subset of a glTF binary buffer
   */
  struct BufferView
  {
    int Buffer;
    int ByteOffset;
    int ByteLength;
    int ByteStride;
    int Target;
    std::string Name;
  };

  /**
   * This struct describes an accessor glTF object.
   * An accessor defines a method for retrieving data as typed arrays from a bufferView.
   * They contain type information, as well as the location and size of of the data within the
   * bufferView.
   */
  struct Accessor
  {
    /**
     * This struct describes an accessor.sparse glTF object.
     * This object describes the elements that deviate from their initialization value.
     */
    struct Sparse
    {
      int Count;
      int IndicesBufferView;
      int IndicesByteOffset;
      ComponentType IndicesComponentType;
      int ValuesBufferView;
      int ValuesByteOffset;
    };
    int BufferView;
    int ByteOffset;
    ComponentType ComponentTypeValue;
    bool Normalized;
    int Count;
    unsigned int NumberOfComponents;
    AccessorType Type;
    std::vector<double> Max;
    std::vector<double> Min;
    bool IsSparse;
    Sparse SparseObject;
    std::string Name;
  };

  /**
   * This struct describes a glTF Morph Target object.
   * A Morph Target is a morphable Mesh where primitives' attributes are obtained by adding the
   * original attributes to a weighted sum of targets attributes.
   * Only three attributes (position, normals and tangents) are supported.
   */
  struct MorphTarget
  {
    // accessor indices from the .gltf file, the map's keys correspond to attribute names
    std::map<std::string, int> AttributeIndices;
    // attribute values
    std::map<std::string, vtkSmartPointer<vtkFloatArray>> AttributeValues;
  };

  /**
   * This struct describes a glTF primitive object.
   * primitives specify vertex attributes, as well as connectivity information for a draw call.
   * A primitive also specifies a material and GPU primitive type (e.g: triangle set)
   * Data is first stored as integer indices, pointing to different accessors, then extracted into
   * vtk data structures and finally used to build a vtkPolyData object.
   */
  struct Primitive
  {
    // accessor indices from the .glTF file, the map's keys correspond to attribute names
    std::map<std::string, int> AttributeIndices;
    int IndicesId;
    vtkSmartPointer<vtkCellArray> Indices;

    // attribute values from buffer data
    std::map<std::string, vtkSmartPointer<vtkDataArray>> AttributeValues;

    vtkSmartPointer<vtkPolyData> Geometry;

    std::vector<MorphTarget> Targets;

    int Material;
    int Mode;
    int CellSize; // 1, 2 or 3, depending on draw mode

    // Primitive-specific extension metadata
    struct Extensions
    {
      // KHR_draco_mesh_compression extension
      // Only metadata are read (decoding and modifying the internal model is not done yet)
      struct KHRDracoMeshCompression
      {
        int BufferView = -1;
        std::map<std::string, int> AttributeIndices;
      };
      Primitive::Extensions::KHRDracoMeshCompression KHRDracoMetaData;
    };
    Primitive::Extensions ExtensionMetaData;
  };

  /**
   * This struct describes a glTF node object.
   * A node represents an object within a scene.
   * Nodes can contain transform properties (stored as vtkMatrix4x4 objects) as well as indices to
   * children nodes, forming a hierarchy. No node may be a direct descendant of more than one node.
   */
  struct Node
  {
    std::vector<int> Children;
    int Camera;
    int Mesh;
    int Skin;

    vtkSmartPointer<vtkMatrix4x4> Transform;
    vtkSmartPointer<vtkMatrix4x4> GlobalTransform;

    bool TRSLoaded;

    vtkSmartPointer<vtkMatrix4x4> Matrix;

    std::vector<float> InitialRotation;
    std::vector<float> InitialTranslation;
    std::vector<float> InitialScale;
    std::vector<float> InitialWeights;
    std::vector<float> Rotation;
    std::vector<float> Translation;
    std::vector<float> Scale;
    std::vector<float> Weights;

    // Object-specific extension metadata
    struct Extensions
    {
      // KHR_lights_punctual extension
      struct KHRLightsPunctual
      {
        int Light = -1;
      };
      Node::Extensions::KHRLightsPunctual KHRLightsPunctualMetaData;
    };
    Node::Extensions ExtensionMetaData;

    std::string Name;

    void UpdateTransform();
  };

  /**
   * This struct describes a glTF mesh object.
   * A mesh contains an array of primitives.
   */
  struct Mesh
  {
    std::vector<struct Primitive> Primitives;
    std::vector<float> Weights;
    std::string Name;
  };

  /**
   * This struct describes a glTF textureInfo object, mostly used in material descriptions
   * They contain two indexes, one to a texture object, and the second being used to construct a
   * string with the format TEXCOORD_<index>, which references a key in mesh.primitives.attributes.
   */
  struct TextureInfo
  {
    int Index = -1;
    int TexCoord = -1;
  };

  /**
   * This struct describes a glTF image object.
   * Images can be referenced either by URI or with a bufferView. mimeType is required in this case.
   */
  struct Image
  {
    int BufferView;
    std::string MimeType;
    std::string Uri;

    vtkSmartPointer<vtkImageData> ImageData;

    std::string Name;
  };

  /**
   * This struct describes a glTF material object.
   * glTF materials are defined using the metallic-roughness model. The values for most properties
   * can be defined using either factors or textures (via textureInfo).
   * Materials also define normal, occlusion and emissive maps.
   */
  struct Material
  {
    enum class AlphaModeType : unsigned char
    {
      OPAQUE,
      MASK,
      BLEND
    };

    struct PbrMetallicRoughness
    {
      TextureInfo BaseColorTexture;
      std::vector<double> BaseColorFactor;

      TextureInfo MetallicRoughnessTexture;
      float MetallicFactor;
      float RoughnessFactor;
    };

    PbrMetallicRoughness PbrMetallicRoughness;

    TextureInfo NormalTexture;
    double NormalTextureScale;
    TextureInfo OcclusionTexture;
    double OcclusionTextureStrength;
    TextureInfo EmissiveTexture;
    std::vector<double> EmissiveFactor;

    AlphaModeType AlphaMode;
    double AlphaCutoff;

    bool DoubleSided;

    std::string Name;

    // extension KHR_materials_unlit
    bool Unlit;
  };

  /**
   * This struct describes a glTF texture object.
   * A texture is defined two indexes, one to an image resource, and the second to a sampler index.
   */
  struct Texture
  {
    int Sampler;
    int Source;
    std::string Name;
  };

  /**
   * This struct describes a glTF sampler object.
   * Samplers specify filter and wrapping options corresponding to GL types.
   */
  struct Sampler : public GLTFSampler
  {
    std::string Name;
  };

  /**
   * This struct describes a glTF scene object.
   * A scene contains a set of indices of nodes to render.
   * Scene.Nodes can be empty, in which case nothing is required to be rendered
   */
  struct Scene
  {
    std::vector<unsigned int> Nodes;
    std::string Name;
  };

  /**
   * This struct describes a glTF asset.
   * It is meant to mimic a .glTF file, containing all its root-level properties, stored as arrays
   * when relevant.
   */
  struct Skin
  {
    std::vector<vtkSmartPointer<vtkMatrix4x4>> InverseBindMatrices;
    std::vector<int> Joints;
    int InverseBindMatricesAccessorId;
    int Skeleton;
    std::string Name;
    vtkSmartPointer<vtkPolyData> Armature;
  };

  /**
   * This struct describes a glTF animation object.
   * Animations contain multiple channel and sampler objects.
   * Channels define the target node and value to be animated.
   * Samplers define keyframes and how to interpolate in between them.
   */
  struct Animation
  {
    struct Sampler
    {
      enum class InterpolationMode : unsigned char
      {
        LINEAR,
        STEP,
        CUBICSPLINE
      };
      InterpolationMode Interpolation;
      unsigned int Input;
      unsigned int Output;
      int NumberOfComponents;

      vtkSmartPointer<vtkFloatArray> InputData;
      vtkSmartPointer<vtkFloatArray> OutputData;

      /**
       * Get the interpolated animation output at time t
       */
      void GetInterpolatedData(float t, size_t numberOfComponents, std::vector<float>* output,
        bool forceStep = false, bool isRotation = false) const;
    };

    struct Channel
    {
      enum class PathType : unsigned char
      {
        ROTATION,
        TRANSLATION,
        SCALE,
        WEIGHTS
      };
      int Sampler;
      int TargetNode;
      PathType TargetPath;
    };

    float Duration; // In seconds
    std::vector<Animation::Channel> Channels;
    std::vector<Animation::Sampler> Samplers;
    std::string Name;
  };

  /**
   * This struct describes a glTF camera object.
   * glTF can define both perspective or orthographic cameras.
   * Some of the struct's members will be unused depending on the camera type.
   */
  struct Camera
  {
    // common properties
    double Znear;
    double Zfar;
    bool IsPerspective; // if not, camera mode is orthographic
    // perspective
    double Xmag;
    double Ymag;
    // orthographic
    double Yfov;
    double AspectRatio;
    std::string Name;
  };

  /**
   * This struct contains extension metadata.
   * This is for extension properties in the root-level 'extensions' object.
   * Object-specific extension metadata is added directly to the extended object (see Node for an
   * example)
   */
  struct Extensions
  {
    // KHR_lights_punctual extension
    struct KHRLightsPunctual
    {
      struct Light
      {
        enum class LightType : unsigned char
        {
          DIRECTIONAL,
          POINT,
          SPOT
        };
        LightType Type;

        std::vector<double> Color;
        double Intensity;
        double Range;

        // Type-specific parameters
        double SpotInnerConeAngle;
        double SpotOuterConeAngle;

        std::string Name;
      };
      std::vector<Light> Lights;
    };
    KHRLightsPunctual KHRLightsPunctualMetaData;
  };

  /**
   * This struct contains all data from a gltf asset
   */
  struct Model
  {
    std::vector<Accessor> Accessors;
    std::vector<Animation> Animations;
    std::vector<std::vector<char>> Buffers;
    std::vector<BufferView> BufferViews;
    std::vector<Camera> Cameras;
    std::vector<Image> Images;
    std::vector<Material> Materials;
    std::vector<Mesh> Meshes;
    std::vector<Node> Nodes;
    std::vector<Sampler> Samplers;
    std::vector<Scene> Scenes;
    std::vector<Skin> Skins;
    std::vector<Texture> Textures;

    Extensions ExtensionMetaData;

    std::string BufferMetaData;
    int DefaultScene;
    std::string FileName;
    vtkSmartPointer<vtkResourceStream> Stream;
    vtkSmartPointer<vtkURILoader> URILoader;
  };

  /**
   * Apply the specified animation, at the specified time value t, to the internal Model. Changes
   * node transforms and morphing weights.
   */
  bool ApplyAnimation(float t, int animationId, bool forceStep = false);

  /**
   * Restore the transforms that were modified by an animation to their initial state
   */
  void ResetAnimation(int animationId);

  ///@{
  /**
   * @brief Load the binary part of a binary glTF (.glb) file.
   * Input can either be a file (LoadFileBuffer) or a stream (LoadStreamBuffer).
   * @return false if no valid binary part was found.
   */
  bool LoadFileBuffer(VTK_FILEPATH const std::string& fileName, std::vector<char>& glbBuffer);
  bool LoadStreamBuffer(vtkResourceStream* stream, std::vector<char>& glbBuffer);
  ///@}

  ///@{
  /**
   * @brief Reset internal Model struct, and serialize glTF metadata (all json information) into it.
   *
   * To load buffers, use LoadModelData.
   * Input can either be a file (LoadModelMetaDataFromFile) or a stream + optional URI loader.
   *
   * @return `true` if internal model is correctly filled, `false` otherwise.
   */
  bool LoadModelMetaDataFromFile(VTK_FILEPATH const std::string& FileName);
  bool LoadModelMetaDataFromStream(vtkResourceStream* stream, vtkURILoader* loader = nullptr);
  ///@}

  /**
   * Load buffer data into the internal Model.
   */
  bool LoadModelData(const std::vector<char>& glbBuffer);

  /**
   * Converts the internal Model's loaded data into more convenient vtk objects.
   */
  bool BuildModelVTKGeometry();

  /**
   * Get the internal Model.
   */
  std::shared_ptr<Model> GetInternalModel();

  /**
   * Returns the number of components for a given accessor type.
   */
  static unsigned int GetNumberOfComponentsForType(vtkGLTFDocumentLoader::AccessorType type);

  /**
   * Get the list of extensions that are supported by this loader
   */
  virtual std::vector<std::string> GetSupportedExtensions();

  /**
   * Get the list of extensions that are used by the current model
   */
  const std::vector<std::string>& GetUsedExtensions();

  /**
   * Concatenate the current node's local transform to its parent's global transform, storing
   * the resulting transform in the node's globalTransform field. Then does the same for the current
   * node's children.
   * Recursive.
   */
  void BuildGlobalTransforms(unsigned int nodeIndex, vtkSmartPointer<vtkMatrix4x4> parentTransform);

  /**
   * Build all global transforms
   */
  void BuildGlobalTransforms();

  /**
   * Compute all joint matrices of the skin of a specific node
   */
  static void ComputeJointMatrices(const Model& model, const Skin& skin, Node& node,
    std::vector<vtkSmartPointer<vtkMatrix4x4>>& jointMats);

  /**
   * Some extensions require a preparation on the model before building VTK objects.
   * For example, a subclass supporting KHR_draco_mesh_compression could override this function
   * to consume the extension metadata and modify the internal model.
   * This is not done in VTK yet which does not modify the internal model once read.
   */
  virtual void PrepareData() {}

  ///@{
  /**
   * Set/Get the Stream start, where the GLB starts. By default it is 0,
   * but can be different than 0 for file formats have a GLB embedded in it,
   * for instance 3D Tiles B3DM.
   */
  vtkSetMacro(GLBStart, vtkTypeInt64);
  vtkGetMacro(GLBStart, vtkTypeInt64);
  ///@}

  ///@{
  /**
   * Set/Get whether to load animation keyframes from buffers
   *
   * Defaults to true
   */
  vtkSetMacro(LoadAnimation, bool);
  vtkGetMacro(LoadAnimation, bool);
  vtkBooleanMacro(LoadAnimation, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to load images from filesystem and bufferView, if available
   *
   * Defaults to true
   */
  vtkSetMacro(LoadImages, bool);
  vtkGetMacro(LoadImages, bool);
  vtkBooleanMacro(LoadImages, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to load inverse bind matrices from buffers into model's Skin structs
   *
   * Defaults to true
   */
  vtkSetMacro(LoadSkinMatrix, bool);
  vtkGetMacro(LoadSkinMatrix, bool);
  vtkBooleanMacro(LoadSkinMatrix, bool);
  ///@}

protected:
  vtkGLTFDocumentLoader() = default;
  ~vtkGLTFDocumentLoader() override = default;

private:
  struct AccessorLoadingWorker;

  struct SparseAccessorLoadingWorker;

  template <typename Type>
  struct BufferDataExtractionWorker;

  vtkGLTFDocumentLoader(const vtkGLTFDocumentLoader&) = delete;
  void operator=(const vtkGLTFDocumentLoader&) = delete;

  /**
   * Load inverse bind matrices from buffers into the model's Skin structs.
   */
  bool LoadSkinMatrixData();

  /**
   * Uses the Primitive's attributeIndices member to extract all vertex attributes from accessors
   * into the Primitive's corresponding vtk arrays.
   */
  bool ExtractPrimitiveAttributes(Primitive& primitive);

  /**
   * Uses all the primitive's different accessor indices to extract the corresponding data from
   * binary buffers. These arrays are then stored in the primitive's different vtk arrays.
   * Extracts connectivity information then calls ExtractPrimitiveAttributes for the vertex
   * attributes
   */
  bool ExtractPrimitiveAccessorData(Primitive& primitive);

  /**
   * Creates and populates the Primitive's geometry vtkPolyData member with all the vertex attribute
   * and connectivity information the Primitive contains.
   */
  bool BuildPolyDataFromPrimitive(Primitive& primitive);

  /**
   * Creates and populates the Skin's geometry vtkPolyData member with all the armature hierarchy
   */
  bool BuildPolyDataFromSkin(Skin& skin);

  /**
   * Load keyframes from buffers.
   */
  bool LoadAnimationData();

  /**
   * Load Model's Images into vtkImageData objects, from filesystem and bufferView when specified.
   */
  bool LoadImageData();

  std::shared_ptr<Model> InternalModel;

  static const std::vector<std::string> SupportedExtensions;
  std::vector<std::string> UsedExtensions;
  vtkTypeInt64 GLBStart = 0;

  /**
   * Selectively load model data
   */
  bool LoadAnimation = true;
  bool LoadImages = true;
  bool LoadSkinMatrix = true;
};

VTK_ABI_NAMESPACE_END
#endif
