// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNITransformWriter
 * @brief   A writer for MNI transformation files.
 *
 * The MNI .xfm file format is used to store geometrical
 * transformations.  Three kinds of transformations are supported by
 * the file format: affine, thin-plate spline, and grid transformations.
 * This file format was developed at the McConnell Brain Imaging Centre
 * at the Montreal Neurological Institute and is used by their software.
 * @sa
 * vtkMINCImageWriter vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
 */

#ifndef vtkMNITransformWriter_h
#define vtkMNITransformWriter_h

#include "vtkAlgorithm.h"
#include "vtkIOMINCModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;
class vtkHomogeneousTransform;
class vtkThinPlateSplineTransform;
class vtkGridTransform;
class vtkCollection;

class VTKIOMINC_EXPORT vtkMNITransformWriter : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkMNITransformWriter, vtkAlgorithm);

  static vtkMNITransformWriter* New();
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

  ///@{
  /**
   * Set the transform.
   */
  virtual void SetTransform(vtkAbstractTransform* transform);
  virtual vtkAbstractTransform* GetTransform() { return this->Transform; }
  ///@}

  /**
   * Add another transform to the file.  The next time that
   * SetTransform is called, all added transforms will be
   * removed.
   */
  virtual void AddTransform(vtkAbstractTransform* transform);

  /**
   * Get the number of transforms that will be written.
   */
  virtual int GetNumberOfTransforms();

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
  virtual void Write();

protected:
  vtkMNITransformWriter();
  ~vtkMNITransformWriter() override;

  char* FileName;
  vtkAbstractTransform* Transform;
  vtkCollection* Transforms;
  char* Comments;

  int WriteLinearTransform(ostream& outfile, vtkHomogeneousTransform* transform);
  int WriteThinPlateSplineTransform(ostream& outfile, vtkThinPlateSplineTransform* transform);
  int WriteGridTransform(ostream& outfile, vtkGridTransform* transform);

  virtual int WriteTransform(ostream& outfile, vtkAbstractTransform* transform);

  virtual int WriteFile();

  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

private:
  vtkMNITransformWriter(const vtkMNITransformWriter&) = delete;
  void operator=(const vtkMNITransformWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
