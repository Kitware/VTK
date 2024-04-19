// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDICOMImageReader
 * @brief   Reads some DICOM images
 *
 * DICOM (stands for Digital Imaging in COmmunications and Medicine)
 * is a medical image file format widely used to exchange data, provided
 * by various modalities.
 * @warning
 * This reader might eventually handle ACR-NEMA file (predecessor of the DICOM
 * format for medical images).
 * This reader does not handle encapsulated format, only plain raw file are
 * handled. This reader also does not handle multi-frames DICOM datasets.
 * @warning
 * Internally DICOMParser assumes the x,y pixel spacing is stored in 0028,0030 and
 * that z spacing is stored in Slice Thickness (correct only when slice were acquired
 * contiguous): 0018,0050. Which means this is only valid for some rare
 * MR Image Storage
 *
 * @sa
 * vtkBMPReader vtkPNMReader vtkTIFFReader
 */

#ifndef vtkDICOMImageReader_h
#define vtkDICOMImageReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDICOMImageReaderVector;
class DICOMParser;
class DICOMAppHelper;

class VTKIOIMAGE_EXPORT vtkDICOMImageReader : public vtkImageReader2
{
public:
  ///@{
  /**
   * Static method for construction.
   */
  static vtkDICOMImageReader* New();
  vtkTypeMacro(vtkDICOMImageReader, vtkImageReader2);
  ///@}

  /**
   * Prints the ivars.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the filename for the file to read. If this method is used,
   * the reader will only read a single file.
   */
  void SetFileName(VTK_FILEPATH const char* fn) override
  {
    delete[] this->DirectoryName;
    delete[] this->FileName;
    this->DirectoryName = nullptr;
    this->FileName = nullptr;
    this->vtkImageReader2::SetFileName(fn);
  }
  ///@}

  /**
   * Set the directory name for the reader to look in for DICOM
   * files. If this method is used, the reader will try to find
   * all the DICOM files in a directory. It will select the subset
   * corresponding to the first series UID it stumbles across and
   * it will try to build an ordered volume from them based on
   * the slice number. The volume building will be upgraded to
   * something more sophisticated in the future.
   */
  void SetDirectoryName(VTK_FILEPATH const char* dn);

  ///@{
  /**
   * Returns the directory name.
   */
  vtkGetFilePathMacro(DirectoryName);
  ///@}

  /**
   * Returns the pixel spacing (in X, Y, Z).
   * Note: if there is only one slice, the Z spacing is set to the slice
   * thickness. If there is more than one slice, it is set to the distance
   * between the first two slices.
   */
  double* GetPixelSpacing() VTK_SIZEHINT(3);

  /**
   * Returns the image width.
   */
  int GetWidth();

  /**
   * Returns the image height.
   */
  int GetHeight();

  /**
   * Get the (DICOM) x,y,z coordinates of the first pixel in the
   * image (upper left hand corner) of the last image processed by the
   * DICOMParser
   */
  float* GetImagePositionPatient() VTK_SIZEHINT(3);

  /**
   * Get the (DICOM) directions cosines. It consist of the components
   * of the first two vectors. The third vector needs to be computed
   * to form an orthonormal basis.
   */
  float* GetImageOrientationPatient() VTK_SIZEHINT(6);

  /**
   * Get the number of bits allocated for each pixel in the file.
   */
  int GetBitsAllocated();

  /**
   * Get the pixel representation of the last image processed by the
   * DICOMParser. A zero is a unsigned quantity.  A one indicates a
   * signed quantity
   */
  int GetPixelRepresentation();

  /**
   * Get the number of components of the image data for the last
   * image processed.
   */
  int GetNumberOfComponents();

  /**
   * Get the transfer syntax UID for the last image processed.
   */
  const char* GetTransferSyntaxUID();

  /**
   * Get the rescale slope for the pixel data.
   */
  float GetRescaleSlope();

  /**
   * Get the rescale offset for the pixel data.
   */
  float GetRescaleOffset();

  /**
   * Get the patient name for the last image processed.
   */
  const char* GetPatientName();

  /**
   * Get the study uid for the last image processed.
   */
  const char* GetStudyUID();

  /**
   * Get the Study ID for the last image processed.
   */
  const char* GetStudyID();

  /**
   * Get the gantry angle for the last image processed.
   */
  float GetGantryAngle();

  //
  // Can I read the file?
  //
  int CanReadFile(VTK_FILEPATH const char* fname) override;

  //
  // What file extensions are supported?
  //
  const char* GetFileExtensions() override { return ".dcm"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "DICOM"; }

protected:
  //
  // Setup the volume size
  //
  void SetupOutputInformation(int num_slices);

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

  //
  // Constructor
  //
  vtkDICOMImageReader();

  //
  // Destructor
  //
  ~vtkDICOMImageReader() override;

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
  VTK_FILEPATH const char* GetDICOMFileName(int index);

private:
  vtkDICOMImageReader(const vtkDICOMImageReader&) = delete;
  void operator=(const vtkDICOMImageReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
