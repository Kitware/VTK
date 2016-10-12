/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkGeoAlignedImageRepresentation
 * @brief   A multi-resolution image tree
 *
 *
 * vtkGeoAlignedImageRepresentation represents a high resolution image
 * over the globle. It has an associated vtkGeoSource which is responsible
 * for fetching new data. This class keeps the fetched data in a quad-tree
 * structure organized by latitude and longitude.
*/

#ifndef vtkGeoAlignedImageRepresentation_h
#define vtkGeoAlignedImageRepresentation_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkDataRepresentation.h"

class vtkGeoImageNode;
class vtkGeoSource;
class vtkGeoTreeNodeCache;

class VTKGEOVISCORE_EXPORT vtkGeoAlignedImageRepresentation : public vtkDataRepresentation
{
public:
  static vtkGeoAlignedImageRepresentation *New();
  vtkTypeMacro(vtkGeoAlignedImageRepresentation,vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Retrieve the most refined image patch that covers the specified
   * latitude and longitude bounds (lat-min, lat-max, long-min, long-max).
   */
  virtual vtkGeoImageNode* GetBestImageForBounds(double bounds[4]);

  /**
   * The source for this representation. This must be set first before
   * calling GetBestImageForBounds.
   */
  virtual vtkGeoSource* GetSource()
    { return this->GeoSource; }
  virtual void SetSource(vtkGeoSource* source);

  /**
   * Serialize the database to the specified directory.
   * Each image is stored as a .vti file.
   * The Origin and Spacing of the saved image contain (lat-min, long-min)
   * and (lat-max, long-max), respectively.
   * Files are named based on their level and id within that level.
   */
  void SaveDatabase(const char* path);

protected:
  vtkGeoAlignedImageRepresentation();
  ~vtkGeoAlignedImageRepresentation();

  //@{
  /**
   * The source for creating image nodes.
   */
  void SetGeoSource(vtkGeoSource* source);
  vtkGeoSource* GeoSource;
  //@}

  /**
   * The root of the image tree.
   */
  vtkGeoImageNode* Root;

  /**
   * Initialize the representation with the current source.
   */
  void Initialize();

  /**
   * Print information about the image tree.
   */
  void PrintTree(ostream& os, vtkIndent indent, vtkGeoImageNode* root);

  vtkGeoTreeNodeCache* Cache;

private:
  vtkGeoAlignedImageRepresentation(const vtkGeoAlignedImageRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoAlignedImageRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
