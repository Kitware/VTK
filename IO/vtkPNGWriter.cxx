/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGWriter.cxx
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
#include "vtkPNGWriter.h"
#include "vtkObjectFactory.h"

#include <png.h>


//----------------------------------------------------------------------------
vtkPNGWriter* vtkPNGWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPNGWriter");
  if(ret)
    {
    return (vtkPNGWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPNGWriter;
}

vtkPNGWriter::vtkPNGWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 2;
}

//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkPNGWriter::Write()
{
  // Error checking
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
    }
  if ( ! this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Write:Please specify either a FileName or a file prefix and pattern");
    return;
    }
  
  // Make sure the file name is allocated
  this->InternalFileName = 
    new char[(this->FileName ? strlen(this->FileName) : 1) +
	    (this->FilePrefix ? strlen(this->FilePrefix) : 1) +
	    (this->FilePattern ? strlen(this->FilePattern) : 1) + 10];
  
  // Fill in image information.
  this->GetInput()->UpdateInformation();
  int *wExtent;
  wExtent = this->GetInput()->GetWholeExtent();
  this->FileNumber = this->GetInput()->GetWholeExtent()[4];
  this->UpdateProgress(0.0);
  // loop over the z axis and write the slices
  for (this->FileNumber = wExtent[4]; this->FileNumber <= wExtent[5]; 
       ++this->FileNumber)
    {
    this->GetInput()->SetUpdateExtent(wExtent[0], wExtent[1],
                                      wExtent[2], wExtent[3],
                                      this->FileNumber, 
                                      this->FileNumber);
    // determine the name
    if (this->FileName)
      {
      sprintf(this->InternalFileName,"%s",this->FileName);
      }
    else 
      {
      if (this->FilePrefix)
        {
        sprintf(this->InternalFileName, this->FilePattern, 
                this->FilePrefix, this->FileNumber);
        }
      else
        {
        sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
        }
      }
    this->GetInput()->UpdateData();
    this->WriteSlice(this->GetInput());
    this->UpdateProgress((this->FileNumber - wExtent[4])/
                         (wExtent[5] - wExtent[4] + 1.0));
    }
  delete [] this->InternalFileName;
  this->InternalFileName = NULL;
}


void vtkPNGWriter::WriteSlice(vtkImageData *data)
{
  // Call the correct templated function for the output
  void *outPtr;
  int bpp = data->GetNumberOfScalarComponents();
  unsigned int ui;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  if (data->GetScalarType() != VTK_UNSIGNED_SHORT &&
      data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkWarningMacro("PNGWriter only supports unsigned char and unsigned short inputs");
    return;
    }   

  FILE *fp = fopen(this->InternalFileName, "wb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }
  
  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (!png_ptr)
    {
    vtkErrorMacro(<<"Unable to write PNG file!");
    fclose(fp);
    return;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    png_destroy_write_struct(&png_ptr,
                             (png_infopp)NULL);
    vtkErrorMacro(<<"Unable to write PNG file!");
    fclose(fp);
    return;
    }

  png_init_io(png_ptr, fp);
  
  int *uExtent = data->GetUpdateExtent();
  png_uint_32 width, height;
  width = uExtent[1] - uExtent[0] + 1;
  height = uExtent[3] - uExtent[2] + 1;  
  int bit_depth = 8;
  if (data->GetScalarType() == VTK_UNSIGNED_SHORT)
    {
    bit_depth = 16;
    }
  int color_type;
  switch (data->GetNumberOfScalarComponents())
    {
    case 1: color_type = PNG_COLOR_TYPE_GRAY;
      break;
    case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      break;
    case 3: color_type = PNG_COLOR_TYPE_RGB;
      break;
    default: color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      break;
    }
  
  png_set_IHDR(png_ptr, info_ptr, width, height,
               bit_depth, color_type, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, 
               PNG_FILTER_TYPE_DEFAULT);
  // interlace_type - PNG_INTERLACE_NONE or
  //                 PNG_INTERLACE_ADAM7
    
  png_write_info(png_ptr, info_ptr);
  // default is big endian
  if (bit_depth > 8)
    {
#ifndef VTK_WORDS_BIGENDIAN
    png_set_swap(png_ptr);
#endif
    }
  png_byte **row_pointers = new png_byte *[height];
  int *outInc = data->GetIncrements();
  for (ui = 0; ui < height; ui++)
    {
    row_pointers[height - ui - 1] = (png_byte *)outPtr;
    outPtr = (unsigned char *)outPtr + outInc[1];
    }
  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
}
