// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJSONSceneExporter
 * @brief   Export the content of a vtkRenderWindow into a directory with
 *          a JSON meta file describing the scene along with the http datasets
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to properly
 * handle endianness.
 */

#ifndef vtkJSONSceneExporter_h
#define vtkJSONSceneExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro
#include "vtkSmartPointer.h"   // For vtkSmartPointer

#include <map>    // For member variables
#include <string> // For string parameter
#include <vector> // For member variables

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkColorTransferFunction;
class vtkDataObject;
class vtkDataSet;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkPropCollection;
class vtkScalarsToColors;
class vtkTexture;
class vtkVolume;
class vtkVolumeCollection;

class VTKIOEXPORT_EXPORT vtkJSONSceneExporter : public vtkExporter
{
public:
  static vtkJSONSceneExporter* New();
  vtkTypeMacro(vtkJSONSceneExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of vtk data file to write.
   * This correspond to the root directory of the data to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Whether or not to write textures.
   * Textures will be written in JPEG format.
   * Default is false.
   */
  vtkSetMacro(WriteTextures, bool);
  vtkGetMacro(WriteTextures, bool);
  ///@}

  ///@{
  /**
   * Whether or not to write texture LODs.
   * This will write out the textures in a series of decreasing
   * resolution JPEG files, which are intended to be uploaded to the
   * web. Each file will be 1/4 the size of the previous one. The files
   * will stop being written out when one is smaller than the
   * TextureLODsBaseSize.
   * Default is false.
   */
  vtkSetMacro(WriteTextureLODs, bool);
  vtkGetMacro(WriteTextureLODs, bool);
  ///@}

  ///@{
  /**
   * The base size to be used for texture LODs. The texture LODs will
   * stop being written out when one is smaller than this size.
   * Default is 100 KB. Units are in bytes.
   */
  vtkSetMacro(TextureLODsBaseSize, size_t);
  vtkGetMacro(TextureLODsBaseSize, size_t);
  ///@}

  ///@{
  /**
   * The base URL to be used for texture LODs.
   * Default is nullptr.
   */
  vtkSetStringMacro(TextureLODsBaseUrl);
  vtkGetStringMacro(TextureLODsBaseUrl);
  ///@}

  ///@{
  /**
   * Whether or not to write poly LODs.
   * This will write out the poly LOD sources in a series of decreasing
   * resolution data sets, which are intended to be uploaded to the
   * web. vtkQuadricCluster is used to decrease the resolution of the
   * poly data. Each will be approximately 1/4 the size of the previous
   * one (unless certain errors occur, and then the defaults for quadric
   * clustering will be used, which will produce an unknown size). The
   * files will stop being written out when one is smaller than the
   * PolyLODsBaseSize, or if the difference in the sizes of the two
   * most recent LODs is less than 5%. The smallest LOD will be written
   * into the vtkjs file, rather than with the rest of the LODs.
   * Default is false.
   */
  vtkSetMacro(WritePolyLODs, bool);
  vtkGetMacro(WritePolyLODs, bool);
  ///@}

  ///@{
  /**
   * The base size to be used for poly LODs. The poly LODs will stop
   * being written out when one is smaller than this size, or if the
   * difference in the sizes of the two most recent LODs is less
   * than 5%.
   * Default is 100 KB. Units are in bytes.
   */
  vtkSetMacro(PolyLODsBaseSize, size_t);
  vtkGetMacro(PolyLODsBaseSize, size_t);
  ///@}

  ///@{
  /**
   * The base URL to be used for poly LODs.
   * Default is nullptr.
   */
  vtkSetStringMacro(PolyLODsBaseUrl);
  vtkGetStringMacro(PolyLODsBaseUrl);
  ///@}

protected:
  vtkJSONSceneExporter();
  ~vtkJSONSceneExporter() override;

  void WritePropCollection(vtkPropCollection* collection, std::ostream& sceneComponents);
  void WriteVolumeCollection(vtkVolumeCollection* volumeCollection, std::ostream& sceneComponents);

  void WriteDataObject(ostream& os, vtkDataObject* dataObject, vtkActor* actor, vtkVolume* volume);
  std::string ExtractPiecewiseFunctionSetup(vtkPiecewiseFunction* pwf);
  std::string ExtractColorTransferFunctionSetup(vtkColorTransferFunction* volume);
  std::string ExtractVolumeRenderingSetup(vtkVolume* volume);
  std::string ExtractActorRenderingSetup(vtkActor* actor);
  std::string WriteDataSet(vtkDataSet* dataset, const char* addOnMeta);
  void WriteLookupTable(const char* name, vtkScalarsToColors* lookupTable);

  void WriteData() override;

  ///@{
  /**
   * Path to temporary folder where files are written to before
   * being renamed to this->FileName.
   */
  std::string GetTemporaryPath() const;
  ///@}
  std::string CurrentDataSetPath() const;

  std::string WriteTexture(vtkTexture* texture);
  std::string WriteTextureLODSeries(vtkTexture* texture);

  // The returned pointer is the smallest poly LOD, intended to be
  // written out in the vtkjs file.
  vtkSmartPointer<vtkPolyData> WritePolyLODSeries(vtkPolyData* polys, std::string& config);

  char* FileName;
  bool WriteTextures;
  bool WriteTextureLODs;
  size_t TextureLODsBaseSize;
  char* TextureLODsBaseUrl;
  bool WritePolyLODs;
  size_t PolyLODsBaseSize;
  char* PolyLODsBaseUrl;
  int DatasetCount;
  std::map<std::string, std::string> LookupTables;
  std::map<vtkTexture*, std::string> TextureStrings;
  std::map<vtkTexture*, std::string> TextureLODStrings;

  // Files that subclasses should zip
  std::vector<std::string> FilesToZip;

private:
  vtkJSONSceneExporter(const vtkJSONSceneExporter&) = delete;
  void operator=(const vtkJSONSceneExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
