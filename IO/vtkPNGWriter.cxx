/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPNGWriter.h"
#include "vtkObjectFactory.h"

#include <png.h>

vtkCxxRevisionMacro(vtkPNGWriter, "1.11");
vtkStandardNewMacro(vtkPNGWriter);

vtkPNGWriter::vtkPNGWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 2;
  this->WriteToMemory = 0;
  this->Result = 0;
}

vtkPNGWriter::~vtkPNGWriter()
{
  if (this->Result)
    {
    this->Result->Delete();
    this->Result = 0;
    }
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
  if (!this->WriteToMemory && !this->FileName && !this->FilePattern)
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

void vtkPNGWriteInit(png_structp png_ptr, png_bytep data, 
                     png_size_t sizeToWrite)
{
  vtkPNGWriter *self = 
    vtkPNGWriter::SafeDownCast(static_cast<vtkObject *>
                               (png_get_io_ptr(png_ptr)));
  if (self)
    {
      vtkUnsignedCharArray *uc = self->GetResult();
      // write to the uc array
      unsigned char *ptr = uc->WritePointer(uc->GetMaxId()+1,sizeToWrite);
      memcpy(ptr, data, sizeToWrite);
    }
}

void vtkPNGWriteFlush(png_structp vtkNotUsed(png_ptr))
{
}


void vtkPNGWriter::WriteSlice(vtkImageData *data)
{
  // Call the correct templated function for the output
  void *outPtr;
  unsigned int ui;

  int* uext = data->GetUpdateExtent();
  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer(uext[0], uext[2], uext[4]);
  if (data->GetScalarType() != VTK_UNSIGNED_SHORT &&
      data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkWarningMacro("PNGWriter only supports unsigned char and unsigned short inputs");
    return;
    }   

  png_structp png_ptr = png_create_write_struct
    (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
  if (!png_ptr)
    {
    vtkErrorMacro(<<"Unable to write PNG file!");
    return;
    }
  
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
    png_destroy_write_struct(&png_ptr,
                             (png_infopp)NULL);
    vtkErrorMacro(<<"Unable to write PNG file!");
    return;
    }

  
  FILE *fp = 0;
  if (this->WriteToMemory)
    {
    vtkUnsignedCharArray *uc = this->GetResult();
    if (!uc || uc->GetReferenceCount() > 1)
      {
      uc = vtkUnsignedCharArray::New();
      this->SetResult(uc);
      uc->Delete();
      }
    // start out with 10K as a guess for the image size
    uc->Allocate(10000);
    png_set_write_fn(png_ptr, static_cast<png_voidp>(this), 
                     vtkPNGWriteInit, vtkPNGWriteFlush);
    }
  else
    {
      fp = fopen(this->InternalFileName, "wb");
      if (!fp)
        {
        vtkErrorMacro("Unable to open file " << this->InternalFileName);
        return;
        }
      png_init_io(png_ptr, fp);
    }

  
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
  int rowInc = outInc[1]*bit_depth/8;
  for (ui = 0; ui < height; ui++)
    {
    row_pointers[height - ui - 1] = (png_byte *)outPtr;
    outPtr = (unsigned char *)outPtr + rowInc;
    }
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);

  delete [] row_pointers;
  png_destroy_write_struct(&png_ptr, &info_ptr);

  if (fp)
    {
    fclose(fp);
    }
}

void vtkPNGWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Result: " << this->Result << "\n";
  os << indent << "WriteToMemory: " << (this->WriteToMemory ? "On" : "Off") << "\n";
}
