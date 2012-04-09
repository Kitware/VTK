/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMetaImageWriter - write a binary UNC meta image data
// .SECTION Description
// One of the formats for which a reader is already available in the toolkit is
// the MetaImage file format. This is a fairly simple yet powerful format
// consisting of a text header and a binary data section. The following
// instructions describe how you can write a MetaImage header for the data that
// you download from the BrainWeb page.
//
// The minimal structure of the MetaImage header is the following:
//
//    NDims = 3
//    DimSize = 181 217 181
//    ElementType = MET_UCHAR
//    ElementSpacing = 1.0 1.0 1.0
//    ElementByteOrderMSB = False
//    ElementDataFile = brainweb1.raw
//
//    * NDims indicate that this is a 3D image. ITK can handle images of
//      arbitrary dimension.
//    * DimSize indicates the size of the volume in pixels along each
//      direction.
//    * ElementType indicate the primitive type used for pixels. In this case
//      is "unsigned char", implying that the data is digitized in 8 bits /
//      pixel.
//    * ElementSpacing indicates the physical separation between the center of
//      one pixel and the center of the next pixel along each direction in space.
//      The units used are millimeters.
//    * ElementByteOrderMSB indicates is the data is encoded in little or big
//      endian order. You might want to play with this value when moving data
//      between different computer platforms.
//    * ElementDataFile is the name of the file containing the raw binary data
//      of the image. This file must be in the same directory as the header.
//
// MetaImage headers are expected to have extension: ".mha" or ".mhd"
//
// Once you write this header text file, it should be possible to read the
// image into your ITK based application using the itk::FileIOToImageFilter
// class.

// .SECTION Caveats
//

// .SECTION See Also
// vtkImageWriter vtkMetaImageReader

#ifndef __vtkMetaImageWriter_h
#define __vtkMetaImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

//BTX
namespace vtkmetaio { class MetaImage; } // forward declaration
//ETX

class VTKIOIMAGE_EXPORT vtkMetaImageWriter : public vtkImageWriter
{
public:
  vtkTypeMacro(vtkMetaImageWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with FlipNormals turned off and Normals set to true.
  static vtkMetaImageWriter *New();

  // Description:
  // Specify file name of meta file
  virtual void SetFileName(const char* fname);
  virtual char* GetFileName() { return this->MHDFileName; }

  // Description:
  // Specify the file name of the raw image data.
  virtual void SetRAWFileName(const char* fname);
  virtual char* GetRAWFileName();

  virtual void SetCompression( bool compress )
    {
    this->Compress = compress;
    }
  virtual bool GetCompression( void )
    {
    return this->Compress;
    }

  // This is called by the superclass.
  // This is the method you should override.
  virtual void Write();

protected:
  vtkMetaImageWriter();
  ~vtkMetaImageWriter();

  vtkSetStringMacro(MHDFileName);
  char* MHDFileName;
  bool Compress;

private:
  vtkMetaImageWriter(const vtkMetaImageWriter&);  // Not implemented.
  void operator=(const vtkMetaImageWriter&);  // Not implemented.

//BTX
  vtkmetaio::MetaImage * MetaImagePtr;
//ETX

};

#endif



