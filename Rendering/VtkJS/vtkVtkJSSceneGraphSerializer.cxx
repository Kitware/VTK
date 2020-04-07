/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVtkJSSceneGraphSerializerGraphSerializer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVtkJSSceneGraphSerializer.h"

#include "vtksys/MD5.h"
#include <vtkActor.h>
#include <vtkAlgorithm.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCollectionIterator.h>
#include <vtkCompositeDataIterator.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkGlyph3DMapper.h>
#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkLightCollection.h>
#include <vtkLookupTable.h>
#include <vtkMapper.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkViewNode.h>
#include <vtkViewNodeCollection.h>
#include <vtksys/SystemTools.hxx>

#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper2.h>
#endif

#include <array>
#include <ios>
#include <sstream>
#include <unordered_map>

//----------------------------------------------------------------------------
namespace
{
static const std::array<char, 13> arrayTypes = {
  ' ', // VTK_VOID            0
  ' ', // VTK_BIT             1
  'b', // VTK_CHAR            2
  'B', // VTK_UNSIGNED_CHAR   3
  'h', // VTK_SHORT           4
  'H', // VTK_UNSIGNED_SHORT  5
  'i', // VTK_INT             6
  'I', // VTK_UNSIGNED_INT    7
  'l', // VTK_LONG            8
  'L', // VTK_UNSIGNED_LONG   9
  'f', // VTK_FLOAT          10
  'd', // VTK_DOUBLE         11
  'L'  // VTK_ID_TYPE        12
};

static const std::unordered_map<char, std::string> javascriptMapping = { { 'b', "Int8Array" },
  { 'B', "Uint8Array" }, { 'h', "Int16Array" }, { 'H', "Int16Array" }, { 'i', "Int32Array" },
  { 'I', "Uint32Array" }, { 'l', "Int32Array" }, { 'L', "Uint32Array" }, { 'f', "Float32Array" },
  { 'd', "Float64Array" } };

static const std::string getJSArrayType(vtkDataArray* array)
{
  return javascriptMapping.at(arrayTypes.at(array->GetDataType()));
}

static const Json::Value getRangeInfo(vtkDataArray* array, vtkIdType component)
{
  double r[2];
  array->GetRange(r, component);
  Json::Value compRange;
  compRange["min"] = r[0];
  compRange["max"] = r[1];
  compRange["component"] =
    array->GetComponentName(component) ? array->GetComponentName(component) : Json::Value();
  return compRange;
}

void computeMD5(const unsigned char* content, int size, std::string& hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  vtksysMD5* md5 = vtksysMD5_New();
  vtksysMD5_Initialize(md5);
  vtksysMD5_Append(md5, content, size);
  vtksysMD5_Finalize(md5, digest);
  vtksysMD5_DigestToHex(digest, md5Hash);
  vtksysMD5_Delete(md5);

  hash = md5Hash;
}

std::string ptrToString(void* ptr)
{
  std::stringstream s;
  s << std::hex << reinterpret_cast<uintptr_t>(ptr);
  return s.str();
}
}

//----------------------------------------------------------------------------
struct vtkVtkJSSceneGraphSerializer::Internal
{
  Internal()
    : UniqueIdCount(0)
  {
  }

  Json::Value Root;
  std::unordered_map<void*, Json::ArrayIndex> UniqueIds;
  std::size_t UniqueIdCount;
  std::vector<std::pair<Json::ArrayIndex, vtkDataObject*> > DataObjects;
  std::vector<std::pair<std::string, vtkDataArray*> > DataArrays;

  Json::Value* entry(const std::string& index, Json::Value* node);
  Json::Value* entry(const Json::ArrayIndex index) { return entry(std::to_string(index), &Root); }
  Json::Value* entry(void* address) { return entry(UniqueIds.at(address)); }

  Json::ArrayIndex uniqueId(void* ptr = nullptr);
};

Json::Value* vtkVtkJSSceneGraphSerializer::Internal::entry(
  const std::string& index, Json::Value* node)
{
  if (node == nullptr || (*node)["id"] == index)
  {
    return node;
  }

  if (node->isMember("dependencies"))
  {
    for (Json::ArrayIndex i = 0; i < (*node)["dependencies"].size(); ++i)
    {
      Json::Value* n = entry(index, &(*node)["dependencies"][i]);
      if (n != nullptr)
      {
        return n;
      }
    }
  }

  return nullptr;
}

