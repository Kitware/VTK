// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAbstractWidget.h"
#include "vtkDeserializer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSerializer.h"

#include "vtk3DCursorRepresentation.h"
#include "vtk3DCursorWidget.h"
#include "vtkAffineRepresentation.h"
#include "vtkAffineWidget.h"
#include "vtkAngleRepresentation.h"
#include "vtkAngleWidget.h"
#include "vtkAxesTransformRepresentation.h"
#include "vtkAxesTransformWidget.h"
#include "vtkBalloonRepresentation.h"
#include "vtkBalloonWidget.h"
#include "vtkBiDimensionalRepresentation.h"
#include "vtkBiDimensionalWidget.h"
#include "vtkBorderRepresentation.h"
#include "vtkBorderWidget.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkButtonRepresentation.h"
#include "vtkButtonWidget.h"
#include "vtkCamera3DRepresentation.h"
#include "vtkCamera3DWidget.h"
#include "vtkCameraPathRepresentation.h"
#include "vtkCameraPathWidget.h"
#include "vtkCaptionRepresentation.h"
#include "vtkCaptionWidget.h"
#include "vtkCenteredSliderRepresentation.h"
#include "vtkCenteredSliderWidget.h"
#include "vtkCheckerboardRepresentation.h"
#include "vtkCheckerboardWidget.h"
#include "vtkCompassRepresentation.h"
#include "vtkCompassWidget.h"
#include "vtkContourRepresentation.h"
#include "vtkContourWidget.h"
#include "vtkCoordinateFrameRepresentation.h"
#include "vtkCoordinateFrameWidget.h"
#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkDisplaySizedImplicitPlaneWidget.h"
#include "vtkDistanceRepresentation.h"
#include "vtkDistanceWidget.h"
#include "vtkFinitePlaneRepresentation.h"
#include "vtkFinitePlaneWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkImplicitCylinderRepresentation.h"
#include "vtkImplicitCylinderWidget.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkLightRepresentation.h"
#include "vtkLightWidget.h"
#include "vtkLineRepresentation.h"
#include "vtkLineWidget2.h"
#include "vtkMagnifierRepresentation.h"
#include "vtkMagnifierWidget.h"
#include "vtkOrientationRepresentation.h"
#include "vtkOrientationWidget.h"
#include "vtkParallelopipedRepresentation.h"
#include "vtkParallelopipedWidget.h"
#include "vtkPointCloudRepresentation.h"
#include "vtkPointCloudWidget.h"
#include "vtkPolyLineRepresentation.h"
#include "vtkPolyLineWidget.h"
#include "vtkProgressBarRepresentation.h"
#include "vtkProgressBarWidget.h"
#include "vtkRectilinearWipeRepresentation.h"
#include "vtkRectilinearWipeWidget.h"
#include "vtkResliceCursorRepresentation.h"
#include "vtkResliceCursorWidget.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkScalarBarWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkSliderRepresentation.h"
#include "vtkSliderWidget.h"
#include "vtkSphereRepresentation.h"
#include "vtkSphereWidget2.h"
#include "vtkSplineRepresentation.h"
#include "vtkSplineWidget2.h"
#include "vtkTensorProbeRepresentation.h"
#include "vtkTensorProbeWidget.h"
#include "vtkTensorRepresentation.h"
#include "vtkTensorWidget.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkAbstractWidget
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkAbstractWidgetSerDesHelper(void* ser, void* deser);
}

static nlohmann::json Serialize_vtkAbstractWidget(
  vtkObjectBase* objectBase, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  json state;
  auto* object = vtkAbstractWidget::SafeDownCast(objectBase);
  if (auto f = serializer->GetHandler(typeid(vtkAbstractWidget::Superclass)))
  {
    state = f(object, serializer);
  }
  state["SuperClassNames"].push_back("vtkInteractorObserver");
  state["Enabled"] = object->GetEnabled();
  if (auto widgetRep = object->GetRepresentation())
  {
    state["WidgetRepresentation"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(widgetRep));
  }
  if (object->GetInteractor())
  {
    state["Interactor"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetInteractor()));
  }
  state["Enabled"] = object->GetEnabled();
  state["ProcessEvents"] = object->GetProcessEvents();
  if (object->GetParent())
  {
    state["Parent"] =
      serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(object->GetParent()));
  }
  state["ManagesCursor"] = object->GetManagesCursor();
  return state;
}

#define SET_WIDGET_REPRESENTATION(vtkWidgetName, vtkWidgetRepresentationName)                      \
  else if (auto* widget_##vtkWidgetName = vtkWidgetName::SafeDownCast(object))                     \
  {                                                                                                \
    widget_##vtkWidgetName->SetRepresentation(                                                     \
      vtkWidgetRepresentationName::SafeDownCast(subObject));                                       \
  }
