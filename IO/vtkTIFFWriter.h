/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTIFFWriter.h
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
// .NAME vtkTIFFWriter - write out structured points as a TIFF file
// .SECTION Description
// vtkTIFFWriter writes structured points as a non-compressed TIFF data file.

#ifndef __vtkTIFFWriter_h
#define __vtkTIFFWriter_h

#include "vtkImageWriter.h"

class VTK_IO_EXPORT vtkTIFFWriter : public vtkImageWriter
{
public:
  static vtkTIFFWriter *New();
  vtkTypeRevisionMacro(vtkTIFFWriter,vtkImageWriter);
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
  void SetCompressionToNoCompression() { this->SetCompression(NoCompression); }
  void SetCompressionToPackBits()      { this->SetCompression(PackBits); }
  void SetCompressionToJPEG()          { this->SetCompression(JPEG); }
  void SetCompressionToDeflate()       { this->SetCompression(Deflate); }
  void SetCompressionToLZW()           { this->SetCompression(LZW); }

protected:
  vtkTIFFWriter();
  ~vtkTIFFWriter() {};

  virtual void WriteFile(ofstream *file, vtkImageData *data, 
                         int ext[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);

  void* TIFFPtr;

  int Compression;

private:
  vtkTIFFWriter(const vtkTIFFWriter&);  // Not implemented.
  void operator=(const vtkTIFFWriter&);  // Not implemented.
};

#endif

