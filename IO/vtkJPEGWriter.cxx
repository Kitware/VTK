/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.cxx
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
#include "vtkJPEGWriter.h"
#include "vtkObjectFactory.h"

extern "C" {
#include <jpeglib.h>
}

vtkCxxRevisionMacro(vtkJPEGWriter, "1.9");
vtkStandardNewMacro(vtkJPEGWriter);

vtkJPEGWriter::vtkJPEGWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 2;

  this->Quality = 95;
  this->Progressive = 1;
  this->WriteToMemory = 0;
  this->Result = 0;
}

vtkJPEGWriter::~vtkJPEGWriter()
{
  if (this->Result)
    {
    this->Result->Delete();
    this->Result = 0;
    }
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
  if (!this->WriteToMemory && ! this->FileName && !this->FilePattern)
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

// these three routines are for wqriting into memory
void vtkJPEGWriteToMemoryInit(j_compress_ptr cinfo)
{
  vtkJPEGWriter *self = vtkJPEGWriter::SafeDownCast(
    static_cast<vtkObject *>(cinfo->client_data));
  if (self)
    {
    vtkUnsignedCharArray *uc = self->GetResult();
    if (!uc || uc->GetReferenceCount() > 1)
      {
      uc = vtkUnsignedCharArray::New();
      self->SetResult(uc);
      uc->Delete();
      // start out with 10K as a guess for the image size
      uc->Allocate(10000);
      }
    cinfo->dest->next_output_byte = uc->GetPointer(0);
    cinfo->dest->free_in_buffer = uc->GetSize();
    }
}

boolean vtkJPEGWriteToMemoryEmpty(j_compress_ptr cinfo)
{
  vtkJPEGWriter *self = vtkJPEGWriter::SafeDownCast(
    static_cast<vtkObject *>(cinfo->client_data));
  if (self)
    {
    vtkUnsignedCharArray *uc = self->GetResult();
    // we must grow the array, we grow by 50% each time
    int oldSize = uc->GetSize();
    uc->Resize(oldSize + oldSize/2);
    cinfo->dest->next_output_byte = uc->GetPointer(oldSize);
    cinfo->dest->free_in_buffer = oldSize/2;
    }
  return TRUE;
}

void vtkJPEGWriteToMemoryTerm(j_compress_ptr cinfo)
{
  vtkJPEGWriter *self = vtkJPEGWriter::SafeDownCast(
    static_cast<vtkObject *>(cinfo->client_data));
  if (self)
    {
    vtkUnsignedCharArray *uc = self->GetResult();
    // we must close the array
    int oldSize = uc->GetSize();
    uc->SetNumberOfTuples(oldSize - cinfo->dest->free_in_buffer);
    }
}

void vtkJPEGWriter::WriteSlice(vtkImageData *data)
{
  // Call the correct templated function for the output
  void *outPtr;
  unsigned int ui;

  int* uext = data->GetUpdateExtent();
  
  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer(uext[0], uext[2], uext[4]);
  if (data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkWarningMacro("JPEGWriter only supports unsigned char input");
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
  struct jpeg_destination_mgr compressionDestination;
  FILE *fp = 0;
  if (this->WriteToMemory)
    {
    // setup the compress structure to write to memory
    compressionDestination.init_destination = vtkJPEGWriteToMemoryInit;
    compressionDestination.empty_output_buffer = vtkJPEGWriteToMemoryEmpty;
    compressionDestination.term_destination = vtkJPEGWriteToMemoryTerm;
    cinfo.dest = &compressionDestination;
    cinfo.client_data = static_cast<void *>(this);
    }
  else
    {
    fp = fopen(this->InternalFileName, "wb");
    if (!fp)
      {
      vtkErrorMacro("Unable to open file " << this->InternalFileName);
      return;
      }
    jpeg_stdio_dest(&cinfo, fp);
    }
  
  // set the information about image
  int *uExtent = data->GetUpdateExtent();
  unsigned int width, height;
  width = uExtent[1] - uExtent[0] + 1;
  height = uExtent[3] - uExtent[2] + 1;  

  cinfo.image_width = width;    /* image width and height, in pixels */
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
  
  if (!this->WriteToMemory)
    {
    fclose(fp);
    }
}

void vtkJPEGWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Quality: " << this->Quality << "\n";
  os << indent << "Progressive: " << (this->Progressive ? "On" : "Off") << "\n";
  os << indent << "Result: " << this->Result << "\n";
  os << indent << "WriteToMemory: " << (this->WriteToMemory ? "On" : "Off") << "\n";
}
