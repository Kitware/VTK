/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGLTFWriter
 * @brief   export a scene into GLTF 2.0 format.
 *
 * vtkGLTFWriter is a concrete subclass of vtkWriter that writes GLTF
 * 2.0 files. Its input is a multiblock dataset as it is produced by
 * the CityGML reader. The dataset contains a list of buildings, each
 * building is made of pieces (polydata), each piece could potentially
 * have a different texture. Materials, including textures, are
 * described as fields in the polydata. If InlineData is false, we
 * only refer to textures files refered in the data, otherwise we read
 * the textures and save them encoded in the file.
 *
 * @sa
 * vtkCityGMLReader
 */

#ifndef vtkGLTFWriter_h
#define vtkGLTFWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

#include <string> // for std::string

class vtkMultiBlockDataSet;
class VTKIOGEOMETRY_EXPORT vtkGLTFWriter : public vtkWriter
{
public:
  static vtkGLTFWriter* New();
  vtkTypeMacro(vtkGLTFWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the GLTF file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the base directory for texture files.
   */
  vtkSetStringMacro(TextureBaseDirectory);
  vtkGetStringMacro(TextureBaseDirectory);
  ///@}

  ///@{
  /**
   * Should the binary data be included in the json file as a base64
   * string.
   */
  vtkGetMacro(InlineData, bool);
  vtkSetMacro(InlineData, bool);
  vtkBooleanMacro(InlineData, bool);
  ///@}

  ///@{
  /**
   * It looks for a point array called
   * NORMAL in the data and it saves it in the
   * GLTF file if found.
   * NORMAL is the vertex normal. Cesium needs this to render buildings correctly
   * if there is no texture.
   */
  vtkGetMacro(SaveNormal, bool);
  vtkSetMacro(SaveNormal, bool);
  vtkBooleanMacro(SaveNormal, bool);
  ///@}

  ///@{
  /**
   * It looks for point arrays called
   * _BATCHID in the data and it saves it in the
   * GLTF file if found.
   * _BATCHID is an index used in 3D Tiles b3dm format. This format stores
   * a binary gltf with a mesh that has several objects (buildings).
   * Objects are indexed from 0 to number of objects - 1, all points
   * of an objects have the same index. These index values are stored
   * in _BATCHID
   */
  vtkGetMacro(SaveBatchId, bool);
  vtkSetMacro(SaveBatchId, bool);
  vtkBooleanMacro(SaveBatchId, bool);
  ///@}

  ///@{
  /**
   */
  vtkGetMacro(SaveTextures, bool);
  vtkSetMacro(SaveTextures, bool);
  vtkBooleanMacro(SaveTextures, bool);
  ///@}

  /**
   * Write the result to a string instead of a file
   */
  std::string WriteToString();

  /**
   * Write the result to a provided ostream
   */
  void WriteToStream(ostream& out, vtkMultiBlockDataSet* in);

protected:
  vtkGLTFWriter();
  ~vtkGLTFWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  char* TextureBaseDirectory;
  bool InlineData;
  bool SaveNormal;
  bool SaveBatchId;
  bool SaveTextures;

private:
  vtkGLTFWriter(const vtkGLTFWriter&) = delete;
  void operator=(const vtkGLTFWriter&) = delete;
};

#endif
