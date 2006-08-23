/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMINCImageReader.h

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
// MINC is a medical image file format that was developed at the Montreal
// Neurological Institute in 1992. It is based on the NetCDF format.
// .SECTION Thanks
// Thanks to David Gobbi for writing this class and Atamai Inc. for
// contributing it to VTK.

#ifndef __vtkMINCImageReader_h
#define __vtkMINCImageReader_h

#include "vtkImageReader2.h"

class vtkStringArray;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkMatrix4x4;

// A special class that holds the attributes
class vtkMINCImageReaderAttributeMap;

class VTK_IO_EXPORT vtkMINCImageReader : public vtkImageReader2
{
public:
  vtkTypeRevisionMacro(vtkMINCImageReader,vtkImageReader2);

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
  virtual vtkMatrix4x4 *GetOrientationMatrix();

  // Description:
  // Get the slope and intercept for rescaling the scalar values
  // to real data values.
  virtual double GetRescaleSlope();
  virtual double GetRescaleIntercept();

  // Description:
  // Get the ValidRange of the data as stored in the file.
  // The ScalarRange of the output data will be equal to this.
  virtual double *GetValidRange();
  virtual void GetValidRange(double range[2]) {
    double *r = this->GetValidRange();
    range[0] = r[0]; range[1] = r[1]; }; 

  // Description:
  // Get the number of frames in the file.  This is for
  // non-spatial dimensions like time.
  virtual int GetNumberOfFrames();

  // Description:
  // Set the frame to read.
  vtkSetMacro(FrameNumber, int);
  vtkGetMacro(FrameNumber, int);

  // Description:
  // Get the names of all of the dimensions.
  virtual vtkStringArray *GetDimensionNames();

  // Description:
  // Get the lengths of all the dimensions.
  virtual vtkIdTypeArray *GetDimensionLengths();

  // Description:
  // Get the names of all the variables.
  virtual vtkStringArray *GetVariableNames();

  // Description:
  // List the attribute names for a variable.  Set the variable
  // to the empty string to get global attributes.
  virtual vtkStringArray *GetAttributeNames(const char *variable);

  // Description:
  // Check to see if a particular attribute exists.
  virtual int HasAttribute(const char *variable, const char *attribute);

  // Description:
  // Get attribute values for a variable as a vtkDataArray.
  // Set the variable to the empty string to get global attributes.
  // A null pointer is returned if the attribute was not found.
  virtual vtkDataArray *GetAttributeValueAsArray(const char *variable,
                                                 const char *attribute);

  // Description:
  // Get an attribute value as a string.  Set the variable
  // to the empty string to get global attributes.  If the
  // specified attribute is not present, a null will be returned.
  virtual const char *GetAttributeValueAsString(const char *variable,
                                                const char *attribute);

  // Description:
  // Get an attribute value as an int.  Set the variable
  // to the empty string to get global attributes.  VTK
  // will report an error if the attribute doesn't exist
  // or if it is not an int.
  virtual int GetAttributeValueAsInt(const char *variable,
                                     const char *attribute);

  // Description:
  // Get an attribute value as a double.  Set the variable
  // to the empty string to get global attributes.  VTK
  // will report an error if the attribute doesn't exist
  // or if it is not a double or int.
  virtual double GetAttributeValueAsDouble(const char *variable,
                                           const char *attribute);

  // Description:
  // A diagnostic function.  Print the header of the file in 
  // the same format as ncdump or mincheader.
  virtual void PrintFileHeader();

protected:
  vtkMINCImageReader();
  ~vtkMINCImageReader();

  int MINCImageType;
  int MINCImageTypeSigned;
  double MINCValidRange[2];
  double MINCImageRange[2];
  int MINCImageMinMaxDims;
  vtkDoubleArray *MINCImageMin;
  vtkDoubleArray *MINCImageMax;

  int NumberOfFrames;
  int FrameNumber;
  vtkMatrix4x4 *OrientationMatrix;
  double RescaleSlope;
  double RescaleIntercept;

  vtkStringArray *DimensionNames;
  vtkIdTypeArray *DimensionLengths;
  vtkStringArray *VariableNames;
  vtkMINCImageReaderAttributeMap *AttributeNames;
  vtkMINCImageReaderAttributeMap *AttributeValues;
  vtkStringArray *StringStore;

  int FileNameHasChanged;

  virtual int OpenNetCDFFile(const char *filename, int& ncid);
  virtual int CloseNetCDFFile(int ncid);
  virtual int IndexFromDimensionName(const char *dimName);
  virtual void FindMINCValidRange();
  virtual void FindMINCImageRange();
  virtual int ReadMINCFileAttributes();
  const char *ConvertDataArrayToString(vtkDataArray *array);
  static int ConvertMINCTypeToVTKType(int minctype, int mincsigned);

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);

private:
  vtkMINCImageReader(const vtkMINCImageReader&); // Not implemented
  void operator=(const vtkMINCImageReader&);  // Not implemented

};

#endif
