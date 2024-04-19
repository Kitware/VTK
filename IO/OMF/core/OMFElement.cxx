// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "OMFElement.h"
#include "OMFFile.h"
#include "OMFHelpers.h"

#include "vtkActor.h"
#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkConnectivityFilter.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkTexture.h"
#include "vtkTextureMapToPlane.h"

#include "vtk_jsoncpp.h"
#include "vtksys/SystemTools.hxx"

#include <numeric>
#include <string>
#include <vector>

namespace omf
{
namespace
{

// originally origin was added to the coordinates here, but the origin needs to be
// added to the points AFTER performing rotation, scaling, etc
void createCoordinatesArray(std::vector<double> spacing, vtkDoubleArray* coords)
{
  std::vector<double> s(spacing.size(), 0);
  std::partial_sum(spacing.begin(), spacing.end(), s.begin());

  coords->SetNumberOfValues(static_cast<vtkIdType>(s.size() + 1));
  coords->SetValue(0, 0);
  for (vtkIdType i = 0; i < static_cast<vtkIdType>(s.size()); ++i)
  {
    coords->SetValue(i + 1, static_cast<vtkIdType>(s[i]));
  }
}

struct AddOriginToArrayWorker
{
  template <typename ArrayT>
  void operator()(ArrayT* array, const double origin[3])
  {
    VTK_ASSUME(array->GetNumberOfComponents() == 3);

    vtkDataArrayAccessor<ArrayT> a(array);
    for (vtkIdType tupleIdx = 0; tupleIdx < array->GetNumberOfTuples(); ++tupleIdx)
    {
      for (vtkIdType compIdx = 0; compIdx < 3; ++compIdx)
      {
        a.Set(tupleIdx, compIdx, a.Get(tupleIdx, compIdx) + origin[compIdx]);
      }
    }
  }
};

bool setPoints(std::shared_ptr<OMFFile>& file, const Json::Value& geometry,
  const double globalOrigin[3], vtkSmartPointer<vtkPoints> points)
{
  if (!points)
  {
    return false;
  }

  std::string vertUID;
  helper::GetStringValue(geometry["vertices"], vertUID);
  auto vertices = file->ReadArrayFromStream(vertUID);
  if (globalOrigin[0] != 0 || globalOrigin[1] != 0 || globalOrigin[2] != 0)
  {
    AddOriginToArrayWorker worker;
    typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;
    if (!Dispatcher::Execute(vertices, worker, globalOrigin))
    {
      worker(vertices.GetPointer(), globalOrigin);
    }
  }

  points->SetData(vertices);
  return true;
}

bool setFieldDataArray(vtkAbstractArray* array, vtkPartitionedDataSet* output,
  const std::string& location, const std::string& name)
{
  if (location == "vertices")
  {
    output->GetPartition(0)->GetPointData()->AddArray(array);
    return true;
  }
  else if (location == "cells" || location == "faces" || location == "segments")
  {
    output->GetPartition(0)->GetCellData()->AddArray(array);
    return true;
  }
  else
  {
    vtkGenericWarningMacro(<< "location " << location << " is not valid for field " << name);
    return false;
  }
}

} // end anon namespace
VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
ProjectElement::ProjectElement(const std::string& uid, double globalOrigin[3])
  : UID(uid)
{
  memcpy(this->GlobalOrigin, globalOrigin, sizeof(double) * 3);
}

//------------------------------------------------------------------------------
void ProjectElement::ProcessJSON(std::shared_ptr<OMFFile>& file, const Json::Value& element,
  vtkPartitionedDataSet* output, bool writeOutTextures, bool columnMajorOrdering)
{
  if (!element.isMember("geometry"))
  {
    vtkGenericWarningMacro(<< "geometry was not found for element " << element["name"]);
    return;
  }
  std::string geometry;
  helper::GetStringValue(element["geometry"], geometry);
  this->ProcessGeometry(file, geometry, output);

  // optional properties: data and textures
  this->ProcessDataFields(file, element["data"], output);

  if (columnMajorOrdering)
  {
    this->ProcessColumnOrdering(file, element["data"], output);
  }

  if (writeOutTextures && element.isMember("textures") && !element["textures"].empty())
  {
    this->ProcessTextures(file, element["textures"], output, element["name"].asString());
  }
}

//------------------------------------------------------------------------------
void ProjectElement::ProcessDataFields(
  std::shared_ptr<OMFFile>& file, const Json::Value& dataJSON, vtkPartitionedDataSet* output)
{
  if (!dataJSON.isNull() && dataJSON.isArray())
  {
    for (Json::Value::ArrayIndex i = 0; i < dataJSON.size(); ++i)
    {
      std::string uid, name, location;
      helper::GetStringValue(dataJSON[i], uid);
      const auto& data = file->JSONRoot()[uid];
      helper::GetStringValue(data["name"], name);
      helper::GetStringValue(data["location"], location);

      std::string arrayUID;
      helper::GetStringValue(data["array"], arrayUID);

      auto dataArray = file->ReadArrayFromStream(arrayUID);
      dataArray->SetName(name.c_str());

      auto dataArrayCasted = vtkTypeInt64Array::SafeDownCast(dataArray);
      if (!data["legends"].isNull() && dataArrayCasted)
      {
        // if there's a string array in the legend data, we need to
        // create an array using that data
        for (Json::Value::ArrayIndex l = 0; l < data["legends"].size(); ++l)
        {
          std::string legUID;
          helper::GetStringValue(data["legends"][l], legUID);
          const auto& legend = file->JSONRoot()[legUID];
          std::string valuesUID;
          helper::GetStringValue(legend["values"], valuesUID);
          const auto& values = file->JSONRoot()[valuesUID];
          if (values["__class__"] == "StringArray")
          {
            auto stringArray = file->ReadStringArrayFromStream(valuesUID);
            vtkNew<vtkStringArray> stringData;
            stringData->SetName(name.c_str());
            stringData->Resize(dataArrayCasted->GetNumberOfValues());
            for (int idx = 0; idx < dataArrayCasted->GetNumberOfValues(); ++idx)
            {
              auto val = dataArrayCasted->GetValue(idx);
              if (0 <= val && val < static_cast<int>(stringArray.size()))
              {
                stringData->InsertNextValue(stringArray[val]);
              }
              else
              {
                stringData->InsertNextValue(std::to_string(val));
              }
            }
            setFieldDataArray(stringData, output, location, name);
          }
        }
      }
      else
      {
        setFieldDataArray(dataArray, output, location, name);
      }
    }
  }
}

void VolumeElement::ProcessColumnOrdering(
  std::shared_ptr<OMFFile>& file, const Json::Value& dataJSON, vtkPartitionedDataSet* output)
{
  if (!dataJSON.isNull() && dataJSON.isArray())
  {
    // for each array
    for (Json::Value::ArrayIndex ai = 0; ai < dataJSON.size(); ++ai)
    {
      std::string uid, name, location;
      helper::GetStringValue(dataJSON[ai], uid);
      const auto& data = file->JSONRoot()[uid];
      helper::GetStringValue(data["name"], name);
      helper::GetStringValue(data["location"], location);

      vtkIdType dstIdx = 0;
      int aid = 0;
      if (location == "vertices")
      {
        vtkAbstractArray* da = output->GetPartition(0)->GetPointData()->GetArray(name.c_str(), aid);
        if (da)
        {
          vtkSmartPointer<vtkAbstractArray> newDA;
          newDA.TakeReference(vtkAbstractArray::CreateArray(da->GetDataType()));
          newDA->DeepCopy(da);
          for (size_t k = 0; k < this->Dimensions[2]; k++)
          {
            for (size_t j = 0; j < this->Dimensions[1]; j++)
            {
              for (size_t i = 0; i < this->Dimensions[0]; i++)
              {
                newDA->SetTuple(
                  dstIdx++, k + this->Dimensions[2] * (j + i * this->Dimensions[1]), da);
              }
            }
          }
          output->GetPartition(0)->GetPointData()->RemoveArray(aid);
          output->GetPartition(0)->GetPointData()->AddArray(newDA);
        }
      }
      else if (location == "cells" || location == "faces" || location == "segments")
      {
        vtkDataArray* da = output->GetPartition(0)->GetCellData()->GetArray(name.c_str(), aid);
        if (da)
        {
          vtkSmartPointer<vtkAbstractArray> newDA;
          newDA.TakeReference(vtkAbstractArray::CreateArray(da->GetDataType()));
          newDA->DeepCopy(da);
          for (size_t k = 0; k < this->Dimensions[2] - 1; k++)
          {
            for (size_t j = 0; j < this->Dimensions[1] - 1; j++)
            {
              for (size_t i = 0; i < this->Dimensions[0] - 1; i++)
              {
                newDA->SetTuple(dstIdx++,
                  k + (this->Dimensions[2] - 1) * (j + i * (this->Dimensions[1] - 1)), da);
              }
            }
          }
          output->GetPartition(0)->GetCellData()->RemoveArray(aid);
          output->GetPartition(0)->GetCellData()->AddArray(newDA);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void ProjectElement::ProcessTextures(std::shared_ptr<OMFFile>& file, const Json::Value& textureJSON,
  vtkPartitionedDataSet* output, const std::string& elementName)
{
  auto* dataset = output->GetPartition(0);
  for (Json::Value::ArrayIndex tex = 0; tex < textureJSON.size(); ++tex)
  {
    std::string texUID;
    helper::GetStringValue(textureJSON[tex], texUID);
    const auto& imageTexture = file->JSONRoot()[texUID];

    double origin[3];
    helper::GetPointFromJSON(imageTexture["origin"], origin);

    std::string name;
    helper::GetStringValue(imageTexture["name"], name);

    double axis_u[3], axis_v[3];
    helper::GetPointFromJSON(imageTexture["axis_u"], axis_u);
    helper::GetPointFromJSON(imageTexture["axis_v"], axis_v);
    double pt1[3], pt2[3];
    for (int i = 0; i < 3; ++i)
    {
      pt1[i] = origin[i] + axis_u[i];
      pt2[i] = origin[i] + axis_v[i];
    }

    vtkNew<vtkTextureMapToPlane> texMap;
    texMap->SetOrigin(origin);
    texMap->SetPoint1(pt1);
    texMap->SetPoint2(pt2);
    texMap->SetInputDataObject(dataset);
    texMap->Update();

    auto* tCoords = texMap->GetOutput()->GetPointData()->GetTCoords();
    tCoords->SetName("TCoords");
    dataset->GetPointData()->SetTCoords(tCoords);

    // image will always be PNG according to OMF docs
    const auto& imageJSON = imageTexture["image"];
    auto image = file->ReadPNGFromStream(imageJSON);
    if (!image)
    {
      continue;
    }

    // Write out texture images to the directory containing the
    // OMF file being read.
    auto filePath = file->GetFileName();
    auto dir = vtksys::SystemTools::GetParentDirectory(filePath);

    std::vector<std::string> pathComponents;
    vtksys::SystemTools::SplitPath(dir, pathComponents);
    pathComponents.emplace_back("textures");
    auto texDir = vtksys::SystemTools::JoinPath(pathComponents);
    vtksys::SystemTools::MakeDirectory(texDir);

    auto fileNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(filePath);
    std::string texFileName = fileNoExt + "-" + elementName + "-texture.png";

    pathComponents.push_back(texFileName);
    auto texFilePath = vtksys::SystemTools::JoinPath(pathComponents);

    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName(texFilePath.c_str());
    writer->SetInputData(image);
    writer->Write();
  }
}

//------------------------------------------------------------------------------
void PointSetElement::ProcessGeometry(
  std::shared_ptr<OMFFile>& file, const std::string& geometryUID, vtkPartitionedDataSet* output)
{
  const auto& geometry = file->JSONRoot()[geometryUID];

  vtkNew<vtkPoints> points;
  setPoints(file, geometry, this->GlobalOrigin, points);

  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);

  // make vertex cell for each point
  vtkIdType nPoints = poly->GetNumberOfPoints();
  if (nPoints > 0)
  {
    vtkNew<vtkCellArray> polyVertex;
    polyVertex->AllocateEstimate(1, nPoints);
    polyVertex->InsertNextCell(nPoints);
    for (vtkIdType i = 0; i < nPoints; ++i)
    {
      polyVertex->InsertCellPoint(i);
    }
    poly->SetVerts(polyVertex);
  }

  output->SetNumberOfPartitions(1);
  output->SetPartition(0, poly);
}

//------------------------------------------------------------------------------
void LineSetElement::ProcessGeometry(
  std::shared_ptr<OMFFile>& file, const std::string& geometryUID, vtkPartitionedDataSet* output)
{
  const auto& geometry = file->JSONRoot()[geometryUID];

  vtkNew<vtkPoints> points;
  setPoints(file, geometry, this->GlobalOrigin, points);

  // the line segments are stored originally in OMF as Int2Array,
  // but we'll just read into a single component array so we can pass it
  // to vtkCellArray::SetData
  std::string segUID;
  helper::GetStringValue(geometry["segments"], segUID);
  auto segments = file->ReadArrayFromStream(segUID, 1);

  vtkNew<vtkCellArray> lines;
  lines->SetData(2, segments);

  vtkNew<vtkPolyData> poly;
  poly->SetPoints(points);
  poly->SetLines(lines);

  vtkNew<vtkConnectivityFilter> connFilter;
  connFilter->SetInputData(poly);
  connFilter->SetExtractionModeToAllRegions();
  connFilter->SetColorRegions(true);
  connFilter->Update();
  auto* regions = connFilter->GetOutput()->GetCellData()->GetAbstractArray("RegionId");
  regions->SetName("LineIndex");
  poly->GetCellData()->AddArray(regions);

  output->SetNumberOfPartitions(1);
  output->SetPartition(0, poly);
}

//------------------------------------------------------------------------------
void SurfaceElement::ProcessGeometry(
  std::shared_ptr<OMFFile>& file, const std::string& geometryUID, vtkPartitionedDataSet* output)
{
  const auto& geometry = file->JSONRoot()[geometryUID];
  std::string geometryClass;
  helper::GetStringValue(geometry["__class__"], geometryClass);

  if (geometryClass == "SurfaceGeometry")
  {
    vtkNew<vtkPoints> points;
    setPoints(file, geometry, this->GlobalOrigin, points);

    // the triangles are stored originally in OMF as Int3Array,
    // but we'll just read into a single component array so we can pass it
    std::string triUID;
    helper::GetStringValue(geometry["triangles"], triUID);
    auto triArray = file->ReadArrayFromStream(triUID, 1);

    vtkNew<vtkCellArray> triangles;
    triangles->SetData(3, triArray);

    vtkNew<vtkPolyData> poly;
    poly->SetPoints(points);
    poly->SetPolys(triangles);

    output->SetNumberOfPartitions(1);
    output->SetPartition(0, poly);
  }
  else if (geometryClass == "SurfaceGridGeometry")
  {
    double origin[3];
    helper::GetPointFromJSON(geometry["origin"], origin);

    double axis_u[3], axis_v[3], axis_w[3];
    helper::GetPointFromJSON(geometry["axis_u"], axis_u);
    helper::GetPointFromJSON(geometry["axis_v"], axis_v);
    vtkMath::Cross(axis_u, axis_v, axis_w);

    std::vector<double> tensor_u, tensor_v;
    helper::GetDoubleArray(geometry["tensor_u"], tensor_u);
    helper::GetDoubleArray(geometry["tensor_v"], tensor_v);

    vtkNew<vtkDoubleArray> x;
    vtkNew<vtkDoubleArray> y;
    createCoordinatesArray(tensor_u, x);
    createCoordinatesArray(tensor_v, y);

    vtkNew<vtkDoubleArray> z;
    z->SetNumberOfValues(1);
    z->SetValue(0, origin[2]);

    vtkSmartPointer<vtkDataArray> offset_w_array;
    if (geometry.isMember("offset_w"))
    {
      std::string offsetUID;
      helper::GetStringValue(geometry["offset_w"], offsetUID);
      offset_w_array = file->ReadArrayFromStream(offsetUID);
    }
    // looks like offset_w is always double
    vtkDoubleArray* offset_w = vtkDoubleArray::SafeDownCast(offset_w_array);
    if (!offset_w)
    {
      vtkGenericWarningMacro("offset_w could not be casted to vtkDoubleArray");
      return;
    }

    std::array<vtkIdType, 3> dims = { { x->GetNumberOfValues(), y->GetNumberOfValues(),
      z->GetNumberOfValues() } };
    vtkNew<vtkStructuredGrid> sgrid;
    sgrid->SetDimensions(dims[0], dims[1], dims[2]);

    vtkNew<vtkPoints> points;
    points->Allocate(dims[0] * dims[1] * dims[2]);
    double pt[3];
    vtkIdType offsetWIdx = 0;
    for (int k = 0; k < dims[2]; ++k)
    {
      pt[2] = z->GetValue(k);
      for (int j = 0; j < dims[1]; ++j)
      {
        pt[1] = y->GetValue(j);
        for (int i = 0; i < dims[0]; ++i)
        {
          pt[0] = x->GetValue(i);
          // if we have offset_w apply it now
          // offset_w length is equal to num points
          if (offset_w && offset_w->GetNumberOfValues() > 0)
          {
            pt[2] = z->GetValue(k) + offset_w->GetValue(offsetWIdx);
            offsetWIdx++;
          }
          // apply rotation
          double rotCol1[3] = { axis_u[0], axis_v[0], axis_w[0] };
          double rotCol2[3] = { axis_u[1], axis_v[1], axis_w[1] };
          double rotCol3[3] = { axis_u[2], axis_v[2], axis_w[2] };
          double newPt[3];
          newPt[0] = vtkMath::Dot(pt, rotCol1);
          newPt[1] = vtkMath::Dot(pt, rotCol2);
          newPt[2] = vtkMath::Dot(pt, rotCol3);
          newPt[0] += this->GlobalOrigin[0] + origin[0];
          newPt[1] += this->GlobalOrigin[1] + origin[1];
          newPt[2] += this->GlobalOrigin[2] + origin[2];
          points->InsertNextPoint(newPt);
        }
      }
    }
    sgrid->SetPoints(points);

    output->SetNumberOfPartitions(1);
    output->SetPartition(0, sgrid);
  }
  else
  {
    vtkGenericWarningMacro("incorrect surface geometry type name");
  }
}

//------------------------------------------------------------------------------
void VolumeElement::ProcessGeometry(
  std::shared_ptr<OMFFile>& file, const std::string& geometryUID, vtkPartitionedDataSet* output)
{
  const auto& geometry = file->JSONRoot()[geometryUID];

  double origin[3];
  helper::GetPointFromJSON(geometry["origin"], origin);

  double axis_u[3], axis_v[3], axis_w[3];
  helper::GetPointFromJSON(geometry["axis_u"], axis_u);
  helper::GetPointFromJSON(geometry["axis_v"], axis_v);
  helper::GetPointFromJSON(geometry["axis_w"], axis_w);

  std::vector<double> tensor_u, tensor_v, tensor_w;
  helper::GetDoubleArray(geometry["tensor_u"], tensor_u);
  helper::GetDoubleArray(geometry["tensor_v"], tensor_v);
  helper::GetDoubleArray(geometry["tensor_w"], tensor_w);

  vtkNew<vtkDoubleArray> x;
  vtkNew<vtkDoubleArray> y;
  vtkNew<vtkDoubleArray> z;
  createCoordinatesArray(tensor_u, x);
  createCoordinatesArray(tensor_v, y);
  createCoordinatesArray(tensor_w, z);

  std::array<vtkIdType, 3> dims = { { x->GetNumberOfValues(), y->GetNumberOfValues(),
    z->GetNumberOfValues() } };
  this->Dimensions[0] = dims[0];
  this->Dimensions[1] = dims[1];
  this->Dimensions[2] = dims[2];
  vtkNew<vtkStructuredGrid> sgrid;
  sgrid->SetDimensions(dims[0], dims[1], dims[2]);

  vtkNew<vtkPoints> points;
  points->Allocate(dims[0] * dims[1] * dims[2]);
  double pt[3];
  for (int k = 0; k < dims[2]; ++k)
  {
    pt[2] = z->GetValue(k);
    for (int j = 0; j < dims[1]; ++j)
    {
      pt[1] = y->GetValue(j);
      for (int i = 0; i < dims[0]; ++i)
      {
        pt[0] = x->GetValue(i);
        // apply rotation
        double rotCol1[3] = { axis_u[0], axis_v[0], axis_w[0] };
        double rotCol2[3] = { axis_u[1], axis_v[1], axis_w[1] };
        double rotCol3[3] = { axis_u[2], axis_v[2], axis_w[2] };
        double newPt[3];
        newPt[0] = vtkMath::Dot(pt, rotCol1);
        newPt[1] = vtkMath::Dot(pt, rotCol2);
        newPt[2] = vtkMath::Dot(pt, rotCol3);
        newPt[0] += this->GlobalOrigin[0] + origin[0];
        newPt[1] += this->GlobalOrigin[1] + origin[1];
        newPt[2] += this->GlobalOrigin[2] + origin[2];
        points->InsertNextPoint(newPt);
      }
    }
  }
  sgrid->SetPoints(points);

  output->SetNumberOfPartitions(1);
  output->SetPartition(0, sgrid);
}

VTK_ABI_NAMESPACE_END
} // end namespace omf