Json::ArrayIndex vtkVtkJSSceneGraphSerializer::Internal::uniqueId(void* ptr)
{
  Json::ArrayIndex id;
  if (ptr == nullptr)
  {
    // There is no associated address for this unique id.
    id = Json::ArrayIndex(UniqueIdCount++);
  }
  else
  {
    // There is an associated address for this unique id, so we use it to ensure
    // that subsequent calls will return the same id.
    auto search = UniqueIds.find(ptr);
    if (search != UniqueIds.end())
    {
      id = search->second;
    }
    else
    {
      id = Json::ArrayIndex(UniqueIdCount++);
      UniqueIds[ptr] = id;
    }
  }
  return id;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVtkJSSceneGraphSerializer);
//----------------------------------------------------------------------------
vtkVtkJSSceneGraphSerializer::vtkVtkJSSceneGraphSerializer()
  : Internals(new vtkVtkJSSceneGraphSerializer::Internal)
{
}

//----------------------------------------------------------------------------
vtkVtkJSSceneGraphSerializer::~vtkVtkJSSceneGraphSerializer()
{
  delete Internals;
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Reset()
{
  this->Internals->Root = Json::Value();
  this->Internals->UniqueIds.clear();
  this->Internals->UniqueIdCount = 0;
  this->Internals->DataObjects.clear();
  this->Internals->DataArrays.clear();
}

//----------------------------------------------------------------------------
const Json::Value& vtkVtkJSSceneGraphSerializer::GetRoot() const
{
  return this->Internals->Root;
}

//----------------------------------------------------------------------------
vtkIdType vtkVtkJSSceneGraphSerializer::GetNumberOfDataObjects() const
{
  return vtkIdType(this->Internals->DataObjects.size());
}

//----------------------------------------------------------------------------
Json::ArrayIndex vtkVtkJSSceneGraphSerializer::GetDataObjectId(vtkIdType i) const
{
  return this->Internals->DataObjects.at(i).first;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkVtkJSSceneGraphSerializer::GetDataObject(vtkIdType i) const
{
  return this->Internals->DataObjects.at(i).second;
}

//----------------------------------------------------------------------------
vtkIdType vtkVtkJSSceneGraphSerializer::GetNumberOfDataArrays() const
{
  return vtkIdType(this->Internals->DataArrays.size());
}

//----------------------------------------------------------------------------
std::string vtkVtkJSSceneGraphSerializer::GetDataArrayId(vtkIdType i) const
{
  return this->Internals->DataArrays.at(i).first;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkVtkJSSceneGraphSerializer::GetDataArray(vtkIdType i) const
{
  return this->Internals->DataArrays.at(i).second;
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkActor* actor)
{
  // Skip actors that are connected to composite mappers (they are dealt with
  // when the mapper is traversed).
  //
  // TODO: this is an awkward consequence of an external scene graph traversal
  //       mechanism where we cannot abort the traversal of subordinate nodes
  //       and an imperfect parity between VTK and vtk-js (namely the lack of
  //       support in vtk-js for composite data structures). This logic should
  //       be removed when vtk-js support for composite data structures is in
  //       place.
  {
    vtkViewNodeCollection* children = node->GetChildren();
    if (children->GetNumberOfItems() > 0)
    {
      children->InitTraversal();

      for (vtkViewNode* child = children->GetNextItem(); child != nullptr;
           child = children->GetNextItem())
      {
        if (vtkCompositePolyDataMapper::SafeDownCast(child->GetRenderable())
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
          || vtkCompositePolyDataMapper2::SafeDownCast(child->GetRenderable())
#endif
        )
        {
          return;
        }
      }
    }
  }

  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, actor);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("addViewProp");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(Json::Value* self, vtkAlgorithm* algorithm)
{
  algorithm->Update();

  // Algorithms have data associated with them, so we construct a unique id for
  // each port and associate it with the data object.
  for (int inputPort = 0; inputPort < algorithm->GetNumberOfInputPorts(); ++inputPort)
  {
    // vtk-js does not support multiple connections, so we always look at
    // connection 0
    static const int connection = 0;
    vtkDataObject* dataObject = algorithm->GetInputDataObject(inputPort, connection);
    Json::ArrayIndex dataId = this->UniqueId(dataObject);
    this->Internals->DataObjects.push_back(std::make_pair(dataId, dataObject));

    (*self)["dependencies"].append(this->ToJson((*self), algorithm, dataObject));
    Json::Value v = Json::arrayValue;
    v.append("setInputData");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + std::to_string(this->UniqueId(dataObject)) + "}");
    w.append(inputPort);
    v.append(w);
    (*self)["calls"].append(v);
  }
}

//----------------------------------------------------------------------------
namespace
{
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
// vtkCompositePolyDataMapper2 provides an API for assigning color and opacity
// to each block in the dataset, but vtkCompositePolyDataMapper does not. This
// logic splits the code to apply per-block coloring when it is available.
template <typename CompositeMapper>
typename std::enable_if<std::is_base_of<vtkCompositePolyDataMapper2, CompositeMapper>::value>::type
SetColorAndOpacity(Json::Value& property, CompositeMapper* mapper, vtkDataObject* block)
{
  static const std::array<std::string, 4> colorProperties = { "ambientColor", "color",
    "diffuseColor", "specularColor" };

  // Set the color and opacity according to the dataset's corresponding block
  // information.
  vtkCompositeDataDisplayAttributes* atts = mapper->GetCompositeDataDisplayAttributes();
  if (atts->HasBlockColor(block))
  {
    for (int i = 0; i < 3; i++)
    {
      for (auto& colorProperty : colorProperties)
      {
        property["properties"][colorProperty.c_str()][i] = atts->GetBlockColor(block)[i];
      }
    }
  }
  if (atts->HasBlockOpacity(block))
  {
    property["properties"]["opacity"] = atts->GetBlockOpacity(block);
  }
  if (atts->HasBlockVisibility(block))
  {
    property["properties"]["visibility"] = atts->GetBlockVisibility(block);
  }
}
#endif

template <typename CompositeMapper>
typename std::enable_if<std::is_base_of<vtkCompositePolyDataMapper, CompositeMapper>::value>::type
SetColorAndOpacity(Json::Value&, CompositeMapper*, vtkDataObject*)
{
}
}

//----------------------------------------------------------------------------
template <typename CompositeMapper>
void vtkVtkJSSceneGraphSerializer::Add(
  vtkViewNode* node, vtkDataObject* dataObject, CompositeMapper* mapper)
{
  if (vtkPolyData::SafeDownCast(dataObject) != nullptr)
  {
    // If the data object associated with the composite mapper is a polydata,
    // treat the mapper as a vtk-js Mapper.

    // First, add an actor for the mapper
    Json::Value* parent;
    {
      Json::Value* renderer =
        this->Internals->entry(node->GetParent()->GetParent()->GetRenderable());
      Json::Value actor =
        this->ToJson(*renderer, vtkActor::SafeDownCast(node->GetParent()->GetRenderable()), true);
      Json::ArrayIndex actorId = this->UniqueId();
      actor["id"] = std::to_string(actorId);

      {
        // Locate the actor's entry for its vtkProperty
        for (Json::Value::iterator it = actor["dependencies"].begin();
             it != actor["dependencies"].end(); ++it)
        {
          if ((*it)["type"] == "vtkProperty")
          {
            // Color the actor using the block color, if available
            SetColorAndOpacity(*it, mapper, dataObject);
            break;
          }
        }
      }

      parent = &(*renderer)["dependencies"].append(actor);
      Json::Value v = Json::arrayValue;
      v.append("addViewProp");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + actor["id"].asString() + "}");
      v.append(w);
      (*renderer)["calls"].append(v);
    }

    // Then, add a new mapper
    {
      Json::ArrayIndex id = this->UniqueId();
      Json::Value value = ToJson(*parent, id, static_cast<vtkMapper*>(mapper), true);

      Json::Value v = Json::arrayValue;
      v.append("setMapper");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + std::to_string(id) + "}");
      v.append(w);
      (*parent)["calls"].append(v);
      parent = &(*parent)["dependencies"].append(value);
    }

    // Finally, add the data object for the mapper
    {
      // Assign the data object a unique id and record it
      Json::ArrayIndex dataId = this->UniqueId(dataObject);
      this->Internals->DataObjects.push_back(std::make_pair(dataId, dataObject));

      (*parent)["dependencies"].append(
        this->ToJson(*parent, static_cast<vtkMapper*>(mapper), dataObject));
      Json::Value v = Json::arrayValue;
      v.append("setInputData");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + std::to_string(dataId) + "}");
      v.append(w);
      (*parent)["calls"].append(v);
    }
  }
  else
  {
    // Otherwise, we must construct a vtk-js Mapper for each nonempty node in
    // the composite dataset.
    vtkCompositeDataSet* composite = vtkCompositeDataSet::SafeDownCast(dataObject);
    vtkSmartPointer<vtkCompositeDataIterator> iter = composite->NewIterator();
    iter->SkipEmptyNodesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      this->Add<CompositeMapper>(node, iter->GetCurrentDataObject(), mapper);
      iter->GoToNextItem();
    }
  }
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkCompositePolyDataMapper* mapper)
{
  this->Add<vtkCompositePolyDataMapper>(node, mapper->GetInputDataObject(0, 0), mapper);
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkCompositePolyDataMapper2* mapper)
{
#if VTK_MODULE_ENABLE_VTK_RenderingOpenGL2
  this->Add<vtkCompositePolyDataMapper2>(node, mapper->GetInputDataObject(0, 0), mapper);
#else
  (void)node;
  (void)mapper;
#endif
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkGlyph3DMapper* mapper)
{
  // TODO: vtkGlyph3DMapper and its derived implementation
  //       vtkOpenGLGlyph3DMapper may have composite datasets for both the glyph
  //       representations and instances. The logic for handling this is rather
  //       complex and is currently inaccessible outside of its implementation.
  //       Rather than duplicate that logic here, there should be exposed
  //       methods on vtkGlyph3DMapper to "flatten" a mapper with composite
  //       inputs into a collection of glyph mappers that use vtkPolyData (as is
  //       currently in the implementation). Until then, we only handle the case
  //       with vtkPolyData for the glyph representations and indices.
  for (int inputPort = 0; inputPort < mapper->GetNumberOfInputPorts(); ++inputPort)
  {
    // vtk-js does not support multiple connections, so we always look at
    // connection 0
    static const int connection = 0;
    vtkDataObject* dataObject = mapper->GetInputDataObject(inputPort, connection);
    if (vtkCompositeDataSet::SafeDownCast(dataObject) != nullptr)
    {
      vtkErrorMacro(<< "Composite data sets are not currently supported for vtk-js glyph mappers.");
      return;
    }
  }

  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, this->UniqueId(mapper), mapper);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("setMapper");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);

  this->Add(this->Internals->entry(node->GetRenderable()), vtkAlgorithm::SafeDownCast(mapper));
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkMapper* mapper)
{
  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, this->UniqueId(mapper), mapper);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("setMapper");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);

  this->Add(this->Internals->entry(node->GetRenderable()), vtkAlgorithm::SafeDownCast(mapper));
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode* node, vtkRenderer* renderer)
{
  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, renderer);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("addRenderer");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::Add(vtkViewNode*, vtkRenderWindow* window)
{
  this->Internals->Root = this->ToJson(window);
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, vtkAlgorithm* algorithm, vtkDataObject* dataObject)
{
  if (vtkImageData* imageData = vtkImageData::SafeDownCast(dataObject))
  {
    return this->ToJson(parent, algorithm, imageData);
  }
  else if (vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataObject))
  {
    return this->ToJson(parent, algorithm, polyData);
  }
  else
  {
    vtkErrorMacro(<< "Cannot export data object of type \"" << dataObject->GetClassName() << "\".");
    return Json::Value();
  }
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, vtkAlgorithm* algorithm, vtkImageData* imageData)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(imageData));
  val["type"] = "vtkImageData";

  Json::Value properties;

  properties["address"] = ptrToString(imageData);

  for (int i = 0; i < 3; i++)
  {
    properties["spacing"][i] = imageData->GetSpacing()[i];
    properties["origin"][i] = imageData->GetOrigin()[i];
  }
  for (int i = 0; i < 6; i++)
  {
    properties["extent"][i] = imageData->GetExtent()[i];
  }

  properties["fields"] = Json::arrayValue;
  this->extractRequiredFields(properties["fields"], vtkMapper::SafeDownCast(algorithm), imageData);

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(vtkDataArray* array)
{
  Json::Value val;
  std::string hash;
  {
    const unsigned char* content = (const unsigned char*)array->GetVoidPointer(0);
    int size = array->GetNumberOfValues() * array->GetDataTypeSize();
    computeMD5(content, size, hash);
  }
  this->Internals->DataArrays.push_back(std::make_pair(hash, array));
  val["hash"] = hash;
  val["vtkClass"] = "vtkDataArray";
  val["name"] = array->GetName() ? array->GetName() : Json::Value();
  val["dataType"] = getJSArrayType(array);
  val["numberOfComponents"] = array->GetNumberOfComponents();
  val["size"] = Json::Value::UInt64(array->GetNumberOfComponents() * array->GetNumberOfTuples());
  val["ranges"] = Json::arrayValue;
  if (array->GetNumberOfComponents() > 1)
  {
    for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
      val["ranges"].append(getRangeInfo(array, i));
    }
    val["ranges"].append(getRangeInfo(array, -1));
  }
  else
  {
    val["ranges"].append(getRangeInfo(array, 0));
  }
  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, vtkAlgorithm* algorithm, vtkPolyData* polyData)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(polyData));
  val["type"] = "vtkPolyData";

  Json::Value properties;

  properties["address"] = ptrToString(polyData);

  {
    properties["points"] = this->ToJson(polyData->GetPoints()->GetData());
    properties["points"]["vtkClass"] = "vtkPoints";
  }

  if (polyData->GetVerts() && polyData->GetVerts()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["verts"] = this->ToJson(polyData->GetVerts()->GetData());
    properties["verts"]["vtkClass"] = "vtkCellArray";
  }

  if (polyData->GetLines() && polyData->GetLines()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["lines"] = this->ToJson(polyData->GetLines()->GetData());
    properties["lines"]["vtkClass"] = "vtkCellArray";
  }

  if (polyData->GetPolys() && polyData->GetPolys()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["polys"] = this->ToJson(polyData->GetPolys()->GetData());
    properties["polys"]["vtkClass"] = "vtkCellArray";
  }

  if (polyData->GetStrips() && polyData->GetStrips()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["strips"] = this->ToJson(polyData->GetStrips()->GetData());
    properties["strips"]["vtkClass"] = "vtkCellArray";
  }

  properties["fields"] = Json::arrayValue;
  this->extractRequiredFields(properties["fields"], vtkMapper::SafeDownCast(algorithm), polyData);

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkProperty* property)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(property));
  val["type"] = "vtkProperty";

  Json::Value properties;

  properties["address"] = ptrToString(property);
  properties["representation"] = property->GetRepresentation();
  for (int i = 0; i < 3; i++)
  {
    properties["diffuseColor"][i] = property->GetDiffuseColor()[i];
    properties["color"][i] = property->GetColor()[i];
    properties["ambientColor"][i] = property->GetAmbientColor()[i];
    properties["specularColor"][i] = property->GetSpecularColor()[i];
    properties["edgeColor"][i] = property->GetEdgeColor()[i];
  }
  properties["ambient"] = property->GetAmbient();
  properties["diffuse"] = property->GetDiffuse();
  properties["specular"] = property->GetSpecular();
  properties["specularPower"] = property->GetSpecularPower();
  properties["opacity"] = property->GetOpacity();
  properties["interpolation"] = property->GetInterpolation();
  properties["edgeVisibility"] = property->GetEdgeVisibility();
  properties["backfaceCulling"] = property->GetBackfaceCulling();
  properties["frontfaceCulling"] = property->GetFrontfaceCulling();
  properties["pointSize"] = property->GetPointSize();
  properties["lineWidth"] = property->GetLineWidth();
  properties["lighting"] = property->GetLighting();

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkTransform* transform)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(transform));
  val["type"] = "vtkTransform";

  Json::Value properties;

  properties["address"] = ptrToString(transform);
  std::array<double, 3> scale;
  transform->GetScale(scale.data());
  for (int i = 0; i < 3; i++)
  {
    properties["scale"][i] = scale[i];
  }
  std::array<double, 4> orientation;
  transform->GetOrientationWXYZ(orientation.data());
  for (int i = 0; i < 4; i++)
  {
    properties["orientationWXYZ"][i] = orientation[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkTexture* texture)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(texture));
  val["type"] = "vtkTexture";

  Json::Value properties;

  properties["address"] = ptrToString(texture);
  properties["repeat"] = texture->GetRepeat();
  properties["edgeClamp"] = texture->GetEdgeClamp();
  properties["interpolate"] = texture->GetInterpolate();
  properties["mipmap"] = texture->GetMipmap();
  properties["maximumAnisotropicFiltering"] = texture->GetMaximumAnisotropicFiltering();
  properties["quality"] = texture->GetQuality();
  properties["colorMode"] = texture->GetColorMode();
  properties["blendingMode"] = texture->GetBlendingMode();
  properties["premulipliedAlpha"] = texture->GetPremultipliedAlpha();
  properties["restrictPowerOf2ImageSmaller"] = texture->GetRestrictPowerOf2ImageSmaller();
  properties["cubeMap"] = texture->GetCubeMap();
  properties["useSRGBColorSpace"] = texture->GetUseSRGBColorSpace();

  vtkLookupTable* lookupTable = vtkLookupTable::SafeDownCast(texture->GetLookupTable());
  if (lookupTable != nullptr)
  {
    Json::Value lut = this->ToJson(val, lookupTable);
    std::string lutId = std::to_string(this->UniqueId(lookupTable));
    lut["id"] = lutId;
    val["dependencies"].append(lut);
    Json::Value v = Json::arrayValue;
    v.append("setLookupTable");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + lutId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  vtkTransform* transform = texture->GetTransform();
  if (transform != nullptr)
  {
    Json::Value trans = this->ToJson(val, transform);
    std::string transId = std::to_string(this->UniqueId(lookupTable));
    trans["id"] = transId;
    val["dependencies"].append(trans);
    Json::Value v = Json::arrayValue;
    v.append("setTransform");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + transId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  val["properties"] = properties;

  this->Add(&val, static_cast<vtkAlgorithm*>(texture));

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, vtkActor* actor, bool newPropertyId)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(actor));
  val["type"] = "vtkActor";

  Json::Value properties;
  properties["address"] = ptrToString(actor);
  for (int i = 0; i < 3; i++)
  {
    properties["origin"][i] = actor->GetOrigin()[i];
    properties["scale"][i] = actor->GetScale()[i];
    properties["position"][i] = actor->GetPosition()[i];
    properties["orientation"][i] = actor->GetOrientation()[i];
  }
  properties["visibility"] = actor->GetVisibility();
  properties["pickable"] = actor->GetPickable();
  properties["dragable"] = actor->GetDragable();
  properties["useBounds"] = actor->GetUseBounds();
  properties["renderTimeMultiplier"] = actor->GetRenderTimeMultiplier();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  vtkProperty* property = vtkProperty::SafeDownCast(actor->GetProperty());
  if (property != nullptr)
  {
    Json::Value prop = this->ToJson(val, property);
    std::string propertyId =
      (newPropertyId ? std::to_string(this->UniqueId()) : std::to_string(this->UniqueId(property)));
    prop["id"] = propertyId;
    val["dependencies"].append(prop);
    Json::Value v = Json::arrayValue;
    v.append("setProperty");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + propertyId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  vtkTexture* texture = actor->GetTexture();
  if (texture != nullptr)
  {
    Json::Value tex = this->ToJson(val, texture);
    std::string textureId = std::to_string(this->UniqueId(texture));
    tex["id"] = textureId;
    val["dependencies"].append(tex);
    Json::Value v = Json::arrayValue;
    v.append("addTexture");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + textureId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkLookupTable* lookupTable)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(lookupTable));
  val["type"] = "vtkLookupTable";

  Json::Value properties;

  properties["address"] = ptrToString(lookupTable);
  properties["numberOfColors"] = static_cast<Json::Value::Int64>(lookupTable->GetNumberOfColors());
  for (int i = 0; i < 2; ++i)
  {
    properties["alphaRange"][i] = lookupTable->GetAlphaRange()[i];
    properties["hueRange"][i] = lookupTable->GetHueRange()[i];
    properties["saturationRange"][i] = lookupTable->GetSaturationRange()[i];
    properties["valueRange"][i] = lookupTable->GetValueRange()[i];
  }
  for (int i = 0; i < 4; ++i)
  {
    properties["nanColor"][i] = lookupTable->GetNanColor()[i];
    properties["belowRangeColor"][i] = lookupTable->GetBelowRangeColor()[i];
    properties["aboveRangeColor"][i] = lookupTable->GetAboveRangeColor()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, Json::ArrayIndex id, vtkMapper* mapper, bool newLUTId)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(id);
  val["type"] = "vtkMapper";

  Json::Value properties;

  properties["address"] = ptrToString(mapper);
  properties["colorByArrayName"] = mapper->GetArrayName();
  properties["arrayAccessMode"] = mapper->GetArrayAccessMode();
  properties["colorMode"] = mapper->GetColorMode();
  properties["fieldDataTupleId"] = static_cast<Json::Value::Int64>(mapper->GetFieldDataTupleId());
  properties["interpolateScalarsBeforeMapping"] = mapper->GetInterpolateScalarsBeforeMapping();
  properties["renderTime"] = mapper->GetRenderTime();
  properties["resolveCoincidentTopology"] = mapper->GetResolveCoincidentTopology();
  properties["scalarMode"] = mapper->GetScalarMode();
  properties["scalarVisibility"] = mapper->GetScalarVisibility();
  properties["static"] = mapper->GetStatic();
  properties["useLookupTableScalarRange"] = mapper->GetUseLookupTableScalarRange();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  vtkLookupTable* lookupTable = vtkLookupTable::SafeDownCast(mapper->GetLookupTable());
  if (lookupTable != nullptr)
  {
    Json::Value lut = this->ToJson(val, lookupTable);
    std::string lutId =
      (newLUTId ? std::to_string(this->UniqueId()) : std::to_string(this->UniqueId(lookupTable)));
    lut["id"] = lutId;
    val["dependencies"].append(lut);
    Json::Value v = Json::arrayValue;
    v.append("setLookupTable");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + lutId + "}");
    v.append(w);
    val["calls"].append(v);
  }
  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, Json::ArrayIndex id, vtkGlyph3DMapper* mapper)
{
  Json::Value val = this->ToJson(parent, id, static_cast<vtkMapper*>(mapper));
  val["type"] = "vtkGlyph3DMapper";

  Json::Value& properties = val["properties"];

  properties["address"] = ptrToString(mapper);
  properties["orient"] = mapper->GetOrient();
  properties["orientationMode"] = mapper->GetOrientationMode();
  properties["scaleFactor"] = mapper->GetScaleFactor();
  properties["scaleMode"] = mapper->GetScaleMode();
  properties["scaling"] = mapper->GetScaling();
  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkCamera* camera)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(camera));
  val["type"] = "vtkCamera";

  Json::Value properties;

  properties["address"] = ptrToString(camera);

  for (int i = 0; i < 3; ++i)
  {
    properties["focalPoint"][i] = camera->GetFocalPoint()[i];
    properties["position"][i] = camera->GetPosition()[i];
    properties["viewUp"][i] = camera->GetViewUp()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkLight* light)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(light));
  val["type"] = "vtkLight";

  Json::Value properties;

  properties["address"] = ptrToString(light);
  properties["intensity"] = light->GetIntensity();
  properties["switch"] = light->GetSwitch();
  properties["positional"] = light->GetPositional();
  properties["exponent"] = light->GetExponent();
  properties["coneAngle"] = light->GetConeAngle();
  std::array<std::string, 4> lightTypes{ "", "HeadLight", "SceneLight", "CameraLight" };
  properties["lightType"] = lightTypes[light->GetLightType()];
  properties["shadowAttenuation"] = light->GetShadowAttenuation();

  for (int i = 0; i < 3; ++i)
  {
    properties["color"][i] = light->GetDiffuseColor()[i];
    properties["focalPoint"][i] = light->GetFocalPoint()[i];
    properties["position"][i] = light->GetPosition()[i];
    properties["attenuationValues"][i] = light->GetAttenuationValues()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, vtkRenderer* renderer)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(renderer));
  val["type"] = renderer->GetClassName();

  Json::Value properties;

  properties["address"] = ptrToString(renderer);
  properties["twoSidedLighting"] = renderer->GetTwoSidedLighting();
  properties["lightFollowCamera"] = renderer->GetLightFollowCamera();
  properties["automaticLightCreation"] = renderer->GetAutomaticLightCreation();
  properties["erase"] = renderer->GetErase();
  properties["draw"] = renderer->GetDraw();
  properties["nearClippingPlaneTolerance"] = renderer->GetNearClippingPlaneTolerance();
  properties["clippingRangeExpansion"] = renderer->GetClippingRangeExpansion();
  properties["backingStore"] = renderer->GetBackingStore();
  properties["interactive"] = renderer->GetInteractive();
  properties["layer"] = renderer->GetLayer();
  properties["preserveColorBuffer"] = renderer->GetPreserveColorBuffer();
  properties["preserveDepthBuffer"] = renderer->GetPreserveDepthBuffer();
  properties["useDepthPeeling"] = renderer->GetUseDepthPeeling();
  properties["occlusionRatio"] = renderer->GetOcclusionRatio();
  properties["maximumNumberOfPeels"] = renderer->GetMaximumNumberOfPeels();
  properties["useShadows"] = renderer->GetUseShadows();
  for (int i = 0; i < 3; ++i)
  {
    properties["background"][i] = renderer->GetBackground()[i];
  }
  properties["background"][3] = 1.;

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  {
    val["dependencies"].append(this->ToJson(val, renderer->GetActiveCamera()));
    Json::Value v = Json::arrayValue;
    v.append("setActiveCamera");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + std::to_string(this->UniqueId(renderer->GetActiveCamera())) + "}");
    v.append(w);
    val["calls"].append(v);
  }

  vtkLightCollection* lights = renderer->GetLights();

  if (lights->GetNumberOfItems() > 0)
  {
    lights->InitTraversal();

    Json::Value v = Json::arrayValue;
    v.append("addLight");
    Json::Value w = Json::arrayValue;
    for (vtkLight* light = lights->GetNextItem(); light != nullptr; light = lights->GetNextItem())
    {
      val["dependencies"].append(this->ToJson(val, light));
      w.append("instance:${" + std::to_string(this->UniqueId(light)) + "}");
    }
    v.append(w);
    val["calls"].append(v);
  }

  return val;
}

