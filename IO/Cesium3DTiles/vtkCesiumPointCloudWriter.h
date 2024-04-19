// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCesiumPointCloudWriter
 * @brief   export a vtkPointSet into a Cesium Point Cloud (PNTS) tile format
 *
 */

#ifndef vtkCesiumPointCloudWriter_h
#define vtkCesiumPointCloudWriter_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkIdList.h"
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN

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

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkCesiumPointCloudWriter.h
