// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCesiumB3DMReader.h"

#include "vtkByteSwap.h"
#include "vtkCesium3DTilesHeader.h"
#include "vtkFileResourceStream.h"
#include "vtkGLTFReader.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkResourceStream.h"

#include <array>
#include <fstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
void read4le(istream& in, uint32_t* p)
{
  in.read(reinterpret_cast<char*>(p), 4);
  if (in)
  {
    vtkByteSwap::Swap4LE(&p);
  }
  else
  {
    throw std::runtime_error("Read " + std::to_string(in.gcount()) + " out of 4 bytes");
  }
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCesiumB3DMReader);

//------------------------------------------------------------------------------
vtkCesiumB3DMReader::vtkCesiumB3DMReader()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkCesiumB3DMReader::~vtkCesiumB3DMReader()
{
  this->SetFileName(nullptr);
}

//------------------------------------------------------------------------------
void vtkCesiumB3DMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//------------------------------------------------------------------------------
void vtkCesiumB3DMReader::StoreTextureData()
{
  if (!this->Textures.empty())
  {
    this->Textures.clear();
  }
}

//------------------------------------------------------------------------------
int vtkCesiumB3DMReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  try
  {
    // Get the output
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);

    if (!this->FileName || std::string(this->FileName).empty())
    {
      vtkErrorMacro("Invalid input filename: nullptr or empty");
      return 0;
    }
    std::ifstream in;
    in.open(this->FileName, std::ios_base::in | std::ios_base::binary);
    if (in.fail())
    {
      vtkErrorMacro(<< "Cannot open " << this->FileName << " for writing.");
      return 0;
    }
    // read the header
    B3DMHeader header;
    in.read(header.magic, 4);
    if (!std::equal(header.magic, header.magic + 4, "b3dm"))
    {
      vtkErrorMacro("Invalid B3DM magic: " << header.magic[0] << header.magic[1] << header.magic[2]
                                           << header.magic[3]);
      return 0;
    }
    read4le(in, &header.version);
    read4le(in, &header.byteLength);
    read4le(in, &header.featureTableJSONByteLength);
    read4le(in, &header.featureTableBinaryByteLength);
    read4le(in, &header.batchTableJSONByteLength);
    read4le(in, &header.batchTableBinaryByteLength);
    in.close();

    vtkNew<vtkFileResourceStream> fileStream;
    if (!fileStream->Open(this->FileName))
    {
      vtkErrorMacro("Invalid input filename: " << this->FileName);
      return 0;
    }
    vtkTypeInt64 glbStart = 28 /*header size on disk*/ + header.featureTableJSONByteLength +
      header.featureTableBinaryByteLength + header.batchTableJSONByteLength +
      header.batchTableBinaryByteLength;
    fileStream->Seek(glbStart, vtkResourceStream::SeekDirection::Begin);
    vtkNew<vtkGLTFReader> reader;
    reader->SetGLBStart(glbStart);
    reader->SetStream(fileStream);
    reader->Update();
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
    output->CompositeShallowCopy(mb);
    return 1;
  }
  catch (std::exception& e)
  {
    vtkErrorMacro("Error: " << e.what());
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkCesiumB3DMReader::GetNumberOfTextures()
{
  return static_cast<vtkIdType>(this->Textures.size());
}

//------------------------------------------------------------------------------
vtkCesiumB3DMReader::Texture vtkCesiumB3DMReader::GetTexture(vtkIdType textureIndex)
{
  if (textureIndex < 0 || textureIndex >= static_cast<vtkIdType>(this->Textures.size()))
  {
    vtkErrorMacro("Out of range texture index");
    return Texture{ nullptr, 0, 0, 0, 0 };
  }
  return this->Textures[textureIndex];
}

VTK_ABI_NAMESPACE_END
