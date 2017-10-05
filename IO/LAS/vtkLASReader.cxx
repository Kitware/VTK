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
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkVertexGlyphFilter.h>

#include <liblas/liblas.hpp>

#include <fstream>
#include <iostream>
#include <valarray>

vtkStandardNewMacro(vtkLASReader)


//----------------------------------------------------------------------------
vtkLASReader::vtkLASReader() :
  ClassificationColorMap{
    //{Red,  Green,   Blue}
    {  0,      0,      0},    //0     Created, Never Classified   Black
    {255,      0,      0},    //1     Unclassified                Red
    {145,    100,     45},    //2     Ground                      Brown
    {  0,    255,      0},    //3     Low Vegetation              Light Green
    {  0,    160,      0},    //4     Medium Vegetation           Medium Green
    {  0,     90,    255},    //5     High Vegetation             Dark Green
    {255,    255,      0},    //6     Building                    Yellow
    {255,    140,      0},    //7     Low Point                   Orange
    {255,      0,    255},    //8     Model Key-Point             Purple
    {  0,    255,    255}     //9     Water                       Blue
  }
{
  this->FileName = NULL;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkLASReader::~vtkLASReader()
{
  if ( ! this->FileName )
    delete[] this->FileName;
}

//----------------------------------------------------------------------------
int vtkLASReader::RequestData(vtkInformation* vtkNotUsed(request),
                                   vtkInformationVector** vtkNotUsed(request),
                                   vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the ouptut
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Open LAS File for reading
  std::ifstream ifs;
  ifs.open(this->FileName, std::ios_base::binary | std::ios_base::in);

  if ( ! ifs.is_open() )
  {
    vtkErrorMacro (<< "Unable to open file for reading: " << this->FileName );
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
void vtkLASReader::SetClassificationColor(ClassificationType type,
                                          unsigned char red,
                                          unsigned char green,
                                          unsigned char blue)
{
  if(type <= 0 || type > Water)
  {
  vtkErrorMacro (<< "Invalid type of Class Specified!");
  return;
  }

  this->ClassificationColorMap[type][0] = red;
  this->ClassificationColorMap[type][1] = green;
  this->ClassificationColorMap[type][2] = blue;
}

//----------------------------------------------------------------------------
void vtkLASReader::SetClassificationColor(ClassificationType type, unsigned char color[])
{
  this->SetClassificationColor(type, color[0], color[1], color[2]);
}

//----------------------------------------------------------------------------
void vtkLASReader::ReadPointRecordData(liblas::Reader &reader, vtkPolyData* pointsPolyData)
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetNumberOfComponents(3);
  liblas::Header header = liblas::Header(reader.GetHeader());
  std::valarray<double> scale = {
    header.GetScaleX(), header.GetScaleY(), header.GetScaleZ()
  };
  std::valarray<double> offset = {
    header.GetOffsetX(), header.GetOffsetY(), header.GetOffsetZ()
  };
  liblas::PointFormatName pointFormat = header.GetDataFormatId();
  int pointRecordsCount = header.GetPointRecordsCount();
  std::cout << "PointFormat: " << pointFormat << std::endl;

  for ( int i= 0; i < pointRecordsCount && reader.ReadNextPoint(); i++)
  {
  liblas::Point const& p = reader.GetPoint();
  std::valarray<double> lasPoint = {
    p.GetX(), p.GetY(), p.GetZ()
  };
  std::valarray<double> point = lasPoint * scale + offset;
  points->InsertNextPoint(&lasPoint[0]);

  unsigned char* color;
  switch(pointFormat)
    {
    case liblas::ePointFormat2:
    case liblas::ePointFormat3:
    case liblas::ePointFormat5:
      {
      unsigned char color[3];
      color[0] = p.GetColor().GetRed() / 256;
      color[1] = p.GetColor().GetGreen() / 256;
      color[2] = p.GetColor().GetBlue() / 256;
      colors->InsertNextTypedTuple(color);
      }
      break;

    case liblas::ePointFormat0:
    case liblas::ePointFormat1:
      colors->InsertNextTypedTuple( this->ClassificationColorMap[p.GetClassification().GetClass()] );
      break;

    case liblas::ePointFormatUnknown:
    default:
      break;
    }
  }

  pointsPolyData->SetPoints(points);

  if (pointFormat != liblas::ePointFormatUnknown)
  {
  pointsPolyData->GetPointData()->SetScalars(colors);
  }
}

//----------------------------------------------------------------------------
void vtkLASReader::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkLASReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
