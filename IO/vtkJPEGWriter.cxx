/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.cxx
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
#include "vtkJPEGWriter.h"
#include "vtkObjectFactory.h"

extern "C" {
#include <jpeglib.h>
}

//----------------------------------------------------------------------------
vtkJPEGWriter* vtkJPEGWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkJPEGWriter");
  if(ret)
    {
    return (vtkJPEGWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkJPEGWriter;
}

vtkJPEGWriter::vtkJPEGWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 2;

  this->Quality = 95;
  this->Progressive = 1;
}

//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkJPEGWriter::Write()
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


void vtkJPEGWriter::WriteSlice(vtkImageData *data)
{
  // Call the correct templated function for the output
  void *outPtr;
  unsigned int ui;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkWarningMacro("JPEGWriter only supports unsigned char input");
    return;
    }   

  FILE *fp = fopen(this->InternalFileName, "wb");
  if (!fp)
    {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
    }

  if (data->GetNumberOfScalarComponents() > MAX_COMPONENTS)
    {
    vtkErrorMacro("Exceed JPEG limits for number of components (" << data->GetNumberOfScalarComponents() << " > " << MAX_COMPONENTS << ")" );
    return;
    }
  
  // Create the jpeg compression object and error handler
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  // set the destination file
  jpeg_stdio_dest(&cinfo, fp);

  // set the information about image
  int *uExtent = data->GetUpdateExtent();
  unsigned int width, height;
  width = uExtent[1] - uExtent[0] + 1;
  height = uExtent[3] - uExtent[2] + 1;  

  cinfo.image_width = width; 	/* image width and height, in pixels */
  cinfo.image_height = height;

  cinfo.input_components = data->GetNumberOfScalarComponents();
  switch (cinfo.input_components)
    {
    case 1: cinfo.in_color_space = JCS_GRAYSCALE;
      break;
    case 3: cinfo.in_color_space = JCS_RGB;
      break;
    default: cinfo.in_color_space = JCS_UNKNOWN;
      break;
    }

  // set the compression parameters
  jpeg_set_defaults(&cinfo);         // start with reasonable defaults
  jpeg_set_quality(&cinfo, this->Quality, TRUE);
  if (this->Progressive)
    {
    jpeg_simple_progression(&cinfo);
    }
  
  // start compression
  jpeg_start_compress(&cinfo, TRUE);

  // write the data. in jpeg, the first row is the top row of the image
  JSAMPROW *row_pointers = new JSAMPROW [height];
  int *outInc = data->GetIncrements();
  int rowInc = outInc[1];
  for (ui = 0; ui < height; ui++)
    {
    row_pointers[height - ui - 1] = (JSAMPROW) outPtr;
    outPtr = (unsigned char *)outPtr + rowInc;
    }
  jpeg_write_scanlines(&cinfo, row_pointers, height);

  // finish the compression
  jpeg_finish_compress(&cinfo);

  // clean up and close the file
  delete [] row_pointers;
  jpeg_destroy_compress(&cinfo);
  fclose(fp);
}

void vtkJPEGWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageWriter::PrintSelf(os,indent);

  os << indent << "Quality: " << this->Quality << "\n";
  os << indent << "Progressive: " << (this->Progressive ? "On" : "Off") << "\n";
}
