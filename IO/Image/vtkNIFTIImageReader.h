/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIFTIImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNIFTIImageReader - Read NIfTI-1 and NIfTI-2 medical image files
// .SECTION Description
// This class reads NIFTI files, either in .nii format or as separate
// .img and .hdr files.  If two files are used, then they can be passed
// by using SetFileNames() instead of SetFileName().  Files ending in .gz
// are decompressed on-the-fly while they are being read.  Files with
// complex numbers or vector dimensions will be read as multi-component
// images.  If a NIFTI file has a time dimension, then by default only the
// first image in the time series will be read, but the TimeAsVector
// flag can be set to read the time steps as vector components.  Files in
// Analyze 7.5 format are also supported by this reader.
// .SECTION Thanks
// This class was contributed to VTK by the Calgary Image Processing and
// Analysis Centre (CIPAC).
// .SECTION See Also
// vtkNIFTIImageWriter, vtkNIFTIImageHeader

#ifndef __vtkNIFTIImageReader_h
#define __vtkNIFTIImageReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class vtkNIFTIImageHeader;
class vtkMatrix4x4;

struct nifti_1_header;

//----------------------------------------------------------------------------
class VTKIOIMAGE_EXPORT vtkNIFTIImageReader : public vtkImageReader2
{
public:
  // Description:
  // Static method for construction.
  static vtkNIFTIImageReader *New();
  vtkTypeMacro(vtkNIFTIImageReader, vtkImageReader2);

  // Description:
  // Print information about this object.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Valid extensions for this file type.
  virtual const char* GetFileExtensions() {
    return ".nii .nii.gz .img .img.gz .hdr .hdr.gz"; }

  // Description:
  // Return a descriptive name that might be useful in a GUI.
  virtual const char* GetDescriptiveName() {
    return "NIfTI"; }

  // Description:
  // Return true if this reader can read the given file.
  int CanReadFile(const char* filename);

  // Description:
  // Read the time dimension as scalar components (default: Off).
  // If this is on, then each time point will be stored as a component in
  // the image data.  If the file has both a time dimension and a vector
  // dimension, then the number of components will be the product of these
  // two dimensions, i.e. the components will store a sequence of vectors.
  vtkGetMacro(TimeAsVector, bool);
  vtkSetMacro(TimeAsVector, bool);
  vtkBooleanMacro(TimeAsVector, bool);

  // Description:
  // Get the time dimension that was stored in the NIFTI header.
  int GetTimeDimension() { return this->Dim[4]; }
  double GetTimeSpacing() { return this->PixDim[4]; }

  // Description:
  // Get the slope and intercept for rescaling the scalar values.
  // These values allow calibration of the data to real values.
  // Use the equation v = u*RescaleSlope + RescaleIntercept.
  // This directly returns the values stored in the scl_slope and
  // scl_inter fields in the NIFTI header.
  double GetRescaleSlope() { return this->RescaleSlope; }
  double GetRescaleIntercept() { return this->RescaleIntercept; }

  // Description:
  // QFac gives the slice order in the NIFTI file versus the VTK image.
  // If QFac is -1, then the VTK slice index J is related to the NIFTI
  // slice index j by the equation J = (num_slices - j - 1).  VTK requires
  // the slices to be ordered so that the voxel indices (I,J,K) provide a
  // right-handed coordinate system, whereas NIFTI does not.  Instead,
  // NIFTI stores a factor called "qfac" in the header to signal when the
  // (i,j,k) indices form a left-handed coordinate system.  QFac will only
  // ever have values of +1 or -1.
  double GetQFac() { return this->QFac; }

  // Description:
  // Get a matrix that gives the "qform" orientation and offset for the data.
  // If no qform matrix was stored in the file, the return value is NULL.
  // This matrix will transform VTK data coordinates into the NIFTI oriented
  // data coordinates, where +X points right, +Y points anterior (toward the
  // front), and +Z points superior (toward the head). The qform matrix will
  // always have a positive determinant. The offset that is stored in the
  // matrix gives the position of the first pixel in the first slice of the
  // VTK image data.  Note that if QFac is -1, then the first slice in the
  // VTK image data is the last slice in the NIFTI file, and the Z offset
  // will automatically be adjusted to compensate for this.
  vtkMatrix4x4 *GetQFormMatrix() { return this->QFormMatrix; }

  // Description:
  // Get a matrix that gives the "sform" orientation and offset for the data.
  // If no sform matrix was stored in the file, the return value is NULL.
  // Like the qform matrix, this matrix will transform VTK data coordinates
  // into a NIFTI coordinate system.  Unlike the qform matrix, the sform
  // matrix can contain scaling information and can even (rarely) have
  // a negative determinant, i.e. a flip.  This matrix is modified slightly
  // as compared to the sform matrix stored in the NIFTI header: the pixdim
  // pixel spacing is factored out.  Also, if QFac is -1, then the VTK slices
  // are in reverse order as compared to the NIFTI slices, hence as compared
  // to the sform matrix stored in the header, the third column of this matrix
  // is multiplied by -1 and the Z offset is shifted to compensate for the
  // fact that the last slice has become the first.
  vtkMatrix4x4 *GetSFormMatrix() { return this->SFormMatrix; }

  // Description:
  // Get the raw header information from the NIfTI file.
  vtkNIFTIImageHeader *GetNIFTIHeader();

protected:
  vtkNIFTIImageReader();
  ~vtkNIFTIImageReader();

  // Description:
  // Read the header information.
  virtual int RequestInformation(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Read the voxel data.
  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Do a case-insensitive check for the given extension.
  // The check will succeed if the filename ends in ".gz", and if the
  // extension matches after removing the ".gz".
  static bool CheckExtension(const char *fname, const char *ext);

  // Description:
  // Make a new filename by replacing extension "ext1" with "ext2".
  // The extensions must include a period, must be three characters
  // long, and must be lower case.  This method also verifies that
  // the file exists, and adds or subtracts a ".gz" as necessary
  // If the file exists, a new string is returned that must be
  // deleted by the caller.  Otherwise, the return value is NULL.
  static char *ReplaceExtension(
    const char *fname, const char *ext1, const char *ext2);

  // Description:
  // Check the version of the header.
  static int CheckNIFTIVersion(const nifti_1_header *hdr);

  // Description:
  // Return true if an Analyze 7.5 header was found.
  static bool CheckAnalyzeHeader(const nifti_1_header *hdr);

  // Description:
  // Read the time dimension as if it was a vector dimension.
  bool TimeAsVector;

  // Description:
  // Information for rescaling data to quantitative units.
  double RescaleIntercept;
  double RescaleSlope;

  // Description:
  // Is -1 if VTK slice order is opposite to NIFTI slice order, +1 otherwise.
  double QFac;

  // Description:
  // The orientation matrices for the NIFTI file.
  vtkMatrix4x4 *QFormMatrix;
  vtkMatrix4x4 *SFormMatrix;

  // Description:
  // The dimensions of the NIFTI file.
  int Dim[8];

  // Description:
  // The spacings in the NIFTI file.
  double PixDim[8];

  // Description:
  // A copy of the header from the file that was most recently read.
  vtkNIFTIImageHeader *NIFTIHeader;

private:
  vtkNIFTIImageReader(const vtkNIFTIImageReader&);  // Not implemented.
  void operator=(const vtkNIFTIImageReader&);  // Not implemented.
};

#endif // __vtkNIFTIImageReader_h
