// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRMLExporter
 * @brief   export a scene into VRML 2.0 format.
 *
 * vtkVRMLExporter is a concrete subclass of vtkExporter that writes VRML 2.0
 * files. This is based on the VRML 2.0 draft #3 but it should be pretty
 * stable since we aren't using any of the newer features.
 *
 * @sa
 * vtkExporter
 */

#ifndef vtkVRMLExporter_h
#define vtkVRMLExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;
class vtkActor;
class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkPolyData;
class vtkPointData;

class VTKIOEXPORT_EXPORT vtkVRMLExporter : public vtkExporter
{
public:
  static vtkVRMLExporter* New();
  vtkTypeMacro(vtkVRMLExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the VRML file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the Speed of navigation. Default is 4.
   */
  vtkSetMacro(Speed, double);
  vtkGetMacro(Speed, double);
  ///@}

  /**
   * Set the file pointer to write to. This will override
   * a FileName if specified.
   */
  void SetFilePointer(FILE*);

protected:
  vtkVRMLExporter();
  ~vtkVRMLExporter() override;

  void WriteData() override;
  void WriteALight(vtkLight* aLight, FILE* fp);
  void WriteAnActor(vtkActor* anActor, FILE* fp);
  void WritePointData(vtkPoints* points, vtkDataArray* normals, vtkDataArray* tcoords,
    vtkUnsignedCharArray* colors, bool cellData, FILE* fp);
  void WriteShapeBegin(vtkActor* actor, FILE* fileP, vtkPolyData* polyData, vtkPointData* pntData,
    vtkUnsignedCharArray* color);
  void WriteShapeEnd(FILE* fileP);
  char* FileName;
  FILE* FilePointer;
  double Speed;

private:
  vtkVRMLExporter(const vtkVRMLExporter&) = delete;
  void operator=(const vtkVRMLExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
