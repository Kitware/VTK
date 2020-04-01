/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHDRReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHDRReader.h"

#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkImagePermute.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtksys/SystemTools.hxx"

#include <sstream>

vtkStandardNewMacro(vtkHDRReader);

#define HDR_DATA_SIZE 3

// Matrix to convert from XYZ into linear RGB
const float matrixXYZ2RGB[3][3] = { { 3.2404542f, -1.5371385f, -0.4985314f },
  { -0.9692660f, 1.8760108f, 0.0415560f }, { 0.0556434f, -0.2040259f, 1.0572252f } };

vtkHDRReader::vtkHDRReader()
{
  this->Gamma = 1.0;
  this->Exposure = 1.0;
  this->PixelAspect = 1.0;
  this->SetDataByteOrderToLittleEndian();
}

//----------------------------------------------------------------------------
vtkHDRReader::~vtkHDRReader() {}

//----------------------------------------------------------------------------
void vtkHDRReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ProgramType: " << this->ProgramType << "\n";
  os << indent << "Format: " << this->Format << "\n";
  os << indent << "Gamma: " << this->Gamma << "\n";
  os << indent << "Exposure: " << this->Exposure << "\n";
  os << indent << "PixelAspect: " << this->PixelAspect << "\n";
  os << indent << "FlippedX: " << this->FlippedX << "\n";
  os << indent << "SwappedAxis: " << this->SwappedAxis << "\n";
}

//----------------------------------------------------------------------------
void vtkHDRReader::ExecuteInformation()
{
  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4] == 0 && this->DataExtent[5] == 0 &&
    (this->DataVOI[4] || this->DataVOI[5]))
  {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
  }

  // Setup filename to read the header
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == nullptr || this->InternalFileName[0] == '\0')
  {
    return;
  }

  // Fill headers data, HeaderSize and DataExtent
  // VTK errors are handled in ReadHeaderData.
  if (!this->ReadHeaderData())
  {
    return;
  }
  this->CloseFile();

  // if the user has set the VOI, just make sure its valid
  if (this->DataVOI[0] || this->DataVOI[1] || this->DataVOI[2] || this->DataVOI[3] ||
    this->DataVOI[4] || this->DataVOI[5])
  {
    if ((this->DataVOI[0] < 0) || (this->DataVOI[1] >= this->GetWidth()) ||
      (this->DataVOI[2] < 0) || (this->DataVOI[3] >= this->GetHeight()))
    {
      vtkWarningMacro(
        "The requested VOI is larger than the file's (" << this->InternalFileName << ") extent ");
      this->DataVOI[0] = this->DataExtent[0];
      this->DataVOI[1] = this->DataExtent[1];
      this->DataVOI[2] = this->DataExtent[2];
      this->DataVOI[3] = this->DataExtent[3];
    }
  }

  this->SetDataScalarTypeToFloat();
  this->SetNumberOfScalarComponents(3);
  this->vtkImageReader::ExecuteInformation();
}

//----------------------------------------------------------------------------
int vtkHDRReader::CanReadFile(const char* fname)
{
  // get the magic number by reading in a file
  std::ifstream ifs(fname, std::ifstream::in);

  if (ifs.fail())
  {
    vtkErrorMacro(<< "Could not open file " << fname);
    return 0;
  }

  // The file must begin with magic number #?
  if ((ifs.get() != '#') && (ifs.get() != '?'))
  {
    ifs.close();
    return 0;
  }

  ifs.close();
  return 1;
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkHDRReader::ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (this->UpdateExtentIsEmpty(outInfo, output))
  {
    return;
  }
  if (this->InternalFileName == nullptr)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("HDRImage");

  this->ComputeDataIncrements();

  // Only float value for RGBE data
  float* outPtr = reinterpret_cast<float*>(data->GetScalarPointer());
  this->HDRReaderUpdate(data, outPtr);
}

