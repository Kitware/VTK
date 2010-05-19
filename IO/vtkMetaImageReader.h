/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMetaImageReader - read binary UNC meta image data
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

#ifndef __vtkMetaImageReader2_h
#define __vtkMetaImageReader2_h

#include "vtkImageReader2.h"

//BTX
namespace vtkmetaio { class MetaImage; } // forward declaration  
//ETX

class VTK_IO_EXPORT vtkMetaImageReader : public vtkImageReader2
{
public:
  vtkTypeMacro(vtkMetaImageReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with FlipNormals turned off and Normals set to true.
  static vtkMetaImageReader *New();

  virtual const char * GetFileExtensions() 
    { return ".mhd .mha"; }

  virtual const char * GetDescriptiveName() 
    { return "MetaIO Library: MetaImage"; }

  // These duplicate functions in vtkImageReader2, vtkMedicalImageReader.
  double * GetPixelSpacing()
    { return this->GetDataSpacing(); }
  int GetWidth()
    { return (this->GetDataExtent()[1] - this->GetDataExtent()[0] + 1); }
  int GetHeight()
    { return (this->GetDataExtent()[3] - this->GetDataExtent()[2] + 1); }
  double * GetImagePositionPatient()
    { return this->GetDataOrigin(); }
  int GetNumberOfComponents()
    { return this->GetNumberOfScalarComponents(); }
  int GetPixelRepresentation()
    { return this->GetDataScalarType(); }
  int GetDataByteOrder(void);

  vtkGetMacro(RescaleSlope, double);
  vtkGetMacro(RescaleOffset, double);
  vtkGetMacro(BitsAllocated, int);
  vtkGetStringMacro(DistanceUnits);
  vtkGetStringMacro(AnatomicalOrientation);
  vtkGetMacro(GantryAngle, double); 
  vtkGetStringMacro(PatientName);
  vtkGetStringMacro(PatientID);
  vtkGetStringMacro(Date);
  vtkGetStringMacro(Series);
  vtkGetStringMacro(ImageNumber);
  vtkGetStringMacro(Modality);
  vtkGetStringMacro(StudyID);
  vtkGetStringMacro(StudyUID);
  vtkGetStringMacro(TransferSyntaxUID);

  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);

protected:
  vtkMetaImageReader();
  ~vtkMetaImageReader();

  // These functions make no sense for this (or most) file readers
  // and should be hidden from the user...but then the getsettest fails.
  /*virtual void SetFilePrefix(const char * arg) 
    { vtkImageReader2::SetFilePrefix(arg); }
  virtual void SetFilePattern(const char * arg)
    { vtkImageReader2::SetFilePattern(arg); }
  virtual void SetDataScalarType(int type) 
    { vtkImageReader2::SetDataScalarType(type); }
  virtual void SetDataScalarTypeToFloat()
    { this->SetDataScalarType(VTK_FLOAT); }
  virtual void SetDataScalarTypeToDouble()
    { this->SetDataScalarType(VTK_DOUBLE); }
  virtual void SetDataScalarTypeToInt()
    { this->SetDataScalarType(VTK_INT); }
  virtual void SetDataScalarTypeToShort()
    { this->SetDataScalarType(VTK_SHORT); }
  virtual void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(VTK_UNSIGNED_SHORT);}
  virtual void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(VTK_UNSIGNED_CHAR);}
  vtkSetMacro(NumberOfScalarComponents, int);
  vtkSetVector6Macro(DataExtent, int);
  vtkSetMacro(FileDimensionality, int);
  vtkSetVector3Macro(DataSpacing, double);
  vtkSetVector3Macro(DataOrigin, double);
  vtkSetMacro(HeaderSize, unsigned long); 
  unsigned long GetHeaderSize(unsigned long) 
    { return 0; }
  virtual void SetDataByteOrderToBigEndian()
    { this->SetDataByteOrderToBigEndian(); }
  virtual void SetDataByteOrderToLittleEndian()
    { this->SetDataByteOrderToBigEndian(); }
  virtual void SetDataByteOrder(int order)
    { this->SetDataByteOrder(order); }
  vtkSetMacro(FileNameSliceOffset,int);
  vtkSetMacro(FileNameSliceSpacing,int);
  vtkSetMacro(SwapBytes, int);
  virtual int OpenFile() 
    { return vtkImageReader2::OpenFile(); }
  virtual void SeekFile(int i, int j, int k) 
    { vtkImageReader2::SeekFile(i, j, k); }
  vtkSetMacro(FileLowerLeft, int);
  virtual void ComputeInternalFileName(int slice) 
    { vtkImageReader2::ComputeInternalFileName(slice); }
  vtkGetStringMacro(InternalFileName)
  const char * GetDataByteOrderAsString(void) 
    { return vtkImageReader2::GetDataByteOrderAsString(); }
  unsigned long GetHeaderSize(void) 
    { return vtkImageReader2::GetHeaderSize(); }*/

  void ExecuteInformation();
  void ExecuteData(vtkDataObject *out);
  virtual int RequestInformation(vtkInformation * request,
                         vtkInformationVector ** inputVector,
                         vtkInformationVector * outputVector);

private:
  vtkMetaImageReader(const vtkMetaImageReader&);  // Not implemented.
  void operator=(const vtkMetaImageReader&);  // Not implemented.

//BTX
  vtkmetaio::MetaImage *MetaImagePtr;
//ETX

  double GantryAngle;
  char PatientName[255];
  char PatientID[255];
  char Date[255];
  char Series[255];
  char Study[255];
  char ImageNumber[255];
  char Modality[255];
  char StudyID[255];
  char StudyUID[255];
  char TransferSyntaxUID[255];

  double RescaleSlope;
  double RescaleOffset;
  int BitsAllocated;
  char DistanceUnits[255];
  char AnatomicalOrientation[255];
};

#endif



