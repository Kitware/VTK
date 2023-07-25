// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkTypeUInt16Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdio>
#include <memory>
#include <sstream>

#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)

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

struct NoOpRgbArrayTypes
{
  template <typename ArrayT>
  void operator()(ArrayT* vtkNotUsed(array))
  {
  }
};

struct SaveRgbArray
{
  template <typename ArrayT>
  void operator()(ArrayT* array, vtkIdList* pointIds, std::ofstream& out)
  {
    vtkTypeUInt16Array* a = vtkTypeUInt16Array::FastDownCast(array);
    bool unsignedCharRange = false;
    int numberOfComponents = array->GetNumberOfComponents();
    if (a)
    {
      int j;
      for (j = 0; j < numberOfComponents; ++j)
      {
        double r[2];
        a->GetFiniteRange(r, j);
        if (r[1] > 255)
        {
          break;
        }
      }
      if (j == numberOfComponents)
      {
        unsignedCharRange = true;
      }
    }
    for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
    {
      std::array<char, 4> rgba = { { 0, 0, 0, 0 } };
      for (int j = 0; j < numberOfComponents; ++j)
      {
        char c = a
          ? (unsignedCharRange
                ? static_cast<char>(array->GetTypedComponent(pointIds->GetId(i), j))
                : static_cast<char>(array->GetTypedComponent(pointIds->GetId(i), j) / 256.0))
          : static_cast<char>(array->GetTypedComponent(pointIds->GetId(i), j));
        rgba[j] = c;
      }
      out.write(rgba.data(), numberOfComponents);
    }
  }
};

}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCesiumPointCloudWriter);

vtkCesiumPointCloudWriter::vtkCesiumPointCloudWriter()
{
  this->FileName = nullptr;
  this->PointIds = nullptr;
}

vtkCesiumPointCloudWriter::~vtkCesiumPointCloudWriter()
{
  this->SetFileName(nullptr);
  this->SetPointIds(nullptr);
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
  auto rgbArray = pointSet->GetPointData()->GetScalars();
  using RgbArrayTypes =
    vtkTypeList::Unique<vtkTypeList::Create<vtkUnsignedCharArray, vtkTypeUInt16Array>>::Result;
  using RgbDispatch = vtkArrayDispatch::DispatchByArray<RgbArrayTypes>;
  bool validRgbArray = RgbDispatch::Execute(rgbArray, NoOpRgbArrayTypes{});
  std::string rgbTypeName;
  if (validRgbArray)
  {
    switch (rgbArray->GetNumberOfComponents())
    {
      case 3:
        rgbTypeName = "RGB";
        break;
      case 4:
        rgbTypeName = "RGBA";
        break;
      default:
        // we don't know how to deal with arrays different than RGB or RGBA
        validRgbArray = false;
    }
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

  // build FeatureTableJSON
  nlohmann::json featureTable;
  featureTable["POINTS_LENGTH"] = this->PointIds->GetNumberOfIds();
  featureTable["RTC_CENTER"] = origin;
  featureTable["POSITION"]["byteOffset"] = 0;
  if (validRgbArray)
  {
    featureTable[rgbTypeName]["byteOffset"] = this->PointIds->GetNumberOfIds() * 3 * sizeof(float);
  }

  std::ostringstream ostr;
  ostr << featureTable;
  Header header;

  // FeatureTableJSON must end on a 8-byte boundary, so we pad with space.
  int featureTableJSONPadding = (8 - ((sizeof(header) + ostr.str().length()) % 8)) % 8;
  for (int i = 0; i < featureTableJSONPadding; ++i)
  {
    ostr << ' ';
  }

  // FeatureTableBinary body must end on a 8-byte boundary, so we pad with 0
  // POSITION ends on 4-byte boundary
  // there is not start requirement for RGB
  // RGBA should start at 4-byte boundary
  vtkIdType featureTableBinarySize = this->PointIds->GetNumberOfIds() * 3 * sizeof(float);
  if (validRgbArray)
  {
    featureTableBinarySize += this->PointIds->GetNumberOfIds() * rgbArray->GetNumberOfComponents();
  }
  int featureTableBinaryPadding = (8 - (featureTableBinarySize % 8)) % 8;

  // build the header
  header.version = 1;
  header.featureTableJSONByteLength =
    static_cast<uint32_t>(ostr.str().length()); // includes padding
  header.featureTableBinaryByteLength = featureTableBinarySize + featureTableBinaryPadding;
  header.batchTableJSONByteLength = 0;
  header.batchTableBinaryByteLength = 0;
  header.byteLength = sizeof(header) + header.featureTableJSONByteLength +
    header.featureTableBinaryByteLength + header.batchTableJSONByteLength +
    header.batchTableBinaryByteLength;

  // write the magic
  out.write("pnts", 4);
  // write header's next 6 uint32_t
  vtkByteSwap::SwapWrite4LERange(&header.version, 6, &out);
  // write FeatureTableJSON
  out.write(ostr.str().c_str(), ostr.str().length());
  // write POSITION
  for (vtkIdType i = 0; i < this->PointIds->GetNumberOfIds(); ++i)
  {
    double pointd[3];
    float pointf[3];
    pointSet->GetPoints()->GetPoint(this->PointIds->GetId(i), pointd);
    vtkMath::Subtract(pointd, origin.data(), pointd);
    for (int j = 0; j < 3; ++j)
    {
      pointf[j] = pointd[j];
    }
    vtkByteSwap::SwapWrite4LERange(pointf, 3, &out);
  }
  // write RGB or RGBA
  if (validRgbArray)
  {
    RgbDispatch::Execute(rgbArray, SaveRgbArray{}, this->PointIds, out);
  }
  // pad FeatureTableBinary
  char c = 0;
  for (int i = 0; i < featureTableBinaryPadding; ++i)
  {
    out.write(&c, sizeof(c));
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
  os << indent << "PointIds number of ids: " << this->PointIds->GetNumberOfIds() << std::endl;
}

int vtkCesiumPointCloudWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  return 1;
}
VTK_ABI_NAMESPACE_END
