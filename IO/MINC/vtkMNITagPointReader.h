// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNITagPointReader
 * @brief   A reader for MNI tag files.
 *
 * The MNI .tag file format is used to store labeled points, it can
 * store either one or two point sets.  All point sets must have the
 * same number of points and they will share the same labels.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * The labels are stored as a vtkStringArray in the PointData of the
 * output dataset, which is a vtkPolyData.
 * @sa
 * vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class.
 */

#ifndef vtkMNITagPointReader_h
#define vtkMNITagPointReader_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;
class vtkPoints;
class vtkStringArray;
class vtkDoubleArray;
class vtkIntArray;

class VTKIOMINC_EXPORT vtkMNITagPointReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMNITagPointReader, vtkPolyDataAlgorithm);

  static vtkMNITagPointReader* New();
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
  virtual const char* GetFileExtensions() { return ".tag"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI tags"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

  /**
   * Get the number of volumes specified by the file, which will be
   * equal to one or two.  There will be an output point set for each
   * volume, so really, this parameter just tells you the number of
   * outputs to expect from this reader.
   */
  virtual int GetNumberOfVolumes();

  /**
   * Get the points.  These are also provided in the first and
   * second output ports of the reader.  This method will return
   * nullptr if there is no data.
   */
  virtual vtkPoints* GetPoints(int port);
  virtual vtkPoints* GetPoints() { return this->GetPoints(0); }

  /**
   * Get the labels.  These same labels are provided in the output
   * point sets, as the PointData data array named "LabelText".
   * This will return nullptr if there were no labels in the file.
   */
  virtual vtkStringArray* GetLabelText();

  /**
   * Get the weights.  These are also provided in the output
   * point sets, as the PointData data array named "Weights".
   * This will return nullptr if there were no weights in the file.
   */
  virtual vtkDoubleArray* GetWeights();

  /**
   * Get the structure ids.  These are also provided in the output
   * point sets, as the PointData data array named "StructureIds".
   * This will return nullptr if there were no ids in the file.
   */
  virtual vtkIntArray* GetStructureIds();

  /**
   * Get the patient ids.  These are also provided in the output
   * point sets, as the PointData data array named "PatientIds".
   * This will return nullptr if there were no ids in the file.
   */
  virtual vtkIntArray* GetPatientIds();

  /**
   * Get any comments that are included in the file.
   */
  virtual const char* GetComments();

protected:
  vtkMNITagPointReader();
  ~vtkMNITagPointReader() override;

  char* FileName;
  int NumberOfVolumes;

  int LineNumber;
  char* Comments;

  int ReadLine(istream& infile, std::string& linetext, std::string::iterator& pos);
  int ReadLineAfterComments(istream& infile, std::string& linetext, std::string::iterator& pos);
  int SkipWhitespace(istream& infile, std::string& linetext, std::string::iterator& pos, int nl);
  int ParseLeftHandSide(
    istream& infile, std::string& linetext, std::string::iterator& pos, std::string& identifier);
  int ParseStringValue(
    istream& infile, std::string& linetext, std::string::iterator& pos, std::string& data);
  int ParseIntValues(
    istream& infile, std::string& linetext, std::string::iterator& pos, int* values, int count);
  int ParseFloatValues(
    istream& infile, std::string& linetext, std::string::iterator& pos, double* values, int count);

  virtual int ReadFile(vtkPolyData* output1, vtkPolyData* output2);

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

private:
  vtkMNITagPointReader(const vtkMNITagPointReader&) = delete;
  void operator=(const vtkMNITagPointReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
