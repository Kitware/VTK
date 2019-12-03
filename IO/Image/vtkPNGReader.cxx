/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPNGReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtk_png.h"
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <vector>

vtkStandardNewMacro(vtkPNGReader);

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object
// destruction is non-portable
#pragma warning(disable : 4611)
#endif

namespace
{
class CompareFirst
{
public:
  bool operator()(const std::pair<std::string, std::string>& left,
    const std::pair<std::string, std::string>& right)
  {
    return left.first < right.first;
  }
};

/**
 * When reading an image from memory, libpng needs to be passed a pointer to a custom
 * read callback function, as well as a pointer to its input data.
 * This callback function has to behave like fread(), so we use a custom stream object as input.
 */
struct MemoryBufferStream
{
  const unsigned char* buffer = nullptr;
  size_t len = 0;
  size_t position = 0;
};

// To be used by libpng instead of fread when reading data from memory.
void PNGReadCallback(png_structp pngPtr, png_bytep output, png_size_t length)
{
  if (output == nullptr)
  {
    png_error(pngPtr, "Invalid output buffer");
  }
  // Get pointer to input buffer
  png_voidp inputVoidP = png_get_io_ptr(pngPtr);
  if (inputVoidP == nullptr)
  {
    png_error(pngPtr, "Invalid input stream");
  }
  // Cast it to MemoryBufferStream
  MemoryBufferStream* input = static_cast<MemoryBufferStream*>(inputVoidP);
  // Check for overflow
  if (input->position + length > input->len)
  {
    png_error(pngPtr, "Attempt to read out of buffer");
  }
  // Copy it
  auto begin = input->buffer + input->position;
  auto end = begin + length;
  std::copy(begin, end, output);
  // Advance cursor
  input->position += length;
}
};

class vtkPNGReader::vtkInternals
{
public:
  std::vector<std::pair<std::string, std::string> > TextKeyValue;
  typedef std::vector<std::pair<std::string, std::string> >::iterator TextKeyValueIterator;
  void ReadTextChunks(png_structp png_ptr, png_infop info_ptr)
  {
    png_textp text_ptr;
    int num_text;
    png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);
    this->TextKeyValue.clear();
    for (int i = 0; i < num_text; ++i)
    {
      if (
        // we don't deal with compressed text yet
        text_ptr[i].compression != PNG_TEXT_COMPRESSION_NONE ||
        // we don't deal with international text yet
        text_ptr[i].text_length == 0)
      {
        continue;
      }
      this->TextKeyValue.push_back(
        std::pair<std::string, std::string>(text_ptr[i].key, text_ptr[i].text));
    }
    std::sort(this->TextKeyValue.begin(), this->TextKeyValue.end(), CompareFirst());
  }

  void GetTextChunks(const char* key, int beginEndIndex[2])
  {
    std::pair<TextKeyValueIterator, TextKeyValueIterator> it =
      std::equal_range(this->TextKeyValue.begin(), this->TextKeyValue.end(),
        std::pair<std::string, std::string>(key, std::string()), CompareFirst());
    beginEndIndex[0] = it.first - this->TextKeyValue.begin();
    beginEndIndex[1] = it.second - this->TextKeyValue.begin();
  }

  // Returns true if the header is valid
  bool IsHeaderValid(unsigned char header[])
  {
    bool is_png = !png_sig_cmp(header, 0, 8);
    if (!is_png)
    {
      vtkErrorWithObjectMacro(nullptr, << "Unknown file type! Not a PNG file!");
    }
    return is_png;
  }

  // Returns true if the file's header is valid
  bool CheckFileHeader(FILE* fp)
  {
    unsigned char header[8];
    if (fread(header, 1, 8, fp) != 8)
    {
      vtkErrorWithObjectMacro(nullptr,
        "PNGReader error reading file."
          << " Premature EOF while reading header.");
      return false;
    }
    return this->IsHeaderValid(header);
  }

  // Returns true if the buffer's header is valid
  bool CheckBufferHeader(const unsigned char* buffer, vtkIdType length)
  {
    unsigned char header[8];
    if (length < 8)
    {
      vtkErrorWithObjectMacro(nullptr, "MemoryBuffer is too short, could not read the header");
      return false;
    }
    std::copy(buffer, buffer + 8, header);
    return this->IsHeaderValid(header);
  }

  bool CreateLibPngStructs(png_structp& pngPtr, png_infop& infoPtr, png_infop& endInfo)
  {
    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
    if (!pngPtr)
    {
      vtkErrorWithObjectMacro(nullptr, "Out of memory.");
      return false;
    }
    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr)
    {
      png_destroy_read_struct(&pngPtr, (png_infopp)nullptr, (png_infopp)nullptr);
      vtkErrorWithObjectMacro(nullptr, "Out of memory.");
      return false;
    }
    endInfo = png_create_info_struct(pngPtr);
    if (!endInfo)
    {
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)nullptr);
      vtkErrorWithObjectMacro(nullptr, "Unable to read PNG file!");
      return false;
    }
    return true;
  }

  void InitLibPngInput(vtkPNGReader* self, png_structp pngPtr, MemoryBufferStream* stream, FILE* fp)
  {
    // Initialize libpng input
    if (self->GetMemoryBuffer())
    {
      // Tell libpng to read from memory.
      // Initialize our input object.
      stream->buffer = static_cast<const unsigned char*>(self->GetMemoryBuffer());
      stream->len = self->GetMemoryBufferLength();
      // We pass a void pointer to our input object and a pointer to our read callback.
      // Reading starts from 0, so png_set_sig_bytes is not needed.
      png_set_read_fn(
        pngPtr, static_cast<png_voidp>(stream), reinterpret_cast<png_rw_ptr>(PNGReadCallback));
    }
    else
    {
      png_init_io(pngPtr, fp);
      png_set_sig_bytes(pngPtr, 8);
    }
  }

  void HandleLibPngError(png_structp pngPtr, png_infop infoPtr, FILE* fp)
  {
    if (setjmp(png_jmpbuf(pngPtr)))
    {
      png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)nullptr);
      if (fp)
      {
        fclose(fp);
      }
    }
  }
};

