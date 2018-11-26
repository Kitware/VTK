/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHttpSceneExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHttpSceneExporter
 * @brief   Export the content of a vtkRenderWindow into a directory with
 *          a JSON meta file describing the scene along with the http datasets
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to properly
 * handle endianness.
 */

#ifndef vtkHttpSceneExporter_h
#define vtkHttpSceneExporter_h

#include "vtkExporter.h"
#include "vtkIOWebModule.h" // For export macro

#include <map>    // For string parameter
#include <string> // For string parameter

class vtkActor;
class vtkDataObject;
class vtkDataSet;
class vtkScalarsToColors;

class VTKIOWEB_EXPORT vtkHttpSceneExporter : public vtkExporter
{
public:
  static vtkHttpSceneExporter* New();
  vtkTypeMacro(vtkHttpSceneExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of vtk data file to write.
   * This correspond to the root directory of the data to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkHttpSceneExporter();
  ~vtkHttpSceneExporter() override;

  void WriteDataObject(ostream& os, vtkDataObject* dataObject, vtkActor* actor);
  std::string ExtractRenderingSetup(vtkActor* actor);
  std::string WriteDataSet(vtkDataSet* dataset, const char* addOnMeta);
  void WriteLookupTable(const char* name, vtkScalarsToColors* lookupTable);

  void WriteData() override;

  char* FileName;
  int DatasetCount;
  std::map<std::string, std::string> LookupTables;

private:
  vtkHttpSceneExporter(const vtkHttpSceneExporter&) = delete;
  void operator=(const vtkHttpSceneExporter&) = delete;
};

#endif
