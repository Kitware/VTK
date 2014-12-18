/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIFTIImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNIFTIImageWriter - Write NIfTI-1 and NIfTI-2 medical image files
// .SECTION Description
// This class writes NIFTI files, either in .nii format or as separate
// .img and .hdr files.  If told to write a file that ends in ".gz",
// then the writer will automatically compress the file with zlib.
// Images of type unsigned char that have 3 or 4 scalar components
// will automatically be written as RGB or RGBA respectively.  Images
// of type float or double that have 2 components will automatically be
// written as complex values.
// .SECTION Thanks
// This class was contributed to VTK by the Calgary Image Processing and
// Analysis Centre (CIPAC).
// .SECTION See Also
// vtkNIFTIImageReader

#ifndef vtkNIFTIImageWriter_h
#define vtkNIFTIImageWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

class vtkMatrix4x4;
class vtkNIFTIImageHeader;

class VTKIOIMAGE_EXPORT vtkNIFTIImageWriter : public vtkImageWriter
{
public:
  // Description:
  // Static method for construction.
  static vtkNIFTIImageWriter *New();
  vtkTypeMacro(vtkNIFTIImageWriter, vtkImageWriter);

  // Description:
  // Print information about this object.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the version number for the NIfTI file format to use.
  // This can be 1, 2, or 0 (the default).  If set to zero, then it
  // will save as NIfTI version 1 unless SetNIFTIHeader() provided
  // header information from a NIfTI version 2 file.
  vtkSetMacro(NIFTIVersion, int);
  vtkGetMacro(NIFTIVersion, int);

  // Description:
  // Set a short description (max 80 chars) of how the file was produced.
  // The default description is "VTKX.Y" where X.Y is the VTK version.
  vtkSetStringMacro(Description);
  vtkGetStringMacro(Description);

  // Description:
  // Set the time dimension to use in the NIFTI file (or zero if none).
  // The number of components of the input data must be divisible by the time
  // dimension if the time dimension is not set to zero.  The vector dimension
  // will be set to the number of components divided by the time dimension.
  vtkGetMacro(TimeDimension, int);
  vtkSetMacro(TimeDimension, int);
  vtkGetMacro(TimeSpacing, double);
  vtkSetMacro(TimeSpacing, double);

   // Description:
   // Set the slope and intercept for calibrating the scalar values.
   // Other programs that read the NIFTI file can use the equation
   // v = u*RescaleSlope + RescaleIntercept to rescale the data to
   // real values.  If both the slope and the intercept are zero,
   // then the SclSlope and SclIntercept in the header info provided
   // via SetNIFTIHeader() are used instead.
   vtkSetMacro(RescaleSlope, double);
   vtkGetMacro(RescaleSlope, double);
   vtkSetMacro(RescaleIntercept, double);
   vtkGetMacro(RescaleIntercept, double);

  // Description:
  // The QFac sets the ordering of the slices in the NIFTI file.
  // If QFac is -1, then the slice ordering in the file will be reversed
  // as compared to VTK. Use with caution.
  vtkSetMacro(QFac, double);
  vtkGetMacro(QFac, double);

  // Description:
  // Set the "qform" orientation and offset for the image data.
  // The 3x3 portion of the matrix must be orthonormal and have a
  // positive determinant, it will be used to compute the quaternion.
  // The last column of the matrix will be used for the offset.
  // In the NIFTI header, the qform_code will be set to 1.
  void SetQFormMatrix(vtkMatrix4x4 *);
  vtkMatrix4x4 *GetQFormMatrix() { return this->QFormMatrix; }

  // Description:
  // Set a matrix for the "sform" transformation stored in the file.
  // Unlike the qform matrix, the sform matrix can contain scaling
  // information.  Before being stored in the NIFTI header, the
  // first three columns of the matrix will be multipled by the voxel
  // spacing. In the NIFTI header, the sform_code will be set to 2.
  void SetSFormMatrix(vtkMatrix4x4 *);
  vtkMatrix4x4 *GetSFormMatrix() { return this->SFormMatrix; }

  // Description:
  // Set the NIFTI header information to use when writing the file.
  // The data dimensions and pixdim from the supplied header will be
  // ignored.  Likewise, the QForm and SForm information in the supplied
  // header will be ignored if you have called SetQFormMatrix() or
  // SetSFormMatrix() to provide the orientation information for the file.
  void SetNIFTIHeader(vtkNIFTIImageHeader *hdr);
  vtkNIFTIImageHeader *GetNIFTIHeader();

protected:
  vtkNIFTIImageWriter();
  ~vtkNIFTIImageWriter();

  // Description:
  // Generate the header information for the file.
  int GenerateHeader(vtkInformation *info, bool singleFile);

  // Description:
  // The main execution method, which writes the file.
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Make a new filename by replacing extension "ext1" with "ext2".
  // The extensions must include a period, must be three characters
  // long, and must be lower case.  A new string is returned that must
  // be deleted by the caller.
  static char *ReplaceExtension(
    const char *fname, const char *ext1, const char *ext2);

  // Description:
  // The size and spacing of the Time dimension to use in the file.
  int TimeDimension;
  double TimeSpacing;

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

  // Description
  // A description of how the file was produced.
  char *Description;

  // Description:
  // The header information.
  vtkNIFTIImageHeader *NIFTIHeader;
  vtkNIFTIImageHeader *OwnHeader;
  int NIFTIVersion;

private:
  vtkNIFTIImageWriter(const vtkNIFTIImageWriter&);  // Not implemented.
  void operator=(const vtkNIFTIImageWriter&);  // Not implemented.
};

#endif // vtkNIFTIImageWriter_h
