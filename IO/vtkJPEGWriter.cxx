/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJPEGWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"
#include "vtkUnsignedCharArray.h"

extern "C" {
#include "vtk_jpeg.h"
#if defined(__sgi) && !defined(__GNUC__)
#  if   (_COMPILER_VERSION >= 730)
#  pragma set woff 3505
#  endif
#endif
#include <setjmp.h>
}

vtkStandardNewMacro(vtkJPEGWriter);

vtkCxxSetObjectMacro(vtkJPEGWriter,Result,vtkUnsignedCharArray);

vtkJPEGWriter::vtkJPEGWriter()
{
  this->FileLowerLeft = 1;
  this->FileDimensionality = 2;

  this->Quality = 95;
  this->Progressive = 1;
  this->WriteToMemory = 0;
  this->Result = 0;
  this->TempFP = 0;
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
  this->SetErrorCode(vtkErrorCode::NoError);
  
  // Error checking
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
    }
  if (!this->WriteToMemory && ! this->FileName && !this->FilePattern)
    {
    vtkErrorMacro(<<"Write:Please specify either a FileName or a file prefix and pattern");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
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
  this->MinimumFileNumber = this->MaximumFileNumber = this->FileNumber;
  this->FilesDeleted = 0;
  this->UpdateProgress(0.0);
  // loop over the z axis and write the slices
  for (this->FileNumber = wExtent[4]; this->FileNumber <= wExtent[5]; 
       ++this->FileNumber)
    {
    this->MaximumFileNumber = this->FileNumber;
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
    this->GetInput()->Update();
    this->WriteSlice(this->GetInput());
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
      vtkErrorMacro("Ran out of disk space; deleting file(s) already written");
      this->DeleteFiles();
      return;
      }
    this->UpdateProgress((this->FileNumber - wExtent[4])/
                         (wExtent[5] - wExtent[4] + 1.0));
    }
  delete [] this->InternalFileName;
  this->InternalFileName = NULL;
}

// these three routines are for wqriting into memory
extern "C"
{
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
}

extern "C"
{
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
}

extern "C"
{
  void vtkJPEGWriteToMemoryTerm(j_compress_ptr cinfo)
  {
    vtkJPEGWriter *self = vtkJPEGWriter::SafeDownCast(
      static_cast<vtkObject *>(cinfo->client_data));
    if (self)
      {
      vtkUnsignedCharArray *uc = self->GetResult();
      // we must close the array
      vtkIdType oldSize = uc->GetSize();
      uc->SetNumberOfTuples(oldSize - static_cast<vtkIdType>(cinfo->dest->free_in_buffer));
      }
  }
}

#if defined ( _MSC_VER )
#if defined ( _WIN64 )
#pragma warning ( disable : 4324 ) // structure was padded at end...
#endif
#endif

struct VTK_JPEG_ERROR_MANAGER
{
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct VTK_JPEG_ERROR_MANAGER* VTK_JPEG_ERROR_PTR;

extern "C"
{
  /* The JPEG library does not expect the error function to return.
     Therefore we must use this ugly longjmp call.  */
  void
  VTK_JPEG_ERROR_EXIT (j_common_ptr cinfo)
{
  VTK_JPEG_ERROR_PTR jpegErr = reinterpret_cast<VTK_JPEG_ERROR_PTR>(cinfo->err);
  longjmp(jpegErr->setjmp_buffer, 1);
}
}


// we disable this warning because even though this is a C++ file, between
// the setjmp and resulting longjmp there should not be any C++ constructors
// or destructors.
#if defined(_MSC_VER) && !defined(VTK_DISPLAY_WIN32_WARNINGS)
#pragma warning ( disable : 4611 )
#endif
void vtkJPEGWriter::WriteSlice(vtkImageData *data)
{
  // Call the correct templated function for the output
  unsigned int ui;

  // Call the correct templated function for the input
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

  // overriding jpeg_error_mgr so we don't exit when an error happens
  
  // Create the jpeg compression object and error handler
  struct jpeg_compress_struct cinfo;
  struct VTK_JPEG_ERROR_MANAGER jerr;
  this->TempFP = 0;
  if (!this->WriteToMemory)
    {
    this->TempFP = fopen(this->InternalFileName, "wb");
    if (!this->TempFP)
      {
      vtkErrorMacro("Unable to open file " << this->InternalFileName);
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      return;
      }
    }

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = VTK_JPEG_ERROR_EXIT;
  if (setjmp(jerr.setjmp_buffer))
    {
    jpeg_destroy_compress(&cinfo);
    if (!this->WriteToMemory)
      {
      fclose(this->TempFP);
      }
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    return;
    }
  
  jpeg_create_compress(&cinfo);

  // set the destination file
  struct jpeg_destination_mgr compressionDestination;
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
    jpeg_stdio_dest(&cinfo, this->TempFP);
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
  void *outPtr;
  outPtr = data->GetScalarPointer(uExtent[0], uExtent[2], uExtent[4]);
  JSAMPROW *row_pointers = new JSAMPROW [height];
  vtkIdType *outInc = data->GetIncrements();
  vtkIdType rowInc = outInc[1];
  for (ui = 0; ui < height; ui++)
    {
    row_pointers[height - ui - 1] = (JSAMPROW) outPtr;
    outPtr = (unsigned char *)outPtr + rowInc;
    }
  jpeg_write_scanlines(&cinfo, row_pointers, height);
  
  if (!this->WriteToMemory)
    {
    if (fflush(this->TempFP) == EOF)
      {
      this->ErrorCode = vtkErrorCode::OutOfDiskSpaceError;
      fclose(this->TempFP);
      return;
      }
    }
  
  // finish the compression
  jpeg_finish_compress(&cinfo);
  
  // clean up and close the file
  delete [] row_pointers;
  jpeg_destroy_compress(&cinfo);
  
  if (!this->WriteToMemory)
    {
    fclose(this->TempFP);
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
