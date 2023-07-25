// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJSONRenderWindowExporter
 * @brief   Exports a render window for vtk-js
 *
 * vtkJSONRenderWindowExporter constructs a scene graph from an input render
 * window and generates an archive for vtk-js. The traversal of the scene graph
 * topology is handled by graph elements constructed by vtkVtkJSViewNodeFactory,
 * the translation from VTK to vtk-js scene elements (renderers, actors,
 * mappers, etc.) is handled by vtkVtkJSSceneGraphSerializer, and the
 * transcription of data is handled by vtkArchiver. The latter two classes are
 * designed to be extensible via inheritance, and derived instances can be used
 * to modify the vtk-js file format and output mode.
 *
 *
 * @sa
 * vtkVtkJSSceneGraphSerializer vtkVtkJSViewNodeFactory
 */

#ifndef vtkJSONRenderWindowExporter_h
#define vtkJSONRenderWindowExporter_h

#include "vtkIOExportModule.h" // For export macro

#include "vtkExporter.h"
#include "vtkNew.h"             // For vtkNew
#include "vtkViewNodeFactory.h" // For vtkViewNodeFactory

VTK_ABI_NAMESPACE_BEGIN
class vtkArchiver;
class vtkVtkJSSceneGraphSerializer;
class vtkVtkJSViewNodeFactory;

class VTKIOEXPORT_EXPORT vtkJSONRenderWindowExporter : public vtkExporter
{
public:
  static vtkJSONRenderWindowExporter* New();
  vtkTypeMacro(vtkJSONRenderWindowExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the Serializer object
   */
  virtual void SetSerializer(vtkVtkJSSceneGraphSerializer*);
  vtkGetObjectMacro(Serializer, vtkVtkJSSceneGraphSerializer);
  ///@}

  ///@{
  /**
   * Specify the Archiver object
   */
  virtual void SetArchiver(vtkArchiver*);
  vtkGetObjectMacro(Archiver, vtkArchiver);
  ///@}

  ///@{
  /**
   * Write scene data.
   */
  void WriteData() override;
  ///@}

  ///@{
  /**
   * Write scene in compact form (default is true).
   */
  vtkSetMacro(CompactOutput, bool);
  vtkGetMacro(CompactOutput, bool);
  vtkBooleanMacro(CompactOutput, bool);
  ///@}

protected:
  vtkJSONRenderWindowExporter();
  ~vtkJSONRenderWindowExporter() override;

private:
  vtkJSONRenderWindowExporter(const vtkJSONRenderWindowExporter&) = delete;
  void operator=(const vtkJSONRenderWindowExporter&) = delete;

  vtkArchiver* Archiver;
  vtkVtkJSSceneGraphSerializer* Serializer;
  vtkVtkJSViewNodeFactory* Factory;
  bool CompactOutput;
};

VTK_ABI_NAMESPACE_END
#endif