//----------------------------------------------------------------------------
void vtkHDRReader::HDRReaderUpdate(vtkImageData* data, float* outPtr)
{
  vtkIdType outIncr[3];
  int outExtent[6];

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  // Total size of the output in bytes
  int outPtrSize = (outExtent[1] - outExtent[0] + 1) * (outExtent[3] - outExtent[2] + 1) *
    this->GetNumberOfScalarComponents();

  // Read multiples files
  for (int idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
  {
    this->ComputeInternalFileName(idx2);
    // read in a HDR file

    if (!this->HDRReaderUpdateSlice(outPtr, outExtent))
    {
      return;
    }

    if (this->Format == FORMAT_32BIT_RLE_XYZE)
    {
      // Convert from xyz to rgb
      this->ConvertAllDataFromRGBToXYZ(outPtr, outPtrSize);
    }

    this->UpdateProgress((idx2 - outExtent[4]) / (outExtent[5] - outExtent[4] + 1.0));
    outPtr += outIncr[2];
  }

  if (this->FlippedX)
  {
    vtkNew<vtkImageFlip> flipXFilter;
    flipXFilter->SetFilteredAxis(0);
    flipXFilter->SetInputData(data);
    flipXFilter->Update();
    data->DeepCopy(flipXFilter->GetOutput());
  }

  if (this->SwappedAxis)
  {
    // Permute X and Y
    vtkNew<vtkImagePermute> permuteAxisFilter;
    permuteAxisFilter->SetFilteredAxes(1, 0, 2);
    permuteAxisFilter->SetInputData(data);
    permuteAxisFilter->Update();
    data->DeepCopy(permuteAxisFilter->GetOutput());
  }
}

void vtkHDRReader::ConvertAllDataFromRGBToXYZ(float* outPtr, int size)
{
  for (int i = 0; i < size; i += HDR_DATA_SIZE)
  {
    this->XYZ2RGB(matrixXYZ2RGB, outPtr[i], outPtr[i + 1], outPtr[i + 2]);
  }
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
bool vtkHDRReader::HDRReaderUpdateSlice(float* outPtr, int* outExt)
{
  this->OpenFile();
  std::istream* is = this->GetFile();

  // Ignore header
  is->ignore(this->HeaderSize);

  // Even if we have a smaller extent, the RLE encoding forces us to read all the line width
  const int width = this->GetWidth();

  const int extentWidth = outExt[1] - outExt[0] + 1;

  // We read only lines that we need (depending on outExt[2] and outExt[3]);
  int nbLinesLeft = outExt[3] - outExt[2] + 1;
  const int totalNbLines = nbLinesLeft;

  float* outPtr2 = outPtr;
  // decrPtr is the number of byte needed to go to the previous line
  int decrPtr = 0;
  if (this->FileLowerLeft)
  {
    // Start at the end of outPtr and go up
    outPtr2 = outPtr + (nbLinesLeft - 1) * extentWidth * 3;
    decrPtr = 2 * extentWidth * 3;
  }

  if ((width < 8) || (width > 0x7fff))
  {
    // File not run length encoded
    this->ReadAllFileNoRLE(is, outPtr2, decrPtr, outExt);
    this->CloseFile();
    return true;
  }

  // We need to skip some lines
  int nbSkipLines = outExt[2];

  // Store the position in case of a non RLE encoded file
  std::streampos afterHeader = is->tellg();

  // rgbe will hold the value of each channels
  unsigned char rgbe[4];
  // lineBuffer will contain data read from the file
  // 4 uchar for each pixel of a line
  std::vector<unsigned char> lineBuffer(width * 4);
  while (nbLinesLeft > 0)
  {
    is->read(reinterpret_cast<char*>(rgbe), sizeof(unsigned char) * 4);
    if (this->HasError(is))
    {
      return false;
    }

    // New RLE encoding.
    // First 4 bytes -> rgbe
    // The record begins with an unnormalized pixel having two
    // bytes equal to 2, followed by the upper byte and the lower
    // byte of the scanline length (which mustbe less than
    // 32768). A run is indicated by a byte with its high-order
    // bit set, corresponding to a count with excess 128. A non-run
    // is indicated with a byte less than 128.

    if ((rgbe[0] != 2) || (rgbe[1] != 2) || (rgbe[2] & 0x80))
    {
      // if it is not the first line that we read, then there is an error
      if (nbLinesLeft != totalNbLines)
      {
        vtkErrorMacro(<< "HDRReader: First 4 bytes of the line " << totalNbLines - nbLinesLeft
                      << " are wrong");
        this->CloseFile();
        return false;
      }
      // else, it means that the file not run length encoded

      // Restore position
      is->seekg(afterHeader);

      // Skip beginning lines
      is->ignore(outExt[2] * this->GetWidth() * 4);

      // Read all the file with no RLE encoding
      if (!this->ReadAllFileNoRLE(is, outPtr2, decrPtr, outExt))
      {
        vtkErrorMacro(<< "HDRReader : Bad line data");
        this->CloseFile();
        return false;
      }

      return true;
    }

    if (((static_cast<int>(rgbe[2])) << 8 | rgbe[3]) != width)
    {
      vtkErrorMacro(<< "HDRReader: Wrong scanline width");
      this->CloseFile();
      return false;
    }

    // File is run length encoded, read line by line
    // Read each of the four channel into the lineBuffer
    if (!this->ReadLineRLE(is, lineBuffer.data()))
    {
      vtkErrorMacro(<< "HDRReader : Bad line data");
      this->CloseFile();
      return false;
    }

    // Skip lines
    if (nbSkipLines > 0)
    {
      nbSkipLines--;
    }
    else
    {
      // Convert lineBuffer to RGB floats
      this->FillOutPtrRLE(outExt, outPtr2, lineBuffer);

      // If we need to read bottom up, go to the beginning of the previous line
      outPtr2 -= decrPtr;

      nbLinesLeft--;
    }
  }

  this->CloseFile();
  return true;
}

//----------------------------------------------------------------------------
bool vtkHDRReader::HasError(std::istream* is)
{
  if (!*is)
  {
    vtkErrorMacro(<< "HDRReader : Read error");
    this->CloseFile();
    return true;
  }
  return false;
}

int vtkHDRReader::GetWidth() const
{
  return this->DataExtent[1] - this->DataExtent[0] + 1;
}

int vtkHDRReader::GetHeight() const
{
  return this->DataExtent[3] - this->DataExtent[2] + 1;
}

//----------------------------------------------------------------------------
bool vtkHDRReader::ReadHeaderData()
{
  // Precondition:CanReadFile return true

  if (!this->OpenFile())
  {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return false;
  }

  int headersize = 0;

  std::istream* is = this->File;
  char buffer[128];

  // First line: program type
  is->getline(buffer, 128);
  if (this->HasError(is))
  {
    return false;
  }
  headersize += is->gcount();
  std::istringstream iss(buffer);
  iss.ignore(2);
  iss >> this->ProgramType;

  std::string format;

  // Read header
  while (true)
  {
    is->getline(buffer, 128);
    if (this->HasError(is))
    {
      return false;
    }

    headersize += is->gcount();

    // skip comments
    if (buffer[0] == '#')
    {
      continue;
    }

    // Header end with a blank line
    if (buffer[0] == 0 || buffer[0] == '\r' || buffer[0] == '\n')
    {
      break;
    }

    iss.str(buffer);
    iss.clear();

    iss.getline(buffer, 128, '=');
    if (strcmp(buffer, "FORMAT") == 0)
    {
      iss >> format;

      if (format == "32-bit_rle_rgbe")
      {
        this->Format = vtkHDRReader::FORMAT_32BIT_RLE_RGBE;
      }
      else if (format == "32-bit_rle_xyze")
      {
        this->Format = vtkHDRReader::FORMAT_32BIT_RLE_XYZE;
      }
    }
    else if (strcmp(buffer, "GAMMA") == 0)
    {
      iss >> this->Gamma;
    }
    else if (strcmp(buffer, "EXPOSURE") == 0)
    {
      iss >> this->Exposure;
    }
    else if (strcmp(buffer, "PIXASPECT") == 0)
    {
      iss >> this->PixelAspect;
    }
  }
  // End of header

  // Line for width, height, and sign
  // The X and Y are immediately preceded by a sign which can be used to indicate flipping,
  // the order of the X and Y indicate rotation

  is->getline(buffer, 128);
  headersize += is->gcount();

  iss.str(buffer);
  iss.clear();
  iss >> std::skipws;

  char x, y, signX, signY;
  int h, w;

  iss >> std::skipws >> signY >> y >> h >> signX >> x >> w;

  if (y == 'X')
  {
    // Column order -> a 90 degrees turn
    this->SwappedAxis = true;
    std::swap(h, w);
  }
  // Row order means that X is horizontal axis, and Y is vertical axis

  if (signX == '-')
  {
    // X axis goes from right to left
    this->FlippedX = true;
  }

  // TODO should be a + here, but after tests, it seems that it is inversed
  if (signY == '-')
  {
    this->FileLowerLeft = 1; // Default 0
  }

  // Set header size
  this->ManualHeaderSize = 1;
  this->HeaderSize = headersize;

  // Set size of the image
  this->DataExtent[0] = 0;
  this->DataExtent[1] = w - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = h - 1;

  // Set data spacing
  this->DataSpacing[0] = 1;
  this->DataSpacing[1] = this->PixelAspect;
  this->DataSpacing[2] = 1;

  return true;
}

//----------------------------------------------------------------------------
void vtkHDRReader::FillOutPtrRLE(
  int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer)
{
  unsigned char rgbe[4];
  int width = this->GetWidth();
  // Convert lineBuffer to RGB floats
  for (int i = outExt[0]; i <= outExt[1]; ++i)
  {
    rgbe[0] = lineBuffer[i];
    rgbe[1] = lineBuffer[i + width];
    rgbe[2] = lineBuffer[i + 2 * width];
    rgbe[3] = lineBuffer[i + 3 * width];

    this->RGBE2Float(rgbe, outPtr[0], outPtr[1], outPtr[2]);

    outPtr += HDR_DATA_SIZE;
  }
}

//----------------------------------------------------------------------------
void vtkHDRReader::FillOutPtrNoRLE(
  int* outExt, float*& outPtr, std::vector<unsigned char>& lineBuffer)
{
  // Convert lineBuffer to RGB floats
  for (int i = outExt[0]; i <= outExt[1]; ++i)
  {
    this->RGBE2Float(lineBuffer.data() + 4 * i, outPtr[0], outPtr[1], outPtr[2]);

    outPtr += HDR_DATA_SIZE;
  }
}

//----------------------------------------------------------------------------
bool vtkHDRReader::ReadAllFileNoRLE(std::istream* is, float* outPtr, int decrPtr, int* outExt)
{
  std::vector<unsigned char> lineBuffer(this->GetWidth() * 4);

  int nbLinesLeft = outExt[3] - outExt[2] + 1;
  while (nbLinesLeft > 0)
  {
    // Read a line
    is->read(reinterpret_cast<char*>(lineBuffer.data()), sizeof(unsigned char) * lineBuffer.size());
    if (this->HasError(is))
    {
      return false;
    }

    // Convert lineBuffer to RGB floats
    this->FillOutPtrNoRLE(outExt, outPtr, lineBuffer);

    // Do nothing if we read top to bottom (decrPtr = 0)
    // Else go to the beginning of the previous line
    outPtr -= decrPtr;

    nbLinesLeft--;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkHDRReader::ReadLineRLE(std::istream* is, unsigned char* lineBufferPtr)
{
  // A line in RLE is sorted by channels, ie. it begins by all the red, then green, blue, and
  // exponent It means that we need to read all the line even if we have an smaller extent
  int width = this->GetWidth();
  int count;
  // The data for each encoded channel
  unsigned char buffer[2];
  for (int i = 0; i < 4; ++i)
  {
    unsigned char* ptrEnd = lineBufferPtr + width;
    while (lineBufferPtr < ptrEnd)
    {
      is->read(reinterpret_cast<char*>(buffer), sizeof(unsigned char) * 2);
      if (this->HasError(is))
      {
        return false;
      }

      if (buffer[0] > 128)
      {
        // A run of the same value
        count = buffer[0] - 128;
        if ((count == 0) || (count > ptrEnd - lineBufferPtr))
        {
          return false;
        }

        // Copy the same value in the lineBuffer
        std::fill(lineBufferPtr, lineBufferPtr + count, buffer[1]);
        lineBufferPtr += count;
      }
      else
      {
        // A non run
        count = buffer[0];
        if ((count == 0) || (count > ptrEnd - lineBufferPtr))
        {
          return false;
        }

        *lineBufferPtr++ = buffer[1];
        // Read count bytes into output
        if (--count > 0)
        {
          is->read(reinterpret_cast<char*>(lineBufferPtr), sizeof(unsigned char) * count);
          if (this->HasError(is))
          {
            return false;
          }

          lineBufferPtr += count;
        }
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkHDRReader::RGBE2Float(unsigned char* rgbe, float& r, float& g, float& b)
{
  if (rgbe[3]) /*nonzero pixel*/
  {
    float f = std::ldexp(1.0, rgbe[3] - static_cast<int>(128 + 8)) / this->Exposure;
    r = rgbe[0] * f;
    g = rgbe[1] * f;
    b = rgbe[2] * f;
  }
  else
  {
    r = g = b = 0.0;
  }
}

//----------------------------------------------------------------------------
void vtkHDRReader::XYZ2RGB(const float convertMatrix[3][3], float& r, float& g, float& b)
{
  // Copy initial xyz values
  float x = r, y = g, z = b;
  r = convertMatrix[0][0] * x + convertMatrix[0][1] * y + convertMatrix[0][2] * z;
  g = convertMatrix[1][0] * x + convertMatrix[1][1] * y + convertMatrix[1][2] * z;
  b = convertMatrix[2][0] * x + convertMatrix[2][1] * y + convertMatrix[2][2] * z;
}
