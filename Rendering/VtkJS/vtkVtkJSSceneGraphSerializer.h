// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVtkJSSceneGraphSerializer
 * @brief   Converts elements of a VTK scene graph into vtk-js elements
 *
 * vtkVtkJSSceneGraphSerializer accepts nodes and their renderables from a scene
 * graph and a) composes the elements' data and topology into a Json data
 * structure and b) correlates unique identifiers for data objects in the Json
 * data structure to the data objects themselves. It is designed to operate with
 * vtkVtkJSViewNodeFactory, which handles the logic for scene graph traversal.
 *
 * When constructing the Json description for a single scene graph node and its
 * renderable, the Add(node, renderable) step processes the renderable into its
 * corresponding vtk-js form. For many renderables this is a no-op, but current
 * restrictions in vtk-js (such as the lack of support for composite mappers and
 * the requirement for data conversion to vtkPolyData) require a nontrival
 * conversion step for certain renderable types. The subsequent
 * ToJson(renderable) is a straightforward conversion of the renderable's data
 * members into a vtk-js Json format.
 *
 *
 * @sa
 * vtkVtkJSViewNodeFactory
 */

#ifndef vtkVtkJSSceneGraphSerializer_h
#define vtkVtkJSSceneGraphSerializer_h

#include "vtkRenderingVtkJSModule.h" // For export macro

#include "vtkObject.h"
#include <vtk_jsoncpp.h> // For Json::Value

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkAlgorithm;
class vtkCamera;
class vtkCompositePolyDataMapper;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkGlyph3DMapper;
class vtkImageData;
class vtkLight;
class vtkLookupTable;
class vtkMapper;
class vtkPolyData;
class vtkProperty;
class vtkRenderer;
class vtkRenderWindow;
class vtkTexture;
class vtkTransform;
class vtkViewNode;

class VTKRENDERINGVTKJS_EXPORT vtkVtkJSSceneGraphSerializer : public vtkObject
{
public:
  static vtkVtkJSSceneGraphSerializer* New();
  vtkTypeMacro(vtkVtkJSSceneGraphSerializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Empty the contents of the scene and the reset the unique id generator.
   */
  void Reset();
  ///@}

  ///@{
  /**
   * Access the Json description of the constructed scene. The returned object
   * is valid for the lifetime of this class.
   */
  const Json::Value& GetRoot() const;
  ///@}

  ///@{
  /**
   * Access the data objects referenced in the constructed scene.
   */
  vtkIdType GetNumberOfDataObjects() const;
  Json::ArrayIndex GetDataObjectId(vtkIdType) const;
  vtkDataObject* GetDataObject(vtkIdType) const;
  ///@}

  ///@{
  /**
   * Access the data arrays referenced in the constructed scene.
   */
  vtkIdType GetNumberOfDataArrays() const;
  std::string GetDataArrayId(vtkIdType) const;
  vtkDataArray* GetDataArray(vtkIdType) const;
  ///@}

  ///@{
  /**
   * Add a scene graph node and its corresponding renderable to the scene.
   */
  virtual void Add(vtkViewNode*, vtkActor*);
  virtual void Add(vtkViewNode*, vtkCompositePolyDataMapper*);
  virtual void Add(vtkViewNode*, vtkGlyph3DMapper*);
  virtual void Add(vtkViewNode*, vtkMapper*);
  virtual void Add(vtkViewNode*, vtkRenderer*);
  virtual void Add(vtkViewNode*, vtkRenderWindow*);
  ///@}

protected:
  vtkVtkJSSceneGraphSerializer();
  ~vtkVtkJSSceneGraphSerializer() override;

  ///@{
  /**
   * Translate from a VTK renderable to a vtk-js renderable.
   */
  virtual Json::Value ToJson(vtkDataArray*);
  virtual Json::Value ToJson(Json::Value&, vtkAlgorithm*, vtkDataObject*);
  virtual Json::Value ToJson(Json::Value&, vtkActor*, bool newPropertyId = false);
  virtual Json::Value ToJson(Json::Value&, Json::ArrayIndex, vtkGlyph3DMapper*);
  virtual Json::Value ToJson(Json::Value&, vtkCamera*);
  virtual Json::Value ToJson(Json::Value&, vtkAlgorithm*, vtkImageData*);
  virtual Json::Value ToJson(Json::Value&, vtkLight*);
  virtual Json::Value ToJson(Json::Value&, vtkLookupTable*);
  virtual Json::Value ToJson(Json::Value&, Json::ArrayIndex, vtkMapper*, bool newLUTId = false);
  virtual Json::Value ToJson(Json::Value&, vtkRenderer*);
  virtual Json::Value ToJson(Json::Value&, vtkAlgorithm*, vtkPolyData*);
  virtual Json::Value ToJson(Json::Value&, vtkProperty*);
  virtual Json::Value ToJson(Json::Value&, vtkTexture*);
  virtual Json::Value ToJson(Json::Value&, vtkTransform*);
  virtual Json::Value ToJson(vtkRenderWindow*);
  ///@}

  ///@{
  /**
   * Associate a unique id with a given object. Subsequent calls to this method
   * with the same object will return the same unique id.
   */
  Json::ArrayIndex UniqueId(void* ptr = nullptr);
  ///@}

  struct Internal;
  Internal* Internals;

private:
  vtkVtkJSSceneGraphSerializer(const vtkVtkJSSceneGraphSerializer&) = delete;
  void operator=(const vtkVtkJSSceneGraphSerializer&) = delete;

  virtual void Add(Json::Value*, vtkAlgorithm*);

  void Add(vtkViewNode* node, vtkDataObject* dataObject, vtkCompositePolyDataMapper* mapper);

  void extractRequiredFields(Json::Value& extractedFields, vtkMapper* mapper, vtkDataSet* dataSet);
};

VTK_ABI_NAMESPACE_END
#endif
