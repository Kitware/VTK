// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCityGMLReader
 * @brief   read CityGML data file
 *
 */

#ifndef vtkCityGMLReader_h
#define vtkCityGMLReader_h

#include "vtkIOCityGMLModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

/**
 * @class   vtkCityGMLReader
 * @brief   reads CityGML files
 *
 * vtkCityGMLReader is a reader for CityGML .gml files. The output is
 * a multiblock dataset. We read objects at level of detail (LOD)
 * specified (default is 3).

 * The leafs of the multiblock dataset (which are polygonal datasets)
 * have a field array with one element called "gml_id" which
 * corresponds to the gml:id for gml:TriangulatedSurface,
 * gml:MultiSurface or gml:CompositeSurface in the CityGML file. If
 * the poly dataset has a texture, we specify this with a float/double point array
 * called "tcoords" and a field array called
 * "texture_uri" containing one tuple per texture file (and one component) with the path
 * to the file. All textures of the same type should be at the same index in the
 * texture_uri array. The path can be relative to the citygml file or it can be absolute.
 * If the dataset has a app::X3DMaterial we store two double field arrays with 3
 * components and 1 tuple: "diffuse_color" and "specular_color" and
 * two double field arrays with 1 component and 1 tuple: "transparency",
 * "shininess"

 * Top level children of the multiblock dataset have a field array
 * with one element called "element" which contains the CityGML
 * element name for example: dem:ReliefFeature, wtr:WaterBody,
 * grp::CityObjectGroup (forest), veg:SolitaryVegetationObject,
 * brid:Bridge, run:Tunel, tran:Railway, tran:Road, bldg:Building,
 * gen:GenericCityObject, luse:LandUse. These nodes also have a gml_id field array.
*/
VTK_ABI_NAMESPACE_BEGIN
class VTKIOCITYGML_EXPORT vtkCityGMLReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCityGMLReader* New();
  vtkTypeMacro(vtkCityGMLReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of the CityGML data file to read.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the level of detail (LOD) to read. Valid values are from 0 (least detailed)
   * through 4 (most detailed), default value is 3.
   */
  vtkSetClampMacro(LOD, int, 0, 4);
  vtkGetMacro(LOD, int);
  ///@}

  ///@{
  /**
   * Certain input files use app:transparency as opacity. Set this field true
   * to show that correctly. The default is false.
   */
  vtkSetMacro(UseTransparencyAsOpacity, int);
  vtkGetMacro(UseTransparencyAsOpacity, int);
  vtkBooleanMacro(UseTransparencyAsOpacity, int);
  ///@}

  ///@{
  /**
   * Number of buildings read from the file.
   * Default is numeric_limits<int>::max() which means the reader will read all
   * buildings from the file. You can set either NumberOfBuidlings to read the range
   * [0, NumberOfBuildings) or you can set BeginBuildingIndex and EndBuildingIndex to
   * read the range [BeginBuildingIndex, EndBuildingIndex). If you send them both,
   * a warning will be printed and we'll use the latter.
   */
  vtkSetMacro(NumberOfBuildings, int);
  vtkGetMacro(NumberOfBuildings, int);
  ///@}

  ///@{
  /**
   * Read a range of buildings from the file [begin, end)
   * Default is begin=0, end = numeric_limits<int>::max() which means the reader
   * will read all buildings from the file.
   */
  vtkSetMacro(BeginBuildingIndex, int);
  vtkGetMacro(BeginBuildingIndex, int);
  vtkSetMacro(EndBuildingIndex, int);
  vtkGetMacro(EndBuildingIndex, int);
  ///@}

  ///@{
  /**
   * Helper functions for setting field arrays. These are used to save texture paths or colors
   * for polydata.
   *
   */
  static void SetField(vtkDataObject* obj, const char* name, const char* value);
  static void SetField(
    vtkDataObject* obj, const char* name, double* value, vtkIdType numberOfComponents);
  ///@}

protected:
  vtkCityGMLReader();
  ~vtkCityGMLReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;
  int LOD;
  int UseTransparencyAsOpacity;
  int NumberOfBuildings;
  int BeginBuildingIndex;
  int EndBuildingIndex;

private:
  vtkCityGMLReader(const vtkCityGMLReader&) = delete;
  void operator=(const vtkCityGMLReader&) = delete;

  class Implementation;
  Implementation* Impl;
};

VTK_ABI_NAMESPACE_END
#endif