static void Deserialize_vtkAbstractWidget(
  const nlohmann::json& state, vtkObjectBase* objectBase, vtkDeserializer* deserializer)
{
  auto* object = vtkAbstractWidget::SafeDownCast(objectBase);
  if (auto f = deserializer->GetHandler(typeid(vtkAbstractWidget::Superclass)))
  {
    f(state, object, deserializer);
  }
  {
    const auto iter = state.find("WidgetRepresentation");
    if ((iter != state.end()) && !iter->is_null())
    {
      const auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      auto subObject = context->GetObjectAtId(identifier);
      deserializer->DeserializeJSON(identifier, subObject);
      // (vtk/vtk#19274) VTK WidgetRepresentations do not implement shallow copy!
      // Either that gets improved or promote vtkAbstractWidget::SetWidgetRepresentation from
      // protected to public access.
      // object->GetRepresentation()->ShallowCopy(vtkWidgetRepresentation::SafeDownCast(subObject));
      if (auto* widget_3DCursorWidget = vtk3DCursorWidget::SafeDownCast(object))
      {
        widget_3DCursorWidget->SetRepresentation(
          vtk3DCursorRepresentation::SafeDownCast(subObject));
      }
      SET_WIDGET_REPRESENTATION(vtkAffineWidget, vtkAffineRepresentation)
      SET_WIDGET_REPRESENTATION(vtkAngleWidget, vtkAngleRepresentation)
      SET_WIDGET_REPRESENTATION(vtkAxesTransformWidget, vtkAxesTransformRepresentation)
      SET_WIDGET_REPRESENTATION(vtkBalloonWidget, vtkBalloonRepresentation)
      SET_WIDGET_REPRESENTATION(vtkBiDimensionalWidget, vtkBiDimensionalRepresentation)
      SET_WIDGET_REPRESENTATION(vtkBorderWidget, vtkBorderRepresentation)
      SET_WIDGET_REPRESENTATION(vtkBoxWidget2, vtkBoxRepresentation)
      SET_WIDGET_REPRESENTATION(vtkButtonWidget, vtkButtonRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCamera3DWidget, vtkCamera3DRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCameraPathWidget, vtkCameraPathRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCaptionWidget, vtkCaptionRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCenteredSliderWidget, vtkCenteredSliderRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCheckerboardWidget, vtkCheckerboardRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCompassWidget, vtkCompassRepresentation)
      SET_WIDGET_REPRESENTATION(vtkContourWidget, vtkContourRepresentation)
      SET_WIDGET_REPRESENTATION(vtkCoordinateFrameWidget, vtkCoordinateFrameRepresentation)
      SET_WIDGET_REPRESENTATION(
        vtkDisplaySizedImplicitPlaneWidget, vtkDisplaySizedImplicitPlaneRepresentation)
      SET_WIDGET_REPRESENTATION(vtkDistanceWidget, vtkDistanceRepresentation)
      SET_WIDGET_REPRESENTATION(vtkFinitePlaneWidget, vtkFinitePlaneRepresentation)
      SET_WIDGET_REPRESENTATION(vtkHandleWidget, vtkHandleRepresentation)
      SET_WIDGET_REPRESENTATION(vtkImplicitCylinderWidget, vtkImplicitCylinderRepresentation)
      SET_WIDGET_REPRESENTATION(vtkImplicitPlaneWidget2, vtkImplicitPlaneRepresentation)
      SET_WIDGET_REPRESENTATION(vtkLightWidget, vtkLightRepresentation)
      SET_WIDGET_REPRESENTATION(vtkLineWidget2, vtkLineRepresentation)
      SET_WIDGET_REPRESENTATION(vtkMagnifierWidget, vtkMagnifierRepresentation)
      SET_WIDGET_REPRESENTATION(vtkOrientationWidget, vtkOrientationRepresentation)
      SET_WIDGET_REPRESENTATION(vtkParallelopipedWidget, vtkParallelopipedRepresentation)
      SET_WIDGET_REPRESENTATION(vtkPointCloudWidget, vtkPointCloudRepresentation)
      SET_WIDGET_REPRESENTATION(vtkPolyLineWidget, vtkPolyLineRepresentation)
      SET_WIDGET_REPRESENTATION(vtkProgressBarWidget, vtkProgressBarRepresentation)
      SET_WIDGET_REPRESENTATION(vtkRectilinearWipeWidget, vtkRectilinearWipeRepresentation)
      SET_WIDGET_REPRESENTATION(vtkResliceCursorWidget, vtkResliceCursorRepresentation)
      SET_WIDGET_REPRESENTATION(vtkScalarBarWidget, vtkScalarBarRepresentation)
      SET_WIDGET_REPRESENTATION(vtkSeedWidget, vtkSeedRepresentation)
      SET_WIDGET_REPRESENTATION(vtkSliderWidget, vtkSliderRepresentation)
      SET_WIDGET_REPRESENTATION(vtkSphereWidget2, vtkSphereRepresentation)
      SET_WIDGET_REPRESENTATION(vtkSplineWidget2, vtkSplineRepresentation)
      SET_WIDGET_REPRESENTATION(vtkTensorProbeWidget, vtkTensorProbeRepresentation)
      SET_WIDGET_REPRESENTATION(vtkTensorWidget, vtkTensorRepresentation)
    }
  }
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    Interactor, vtkRenderWindowInteractor, state, object, deserializer);
  // order matters, must be after interactor is set.
  VTK_DESERIALIZE_VALUE_FROM_STATE(Enabled, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ProcessEvents, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ManagesCursor, int, state, object);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Priority, float, state, object);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(Parent, vtkAbstractWidget, state, object, deserializer);
}

int RegisterHandlers_vtkAbstractWidgetSerDesHelper(void* ser, void* deser)
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkAbstractWidget), Serialize_vtkAbstractWidget);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkAbstractWidget), Deserialize_vtkAbstractWidget);
      deserializer->RegisterConstructor(
        "vtkAbstractWidget", []() { return vtkAbstractWidget::New(); });
      success = 1;
    }
  }
  return success;
}
