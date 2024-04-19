// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBYUReader
 * @brief   read MOVIE.BYU polygon files
 *
 * vtkBYUReader is a source object that reads MOVIE.BYU polygon files.
 * These files consist of a geometry file (.g), a scalar file (.s), a
 * displacement or vector file (.d), and a 2D texture coordinate file
 * (.t).
 */

#ifndef vtkBYUReader_h
#define vtkBYUReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOGEOMETRY_EXPORT vtkBYUReader : public vtkPolyDataAlgorithm
{
public:
  static vtkBYUReader* New();

  vtkTypeMacro(vtkBYUReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify name of geometry FileName.
   */
  vtkSetFilePathMacro(GeometryFileName);
  vtkGetFilePathMacro(GeometryFileName);
  ///@}

  /**
   * Specify name of geometry FileName (alias).
   */
  virtual void SetFileName(VTK_FILEPATH const char* f) { this->SetGeometryFileName(f); }
  virtual VTK_FILEPATH VTK_FUTURE_CONST char* GetFileName() VTK_FUTURE_CONST
  {
    return this->GetGeometryFileName();
  }

  ///@{
  /**
   * Specify name of displacement FileName.
   */
  vtkSetFilePathMacro(DisplacementFileName);
  vtkGetFilePathMacro(DisplacementFileName);
  ///@}

  ///@{
  /**
   * Specify name of scalar FileName.
   */
  vtkSetFilePathMacro(ScalarFileName);
  vtkGetFilePathMacro(ScalarFileName);
  ///@}

  ///@{
  /**
   * Specify name of texture coordinates FileName.
   */
  vtkSetFilePathMacro(TextureFileName);
  vtkGetFilePathMacro(TextureFileName);
  ///@}

  ///@{
  /**
   * Turn on/off the reading of the displacement file.
   */
  vtkSetMacro(ReadDisplacement, vtkTypeBool);
  vtkGetMacro(ReadDisplacement, vtkTypeBool);
  vtkBooleanMacro(ReadDisplacement, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the reading of the scalar file.
   */
  vtkSetMacro(ReadScalar, vtkTypeBool);
  vtkGetMacro(ReadScalar, vtkTypeBool);
  vtkBooleanMacro(ReadScalar, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the reading of the texture coordinate file.
   * Specify name of geometry FileName.
   */
  vtkSetMacro(ReadTexture, vtkTypeBool);
  vtkGetMacro(ReadTexture, vtkTypeBool);
  vtkBooleanMacro(ReadTexture, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the part number to be read.
   */
  vtkSetClampMacro(PartNumber, int, 1, VTK_INT_MAX);
  vtkGetMacro(PartNumber, int);
  ///@}

  /**
   * Returns 1 if this file can be read and 0 if the file cannot be read.
   * Because BYU files do not have anything in the header specifying the file
   * type, the result is not definitive.  Invalid files may still return 1
   * although a valid file will never return 0.
   */
  static int CanReadFile(VTK_FILEPATH const char* filename);

protected:
  vtkBYUReader();
  ~vtkBYUReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(vtkDataObject* output, int idx, int numDivisions);

  char* GeometryFileName;
  char* DisplacementFileName;
  char* ScalarFileName;
  char* TextureFileName;
  vtkTypeBool ReadDisplacement;
  vtkTypeBool ReadScalar;
  vtkTypeBool ReadTexture;
  int PartNumber;

  void ReadGeometryFile(FILE* fp, int& numPts, vtkInformation* outInfo);
  void ReadDisplacementFile(int numPts, vtkInformation* outInfo);
  void ReadScalarFile(int numPts, vtkInformation* outInfo);
  void ReadTextureFile(int numPts, vtkInformation* outInfo);

private:
  vtkBYUReader(const vtkBYUReader&) = delete;
  void operator=(const vtkBYUReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
