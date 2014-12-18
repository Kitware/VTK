/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDICOMImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDICOMImageReader - Reads some DICOM images
// .SECTION Description
// DICOM (stands for Digital Imaging in COmmunications and Medicine)
// is a medical image file format widely used to exchange data, provided
// by various modalities.
// .SECTION Warnings
// This reader might eventually handle ACR-NEMA file (predecessor of the DICOM
// format for medical images).
// This reader does not handle encapsulated format, only plain raw file are
// handled. This reader also does not handle multi-frames DICOM datasets.
// .SECTION Warnings
// Internally DICOMParser assumes the x,y pixel spacing is stored in 0028,0030 and
// that z spacing is stored in Slice Thickness (correct only when slice were acquired
// contiguous): 0018,0050. Which means this is only valid for some rare
// MR Image Storage
//
// .SECTION See Also
// vtkBMPReader vtkPNMReader vtkTIFFReader

#ifndef vtkDICOMImageReader_h
#define vtkDICOMImageReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

//BTX
class vtkDICOMImageReaderVector;
class DICOMParser;
class DICOMAppHelper;
//ETX

class VTKIOIMAGE_EXPORT vtkDICOMImageReader : public vtkImageReader2
{
 public:
  // Description:
  // Static method for construction.
  static vtkDICOMImageReader *New();
  vtkTypeMacro(vtkDICOMImageReader,vtkImageReader2);

  // Description:
  // Prints the ivars.
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the filename for the file to read. If this method is used,
  // the reader will only read a single file.
  void SetFileName(const char* fn)
  {
    delete [] this->DirectoryName;
    delete [] this->FileName;
    this->DirectoryName = NULL;
    this->FileName = NULL;
    this->vtkImageReader2::SetFileName(fn);
  }

  // Description:
  // Set the directory name for the reader to look in for DICOM
  // files. If this method is used, the reader will try to find
  // all the DICOM files in a directory. It will select the subset
  // corresponding to the first series UID it stumbles across and
  // it will try to build an ordered volume from them based on
  // the slice number. The volume building will be upgraded to
  // something more sophisticated in the future.
  void SetDirectoryName(const char* dn);

  // Description:
  // Returns the directory name.
  vtkGetStringMacro(DirectoryName);

  // Description:
  // Returns the pixel spacing (in X, Y, Z).
  // Note: if there is only one slice, the Z spacing is set to the slice
  // thickness. If there is more than one slice, it is set to the distance
  // between the first two slices.
  double* GetPixelSpacing();

  // Description:
  // Returns the image width.
  int GetWidth();

  // Description:
  // Returns the image height.
  int GetHeight();

  // Description:
  // Get the (DICOM) x,y,z coordinates of the first pixel in the
  // image (upper left hand corner) of the last image processed by the
  // DICOMParser
  float* GetImagePositionPatient();

  // Description:
  // Get the (DICOM) directions cosines. It consist of the components
  // of the first two vectors. The third vector needs to be computed
  // to form an orthonormal basis.
  float* GetImageOrientationPatient();

  // Description:
  // Get the number of bits allocated for each pixel in the file.
  int GetBitsAllocated();

  // Description:
  // Get the pixel representation of the last image processed by the
  // DICOMParser. A zero is a unsigned quantity.  A one indicates a
  // signed quantity
  int GetPixelRepresentation();

  // Description:
  // Get the number of components of the image data for the last
  // image processed.
  int GetNumberOfComponents();

  // Description:
  // Get the transfer syntax UID for the last image processed.
  const char* GetTransferSyntaxUID();

  // Description:
  // Get the rescale slope for the pixel data.
  float GetRescaleSlope();

  // Description:
  // Get the rescale offset for the pixel data.
  float GetRescaleOffset();

  // Description:
  // Get the patient name for the last image processed.
  const char* GetPatientName();

  // Description:
  // Get the study uid for the last image processed.
  const char* GetStudyUID();

  // Description:
  // Get the Study ID for the last image processed.
  const char* GetStudyID();

  // Description:
  // Get the gantry angle for the last image processed.
  float GetGantryAngle();

  //
  // Can I read the file?
  //
  virtual int CanReadFile(const char* fname);

  //
  // What file extensions are supported?
  //
  virtual const char* GetFileExtensions()
  {
    return ".dcm";
  }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
  {
    return "DICOM";
  }

protected:
  //
  // Setup the volume size
  //
  void SetupOutputInformation(int num_slices);

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);

  //
  // Constructor
  //
  vtkDICOMImageReader();

  //
  // Destructor
  //
  virtual ~vtkDICOMImageReader();

  //
  // Instance of the parser used to parse the file.
  //
  DICOMParser* Parser;

  //
  // Instance of the callbacks that get the data from the file.
  //
  DICOMAppHelper* AppHelper;

  //
  // vtkDICOMImageReaderVector wants to be a PIMPL and it will be, but not quite yet.
  //
  vtkDICOMImageReaderVector* DICOMFileNames;
  char* DirectoryName;

  char* PatientName;
  char* StudyUID;
  char* StudyID;
  char* TransferSyntaxUID;

  // DICOMFileNames accessor methods for subclasses:
  int GetNumberOfDICOMFileNames();
  const char* GetDICOMFileName(int index);
private:
  vtkDICOMImageReader(const vtkDICOMImageReader&);  // Not implemented.
  void operator=(const vtkDICOMImageReader&);  // Not implemented.

};

#endif
