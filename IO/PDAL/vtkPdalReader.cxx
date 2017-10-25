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

#include "vtkPdalReader.h"

#include <vtkDoubleArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkFloatArray.h>
#include <vtkTypeInt32Array.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkTypeUInt8Array.h>
#include <vtkTypeUInt16Array.h>
#include <vtkTypeUInt32Array.h>
#include <vtkTypeUInt64Array.h>
#include <vtkVertexGlyphFilter.h>

#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>

vtkStandardNewMacro(vtkPdalReader)


//----------------------------------------------------------------------------
vtkPdalReader::vtkPdalReader()
{
  this->FileName = NULL;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPdalReader::~vtkPdalReader()
{
  if ( ! this->FileName )
    delete[] this->FileName;
}

//----------------------------------------------------------------------------
int vtkPdalReader::RequestData(vtkInformation* vtkNotUsed(request),
                                   vtkInformationVector** vtkNotUsed(request),
                                   vtkInformationVector* outputVector)
{
  try
  {
    // Get the info object
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    // Get the ouptut
    vtkPolyData* output = vtkPolyData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

    pdal::StageFactory factory;
    std::string driverName = factory.inferReaderDriver(this->FileName);
    std::cout << driverName << std::endl;
    if (driverName.empty())
    {
      vtkErrorMacro("Cannot infer the reader driver for " << this->FileName);
      return 0;
    }

    pdal::Option las_opt_filename("filename", this->FileName);
    pdal::Options las_opts;
    las_opts.add(las_opt_filename);
    pdal::Stage* stage = factory.createStage(driverName);
    pdal::Reader* reader = static_cast<pdal::Reader*>(stage);
    if (! reader)
    {
      vtkErrorMacro("Cannot open file " << this->FileName);
      return 0;
    }
    reader->setOptions(las_opts);

    vtkNew<vtkPolyData> pointsPolyData;
    this->ReadPointRecordData(*reader, pointsPolyData);

    // Convert points to verts in output polydata
    vtkNew<vtkVertexGlyphFilter> vertexFilter;
    vertexFilter->SetInputData(pointsPolyData);
    vertexFilter->Update();
    output->ShallowCopy(vertexFilter->GetOutput());
    return VTK_OK;
  }
  catch (std::exception e)
  {
    vtkErrorMacro("exception: " << e.what());
    return 0;
  }
  vtkErrorMacro("Unknown exception");
  return 0;
}

//----------------------------------------------------------------------------
void vtkPdalReader::ReadPointRecordData(pdal::Reader &reader,
                                        vtkPolyData* pointsPolyData)
{
  vtkNew<vtkPoints> points;
  pointsPolyData->SetPoints(points);
  pdal::PointTable table;
  reader.prepare(table);
  pdal::PointViewSet point_view_set = reader.execute(table);
  std::cout << "size=" << point_view_set.size() << std::endl;
  pdal::PointViewPtr point_view = *point_view_set.begin();
  pdal::Dimension::IdList dims = point_view->dims();
  std::vector<vtkDoubleArray*> doubleArray(dims.size(), nullptr);
  std::vector<vtkFloatArray*> floatArray(dims.size(), nullptr);
  std::vector<vtkTypeUInt8Array*> uInt8Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt16Array*> uInt16Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt32Array*> uInt32Array(dims.size(), nullptr);
  std::vector<vtkTypeInt32Array*> int32Array(dims.size(), nullptr);
  std::vector<vtkTypeUInt64Array*> uInt64Array(dims.size(), nullptr);
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
    }
  }
  if (hasRed && hasGreen && hasBlue)
  {
    hasColor = true;
    vtkNew<vtkTypeUInt16Array> a;
    a->SetNumberOfComponents(3);
    a->SetName("Color");
    pointsPolyData->GetPointData()->AddArray(a);
    colorArray = a;
  }
  // create arrays for fields
  for (size_t i = 0; i < dims.size(); ++i)
  {
    pdal::Dimension::Id dimensionId = dims[i];
    if (dimensionId == pdal::Dimension::Id::X ||
        dimensionId == pdal::Dimension::Id::Y ||
        dimensionId == pdal::Dimension::Id::Z)
    {
      continue;
    }
    if (hasColor && (dimensionId == pdal::Dimension::Id::Red ||
                     dimensionId == pdal::Dimension::Id::Green ||
                     dimensionId == pdal::Dimension::Id::Blue))
    {
      continue;
    }
    pdal::Dimension::Type type = pdal::Dimension::Type::Double;
    try
    {
      type = pdal::Dimension::defaultType(dimensionId);
    }
    catch(...)
    {
      // we ignore fields we don't know about
      continue;
    }
    std::cout << pdal::Dimension::name(dimensionId) << std::endl;
    switch(type)
    {
    default:
    case pdal::Dimension::Type::Double:
      {
        vtkNew<vtkDoubleArray> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        doubleArray[i] = a;
        break;
      }
    case pdal::Dimension::Type::Float:
      {
        vtkNew<vtkFloatArray> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        floatArray[i] = a;
        break;
      }
    case pdal::Dimension::Type::Unsigned8:
      {
        vtkNew<vtkTypeUInt8Array> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt8Array[i] = a;
        break;
      }
    case pdal::Dimension::Type::Unsigned16:
      {
        vtkNew<vtkTypeUInt16Array> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt16Array[i] = a;
        break;
      }
    case pdal::Dimension::Type::Unsigned32:
      {
        vtkNew<vtkTypeUInt32Array> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt32Array[i] = a;
        break;
      }
    case pdal::Dimension::Type::Signed32:
      {
        vtkNew<vtkTypeInt32Array> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        int32Array[i] = a;
        break;
      }
    case pdal::Dimension::Type::Unsigned64:
      {
        vtkNew<vtkTypeUInt64Array> a;
        a->SetName(pdal::Dimension::name(dimensionId).c_str());
        pointsPolyData->GetPointData()->AddArray(a);
        uInt64Array[i] = a;
        break;
      }
    }
  }
  for (pdal::PointId pointId = 0; pointId < point_view->size(); ++pointId)
  {
    double point[3] = {
      point_view->getFieldAs<double>(pdal::Dimension::Id::X, pointId),
      point_view->getFieldAs<double>(pdal::Dimension::Id::Y, pointId),
      point_view->getFieldAs<double>(pdal::Dimension::Id::Z, pointId)
    };
    points->InsertNextPoint(point);
    if (hasColor)
    {
      uint16_t color[3] = {
        point_view->getFieldAs<uint16_t>(pdal::Dimension::Id::Red, pointId),
        point_view->getFieldAs<uint16_t>(pdal::Dimension::Id::Green, pointId),
        point_view->getFieldAs<uint16_t>(pdal::Dimension::Id::Blue, pointId),
      };
      colorArray->InsertNextTypedTuple(color);
    }
    for (size_t i = 0; i < dims.size(); ++i)
    {
      pdal::Dimension::Id dimensionId = dims[i];
      if (dimensionId == pdal::Dimension::Id::X ||
          dimensionId == pdal::Dimension::Id::Y ||
          dimensionId == pdal::Dimension::Id::Z)
      {
        continue;
      }
      if (hasColor && (dimensionId == pdal::Dimension::Id::Red ||
                       dimensionId == pdal::Dimension::Id::Green ||
                       dimensionId == pdal::Dimension::Id::Blue))
      {
        continue;
      }
      pdal::Dimension::Type type = pdal::Dimension::Type::Double;
      try
      {
        type = pdal::Dimension::defaultType(dimensionId);
      }
      catch(...)
      {
        // we ignore fields we don't know about
        continue;
      }
      switch(type)
      {
      default:
      case pdal::Dimension::Type::Double:
        {
          vtkDoubleArray* a = doubleArray[i];
          double value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Float:
        {
          vtkFloatArray* a = floatArray[i];
          float value = point_view->getFieldAs<float>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Unsigned8:
        {
          vtkTypeUInt8Array* a = uInt8Array[i];
          uint8_t value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Unsigned16:
        {
          vtkTypeUInt16Array* a = uInt16Array[i];
          uint16_t value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Unsigned32:
        {
          vtkTypeUInt32Array* a = uInt32Array[i];
          uint32_t value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Signed32:
        {
          vtkTypeInt32Array* a = int32Array[i];
          int32_t value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      case pdal::Dimension::Type::Unsigned64:
        {
          vtkTypeUInt64Array* a = uInt64Array[i];
          uint64_t value = point_view->getFieldAs<double>(dimensionId, pointId);
          a->InsertNextValue(value);
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkPdalReader::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkPdalReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
