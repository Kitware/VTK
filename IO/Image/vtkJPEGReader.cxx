/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJPEGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJPEGReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkToolkits.h"

extern "C" {
#include "vtk_jpeg.h"
#if defined(__sgi) && !defined(__GNUC__)
#  if   (_COMPILER_VERSION >= 730)
#  pragma set woff 3505
#  endif
#endif
#include <setjmp.h>
}


vtkStandardNewMacro(vtkJPEGReader);


#if defined ( _MSC_VER )
#if defined ( _WIN64 )
#pragma warning ( disable : 4324 ) // structure was padded at end...
#endif
#endif

// create an error handler for jpeg that
// can longjmp out of the jpeg library
struct vtk_jpeg_error_mgr
{
  struct jpeg_error_mgr pub;    /* "public" fields */
  jmp_buf setjmp_buffer;        /* for return to caller */
  vtkJPEGReader* JPEGReader;
};

// this is called on jpeg error conditions
extern "C" void vtk_jpeg_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  vtk_jpeg_error_mgr * err = reinterpret_cast<vtk_jpeg_error_mgr*>(cinfo->err);

  /* Return control to the setjmp point */
  longjmp(err->setjmp_buffer, 1);
}

extern "C" void vtk_jpeg_output_message (j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);
  vtk_jpeg_error_mgr * err = reinterpret_cast<vtk_jpeg_error_mgr*>(cinfo->err);
  vtkWarningWithObjectMacro(err->JPEGReader,
                            "libjpeg error: " <<  buffer);
}

#ifdef _MSC_VER
// Let us get rid of this funny warning on /W4:
// warning C4611: interaction between '_setjmp' and C++ object
// destruction is non-portable
#pragma warning( disable : 4611 )
#endif

void vtkJPEGReader::ExecuteInformation()
{
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL)
    {
    return;
    }

  FILE *fp = fopen(this->InternalFileName, "rb");
  if (!fp)
    {
    vtkErrorWithObjectMacro(this,
                            "Unable to open file "
                            << this->InternalFileName);
    return;
    }

  // create jpeg decompression object and error handler
  struct jpeg_decompress_struct cinfo;
  struct vtk_jpeg_error_mgr jerr;
  jerr.JPEGReader = this;

  cinfo.err = jpeg_std_error(&jerr.pub);
  // for any jpeg error call vtk_jpeg_error_exit
  jerr.pub.error_exit = vtk_jpeg_error_exit;
  // for any output message call vtk_jpeg_output_message
  jerr.pub.output_message = vtk_jpeg_output_message;
  if (setjmp(jerr.setjmp_buffer))
    {
    // clean up
    jpeg_destroy_decompress(&cinfo);
    // close the file
    fclose(fp);
    // this is not a valid jpeg file
    vtkErrorWithObjectMacro(this, "libjpeg could not read file: "
                            << this->InternalFileName);
    return;
    }
  jpeg_create_decompress(&cinfo);

  // set the source file
  jpeg_stdio_src(&cinfo, fp);

  // read the header
  jpeg_read_header(&cinfo, TRUE);

  // force the output image size to be calculated (we could have used
  // cinfo.image_height etc. but that would preclude using libjpeg's
  // ability to scale an image on input).
  jpeg_calc_output_dimensions(&cinfo);

  // pull out the width/height, etc.
  this->DataExtent[0] = 0;
  this->DataExtent[1] = cinfo.output_width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = cinfo.output_height - 1;

  this->SetDataScalarTypeToUnsignedChar();
  this->SetNumberOfScalarComponents( cinfo.output_components );

  this->vtkImageReader2::ExecuteInformation();

  // close the file
  jpeg_destroy_decompress(&cinfo);
  fclose(fp);
}

