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

vtkStandardNewMacro(vtkPNGReader);

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object
// destruction is non-portable
#pragma warning( disable : 4611 )
#endif

//----------------------------------------------------------------------------
void vtkPNGReader::ExecuteInformation()
{
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
void vtkPNGReaderUpdate2(vtkPNGReader *self, OT *outPtr,
                         int *outExt, vtkIdType *outInc, long pixSize)
{
  unsigned int ui;
  int i;
  FILE *fp = fopen(self->GetInternalFileName(), "rb");
  if (!fp)
    {
    return;
    }
  unsigned char header[8];
  if (fread(header, 1, 8, fp) != 8)
    {
    vtkGenericWarningMacro ("PNGReader error reading file: " << self->GetInternalFileName()
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
void vtkPNGReaderUpdate(vtkPNGReader *self, vtkImageData *data, OT *outPtr)
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
    self->ComputeInternalFileName(idx2);
    // read in a PNG file
    vtkPNGReaderUpdate2(self, outPtr2, outExtent, outIncr, pixSize);
    self->UpdateProgress((idx2 - outExtent[4])/
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
    vtkTemplateMacro(vtkPNGReaderUpdate(this, data, (VTK_TT *)(outPtr)));
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
}
