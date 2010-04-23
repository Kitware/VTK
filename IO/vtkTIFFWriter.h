/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTIFFWriter - write out image data as a TIFF file
// .SECTION Description
// vtkTIFFWriter writes image data as a TIFF data file. Data can be written
// uncompressed or compressed. Several forms of compression are supported
// including packed bits, JPEG, deflation, and LZW. (Note: LZW compression
// is currently under patent in the US and is disabled until the patent
// expires. However, the mechanism for supporting this compression is available
// for those with a valid license or to whom the patent does not apply.)

#ifndef __vtkTIFFWriter_h
#define __vtkTIFFWriter_h

#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkTIFFWriter : public vtkImageWriter
{
public:
  static vtkTIFFWriter *New();
  vtkTypeMacro(vtkTIFFWriter,vtkImageWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum { // Compression types
    NoCompression,
    PackBits,
    JPEG,
    Deflate,
    LZW
  };
//ETX

  // Description:
  // Set compression type. Sinze LZW compression is patented outside US, the
  // additional work steps have to be taken in order to use that compression.
  vtkSetClampMacro(Compression, int, NoCompression, LZW);
  vtkGetMacro(Compression, int);
  void SetCompressionToNoCompression() { this->SetCompression(NoCompression); }
  void SetCompressionToPackBits()      { this->SetCompression(PackBits); }
  void SetCompressionToJPEG()          { this->SetCompression(JPEG); }
  void SetCompressionToDeflate()       { this->SetCompression(Deflate); }
  void SetCompressionToLZW()           { this->SetCompression(LZW); }

protected:
  vtkTIFFWriter();
  ~vtkTIFFWriter() {}

  virtual void WriteFile(ofstream *file, vtkImageData *data, int ext[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);

  void* TIFFPtr;
  int Compression;

private:
  vtkTIFFWriter(const vtkTIFFWriter&);  // Not implemented.
  void operator=(const vtkTIFFWriter&);  // Not implemented.
};

#endif

