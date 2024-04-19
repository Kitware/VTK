// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNITagPointWriter
 * @brief   A writer for MNI tag point files.
 *
 * The MNI .tag file format is used to store tag points, for use in
 * either registration or labeling of data volumes.  This file
 * format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Tag points can be stored for either one volume or two volumes,
 * and this filter can take one or two inputs.  Alternatively, the
 * points to be written can be specified by calling SetPoints().
 * @sa
 * vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
 */

#ifndef vtkMNITagPointWriter_h
#define vtkMNITagPointWriter_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkPointSet;
class vtkStringArray;
class vtkDoubleArray;
class vtkIntArray;
class vtkPoints;

class VTKIOMINC_EXPORT vtkMNITagPointWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkMNITagPointWriter, vtkWriter);

  static vtkMNITagPointWriter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".tag"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI tags"; }

  ///@{
  /**
   * Set the points (unless you set them as inputs).
   */
  virtual void SetPoints(int port, vtkPoints* points);
  virtual void SetPoints(vtkPoints* points) { this->SetPoints(0, points); }
  virtual vtkPoints* GetPoints(int port);
  virtual vtkPoints* GetPoints() { return this->GetPoints(0); }
  ///@}

  ///@{
  /**
   * Set the labels (unless the input PointData has an
   * array called LabelText). Labels are optional.
   */
  virtual void SetLabelText(vtkStringArray* a);
  vtkGetObjectMacro(LabelText, vtkStringArray);
  ///@}

  ///@{
  /**
   * Set the weights (unless the input PointData has an
   * array called Weights).  Weights are optional.
   */
  virtual void SetWeights(vtkDoubleArray* a);
  vtkGetObjectMacro(Weights, vtkDoubleArray);
  ///@}

  ///@{
  /**
   * Set the structure ids (unless the input PointData has
   * an array called StructureIds).  These are optional.
   */
  virtual void SetStructureIds(vtkIntArray* a);
  vtkGetObjectMacro(StructureIds, vtkIntArray);
  ///@}

  ///@{
  /**
   * Set the structure ids (unless the input PointData has
   * an array called PatientIds).  These are optional.
   */
  virtual void SetPatientIds(vtkIntArray* a);
  vtkGetObjectMacro(PatientIds, vtkIntArray);
  ///@}

  ///@{
  /**
   * Set comments to be added to the file.
   */
  vtkSetStringMacro(Comments);
  vtkGetStringMacro(Comments);
  ///@}

  /**
   * Write the file.
   */
  int Write() override;

  /**
   * Get the MTime.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Specify file name of vtk polygon data file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkMNITagPointWriter();
  ~vtkMNITagPointWriter() override;

  vtkPoints* Points[2];
  vtkStringArray* LabelText;
  vtkDoubleArray* Weights;
  vtkIntArray* StructureIds;
  vtkIntArray* PatientIds;
  char* Comments;

  void WriteData() override {}
  virtual void WriteData(vtkPointSet* inputs[2]);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;

  int FileType;

  ostream* OpenFile();
  void CloseFile(ostream* fp);

private:
  vtkMNITagPointWriter(const vtkMNITagPointWriter&) = delete;
  void operator=(const vtkMNITagPointWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
