/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCesiumPointCloudWriter.h"
#include "vtkArrayDispatch.h"
#include "vtkAssemblyPath.h"
#include "vtkBase64OutputStream.h"
#include "vtkByteSwap.h"
#include "vtkCamera.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageReader.h"
#include "vtkInformation.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdio>
#include <memory>
#include <sstream>

#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)

vtkStandardNewMacro(vtkCesiumPointCloudWriter);

vtkCesiumPointCloudWriter::vtkCesiumPointCloudWriter()
{
  this->FileName = nullptr;
  this->SaveNormal = false;
  this->SaveBatchId = false;
}

vtkCesiumPointCloudWriter::~vtkCesiumPointCloudWriter()
{
  this->SetFileName(nullptr);
}

namespace
{
struct Header
{
  char magic[4]; // magic is written directly instead of being copied here first.
  uint32_t version;
  uint32_t byteLength;
  uint32_t featureTableJSONByteLength;
  uint32_t featureTableBinaryByteLength;
  uint32_t batchTableJSONByteLength;
  uint32_t batchTableBinaryByteLength;
};

}

void vtkCesiumPointCloudWriter::WriteData()
{
  // make sure the user specified a FileName
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }
  vtkPointSet* pointSet = vtkPointSet::SafeDownCast(this->GetInput());
  if (pointSet == nullptr)
  {
    vtkErrorMacro(<< "Please specify a vtkPointSet input");
    return;
  }
  if (this->PointIds == nullptr)
  {
    vtkErrorMacro(<< "Please specify the point Ids to save");
    return;
  }
  std::ofstream out;
  out.open(this->FileName, std::ios_base::out | std::ios_base::binary);
  if (out.fail())
  {
    vtkErrorMacro(<< "Cannot open " << this->FileName << " for writing.");
    return;
  }
  double bb[6];
  std::array<double, 3> origin;
  pointSet->GetBounds(bb);
  origin = { { bb[0], bb[2], bb[4] } };
  nlohmann::json featureTable;
  featureTable["POINTS_LENGTH"] = this->PointIds->GetNumberOfIds();
  featureTable["RTC_CENTER"] = origin;
  featureTable["POSITION"]["byteOffset"] = 0;
  std::ostringstream ostr;
  ostr << featureTable;
  Header header;
  header.version = 1;
  header.featureTableJSONByteLength = ostr.str().length();
  header.featureTableBinaryByteLength = this->PointIds->GetNumberOfIds() * 4;
  header.batchTableJSONByteLength = 0;
  header.batchTableBinaryByteLength = 0;
  header.byteLength = sizeof(header) + header.featureTableJSONByteLength +
    header.featureTableBinaryByteLength + header.batchTableJSONByteLength +
    header.batchTableBinaryByteLength;
  out.write("pnts", 4);
  vtkByteSwap::Swap4LE(&header.version);
  vtkByteSwap::Swap4LE(&header.byteLength);
  vtkByteSwap::Swap4LE(&header.featureTableJSONByteLength);
  vtkByteSwap::Swap4LE(&header.featureTableBinaryByteLength);
  vtkByteSwap::Swap4LE(&header.batchTableJSONByteLength);
  vtkByteSwap::Swap4LE(&header.batchTableBinaryByteLength);
  out.write(ostr.str().c_str(), ostr.str().length());
  for (vtkIdType i = 0; i < this->PointIds->GetNumberOfIds(); ++i)
  {
    double pointd[3];
    float pointf[3];
    pointSet->GetPoints()->GetPoint(this->PointIds->GetId(i), pointd);
    vtkMath::Subtract(pointd, &origin[0], pointd);
    for (int j = 0; j < 3; ++j)
    {
      pointf[0] = pointd[0];
    }
    vtkByteSwap::Swap4LE(pointf);
  }
}

void vtkCesiumPointCloudWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
  os << indent << "SaveNormal: " << this->SaveNormal << std::endl;
  os << indent << "SaveBatchId: " << this->SaveBatchId << std::endl;
  os << indent << "PointIds number of ids: " << this->PointIds->GetNumberOfIds() << std::endl;
}

int vtkCesiumPointCloudWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}
