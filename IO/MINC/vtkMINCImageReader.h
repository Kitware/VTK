// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMINCImageReader
 * @brief   A reader for MINC files.
 *
 * MINC is a NetCDF-based medical image file format that was developed
 * at the Montreal Neurological Institute in 1992.
 * This class will read a MINC file into VTK, rearranging the data to
 * match the VTK x, y, and z dimensions, and optionally rescaling
 * real-valued data to VTK_FLOAT if RescaleRealValuesOn() is set.
 * If RescaleRealValues is off, then the data will be stored in its
 * original data type and the GetRescaleSlope(), GetRescaleIntercept()
 * method can be used to retrieve global rescaling parameters.
 * If the original file had a time dimension, the SetTimeStep() method
 * can be used to specify a time step to read.
 * All of the original header information can be accessed though the
 * GetImageAttributes() method.
 * @sa
 * vtkMINCImageWriter vtkMINCImageAttributes
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
 */

#ifndef vtkMINCImageReader_h
#define vtkMINCImageReader_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkImageReader2.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageAttributes;

class VTKIOMINC_EXPORT vtkMINCImageReader : public vtkImageReader2
{
public:
  vtkTypeMacro(vtkMINCImageReader, vtkImageReader2);

  static vtkMINCImageReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the file name.
   */
  void SetFileName(VTK_FILEPATH const char* name) override;

  /**
   * Get the extension for this file format.
   */
  const char* GetFileExtensions() override { return ".mnc"; }

  /**
   * Get the name of this file format.
   */
  const char* GetDescriptiveName() override { return "MINC"; }

  /**
   * Test whether the specified file can be read.
   */
  int CanReadFile(VTK_FILEPATH const char* name) override;

  /**
   * Get a matrix that describes the orientation of the data.
   * The three columns of the matrix are the direction cosines
   * for the x, y and z dimensions respectively.
   */
  virtual vtkMatrix4x4* GetDirectionCosines();

  ///@{
  /**
   * Get the slope and intercept for rescaling the scalar values
   * to real data values.  To convert scalar values to real values,
   * use the equation y = x*RescaleSlope + RescaleIntercept.
   */
  virtual double GetRescaleSlope();
  virtual double GetRescaleIntercept();
  ///@}

  ///@{
  /**
   * Rescale real data values to float.  If this is done, the
   * RescaleSlope and RescaleIntercept will be set to 1 and 0
   * respectively.  This is off by default.
   */
  vtkSetMacro(RescaleRealValues, vtkTypeBool);
  vtkBooleanMacro(RescaleRealValues, vtkTypeBool);
  vtkGetMacro(RescaleRealValues, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the scalar range of the output from the information in
   * the file header.  This is more efficient that computing the
   * scalar range, but in some cases the MINC file stores an
   * incorrect valid_range and the DataRange will be incorrect.
   */
  virtual double* GetDataRange() VTK_SIZEHINT(2);
  virtual void GetDataRange(double range[2])
  {
    double* r = this->GetDataRange();
    range[0] = r[0];
    range[1] = r[1];
  }
  ///@}

  /**
   * Get the number of time steps in the file.
   */
  virtual int GetNumberOfTimeSteps();

  ///@{
  /**
   * Set the time step to read.
   */
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);
  ///@}

  /**
   * Get the image attributes, which contain patient information and
   * other useful metadata.
   */
  virtual vtkMINCImageAttributes* GetImageAttributes();

protected:
  vtkMINCImageReader();
  ~vtkMINCImageReader() override;

  int MINCImageType;
  int MINCImageTypeSigned;

  double ValidRange[2];
  double ImageRange[2];
  double DataRange[2];

  int NumberOfTimeSteps;
  int TimeStep;
  vtkMatrix4x4* DirectionCosines;
  double RescaleSlope;
  double RescaleIntercept;
  vtkTypeBool RescaleRealValues;
  vtkMINCImageAttributes* ImageAttributes;

  int FileNameHasChanged;

  virtual int OpenNetCDFFile(const char* filename, int& ncid);
  virtual int CloseNetCDFFile(int ncid);
  virtual int IndexFromDimensionName(const char* dimName);
  virtual int ReadMINCFileAttributes();
  virtual void FindRangeAndRescaleValues();
  static int ConvertMINCTypeToVTKType(int minctype, int mincsigned);

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  vtkMINCImageReader(const vtkMINCImageReader&) = delete;
  void operator=(const vtkMINCImageReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
