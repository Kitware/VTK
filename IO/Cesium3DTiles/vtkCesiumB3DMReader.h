// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCesiumB3DMReader
 * @brief   Read a CesiumB3DM file.
 *
 */

#ifndef vtkCesiumB3DMReader_h
#define vtkCesiumB3DMReader_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkGLTFReader;

/**
 * @class vtkCesiumB3DMReader
 * @class Reads a Cesium B3DM dataset (tile)
 *
 *
 */
class VTKIOCESIUM3DTILES_EXPORT vtkCesiumB3DMReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCesiumB3DMReader* New();
  vtkTypeMacro(vtkCesiumB3DMReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   *
   */
  vtkGetObjectMacro(GLTFReader, vtkGLTFReader);
  ///@}

protected:
  vtkCesiumB3DMReader();
  ~vtkCesiumB3DMReader() override;

  /**
   * Create and store Texture struct for each image present in the model.
   */
  void StoreTextureData();

  char* FileName = nullptr;
  vtkNew<vtkGLTFReader> GLTFReader;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCesiumB3DMReader(const vtkCesiumB3DMReader&) = delete;
  void operator=(const vtkCesiumB3DMReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
