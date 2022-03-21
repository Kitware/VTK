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
 * @class   vtkCesiumPointCloudWriter
 * @brief   export a vtkPointSet into a Cesium Point Cloud tile format
 *
 */

#ifndef vtkCesiumPointCloudWriter_h
#define vtkCesiumPointCloudWriter_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkSmartPointer.h"
#include "vtkWriter.h"

class vtkIdList;

class VTKIOCESIUM3DTILES_EXPORT vtkCesiumPointCloudWriter : public vtkWriter
{
public:
  static vtkCesiumPointCloudWriter* New();
  vtkTypeMacro(vtkCesiumPointCloudWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Name of the file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * List of points to be saved.
   */
  vtkSetSmartPointerMacro(PointIds, vtkIdList);
  vtkGetObjectMacro(PointIds, vtkIdList);
  ///@}

  ///@{
  /**
   * It looks for the normals point attribute and saves it in the
   * file if found with the name NORMAL
   * Cesium needs this to render buildings correctly
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

protected:
  vtkCesiumPointCloudWriter();
  ~vtkCesiumPointCloudWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  bool SaveNormal;
  bool SaveBatchId;
  vtkSmartPointer<vtkIdList> PointIds;

private:
  vtkCesiumPointCloudWriter(const vtkCesiumPointCloudWriter&) = delete;
  void operator=(const vtkCesiumPointCloudWriter&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkCesiumPointCloudWriter.h
