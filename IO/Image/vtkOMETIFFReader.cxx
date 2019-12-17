/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOMETIFFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOMETIFFReader.h"
#include "vtkTIFFReaderInternal.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredData.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtk_pugixml.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

class vtkOMETIFFReader::vtkOMEInternals
{
public:
  bool IsValid = false;
  pugi::xml_document XMLDocument;
  int SizeX = 0;
  int SizeY = 0;
  int SizeZ = 0;
  int SizeC = 0;
  int SizeT = 0;
  double TimeIncrement = 1.0;
  vtkVector3d PhysicalSize;
  vtkVector3<std::string> PhysicalSizeUnit;

  // key = vtkVector3i(C, T, Z)
  std::map<vtkVector3i, int> IFDMap;
  std::vector<vtkSmartPointer<vtkImageData> > Cache;
  vtkSmartPointer<vtkStringArray> PhysicalSizeUnitArray;
  std::vector<vtkSmartPointer<vtkDoubleArray> > RangeArrays;
  vtkTimeStamp CacheMTime;

  void UpdateCache(vtkImageData* output);
  void ExtractFromCache(vtkImageData* output, int t)
  {
    if (!this->IsValid)
    {
      return;
    }
    t = std::min(t, static_cast<int>(this->Cache.size()) - 1);
    t = std::max(0, t);
    if (t >= 0 && t < static_cast<int>(this->Cache.size()))
    {
      output->ShallowCopy(this->Cache[t]);
    }
    output->GetFieldData()->AddArray(this->PhysicalSizeUnitArray);

    for (auto& array : this->RangeArrays)
    {
      output->GetFieldData()->AddArray(array);
    }
  }
};

//----------------------------------------------------------------------------
void vtkOMETIFFReader::vtkOMEInternals::UpdateCache(vtkImageData* source)
{
  if (!this->IsValid)
  {
    return;
  }

  int dims[3];
  source->GetDimensions(dims);
  assert(dims[0] <= this->SizeX && dims[1] <= this->SizeY &&
    dims[2] == this->SizeZ * this->SizeT * this->SizeC);

  int ext[6];
  source->GetExtent(ext);

  vtkIdType inIncrements[3];
  source->GetIncrements(inIncrements);

  std::vector<vtkVector2d> channel_ranges;
  channel_ranges.resize(this->SizeC, vtkVector2d(VTK_DOUBLE_MAX, VTK_DOUBLE_MIN));

  for (int t = 0; t < this->SizeT; ++t)
  {
    vtkNew<vtkImageData> img;
    img->SetExtent(ext[0], ext[1], ext[2], ext[3], 0, this->SizeZ - 1);
    img->AllocateScalars(source->GetScalarType(), source->GetNumberOfScalarComponents());
    this->Cache.push_back(img);

    auto pd = img->GetPointData();
    std::vector<vtkDataArray*> scalar_arrays;
    scalar_arrays.push_back(pd->GetScalars());
    for (int c = 1; c < this->SizeC; ++c)
    {
      auto array = vtkDataArray::CreateDataArray(source->GetScalarType());
      array->SetNumberOfComponents(source->GetNumberOfScalarComponents());
      array->SetNumberOfTuples(img->GetNumberOfPoints());
      pd->AddArray(array);
      scalar_arrays.push_back(array);
      array->Delete();
    }

    // rename arrays.
    for (size_t c = 0; c < scalar_arrays.size(); ++c)
    {
      std::ostringstream str;
      str << "Channel_" << (c + 1); // channel names start with 1.
      scalar_arrays[c]->SetName(str.str().c_str());
    }

    for (int c = 0; c < this->SizeC; ++c)
    {
      int outpageIdx = 0;
      for (int z = 0; z < this->SizeZ; ++z)
      {
        auto iter = this->IFDMap.find(vtkVector3i(c, t, z));
        assert(iter != this->IFDMap.end());

        auto srcptr =
          reinterpret_cast<unsigned char*>(source->GetScalarPointer(ext[0], ext[2], iter->second));

        int coordinate[] = { ext[0], ext[2], outpageIdx++ };
        auto destptr =
          reinterpret_cast<unsigned char*>(img->GetArrayPointer(scalar_arrays[c], coordinate));
        std::copy(srcptr, srcptr + inIncrements[2] * img->GetScalarSize(), destptr);
      }
    }

    for (int c = 0; c < this->SizeC; ++c)
    {
      vtkVector2d range;
      scalar_arrays[c]->GetRange(range.GetData(), -1);
      if (range[0] <= range[1])
      {
        channel_ranges[c][0] = std::min(channel_ranges[c][0], range[0]);
        channel_ranges[c][1] = std::max(channel_ranges[c][1], range[1]);
      }
    }
  }

  this->PhysicalSizeUnitArray = vtkSmartPointer<vtkStringArray>::New();
  this->PhysicalSizeUnitArray->SetName("PhysicalSizeUnit");
  this->PhysicalSizeUnitArray->SetNumberOfTuples(3);
  this->PhysicalSizeUnitArray->SetValue(0, this->PhysicalSizeUnit[0]);
  this->PhysicalSizeUnitArray->SetValue(1, this->PhysicalSizeUnit[1]);
  this->PhysicalSizeUnitArray->SetValue(2, this->PhysicalSizeUnit[2]);

  // update temporal channel ranges.
  this->RangeArrays.clear();
  this->RangeArrays.resize(this->SizeC, nullptr);
  for (int c = 0; c < this->SizeC; ++c)
  {
    this->RangeArrays[c] = vtkSmartPointer<vtkDoubleArray>::New();
    std::ostringstream str;
    str << "Channel_" << (c + 1) << "_Range";
    this->RangeArrays[c]->SetName(str.str().c_str());
    this->RangeArrays[c]->SetNumberOfComponents(2);
    this->RangeArrays[c]->SetNumberOfTuples(1);
    this->RangeArrays[c]->SetTypedTuple(0, channel_ranges[c].GetData());
  }

  this->CacheMTime.Modified();
}

