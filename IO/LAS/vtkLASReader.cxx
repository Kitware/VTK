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

#include "vtkLASReader.h"

#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkUnsignedShortArray.h>
#include <vtkVertexGlyphFilter.h>
#include <vtksys/FStream.hxx>

#include <liblas/liblas.hpp>

#include <fstream>
#include <iostream>
#include <valarray>

vtkStandardNewMacro(vtkLASReader);

//----------------------------------------------------------------------------
vtkLASReader::vtkLASReader()
{
  this->FileName = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkLASReader::~vtkLASReader()
{
  delete[] this->FileName;
}

//----------------------------------------------------------------------------
int vtkLASReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(request), vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Open LAS File for reading
  vtksys::ifstream ifs;
  ifs.open(this->FileName, std::ios_base::binary | std::ios_base::in);

  if (!ifs.is_open())
  {
    vtkErrorMacro(<< "Unable to open file for reading: " << this->FileName);
    return VTK_ERROR;
  }

  // Read header data
  liblas::ReaderFactory readerFactory;
  liblas::Reader reader = readerFactory.CreateWithStream(ifs);

  vtkNew<vtkPolyData> pointsPolyData;
  this->ReadPointRecordData(reader, pointsPolyData);
  ifs.close();

  // Convert points to verts in output polydata
  vtkNew<vtkVertexGlyphFilter> vertexFilter;
  vertexFilter->SetInputData(pointsPolyData);
  vertexFilter->Update();
  output->ShallowCopy(vertexFilter->GetOutput());

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkLASReader::ReadPointRecordData(liblas::Reader& reader, vtkPolyData* pointsPolyData)
{
  vtkNew<vtkPoints> points;
  // scalars associated with points
  vtkNew<vtkUnsignedShortArray> color;
  color->SetName("color");
  color->SetNumberOfComponents(3);
  vtkNew<vtkUnsignedShortArray> classification;
  classification->SetName("classification");
  classification->SetNumberOfComponents(1);
  vtkNew<vtkUnsignedShortArray> intensity;
  intensity->SetName("intensity");
  intensity->SetNumberOfComponents(1);

  liblas::Header header = liblas::Header(reader.GetHeader());
  std::valarray<double> scale = { header.GetScaleX(), header.GetScaleY(), header.GetScaleZ() };
  std::valarray<double> offset = { header.GetOffsetX(), header.GetOffsetY(), header.GetOffsetZ() };
  liblas::PointFormatName pointFormat = header.GetDataFormatId();
  int pointRecordsCount = header.GetPointRecordsCount();

  for (int i = 0; i < pointRecordsCount && reader.ReadNextPoint(); i++)
  {
    liblas::Point const& p = reader.GetPoint();
    std::valarray<double> lasPoint = { p.GetX(), p.GetY(), p.GetZ() };
    points->InsertNextPoint(&lasPoint[0]);
    // std::valarray<double> point = lasPoint * scale + offset;
    // We have seen a file where the scaled points were much smaller than the offset
    // So, all points ended up in the same place.
    std::valarray<double> point = lasPoint * scale;
    switch (pointFormat)
    {
      case liblas::ePointFormat2:
      case liblas::ePointFormat3:
      case liblas::ePointFormat5:
      {
        unsigned short c[3];
        c[0] = p.GetColor().GetRed();
        c[1] = p.GetColor().GetGreen();
        c[2] = p.GetColor().GetBlue();
        color->InsertNextTypedTuple(c);
        intensity->InsertNextValue(p.GetIntensity());
      }
      break;

      case liblas::ePointFormat0:
      case liblas::ePointFormat1:
        classification->InsertNextValue(p.GetClassification().GetClass());
        intensity->InsertNextValue(p.GetIntensity());
        break;

      case liblas::ePointFormatUnknown:
      default:
        intensity->InsertNextValue(p.GetIntensity());
        break;
    }
  }

  pointsPolyData->SetPoints(points);
  pointsPolyData->GetPointData()->AddArray(intensity);
  switch (pointFormat)
  {
    case liblas::ePointFormat2:
    case liblas::ePointFormat3:
    case liblas::ePointFormat5:
      pointsPolyData->GetPointData()->AddArray(color);
      break;

    case liblas::ePointFormat0:
    case liblas::ePointFormat1:
      pointsPolyData->GetPointData()->AddArray(classification);
      break;

    case liblas::ePointFormatUnknown:
    default:
      break;
  }
}

//----------------------------------------------------------------------------
void vtkLASReader::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkLASReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
