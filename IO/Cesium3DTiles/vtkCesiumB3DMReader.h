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
   * Materials are not directly applied to this reader's output.
   * Use GetTexture to access a specific texture's image data, and the indices present in the
   * output dataset's field data to create vtkTextures and apply them to the geometry.
   */
  struct Texture
  {
    vtkSmartPointer<vtkImageData> Image;
    unsigned short MinFilterValue;
    unsigned short MaxFilterValue;
    unsigned short WrapSValue;
    unsigned short WrapTValue;
  };

  vtkIdType GetNumberOfTextures();
  Texture GetTexture(vtkIdType textureIndex);
  ///@}

  ///@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkCesiumB3DMReader();
  ~vtkCesiumB3DMReader() override;

  std::vector<Texture> Textures;

  /**
   * Create and store Texture struct for each image present in the model.
   */
  void StoreTextureData();

  char* FileName = nullptr;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCesiumB3DMReader(const vtkCesiumB3DMReader&) = delete;
  void operator=(const vtkCesiumB3DMReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
