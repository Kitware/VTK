// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDeserializer.h"
#include "vtkInformation.h"
#include "vtkSerializer.h"
#include "vtkTextProperty.h"
#include "vtkXYPlotActor.h"

// clang-format off
#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)
// clang-format on

extern "C"
{
  /**
   * Register the (de)serialization handlers of vtkXYPlotActor
   * @param ser   a vtkSerializer instance
   * @param deser a vtkDeserializer instance
   */
  int RegisterHandlers_vtkXYPlotActorSerDesHelper(void* ser, void* deser, void* invoker);
}

static nlohmann::json Serialize_vtkXYPlotActor(vtkObjectBase* object, vtkSerializer* serializer)
{
  using json = nlohmann::json;
  if (auto* xyPlotActor = vtkXYPlotActor::SafeDownCast(object))
  {
    json state;
    if (auto superSerializer = serializer->GetHandler(typeid(vtkXYPlotActor::Superclass)))
    {
      state = superSerializer(object, serializer);
    }
    state["SuperClassNames"].push_back("vtkObject");

    auto& stateOfInputDataObjects = state["InputDataObjects"] = json::array();
    for (unsigned int index = 0; index < xyPlotActor->GetNumberOfDataObjectInputConnections();
         ++index)
    {
      auto* inputAlgorithm = xyPlotActor->GetDataObjectInputConnection(index)->GetProducer();
      inputAlgorithm->Update();
      auto* inputDataObject = inputAlgorithm->GetOutputDataObject(0);
      stateOfInputDataObjects.push_back(serializer->SerializeJSON(inputDataObject));
    }

    auto& stateOfInputDataSets = state["InputDataSets"] = json::array();
    for (unsigned int index = 0; index < xyPlotActor->GetNumberOfDataSetInputConnections(); ++index)
    {
      auto* inputAlgorithm = xyPlotActor->GetDataSetInputConnection(index)->GetProducer();
      inputAlgorithm->Update();
      auto* inputDataObject = inputAlgorithm->GetOutputDataObject(0);
      stateOfInputDataSets.push_back(serializer->SerializeJSON(inputDataObject));
    }

    state["DataObjectPlotMode"] = xyPlotActor->GetDataObjectPlotMode();
    state["PlotCurvePoints"] = xyPlotActor->GetPlotCurvePoints();
    state["PlotCurveLines"] = xyPlotActor->GetPlotCurveLines();
    state["ExchangeAxes"] = xyPlotActor->GetExchangeAxes();
    state["ReverseXAxis"] = xyPlotActor->GetReverseXAxis();
    state["ReverseYAxis"] = xyPlotActor->GetReverseYAxis();
    if (auto ptr = xyPlotActor->GetTitle())
    {
      state["Title"] = ptr;
    }
    if (auto ptr = xyPlotActor->GetXTitle())
    {
      state["XTitle"] = ptr;
    }
    if (auto ptr = xyPlotActor->GetYTitle())
    {
      state["YTitle"] = ptr;
    }
    if (auto ptr = xyPlotActor->GetXRange())
    {
      auto& dst = state["XRange"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    if (auto ptr = xyPlotActor->GetYRange())
    {
      auto& dst = state["YRange"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    state["NumberOfXLabels"] = xyPlotActor->GetNumberOfXLabels();
    state["NumberOfYLabels"] = xyPlotActor->GetNumberOfYLabels();
    state["AdjustXLabels"] = xyPlotActor->GetAdjustXLabels();
    state["AdjustYLabels"] = xyPlotActor->GetAdjustYLabels();
    state["NumberOfXMinorTicks"] = xyPlotActor->GetNumberOfXMinorTicks();
    state["NumberOfYMinorTicks"] = xyPlotActor->GetNumberOfYMinorTicks();
    state["Legend"] = xyPlotActor->GetLegend();
    if (auto ptr = xyPlotActor->GetTitlePosition())
    {
      auto& dst = state["TitlePosition"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    state["AdjustTitlePosition"] = xyPlotActor->GetAdjustTitlePosition();
    state["AdjustTitlePositionMode"] = xyPlotActor->GetAdjustTitlePositionMode();
    if (auto ptr = xyPlotActor->GetLegendPosition())
    {
      auto& dst = state["LegendPosition"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    if (auto ptr = xyPlotActor->GetLegendPosition2())
    {
      auto& dst = state["LegendPosition2"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    {
      auto value = xyPlotActor->GetTitleTextProperty();
      if (value)
      {
        state["TitleTextProperty"] =
          serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = xyPlotActor->GetAxisTitleTextProperty();
      if (value)
      {
        state["AxisTitleTextProperty"] =
          serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = xyPlotActor->GetAxisLabelTextProperty();
      if (value)
      {
        state["AxisLabelTextProperty"] =
          serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = xyPlotActor->GetLegendActor();
      if (value)
      {
        state["LegendActor"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = xyPlotActor->GetXAxisActor2D();
      if (value)
      {
        state["XAxis"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    {
      auto value = xyPlotActor->GetYAxisActor2D();
      if (value)
      {
        state["YAxis"] = serializer->SerializeJSON(reinterpret_cast<vtkObjectBase*>(value));
      }
    }
    state["Logx"] = xyPlotActor->GetLogx();
    if (auto ptr = xyPlotActor->GetLabelFormat())
    {
      state["LabelFormat"] = ptr;
    }
    if (auto ptr = xyPlotActor->GetXLabelFormat())
    {
      state["XLabelFormat"] = ptr;
    }
    if (auto ptr = xyPlotActor->GetYLabelFormat())
    {
      state["YLabelFormat"] = ptr;
    }
    state["Border"] = xyPlotActor->GetBorder();
    state["PlotPoints"] = xyPlotActor->GetPlotPoints();
    state["PlotLines"] = xyPlotActor->GetPlotLines();
    state["GlyphSize"] = xyPlotActor->GetGlyphSize();
    if (auto ptr = xyPlotActor->GetPlotCoordinate())
    {
      auto& dst = state["PlotCoordinate"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    if (auto ptr = xyPlotActor->GetViewportCoordinate())
    {
      auto& dst = state["ViewportCoordinate"] = json::array();
      for (int i = 0; i < 2; ++i)
      {
        dst.push_back(ptr[i]);
      }
    }
    state["ChartBox"] = xyPlotActor->GetChartBox();
    state["ChartBorder"] = xyPlotActor->GetChartBorder();
    state["ShowReferenceXLine"] = xyPlotActor->GetShowReferenceXLine();
    state["ReferenceXValue"] = xyPlotActor->GetReferenceXValue();
    state["ShowReferenceYLine"] = xyPlotActor->GetShowReferenceYLine();
    state["ReferenceYValue"] = xyPlotActor->GetReferenceYValue();
    state["XTitlePosition"] = xyPlotActor->GetXTitlePosition();
    state["YTitlePosition"] = xyPlotActor->GetYTitlePosition();

    return state;
  }
  else
  {
    return {};
  }
}

static bool Deserialize_vtkXYPlotActor(
  const nlohmann::json& state, vtkObjectBase* object, vtkDeserializer* deserializer)
{
  bool success = true;
  using json = nlohmann::json;
  auto* xyPlotActor = vtkXYPlotActor::SafeDownCast(object);
  if (!xyPlotActor)
  {
    vtkErrorWithObjectMacro(deserializer, << __func__ << ": object not a vtkXYPlotActor");
    return false;
  }

  if (auto superDeserializer = deserializer->GetHandler(typeid(vtkXYPlotActor::Superclass)))
  {
    success &= superDeserializer(state, object, deserializer);
  }
  if (!success)
  {
    return false;
  }
  {
    const auto* context = deserializer->GetContext();
    const auto iter = state.find("InputDataObjects");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto stateOfInputDataObjects = iter->get<json::array_t>();
      std::vector<vtkSmartPointer<vtkDataObject>> inputDataObjects;
      for (std::size_t index = 0; index < stateOfInputDataObjects.size(); ++index)
      {
        const auto identifier = stateOfInputDataObjects[index]["Id"].get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        success &= deserializer->DeserializeJSON(identifier, subObject);
        if (auto* dataObject = vtkDataObject::SafeDownCast(subObject))
        {
          inputDataObjects.emplace_back(dataObject);
        }
        xyPlotActor->RemoveAllDataObjectInputConnections();
        for (auto& dataObject : inputDataObjects)
        {
          xyPlotActor->AddDataObjectInput(dataObject);
        }
      }
    }
  }

  {
    const auto* context = deserializer->GetContext();
    const auto iter = state.find("InputDataSets");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto stateOfInputDataSets = iter->get<json::array_t>();
      std::vector<vtkSmartPointer<vtkDataSet>> inputDataSets;
      for (std::size_t index = 0; index < stateOfInputDataSets.size(); ++index)
      {
        const auto identifier = stateOfInputDataSets[index]["Id"].get<vtkTypeUInt32>();
        auto subObject = context->GetObjectAtId(identifier);
        success &= deserializer->DeserializeJSON(identifier, subObject);
        if (auto* dataSet = vtkDataSet::SafeDownCast(subObject))
        {
          inputDataSets.emplace_back(dataSet);
        }
        xyPlotActor->RemoveAllDataSetInputConnections();
        for (auto& dataSet : inputDataSets)
        {
          xyPlotActor->AddDataSetInput(dataSet);
        }
      }
    }
  }

  {
    auto iter = state.find("LegendActor");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      vtkSmartPointer<vtkObjectBase> subObject =
        reinterpret_cast<vtkObjectBase*>(xyPlotActor->GetLegendActor());
      if (subObject == nullptr)
      {
        vtkErrorWithObjectMacro(context, << "An internal collection object is null!");
      }
      else
      {
        if (context->GetObjectAtId(identifier) != subObject)
        {
          auto registrationId = identifier;
          context->RegisterObject(subObject, registrationId);
        }
        success &= deserializer->DeserializeJSON(identifier, subObject);
      }
    }
  }

  {
    auto iter = state.find("XAxis");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      vtkSmartPointer<vtkObjectBase> subObject =
        reinterpret_cast<vtkObjectBase*>(xyPlotActor->GetXAxisActor2D());
      if (subObject == nullptr)
      {
        vtkErrorWithObjectMacro(context, << "An internal collection object is null!");
      }
      else
      {
        if (context->GetObjectAtId(identifier) != subObject)
        {
          auto registrationId = identifier;
          context->RegisterObject(subObject, registrationId);
        }
        success &= deserializer->DeserializeJSON(identifier, subObject);
      }
    }
  }

  {
    auto iter = state.find("YAxis");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto* context = deserializer->GetContext();
      const auto identifier = iter->at("Id").get<vtkTypeUInt32>();
      vtkSmartPointer<vtkObjectBase> subObject =
        reinterpret_cast<vtkObjectBase*>(xyPlotActor->GetYAxisActor2D());
      if (subObject == nullptr)
      {
        vtkErrorWithObjectMacro(context, << "An internal collection object is null!");
      }
      else
      {
        if (context->GetObjectAtId(identifier) != subObject)
        {
          auto registrationId = identifier;
          context->RegisterObject(subObject, registrationId);
        }
        success &= deserializer->DeserializeJSON(identifier, subObject);
      }
    }
  }

  VTK_DESERIALIZE_VALUE_FROM_STATE(DataObjectPlotMode, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PlotCurvePoints, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PlotCurveLines, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ExchangeAxes, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ReverseXAxis, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ReverseYAxis, int, state, xyPlotActor);
  {
    const auto iter = state.find("Title");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetTitle(values.c_str());
    }
  }
  {
    const auto iter = state.find("XTitle");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetXTitle(values.c_str());
    }
  }
  {
    const auto iter = state.find("YTitle");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetYTitle(values.c_str());
    }
  }
  VTK_DESERIALIZE_VECTOR_FROM_STATE(XRange, double, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(YRange, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfXLabels, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfYLabels, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AdjustXLabels, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AdjustYLabels, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfXMinorTicks, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(NumberOfYMinorTicks, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Legend, int, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(TitlePosition, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AdjustTitlePosition, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(AdjustTitlePositionMode, int, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(LegendPosition, double, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(LegendPosition2, double, state, xyPlotActor);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    TitleTextProperty, vtkTextProperty, state, xyPlotActor, deserializer);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    AxisTitleTextProperty, vtkTextProperty, state, xyPlotActor, deserializer);
  VTK_DESERIALIZE_VTK_OBJECT_FROM_STATE(
    AxisLabelTextProperty, vtkTextProperty, state, xyPlotActor, deserializer);
  VTK_DESERIALIZE_VALUE_FROM_STATE(Logx, int, state, xyPlotActor);
  {
    const auto iter = state.find("LabelFormat");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetLabelFormat(values.c_str());
    }
  }
  {
    const auto iter = state.find("XLabelFormat");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetXLabelFormat(values.c_str());
    }
  }
  {
    const auto iter = state.find("YLabelFormat");
    if ((iter != state.end()) && !iter->is_null())
    {
      auto values = iter->get<std::string>();
      xyPlotActor->SetYLabelFormat(values.c_str());
    }
  }
  VTK_DESERIALIZE_VALUE_FROM_STATE(Border, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PlotPoints, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(PlotLines, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(GlyphSize, int, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(PlotCoordinate, double, state, xyPlotActor);
  VTK_DESERIALIZE_VECTOR_FROM_STATE(ViewportCoordinate, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ChartBox, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ChartBorder, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ShowReferenceXLine, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ReferenceXValue, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ShowReferenceYLine, int, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(ReferenceYValue, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(XTitlePosition, double, state, xyPlotActor);
  VTK_DESERIALIZE_VALUE_FROM_STATE(YTitlePosition, int, state, xyPlotActor);
  return success;
}

int RegisterHandlers_vtkXYPlotActorSerDesHelper(void* ser, void* deser, void* vtkNotUsed(invoker))
{
  int success = 0;
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(ser))
  {
    if (auto* serializer = vtkSerializer::SafeDownCast(asObjectBase))
    {
      serializer->RegisterHandler(typeid(vtkXYPlotActor), Serialize_vtkXYPlotActor);
      success = 1;
    }
  }
  if (auto* asObjectBase = static_cast<vtkObjectBase*>(deser))
  {
    if (auto* deserializer = vtkDeserializer::SafeDownCast(asObjectBase))
    {
      deserializer->RegisterHandler(typeid(vtkXYPlotActor), Deserialize_vtkXYPlotActor);
      deserializer->RegisterConstructor("vtkXYPlotActor", vtkXYPlotActor::New);
      success = 1;
    }
  }
  return success;
}
