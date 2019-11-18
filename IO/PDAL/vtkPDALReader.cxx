/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALRasterReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPDALReader.h"

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkTypeInt16Array.h>
#include <vtkTypeInt32Array.h>
#include <vtkTypeInt64Array.h>
#include <vtkTypeInt8Array.h>
#include <vtkTypeUInt16Array.h>
#include <vtkTypeUInt32Array.h>
#include <vtkTypeUInt64Array.h>
#include <vtkTypeUInt8Array.h>
#include <vtkVertexGlyphFilter.h>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if __GNUC__ > 6
#pragma GCC diagnostic ignored "-Wnoexcept-type"
#endif
#endif
#include <pdal/Reader.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <pdal/Options.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>

vtkStandardNewMacro(vtkPDALReader);

//----------------------------------------------------------------------------
vtkPDALReader::vtkPDALReader()
{
  this->FileName = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPDALReader::~vtkPDALReader()
{
  if (this->FileName)
  {
    delete[] this->FileName;
  }
}

//----------------------------------------------------------------------------
int vtkPDALReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(request), vtkInformationVector* outputVector)
{
  try
  {
    // Get the info object
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // Get the output
    vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    pdal::StageFactory factory;
    std::string driverName = factory.inferReaderDriver(this->FileName);
    if (driverName.empty())
    {
      vtkErrorMacro("Cannot infer the reader driver for " << this->FileName);
      return 0;
    }

    pdal::Option opt_filename("filename", this->FileName);
    pdal::Options opts;
    opts.add(opt_filename);
    pdal::Stage* reader = factory.createStage(driverName);
    if (!reader)
    {
      vtkErrorMacro("Cannot open file " << this->FileName);
      return 0;
    }
    reader->setOptions(opts);

    vtkNew<vtkPolyData> pointsPolyData;
    this->ReadPointRecordData(*reader, pointsPolyData);

    // Convert points to verts in output polydata
    vtkNew<vtkVertexGlyphFilter> vertexFilter;
    vertexFilter->SetInputData(pointsPolyData);
    vertexFilter->Update();
    output->ShallowCopy(vertexFilter->GetOutput());
    return VTK_OK;
  }
  catch (const std::exception& e)
  {
    vtkErrorMacro("exception: " << e.what());
  }
  catch (...)
  {
    vtkErrorMacro("Unknown exception");
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPDALReader::ReadPointRecordData(pdal::Stage& reader, vtkPolyData* pointsPolyData)
{
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  pointsPolyData->SetPoints(points);
  pdal::PointTable table;
  reader.prepare(table);
  pdal::PointViewSet pointViewSet = reader.execute(table);
  pdal::PointViewPtr pointView = *pointViewSet.begin();
  points->SetNumberOfPoints(pointView->size());
  pdal::Dimension::IdList dims = pointView->dims();
  std::vector<vtkDoubleArray*> doubleArray(dims.size(), nullptr);
  std::vector<vtkFloatArray*> floatArray(dims.size(), nullptr);
  std::vector<vtkTypeUInt8Array*> uInt8Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt16Array*> uInt16Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt32Array*> uInt32Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt64Array*> uInt64Array(dims.size(), nullptr);
  std::vector<vtkTypeInt8Array*> int8Array(dims.size(), nullptr);
  std::vector<vtkTypeInt16Array*> int16Array(dims.size(), nullptr);
  std::vector<vtkTypeInt32Array*> int32Array(dims.size(), nullptr);
  std::vector<vtkTypeInt64Array*> int64Array(dims.size(), nullptr);
  vtkTypeUInt16Array* colorArray = nullptr;
  // check if we have a color field, and create the required array
  bool hasColor = false, hasRed = false, hasGreen = false, hasBlue = false;
  for (size_t i = 0; i < dims.size(); ++i)
  {
    pdal::Dimension::Id dimensionId = dims[i];
    switch (dimensionId)
    {
      case pdal::Dimension::Id::Red:
        hasRed = true;
        break;
      case pdal::Dimension::Id::Green:
        hasGreen = true;
        break;
      case pdal::Dimension::Id::Blue:
        hasBlue = true;
        break;
      default:
        continue;
    }
  }
  if (hasRed && hasGreen && hasBlue)
  {
    hasColor = true;
    vtkNew<vtkTypeUInt16Array> a;
    a->SetNumberOfComponents(3);
    a->SetNumberOfTuples(pointView->size());
    a->SetName("Color");
    pointsPolyData->GetPointData()->AddArray(a);
    colorArray = a;
  }
  // create arrays for fields
  for (size_t i = 0; i < dims.size(); ++i)
  {
    pdal::Dimension::Id dimensionId = dims[i];
    if (dimensionId == pdal::Dimension::Id::X || dimensionId == pdal::Dimension::Id::Y ||
      dimensionId == pdal::Dimension::Id::Z)
    {
      continue;
    }
    if (hasColor &&
      (dimensionId == pdal::Dimension::Id::Red || dimensionId == pdal::Dimension::Id::Green ||
        dimensionId == pdal::Dimension::Id::Blue))
    {
      continue;
    }
    switch (pointView->dimType(dimensionId))
    {
      case pdal::Dimension::Type::Double:
      {
        vtkNew<vtkDoubleArray> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        doubleArray[i] = a;
        break;
      }
      case pdal::Dimension::Type::Float:
      {
        vtkNew<vtkFloatArray> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        floatArray[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned8:
      {
        vtkNew<vtkTypeUInt8Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt8Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned16:
      {
        vtkNew<vtkTypeUInt16Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt16Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned32:
      {
        vtkNew<vtkTypeUInt32Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt32Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Unsigned64:
      {
        vtkNew<vtkTypeUInt64Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt64Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed8:
      {
        vtkNew<vtkTypeInt8Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int8Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed16:
      {
        vtkNew<vtkTypeInt16Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int16Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed32:
      {
        vtkNew<vtkTypeInt32Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int32Array[i] = a;
        break;
      }
      case pdal::Dimension::Type::Signed64:
      {
        vtkNew<vtkTypeInt64Array> a;
        a->SetName(pointView->dimName(dimensionId).c_str());
        a->SetNumberOfTuples(pointView->size());
        pointsPolyData->GetPointData()->AddArray(a);
        int64Array[i] = a;
        break;
      }
      default:
        throw std::runtime_error("Invalid pdal::Dimension::Type");
    }
  }
  for (pdal::PointId pointId = 0; pointId < pointView->size(); ++pointId)
  {
    double point[3] = { pointView->getFieldAs<double>(pdal::Dimension::Id::X, pointId),
      pointView->getFieldAs<double>(pdal::Dimension::Id::Y, pointId),
      pointView->getFieldAs<double>(pdal::Dimension::Id::Z, pointId) };
    points->SetPoint(pointId, point);
    if (hasColor)
    {
      uint16_t color[3] = {
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, pointId),
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, pointId),
        pointView->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, pointId),
      };
      colorArray->SetTypedTuple(pointId, color);
    }
    for (size_t i = 0; i < dims.size(); ++i)
    {
      pdal::Dimension::Id dimensionId = dims[i];
      if (dimensionId == pdal::Dimension::Id::X || dimensionId == pdal::Dimension::Id::Y ||
        dimensionId == pdal::Dimension::Id::Z)
      {
        continue;
      }
      if (hasColor &&
        (dimensionId == pdal::Dimension::Id::Red || dimensionId == pdal::Dimension::Id::Green ||
          dimensionId == pdal::Dimension::Id::Blue))
      {
        continue;
      }
      switch (pointView->dimType(dimensionId))
      {
        case pdal::Dimension::Type::Double:
        {
          vtkDoubleArray* a = doubleArray[i];
          double value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Float:
        {
          vtkFloatArray* a = floatArray[i];
          float value = pointView->getFieldAs<float>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned8:
        {
          vtkTypeUInt8Array* a = uInt8Array[i];
          uint8_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned16:
        {
          vtkTypeUInt16Array* a = uInt16Array[i];
          uint16_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned32:
        {
          vtkTypeUInt32Array* a = uInt32Array[i];
          uint32_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Unsigned64:
        {
          vtkTypeUInt64Array* a = uInt64Array[i];
          uint64_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed8:
        {
          vtkTypeInt8Array* a = int8Array[i];
          int8_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed16:
        {
          vtkTypeInt16Array* a = int16Array[i];
          int16_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed32:
        {
          vtkTypeInt32Array* a = int32Array[i];
          int32_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        case pdal::Dimension::Type::Signed64:
        {
          vtkTypeInt64Array* a = int64Array[i];
          int64_t value = pointView->getFieldAs<double>(dimensionId, pointId);
          a->SetValue(pointId, value);
          break;
        }
        default:
          throw std::runtime_error("Invalid pdal::Dimension::Type");
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPDALReader::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkPDALReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