//============================================================================
vtkStandardNewMacro(vtkOMETIFFReader);
//----------------------------------------------------------------------------
vtkOMETIFFReader::vtkOMETIFFReader()
  : OMEInternals(new vtkOMETIFFReader::vtkOMEInternals())
{
}

//----------------------------------------------------------------------------
vtkOMETIFFReader::~vtkOMETIFFReader()
{
  delete this->OMEInternals;
  this->OMEInternals = nullptr;
}

//----------------------------------------------------------------------------
void vtkOMETIFFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkOMETIFFReader::CanReadFile(const char* fname)
{
  if (!this->Superclass::CanReadFile(fname))
  {
    return 0;
  }

  int status = 0;
  auto tiffImage = TIFFOpen(fname, "r");
  char* description[255];
  if (TIFFGetField(tiffImage, TIFFTAG_IMAGEDESCRIPTION, description))
  {
    pugi::xml_document doc;
    auto result = doc.load_buffer(description[0], strlen(description[0]));
    status = (result && doc.root().child("OME")) ? 1 : 0;
  }
  TIFFClose(tiffImage);
  return status;
}

//----------------------------------------------------------------------------
void vtkOMETIFFReader::ExecuteInformation()
{
  this->Superclass::ExecuteInformation();
  auto& interals = (*this->InternalImage);
  if (!interals.Image || !interals.IsOpen)
  {
    return;
  }

  auto& omeinternals = (*this->OMEInternals);
  omeinternals.IsValid = false;

  auto& doc = omeinternals.XMLDocument;

  char* description[255];
  if (TIFFGetField(interals.Image, TIFFTAG_IMAGEDESCRIPTION, description))
  {
    auto result = doc.load_buffer(description[0], strlen(description[0]));
    if (!result)
    {
      return;
    }
  }

  // Superclass sets up data-extent without considering the OME header.
  // We update it here appropriately.
  auto pixelsXML = doc.root().child("OME").child("Image").child("Pixels");
  if (!pixelsXML)
  {
    return;
  }

  omeinternals.IsValid = true;
  omeinternals.SizeX = pixelsXML.attribute("SizeX").as_int(0);
  omeinternals.SizeY = pixelsXML.attribute("SizeY").as_int(0);
  omeinternals.SizeZ = pixelsXML.attribute("SizeZ").as_int(1);
  omeinternals.SizeC = pixelsXML.attribute("SizeC").as_int(1);
  omeinternals.SizeT = pixelsXML.attribute("SizeT").as_int(1);
  omeinternals.TimeIncrement = pixelsXML.attribute("TimeIncrement").as_double(1.0);
  omeinternals.PhysicalSize[0] = pixelsXML.attribute("PhysicalSizeX").as_double(1.0);
  omeinternals.PhysicalSize[1] = pixelsXML.attribute("PhysicalSizeY").as_double(1.0);
  omeinternals.PhysicalSize[2] = pixelsXML.attribute("PhysicalSizeZ").as_double(1.0);
  omeinternals.PhysicalSizeUnit[0] = pixelsXML.attribute("PhysicalSizeXUnit").as_string();
  omeinternals.PhysicalSizeUnit[1] = pixelsXML.attribute("PhysicalSizeYUnit").as_string();
  omeinternals.PhysicalSizeUnit[2] = pixelsXML.attribute("PhysicalSizeZUnit").as_string();

  if (!this->GetSpacingSpecifiedFlag())
  {
    this->DataSpacing[0] = omeinternals.PhysicalSize[0];
    this->DataSpacing[1] = omeinternals.PhysicalSize[1];
    this->DataSpacing[2] = omeinternals.PhysicalSize[2];
  }

  assert(omeinternals.SizeX == (this->DataExtent[1] - this->DataExtent[0] + 1) &&
    omeinternals.SizeY == (this->DataExtent[3] - this->DataExtent[2] + 1));

  // based on `DimensionOrder` decide indexes for each.
  const std::string dimensionsOrder{ pixelsXML.attribute("DimensionOrder").as_string("XYZTC") };
  const auto z_idx = static_cast<int>(dimensionsOrder.find('Z')) - 2;
  const auto c_idx = static_cast<int>(dimensionsOrder.find('C')) - 2;
  const auto t_idx = static_cast<int>(dimensionsOrder.find('T')) - 2;

  int dims[3];
  dims[z_idx] = omeinternals.SizeZ;
  dims[c_idx] = omeinternals.SizeC;
  dims[t_idx] = omeinternals.SizeT;

  // Populate the IFD map.
  // We build an explicit map to handle all the cases that OMETiff supports
  // with TiffData.
  // ref:
  // https://docs.openmicroscopy.org/ome-model/5.6.3/ome-tiff/specification.html#the-tiffdata-element
  int nextIFD = 0;
  int next[3] = { 0, 0, 0 };
  for (auto tiffdataXML : pixelsXML.child("TiffData"))
  {
    next[z_idx] = tiffdataXML.attribute("FirstZ").as_int(next[z_idx]);
    next[c_idx] = tiffdataXML.attribute("FirstC").as_int(next[c_idx]);
    next[t_idx] = tiffdataXML.attribute("FirstT").as_int(next[t_idx]);
    nextIFD = tiffdataXML.attribute("IFD").as_int(nextIFD);

    const int planeCount = tiffdataXML.attribute("PlaneCount")
                             .as_int(tiffdataXML.attribute("IFD") ? 1 : interals.NumberOfPages);
    for (int plane = 0; plane < planeCount; ++plane)
    {
      omeinternals.IFDMap[vtkVector3i(next[c_idx], next[t_idx], next[z_idx])] = nextIFD;

      ++nextIFD;
      ++next[0];
      if (next[0] == dims[0])
      {
        next[0] = 0;
        ++next[1];
        if (next[1] == dims[1])
        {
          next[1] = 0;
          ++next[2];
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkOMETIFFReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto& omeinternals = (*this->OMEInternals);
  // when `RequestInformation` is called, we're assured that the reader's Mtime
  // may have changed, so we discard cache since the changes may impact how and
  // what we're reading.
  omeinternals.Cache.clear();
  omeinternals.CacheMTime = vtkTimeStamp();

  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  if (!omeinternals.IsValid)
  {
    return 0;
  }

  auto outInfo = outputVector->GetInformationObject(0);

  // change whole-extent.
  int whole_extent[6];
  whole_extent[0] = whole_extent[2] = whole_extent[4] = 0;
  whole_extent[1] = omeinternals.SizeX - 1;
  whole_extent[3] = omeinternals.SizeY - 1;
  whole_extent[5] = omeinternals.SizeZ - 1;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
  outInfo->Set(vtkDataObject::SPACING(), this->DataSpacing, 3);

  // add timesteps information.
  if (omeinternals.SizeT >= 1)
  {
    std::vector<double> timesteps(omeinternals.SizeT);

    double start = 0.0;
    const double increment = omeinternals.TimeIncrement;
    std::generate(timesteps.begin(), timesteps.end(), [&start, &increment]() {
      double ret = start;
      start += increment;
      return ret;
    });
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timesteps[0], omeinternals.SizeT);

    double range[2] = { timesteps.front(), timesteps.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }
  outInfo->Remove(CAN_PRODUCE_SUB_EXTENT());
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkOMETIFFReader::ExecuteDataWithInformation(vtkDataObject* dobj, vtkInformation* outInfo)
{
  // we want to make superclass read all channels for all timesteps at the same
  // time.
  auto& omeinternals = (*this->OMEInternals);
  if (omeinternals.CacheMTime < this->GetMTime())
  {
    vtkNew<vtkExtentTranslator> extTranslator;
    extTranslator->SetPiece(vtkStreamingDemandDrivenPipeline::GetUpdatePiece(outInfo));
    extTranslator->SetNumberOfPieces(
      vtkStreamingDemandDrivenPipeline::GetUpdateNumberOfPieces(outInfo));
    extTranslator->SetGhostLevel(vtkStreamingDemandDrivenPipeline::GetUpdateGhostLevel(outInfo));

    // We can only split in XY since that z-planes could be spliced arbitrary
    // between z,c, and t, and it becomes too convoluted to compute the extent to
    // read. Splitting in XY can be achieved by forcing the Z dims to be 1 and
    // using vtkExtentTranslator::BLOCK_MODE.
    extTranslator->SetWholeExtent(
      this->DataExtent[0], this->DataExtent[1], this->DataExtent[2], this->DataExtent[3], 0, 0);
    extTranslator->SetSplitModeToBlock();
    extTranslator->PieceToExtent();

    int uExtent[6];
    extTranslator->GetExtent(uExtent);

    // adjust z-extent
    uExtent[4] = this->DataExtent[4];
    uExtent[5] = this->DataExtent[5];

    vtkLogF(TRACE, "update-ext (%d, %d, %d, %d, %d, %d)", uExtent[0], uExtent[1], uExtent[2],
      uExtent[3], uExtent[4], uExtent[5]);

    vtkNew<vtkInformation> info;
    info->Copy(outInfo);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExtent, 6);

    vtkNew<vtkImageData> cache;
    this->Superclass::ExecuteDataWithInformation(cache, info);

    // pre-process data to extract each channel.
    omeinternals.UpdateCache(cache);
  }

  // copy appropriate timestep from cache to the output.
  auto output = vtkImageData::SafeDownCast(dobj);
  assert(output != nullptr);

  double time = outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
    ? outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP())
    : 0.0;
  int time_step = static_cast<int>(std::floor(time / omeinternals.TimeIncrement));
  omeinternals.ExtractFromCache(output, time_step);
  output->SetSpacing(this->DataSpacing);
}
