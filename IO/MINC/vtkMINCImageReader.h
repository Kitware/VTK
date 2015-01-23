/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
// .NAME vtkMINCImageReader - A reader for MINC files.
// .SECTION Description
// MINC is a NetCDF-based medical image file format that was developed
// at the Montreal Neurological Institute in 1992.
// This class will read a MINC file into VTK, rearranging the data to
// match the VTK x, y, and z dimensions, and optionally rescaling
// real-valued data to VTK_FLOAT if RescaleRealValuesOn() is set.
// If RescaleRealValues is off, then the data will be stored in its
// original data type and the GetRescaleSlope(), GetRescaleIntercept()
// method can be used to retrieve global rescaling parameters.
// If the original file had a time dimension, the SetTimeStep() method
// can be used to specify a time step to read.
// All of the original header information can be accessed though the
// GetImageAttributes() method.
// .SECTION See Also
// vtkMINCImageWriter vtkMINCImageAttributes
// .SECTION Thanks
// Thanks to David Gobbi for writing this class and Atamai Inc. for
// contributing it to VTK.

#ifndef vtkMINCImageReader_h
#define vtkMINCImageReader_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkImageReader2.h"

class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageAttributes;

class VTKIOMINC_EXPORT vtkMINCImageReader : public vtkImageReader2
{
public:
  vtkTypeMacro(vtkMINCImageReader,vtkImageReader2);

  static vtkMINCImageReader *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file name.
  virtual void SetFileName(const char *name);

  // Description:
  // Get the entension for this file format.
  virtual const char* GetFileExtensions() {
    return ".mnc"; }

  // Description:
  // Get the name of this file format.
  virtual const char* GetDescriptiveName() {
    return "MINC"; }

  // Description:
  // Test whether the specified file can be read.
  virtual int CanReadFile(const char* name);

  // Description:
  // Get a matrix that describes the orientation of the data.
  // The three columns of the matrix are the direction cosines
  // for the x, y and z dimensions respectively.
  virtual vtkMatrix4x4 *GetDirectionCosines();

  // Description:
  // Get the slope and intercept for rescaling the scalar values
  // to real data values.  To convert scalar values to real values,
  // use the equation y = x*RescaleSlope + RescaleIntercept.
  virtual double GetRescaleSlope();
  virtual double GetRescaleIntercept();

  // Description:
  // Rescale real data values to float.  If this is done, the
  // RescaleSlope and RescaleIntercept will be set to 1 and 0
  // respectively.  This is off by default.
  vtkSetMacro(RescaleRealValues, int);
  vtkBooleanMacro(RescaleRealValues, int);
  vtkGetMacro(RescaleRealValues, int);

  // Description:
  // Get the scalar range of the output from the information in
  // the file header.  This is more efficient that computing the
  // scalar range, but in some cases the MINC file stores an
  // incorrect valid_range and the DataRange will be incorrect.
  virtual double *GetDataRange();
  virtual void GetDataRange(double range[2]) {
    double *r = this->GetDataRange();
    range[0] = r[0]; range[1] = r[1]; };

  // Description:
  // Get the number of time steps in the file.
  virtual int GetNumberOfTimeSteps();

  // Description:
  // Set the time step to read.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the image attributes, which contain patient information and
  // other useful metadata.
  virtual vtkMINCImageAttributes *GetImageAttributes();

protected:
  vtkMINCImageReader();
  ~vtkMINCImageReader();

  int MINCImageType;
  int MINCImageTypeSigned;

  double ValidRange[2];
  double ImageRange[2];
  double DataRange[2];

  int NumberOfTimeSteps;
  int TimeStep;
  vtkMatrix4x4 *DirectionCosines;
  double RescaleSlope;
  double RescaleIntercept;
  int RescaleRealValues;
  vtkMINCImageAttributes *ImageAttributes;

  int FileNameHasChanged;

  virtual int OpenNetCDFFile(const char *filename, int& ncid);
  virtual int CloseNetCDFFile(int ncid);
  virtual int IndexFromDimensionName(const char *dimName);
  virtual int ReadMINCFileAttributes();
  virtual void FindRangeAndRescaleValues();
  static int ConvertMINCTypeToVTKType(int minctype, int mincsigned);

  virtual void ExecuteInformation();
  virtual void ExecuteDataWithInformation(vtkDataObject *out, vtkInformation *outInfo);

private:
  vtkMINCImageReader(const vtkMINCImageReader&); // Not implemented
  void operator=(const vtkMINCImageReader&);  // Not implemented

};

#endif