//----------------------------------------------------------------------------
Json::Value vtkVtkJSSceneGraphSerializer::ToJson(vtkRenderWindow* renderWindow)
{
  Json::Value val;
  val["parent"] = "0x0";
  val["id"] = std::to_string(this->UniqueId(renderWindow));
  val["type"] = renderWindow->GetClassName();
  val["mtime"] = static_cast<Json::UInt64>(renderWindow->GetMTime());

  Json::Value properties;
  properties["address"] = ptrToString(renderWindow);
  properties["numberOfLayers"] = renderWindow->GetNumberOfLayers();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  return val;
}

//----------------------------------------------------------------------------
Json::ArrayIndex vtkVtkJSSceneGraphSerializer::UniqueId(void* ptr)
{
  return this->Internals->uniqueId(ptr);
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::extractRequiredFields(
  Json::Value& extractedFields, vtkMapper* mapper, vtkDataSet* dataSet)
{
  // FIXME should evolve and support funky mapper which leverage many arrays
  vtkDataArray* pointDataArray = nullptr;
  vtkDataArray* cellDataArray = nullptr;
  if (mapper != nullptr && mapper->IsA("vtkMapper"))
  {
    vtkTypeBool scalarVisibility = mapper->GetScalarVisibility();
    int arrayAccessMode = mapper->GetArrayAccessMode();

    int scalarMode = mapper->GetScalarMode();
    if (scalarVisibility && scalarMode == 3)
    {
      pointDataArray =
        (arrayAccessMode == 1 ? dataSet->GetPointData()->GetArray(mapper->GetArrayName())
                              : dataSet->GetPointData()->GetArray(mapper->GetArrayId()));

      if (pointDataArray != nullptr)
      {
        Json::Value arrayMeta = this->ToJson(pointDataArray);
        arrayMeta["location"] = "pointData";
        extractedFields.append(arrayMeta);
      }
    }

    if (scalarVisibility && scalarMode == 4)
    {
      cellDataArray =
        (arrayAccessMode == 1 ? dataSet->GetCellData()->GetArray(mapper->GetArrayName())
                              : dataSet->GetCellData()->GetArray(mapper->GetArrayId()));
      if (cellDataArray != nullptr)
      {
        Json::Value arrayMeta = this->ToJson(cellDataArray);
        arrayMeta["location"] = "cellData";
        extractedFields.append(arrayMeta);
      }
    }
  }

  if (pointDataArray == nullptr)
  {
    vtkDataArray* array = dataSet->GetPointData()->GetScalars();
    if (array != nullptr)
    {
      Json::Value arrayMeta = this->ToJson(array);
      arrayMeta["location"] = "pointData";
      arrayMeta["registration"] = "setScalars";
      extractedFields.append(arrayMeta);
    }
  }

  if (cellDataArray == nullptr)
  {
    vtkDataArray* array = dataSet->GetCellData()->GetScalars();
    if (array != nullptr)
    {
      Json::Value arrayMeta = this->ToJson(array);
      arrayMeta["location"] = "cellData";
      arrayMeta["registration"] = "setScalars";
      extractedFields.append(arrayMeta);
    }
  }

  // Normal handling
  vtkDataArray* normals = dataSet->GetPointData()->GetNormals();
  if (normals != nullptr)
  {
    Json::Value arrayMeta = this->ToJson(normals);
    arrayMeta["location"] = "pointData";
    arrayMeta["registration"] = "setNormals";
    extractedFields.append(arrayMeta);
  }

  // TCoord handling
  vtkDataArray* tcoords = dataSet->GetPointData()->GetTCoords();
  if (tcoords != nullptr)
  {
    Json::Value arrayMeta = this->ToJson(tcoords);
    arrayMeta["location"] = "pointData";
    arrayMeta["registration"] = "setTCoords";
    extractedFields.append(arrayMeta);
  }
}

//----------------------------------------------------------------------------
void vtkVtkJSSceneGraphSerializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