//----------------------------------------------------------------------------
vtkPNGReader::vtkPNGReader()
{
  this->Internals = new vtkInternals();
  this->ReadSpacingFromFile = false;
}

//----------------------------------------------------------------------------
vtkPNGReader::~vtkPNGReader()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPNGReader::ExecuteInformation()
{
  vtkInternals* impl = this->Internals;
  FILE* fp = nullptr;
  MemoryBufferStream stream;

  if (this->GetMemoryBuffer())
  {
    // Read the header from MemoryBuffer
    const unsigned char* memBuffer = static_cast<const unsigned char*>(this->GetMemoryBuffer());
    if (!impl->CheckBufferHeader(memBuffer, this->GetMemoryBufferLength()))
    {
      vtkErrorMacro("Invalid MemoryBuffer header: not a PNG file");
      return;
    }
  }
  else
  {
    // Attempt to open the file and read the header
    this->ComputeInternalFileName(this->DataExtent[4]);
    if (this->InternalFileName == nullptr)
    {
      vtkErrorMacro("A filename must be specified");
      return;
    }
    fp = vtksys::SystemTools::Fopen(this->InternalFileName, "rb");
    if (!fp)
    {
      vtkErrorMacro("Unable to open file " << this->InternalFileName);
      return;
    }
    if (!impl->CheckFileHeader(fp))
    {
      vtkErrorMacro("Invalid file header: not a PNG file");
      fclose(fp);
      return;
    }
  }

  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  png_infop end_info = nullptr;
  if (!impl->CreateLibPngStructs(png_ptr, info_ptr, end_info))
  {
    if (fp)
    {
      fclose(fp);
    }
    return;
  }

  impl->HandleLibPngError(png_ptr, info_ptr, fp);
  impl->InitLibPngInput(this, png_ptr, &stream, fp);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  // get size and bit-depth of the PNG-image
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type,
    &compression_type, &filter_method);

  impl->ReadTextChunks(png_ptr, info_ptr);

  // set-up the transformations
  // convert palettes to RGB
  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
  }

  // minimum of a byte per pixel
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
  {
#if PNG_LIBPNG_VER >= 10400
    png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
    png_set_gray_1_2_4_to_8(png_ptr);
#endif
  }

  // add alpha if any alpha found
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  {
    png_set_tRNS_to_alpha(png_ptr);
  }

  // update the info now that we have defined the filters
  png_read_update_info(png_ptr, info_ptr);

  this->DataExtent[0] = 0;
  this->DataExtent[1] = width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = height - 1;

  if (ReadSpacingFromFile)
  {
    png_uint_32 x_pixels_per_meter, y_pixels_per_meter;
    x_pixels_per_meter = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    y_pixels_per_meter = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    if (x_pixels_per_meter > 0 && y_pixels_per_meter > 0)
    {
      this->SetDataSpacing(1000.0 / x_pixels_per_meter, 1000.0 / y_pixels_per_meter, 1);
    }
  }

  if (bit_depth <= 8)
  {
    this->SetDataScalarTypeToUnsignedChar();
  }
  else
  {
    this->SetDataScalarTypeToUnsignedShort();
  }
  this->SetNumberOfScalarComponents(png_get_channels(png_ptr, info_ptr));
  this->vtkImageReader2::ExecuteInformation();

  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  // close the file if necessary
  if (fp)
  {
    fclose(fp);
  }
}

