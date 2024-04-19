// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNITransformReader
 * @brief   A reader for MNI transformation files.
 *
 * The MNI .xfm file format is used to store geometrical
 * transformations.  Three kinds of transformations are supported by
 * the file format: affine, thin-plate spline, and grid transformations.
 * This file format was developed at the McConnell Brain Imaging Centre
 * at the Montreal Neurological Institute and is used by their software.
 * @sa
 * vtkMINCImageReader vtkMNITransformWriter
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
 */

#ifndef vtkMNITransformReader_h
#define vtkMNITransformReader_h

#include "vtkAlgorithm.h"
#include "vtkIOMINCModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;
class vtkDoubleArray;
class vtkCollection;

class VTKIOMINC_EXPORT vtkMNITransformReader : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkMNITransformReader, vtkAlgorithm);

  static vtkMNITransformReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".xfm"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI Transform"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

  /**
   * Get the number of transforms in the file.
   */
  virtual int GetNumberOfTransforms();

  /**
   * Get one of the transforms listed in the file.
   */
  virtual vtkAbstractTransform* GetNthTransform(int i);

  /**
   * Get the transform that results from concatenating all
   * of the transforms in the file.  This will return null
   * if you have not specified a file name.
   */
  virtual vtkAbstractTransform* GetTransform();

  /**
   * Get any comments that are included in the file.
   */
  virtual const char* GetComments();

protected:
  vtkMNITransformReader();
  ~vtkMNITransformReader() override;

  char* FileName;
  vtkAbstractTransform* Transform;
  vtkCollection* Transforms;
  int LineNumber;
  char* Comments;

  void SetTransform(vtkAbstractTransform* transform);

  int ReadLine(istream& infile, char result[256]);
  int ReadLineAfterComments(istream& infile, char result[256]);
  int SkipWhitespace(istream& infile, char linetext[256], char** cpp);
  int ParseLeftHandSide(istream& infile, char linetext[256], char** cpp, char identifier[256]);
  int ParseStringValue(istream& infile, char linetext[256], char** cpp, char data[256]);
  int ParseFloatValues(istream& infile, char linetext[256], char** cpp, vtkDoubleArray* array);
  int ParseInvertFlagValue(istream& infile, char linetext[256], char** cpp, int* invertFlag);

  int ReadLinearTransform(istream& infile, char linetext[256], char** cp);
  int ReadThinPlateSplineTransform(istream& infile, char linetext[256], char** cp);
  int ReadGridTransform(istream& infile, char linetext[256], char** cp);

  virtual int ReadNextTransform(istream& infile, char linetext[256]);

  virtual int ReadFile();

  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

private:
  vtkMNITransformReader(const vtkMNITransformReader&) = delete;
  void operator=(const vtkMNITransformReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