template <class OT>
int vtkJPEGReaderUpdate2(vtkJPEGReader *self, OT *outPtr,
                          int *outExt, vtkIdType *outInc, long)
{
  unsigned int ui;
  int i;
  FILE *fp = fopen(self->GetInternalFileName(), "rb");
  if (!fp)
    {
    return 1;
    }

  // create jpeg decompression object and error handler
  struct jpeg_decompress_struct cinfo;
  struct vtk_jpeg_error_mgr jerr;
  jerr.JPEGReader = self;

  cinfo.err = jpeg_std_error(&jerr.pub);
  // for any jpeg error call vtk_jpeg_error_exit
  jerr.pub.error_exit = vtk_jpeg_error_exit;
  // for any output message call vtk_jpeg_output_message
  jerr.pub.output_message = vtk_jpeg_output_message;
  if (setjmp(jerr.setjmp_buffer))
    {
    // clean up
    jpeg_destroy_decompress(&cinfo);
    // close the file
    fclose(fp);

    // this is not a valid jpeg file
    return 2;
    }
  jpeg_create_decompress(&cinfo);

  // set the source file
  jpeg_stdio_src(&cinfo, fp);

  // read the header
  jpeg_read_header(&cinfo, TRUE);

  // prepare to read the bulk data
  jpeg_start_decompress(&cinfo);


  int rowbytes = cinfo.output_components * cinfo.output_width;
  unsigned char *tempImage = new unsigned char [rowbytes*cinfo.output_height];
  JSAMPROW *row_pointers = new JSAMPROW [cinfo.output_height];
  for (ui = 0; ui < cinfo.output_height; ++ui)
    {
    row_pointers[ui] = tempImage + rowbytes*ui;
    }

  // read the bulk data
  unsigned int remainingRows;
  while (cinfo.output_scanline < cinfo.output_height)
    {
    remainingRows = cinfo.output_height - cinfo.output_scanline;
    jpeg_read_scanlines(&cinfo, &row_pointers[cinfo.output_scanline],
                        remainingRows);
    }

  // finish the decompression step
  jpeg_finish_decompress(&cinfo);

  // destroy the decompression object
  jpeg_destroy_decompress(&cinfo);

  // copy the data into the outPtr
  OT *outPtr2;
  outPtr2 = outPtr;
  long outSize = cinfo.output_components*(outExt[1] - outExt[0] + 1);
  for (i = outExt[2]; i <= outExt[3]; ++i)
    {
    memcpy(outPtr2,
           row_pointers[cinfo.output_height - i - 1]
           + outExt[0]*cinfo.output_components,
           outSize);
    outPtr2 += outInc[1];
    }
  delete [] tempImage;
  delete [] row_pointers;

  // close the file
  fclose(fp);
  return 0;
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkJPEGReaderUpdate(vtkJPEGReader *self, vtkImageData *data, OT *outPtr)
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
    // read in a JPEG file
    if ( vtkJPEGReaderUpdate2(self, outPtr2, outExtent, outIncr, pixSize) == 2 )
      {
      const char* fn = self->GetInternalFileName();
      vtkErrorWithObjectMacro(self, "libjpeg could not read file: " << fn);
      }

    self->UpdateProgress((idx2 - outExtent[4])/
                         (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
    }
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkJPEGReader::ExecuteDataWithInformation(vtkDataObject *output,
                                               vtkInformation *outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  if (this->InternalFileName == NULL)
    {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
    }

  this->ComputeDataIncrements();

  data->GetPointData()->GetScalars()->SetName("JPEGImage");

  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
    {
    vtkTemplateMacro(vtkJPEGReaderUpdate(this, data, (VTK_TT *)(outPtr)));
    default:
      vtkErrorMacro(<< "UpdateFromFile: Unknown data type");
    }
}



int vtkJPEGReader::CanReadFile(const char* fname)
{
  // open the file
  FILE *fp = fopen(fname, "rb");
  if (!fp)
    {
    return 0;
    }
  // read the first two bytes
  char magic[2];
  int n = static_cast<int>(fread(magic, sizeof(magic), 1, fp));
  if (n != 1)
    {
    fclose(fp);
    return 0;
    }
  // check for the magic stuff:
  // 0xFF followed by 0xD8
  if( ( (static_cast<unsigned char>(magic[0]) != 0xFF) ||
        (static_cast<unsigned char>(magic[1]) != 0xD8) ) )
    {
    fclose(fp);
    return 0;
    }
  // go back to the start of the file
  fseek(fp, 0, SEEK_SET);
  // magic number is ok, try and read the header
  struct vtk_jpeg_error_mgr jerr;
  jerr.JPEGReader = this;
  struct jpeg_decompress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr.pub);
  // for any jpeg error call vtk_jpeg_error_exit
  jerr.pub.error_exit = vtk_jpeg_error_exit;
  // for any output message call vtk_jpeg_error_exit
  jerr.pub.output_message = vtk_jpeg_error_exit;
  // set the jump point, if there is a jpeg error or warning
  // this will evaluate to true
  if (setjmp(jerr.setjmp_buffer))
    {
    // clean up
    jpeg_destroy_decompress(&cinfo);
    // close the file
    fclose(fp);
    // this is not a valid jpeg file
    return 0;
    }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);
  /* Step 2: specify data source (eg, a file) */
  jpeg_stdio_src(&cinfo, fp);
  /* Step 3: read file parameters with jpeg_read_header() */
  jpeg_read_header(&cinfo, TRUE);

  // if no errors have occurred yet, then it must be jpeg
  jpeg_destroy_decompress(&cinfo);
  fclose(fp);
  return 3;
}
#ifdef _MSC_VER
// Put the warning back
#pragma warning( default : 4611 )
#endif


//----------------------------------------------------------------------------
void vtkJPEGReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
