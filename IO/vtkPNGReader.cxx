/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPNGReader.h"
#include "vtkObjectFactory.h"

#include <png.h>

//-------------------------------------------------------------------------
vtkPNGReader* vtkPNGReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPNGReader");
  if(ret)
    {
    return (vtkPNGReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPNGReader;
}


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
  fread(header, 1, 8, fp);
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
    vtkErrorMacro(<<"Unable to read PNG file!");
    fclose(fp);
    return;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    vtkErrorMacro(<<"Unable to read PNG file!");
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
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  png_get_IHDR(png_ptr, info_ptr, 
               &width, &height,
               &bit_depth, &color_type, &interlace_type,
               &compression_type, &filter_method);

  // convert palettes to RGB
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
    png_set_palette_to_rgb(png_ptr);
    }

  // minimum of a byte per pixel
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
    {
    png_set_gray_1_2_4_to_8(png_ptr);
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
  png_destroy_read_struct(&png_ptr, &info_ptr,NULL);
  fclose(fp);
}


template <class OT>
static void vtkPNGReaderUpdate2(vtkPNGReader *self, OT *outPtr,
                                int *outExt, int *outInc, long pixSize)
{
  unsigned int ui;
  int i;
  FILE *fp = fopen(self->GetInternalFileName(), "rb");
  if (!fp)
    {
    return;
    }
  unsigned char header[8];
  fread(header, 1, 8, fp);
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
    return;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    png_destroy_read_struct(&png_ptr,
                            (png_infopp)NULL, (png_infopp)NULL);
    return;
    }

  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
    {
    png_destroy_read_struct(&png_ptr, &info_ptr,
                            (png_infopp)NULL);
    return;
    }
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  int compression_type, filter_method;
  png_get_IHDR(png_ptr, info_ptr, 
               &width, &height,
               &bit_depth, &color_type, &interlace_type,
               &compression_type, &filter_method);

  // convert palettes to RGB
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
    png_set_palette_to_rgb(png_ptr);
    }

  // minimum of a byte per pixel
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
    {
    png_set_gray_1_2_4_to_8(png_ptr);
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
  
  // close the file
  png_read_end(png_ptr, NULL);
  png_destroy_read_struct(&png_ptr, &info_ptr,NULL);
  fclose(fp);
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
static void vtkPNGReaderUpdate(vtkPNGReader *self, vtkImageData *data, 
                               OT *outPtr)
{
  int outIncr[3];
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
void vtkPNGReader::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);

  if (this->InternalFileName == NULL)
    {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
    }

  this->ComputeDataIncrements();
  
  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
    {
    vtkTemplateMacro3(vtkPNGReaderUpdate, this, data, (VTK_TT *)(outPtr));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }   
}

