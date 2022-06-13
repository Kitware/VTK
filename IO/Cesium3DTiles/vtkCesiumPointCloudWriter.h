/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCesiumPointCloudWriter.h

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
#include "vtkIdList.h"
#include "vtkWriter.h"

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
  vtkSetObjectMacro(PointIds, vtkIdList);
  vtkGetObjectMacro(PointIds, vtkIdList);
  ///@}

protected:
  vtkCesiumPointCloudWriter();
  ~vtkCesiumPointCloudWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;
  vtkIdList* PointIds;

private:
  vtkCesiumPointCloudWriter(const vtkCesiumPointCloudWriter&) = delete;
  void operator=(const vtkCesiumPointCloudWriter&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkCesiumPointCloudWriter.h
