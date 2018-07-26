/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCityGMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * coresponds to the gml:id for gml:TriangulatedSurface,
 * gml:MultiSurface or gml:CompositeSurface in the CityGML file. If
 * the poly dataset has a texture, we specify this with a point array
 * called "tcoords" and a field array with one element called
 * "texture_uri" containing the path to the texture file. If the poly
 * dataset has a app::X3DMaterial we store two fields arrays with 3
 * components and 1 tuple: "diffuse_color" and "specular_color" and
 * one field array with 1 component and 1 tuple: "transparency".

 * Top level children of the multiblock dataset have a field array
 * with one element called "element" which contains the CityGML
 * element name for example: dem:ReliefFeature, wtr:WaterBody,
 * grp::CityObjectGroup (forest), veg:SolitaryVegetationObject,
 * brid:Bridge, run:Tunel, tran:Railway, tran:Road, bldg:Building,
 * gen:GenericCityObject, luse:LandUse
*/
class VTKIOCITYGML_EXPORT vtkCityGMLReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCityGMLReader *New();
  vtkTypeMacro(vtkCityGMLReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of the CityGML data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the level of detail (LOD) to read. Valid values are from 0 (least detailed)
   * through 4 (most detailed), default value is 3.
   */
  vtkSetClampMacro(LOD, int, 0, 4);
  vtkGetMacro(LOD, int);
  //@}


  //@{
  /**
   * Certain input files use app:transparency as opacity. Set this field true
   * to show that correctly. The default is false.
   */
  vtkSetMacro(UseTransparencyAsOpacity, int);
  vtkGetMacro(UseTransparencyAsOpacity, int);
  vtkBooleanMacro(UseTransparencyAsOpacity, int);
  //@}

protected:
  vtkCityGMLReader();
  ~vtkCityGMLReader() override;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;


  char *FileName;
  int LOD;
  int UseTransparencyAsOpacity;

private:
  vtkCityGMLReader(const vtkCityGMLReader&) = delete;
  void operator=(const vtkCityGMLReader&) = delete;

  class Implementation;
  Implementation* Impl;
};

#endif
