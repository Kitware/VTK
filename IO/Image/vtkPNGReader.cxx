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

#include <algorithm>
#include <vector>


vtkStandardNewMacro(vtkPNGReader);

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object
// destruction is non-portable
#pragma warning( disable : 4611 )
#endif

namespace
{
  class CompareFirst
  {
  public:
    bool operator() (const std::pair<std::string,std::string>& left,
                     const std::pair<std::string,std::string>& right)
    {
      return left.first < right.first;
    }

  };
};

class vtkPNGReader::vtkInternals
{
public:
  std::vector<std::pair<std::string, std::string> > TextKeyValue;
  typedef std::vector<std::pair<std::string, std::string> >::iterator
  TextKeyValueIterator;
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
        text_ptr[i].text_length == 0
          )
      {
        continue;
      }
      this->TextKeyValue.push_back(std::pair<std::string, std::string>(
                                     text_ptr[i].key, text_ptr[i].text));
    }
    std::sort(this->TextKeyValue.begin(), this->TextKeyValue.end(),
              CompareFirst());
  }

  void GetTextChunks(const char* key, int beginEndIndex[2])
  {
    std::pair<TextKeyValueIterator, TextKeyValueIterator> it =
      std::equal_range(this->TextKeyValue.begin(), this->TextKeyValue.end(),
                       std::pair<std::string,std::string>(key,std::string()),
                       CompareFirst());
    beginEndIndex[0] = it.first - this->TextKeyValue.begin();
    beginEndIndex[1] = it.second - this->TextKeyValue.begin();
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
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL)
  {
    return;
  }

  FILE *fp = fopen(this->InternalFileName, "rb");
  if (!fp)
  {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
  }
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8)
  {
    vtkErrorMacro ("PNGReader error reading file."
                   << " Premature EOF while reading header.");
    fclose (fp);
    return;
  }
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png)
  {
    vtkErrorMacro(<<"Unknown file type! Not a PNG file!");
    fclose(fp);
    return;
  }

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
     NULL, NULL);
  if (!png_ptr)
  {
    vtkErrorMacro(<< "Out of memory." );
    fclose(fp);
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    vtkErrorMacro(<< "Out of memory.");
    fclose(fp);
    return;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            (png_infopp)NULL);
    vtkErrorMacro(<<"Unable to read PNG file!");
    fclose(fp);
    return;
  }

  // Set error handling
  if (setjmp (png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  // get size and bit-depth of the PNG-image
  png_get_IHDR(png_ptr, info_ptr,
               &width, &height,
               &bit_depth, &color_type, &interlace_type,
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

  if(ReadSpacingFromFile)
  {
    png_uint_32 x_pixels_per_meter, y_pixels_per_meter;
    x_pixels_per_meter = png_get_x_pixels_per_meter(png_ptr, info_ptr);
    y_pixels_per_meter = png_get_y_pixels_per_meter(png_ptr, info_ptr);
    if (x_pixels_per_meter > 0 && y_pixels_per_meter > 0)
    {
      this->SetDataSpacing(1000.0/x_pixels_per_meter, 1000.0/y_pixels_per_meter, 1);
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
  this->SetNumberOfScalarComponents(
    png_get_channels(png_ptr, info_ptr));
  this->vtkImageReader2::ExecuteInformation();

  // close the file
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
}

//----------------------------------------------------------------------------
template <class OT>
void vtkPNGReader::vtkPNGReaderUpdate2(
  OT *outPtr, int *outExt, vtkIdType *outInc, long pixSize)
{
  vtkPNGReader::vtkInternals* impl = this->Internals;
  unsigned int ui;
  int i;
  FILE *fp = fopen(this->GetInternalFileName(), "rb");
  if (!fp)
  {
    return;
  }
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8)
  {
    vtkGenericWarningMacro ("PNGReader error reading file: " << this->GetInternalFileName()
                   << " Premature EOF while reading header.");
    fclose (fp);
    return;
  }
  int is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png)
  {
    fclose(fp);
    return;
  }

  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (!png_ptr)
  {
    fclose(fp);
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            (png_infopp)NULL);
    fclose(fp);
    return;
  }

  // Set error handling
  if (setjmp (png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    return;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  // get size and bit-depth of the PNG-image
  png_get_IHDR(png_ptr, info_ptr,
               &width, &height,
               &bit_depth, &color_type, &interlace_type,
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
  //int number_of_passes = png_set_interlace_handling(png_ptr);
  // update the info now that we have defined the filters
  png_read_update_info(png_ptr, info_ptr);

  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  unsigned char *tempImage = new unsigned char [rowbytes*height];
  png_bytep *row_pointers = new png_bytep [height];
  for (ui = 0; ui < height; ++ui)
  {
    row_pointers[ui] = tempImage + rowbytes*ui;
  }
  png_read_image(png_ptr, row_pointers);

  // copy the data into the outPtr
  OT *outPtr2;
  outPtr2 = outPtr;
  long outSize = pixSize*(outExt[1] - outExt[0] + 1);
  for (i = outExt[2]; i <= outExt[3]; ++i)
  {
    memcpy(outPtr2,row_pointers[height - i - 1] + outExt[0]*pixSize,outSize);
    outPtr2 += outInc[1];
  }
  delete [] tempImage;
  delete [] row_pointers;

  // close the file
  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkPNGReader::vtkPNGReaderUpdate(
  vtkImageData *data, OT *outPtr)
{
  vtkIdType outIncr[3];
  int outExtent[6];
  OT *outPtr2;

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  long pixSize = data->GetNumberOfScalarComponents()*sizeof(OT);

  outPtr2 = outPtr;
  int idx2;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
  {
    this->ComputeInternalFileName(idx2);
    // read in a PNG file
    this->vtkPNGReaderUpdate2(outPtr2, outExtent, outIncr, pixSize);
    this->UpdateProgress((idx2 - outExtent[4])/
                         (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
  }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkPNGReader::ExecuteDataWithInformation(vtkDataObject *output,
                                              vtkInformation *outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  if (this->InternalFileName == NULL)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("PNGImage");

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
  {
    vtkTemplateMacro(this->vtkPNGReaderUpdate(data, (VTK_TT *)(outPtr)));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
  }
}


//----------------------------------------------------------------------------
int vtkPNGReader::CanReadFile(const char* fname)
{
  FILE* fp = fopen(fname, "rb");
  if(!fp)
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
  if(!is_png)
  {
    fclose(fp);
    return 0;
  }
  png_structp png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
     NULL, NULL);
  if (!png_ptr)
  {
    fclose(fp);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    return 0;
  }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            (png_infopp)NULL);
    fclose(fp);
    return 0;
  }
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  fclose(fp);
  return 3;
}
#ifdef _MSC_VER
// Put the warning back
#pragma warning( default : 4611 )
#endif

//----------------------------------------------------------------------------
void vtkPNGReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Read Spacing From File: " <<
    (this->ReadSpacingFromFile ? "On\n" : "Off\n");
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