//----------------------------------------------------------------------------
template <class OT>
void vtkPNGReader::vtkPNGReaderUpdate2(OT* outPtr, int* outExt, vtkIdType* outInc, long pixSize)
{
  vtkPNGReader::vtkInternals* impl = this->Internals;
  unsigned int ui;
  int i;
  FILE* fp = nullptr;
  MemoryBufferStream stream;

  if (this->GetMemoryBuffer())
  {
    // Read the header from MemoryBuffer
    const unsigned char* memBuffer = static_cast<const unsigned char*>(this->GetMemoryBuffer());
    if (!impl->CheckBufferHeader(memBuffer, this->GetMemoryBufferLength()))
    {
      vtkErrorMacro("Invalid MemoryBuffer header: not a PNG file");
      return;
    }
  }
  else
  {
    // Attempt to open the file and read the header
    fp = vtksys::SystemTools::Fopen(this->InternalFileName, "rb");
    if (!fp)
    {
      vtkErrorMacro("Unable to open file " << this->InternalFileName);
      return;
    }
    if (!impl->CheckFileHeader(fp))
    {
      vtkErrorMacro("Invalid file header: not a PNG file");
      fclose(fp);
      return;
    }
  }

  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  png_infop end_info = nullptr;
  if (!impl->CreateLibPngStructs(png_ptr, info_ptr, end_info))
  {
    if (fp)
    {
      fclose(fp);
    }
    return;
  }

  impl->HandleLibPngError(png_ptr, info_ptr, fp);
  impl->InitLibPngInput(this, png_ptr, &stream, fp);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  // get size and bit-depth of the PNG-image
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type,
    &compression_type, &filter_method);

  impl->ReadTextChunks(png_ptr, info_ptr);

  // set-up the transformations
  // convert palettes to RGB
  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png_ptr);
  }

  // minimum of a byte per pixel
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
  {
#if PNG_LIBPNG_VER >= 10400
    png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
    png_set_gray_1_2_4_to_8(png_ptr);
#endif
  }

  // add alpha if any alpha found
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  {
    png_set_tRNS_to_alpha(png_ptr);
  }

  if (bit_depth > 8)
  {
#ifndef VTK_WORDS_BIGENDIAN
    png_set_swap(png_ptr);
#endif
  }

  // have libpng handle interlacing
  // int number_of_passes = png_set_interlace_handling(png_ptr);
  // update the info now that we have defined the filters
  png_read_update_info(png_ptr, info_ptr);

  size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  std::vector<unsigned char> tempImage(rowbytes * height);
  std::vector<png_bytep> row_pointers(height);
  for (ui = 0; ui < height; ++ui)
  {
    row_pointers[ui] = tempImage.data() + rowbytes * ui;
  }
  png_read_image(png_ptr, row_pointers.data());

  // copy the data into the outPtr
  OT* outPtr2;
  outPtr2 = outPtr;
  long outSize = pixSize * (outExt[1] - outExt[0] + 1);
  for (i = outExt[2]; i <= outExt[3]; ++i)
  {
    memcpy(outPtr2, row_pointers[height - i - 1] + outExt[0] * pixSize, outSize);
    outPtr2 += outInc[1];
  }

  png_read_end(png_ptr, nullptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  // close the file if necessary
  if (fp)
  {
    fclose(fp);
  }
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkPNGReader::vtkPNGReaderUpdate(vtkImageData* data, OT* outPtr)
{
  vtkIdType outIncr[3];
  int outExtent[6];
  OT* outPtr2;

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  long pixSize = data->GetNumberOfScalarComponents() * sizeof(OT);

  outPtr2 = outPtr;
  int idx2;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
  {
    this->ComputeInternalFileName(idx2);
    // read in a PNG file
    this->vtkPNGReaderUpdate2(outPtr2, outExtent, outIncr, pixSize);
    this->UpdateProgress((idx2 - outExtent[4]) / (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
  }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkPNGReader::ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (!this->GetMemoryBuffer() && this->InternalFileName == nullptr)
  {
    vtkErrorMacro(<< "Either a FileName, FilePrefix or MemoryBuffer must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("PNGImage");

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  void* outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
  {
    vtkTemplateMacro(this->vtkPNGReaderUpdate(data, (VTK_TT*)(outPtr)));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
  }
}

//----------------------------------------------------------------------------
int vtkPNGReader::CanReadFile(const char* fname)
{
  FILE* fp = vtksys::SystemTools::Fopen(fname, "rb");
  if (!fp)
  {
    return 0;
  }
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8)
  {
    fclose(fp);
    return 0;
  }
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png)
  {
    fclose(fp);
    return 0;
  }
  png_structp png_ptr =
    png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)nullptr, nullptr, nullptr);
  if (!png_ptr)
  {
    fclose(fp);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
    fclose(fp);
    return 0;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)nullptr);
    fclose(fp);
    return 0;
  }
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  fclose(fp);
  return 3;
}
#ifdef _MSC_VER
// Put the warning back
#pragma warning(default : 4611)
#endif

//----------------------------------------------------------------------------
void vtkPNGReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Read Spacing From File: " << (this->ReadSpacingFromFile ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkPNGReader::GetTextChunks(const char* key, int beginEndIndex[2])
{
  this->Internals->GetTextChunks(key, beginEndIndex);
}

//----------------------------------------------------------------------------
const char* vtkPNGReader::GetTextKey(int index)
{
  return this->Internals->TextKeyValue[index].first.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPNGReader::GetTextValue(int index)
{
  return this->Internals->TextKeyValue[index].second.c_str();
}

size_t vtkPNGReader::GetNumberOfTextChunks()
{
  return this->Internals->TextKeyValue.size();
}
