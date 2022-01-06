/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTGAReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTGAReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include <vtksys/FStream.hxx>

vtkStandardNewMacro(vtkTGAReader);

namespace
{
constexpr int HeaderSize = 18;

enum TGAFormat : unsigned char
{
  Uncompressed_RGB = 2,
  RLE_RGB = 10
};
}

//----------------------------------------------------------------------------
void vtkTGAReader::ExecuteInformation()
{
  char header[::HeaderSize];

  if (this->GetMemoryBuffer())
  {
    const char* memBuffer = static_cast<const char*>(this->GetMemoryBuffer());
    std::copy(memBuffer, memBuffer + ::HeaderSize, header);
  }
  else
  {
    this->ComputeInternalFileName(0);
    vtksys::ifstream file(this->InternalFileName, std::ios::binary);

    file.read(header, ::HeaderSize * sizeof(char));

    file.close();
  }

  // tmp char needed to avoid strict anti aliasing warning
  char* tmp = &header[8];
  this->DataOrigin[0] = static_cast<double>(*reinterpret_cast<short*>(tmp));
  tmp = &header[10];
  this->DataOrigin[1] = static_cast<double>(*reinterpret_cast<short*>(tmp));
  this->DataOrigin[2] = 0.0;

  this->DataExtent[0] = 0;
  tmp = &header[12];
  this->DataExtent[1] = static_cast<int>(*reinterpret_cast<short*>(tmp) - 1);
  this->DataExtent[2] = 0;
  tmp = &header[14];
  this->DataExtent[3] = static_cast<int>(*reinterpret_cast<short*>(tmp) - 1);

  bool upperLeft = static_cast<bool>((header[17] >> 5) & 1);
  this->SetFileLowerLeft(!upperLeft);

  this->SetHeaderSize(::HeaderSize);
  this->SetDataScalarTypeToUnsignedChar();

  this->SetNumberOfScalarComponents(header[16] / 8);

  this->vtkImageReader2::ExecuteInformation();
}

//------------------------------------------------------------------------------
void vtkTGAReader::ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);
  data->GetPointData()->GetScalars()->SetName("TGAImage");

  this->ComputeDataIncrements();

  std::vector<unsigned char> content;

  if (this->GetMemoryBuffer())
  {
    const unsigned char* uBuffer = reinterpret_cast<const unsigned char*>(this->GetMemoryBuffer());
    content.assign(uBuffer, uBuffer + this->GetMemoryBufferLength());
  }
  else
  {
    vtksys::ifstream imageFile(this->InternalFileName, std::ios::binary);
    content.assign(std::istreambuf_iterator<char>(imageFile), std::istreambuf_iterator<char>());
  }

  bool encoded = (content[2] == ::TGAFormat::RLE_RGB);
  vtkIdType nComponents = this->GetNumberOfScalarComponents();

  // buffer to image data
  unsigned char* outPtr = reinterpret_cast<unsigned char*>(data->GetScalarPointer());
  unsigned char* endPtr =
    outPtr + ((this->DataExtent[3] + 1) * (this->DataExtent[1] + 1) * nComponents);

  auto GetColor = [&](size_t& currentIndex, unsigned char*& buffer) {
    for (vtkIdType j = 0; j < nComponents; j++)
    {
      buffer[j] = content[currentIndex + j];
    }
    if (nComponents >= 3) // swap red and blue
    {
      std::swap(buffer[0], buffer[2]);
    }

    currentIndex += nComponents;
    buffer += nComponents;
  };

  size_t skip = ::HeaderSize + static_cast<size_t>(content[0]);

  size_t index = skip;
  while (outPtr < endPtr && index < content.size())
  {
    if (encoded)
    {
      unsigned char packet = content[index++];
      if (packet & 0x80) // packet is RLE if highest bit is 1
      {
        // RLE packet (clear highest bit and add 1)
        packet = (packet & 0x7f) + 1;
        unsigned char* dupBuffer = outPtr;
        GetColor(index, outPtr);
        for (unsigned char i = 0; i < packet - 1; i++)
        {
          for (vtkIdType k = 0; k < nComponents; k++)
          {
            outPtr[k] = dupBuffer[k];
          }
          outPtr += nComponents;
        }
      }
      else
      {
        // raw packet (add 1)
        packet += 1;
        for (unsigned char i = 0; i < packet; i++)
        {
          GetColor(index, outPtr);
        }
      }
    }
    else
    {
      GetColor(index, outPtr);
    }
  }

  if (!this->FileLowerLeft)
  {
    vtkNew<vtkImageFlip> flipY;
    flipY->SetFilteredAxis(1);
    flipY->SetInputData(data);
    flipY->Update();
    data->ShallowCopy(flipY->GetOutput());
  }
}

//----------------------------------------------------------------------------
int vtkTGAReader::CanReadFile(const char* fname)
{
  vtksys::ifstream file(fname, std::ios::binary);

  if (!file.is_open())
  {
    return 0;
  }

  char header[::HeaderSize];
  file.read(header, ::HeaderSize * sizeof(char));

  // only uncompressed RGB and RLE encoded RGB formats are supported
  if (header[2] != ::TGAFormat::RLE_RGB && header[2] != ::TGAFormat::Uncompressed_RGB)
  {
    vtkWarningMacro("Only RLE RGB and uncompressed RGB TGA files are supported");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkTGAReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
