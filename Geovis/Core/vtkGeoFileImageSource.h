/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoFileImageSource.h

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
 * @class   vtkGeoFileImageSource
 * @brief   A tiled image source on disk.
 *
 *
 * vtkGeoFileImageSource is a vtkGeoSource that fetches .vti images from
 * disk in a directory with a certain naming scheme. You may use
 * vtkGeoAlignedImageRepresentation's SaveDatabase method to generate
 * an database of image tiles in this format.
*/

#ifndef vtkGeoFileImageSource_h
#define vtkGeoFileImageSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoSource.h"
#include "vtkSmartPointer.h" // For smart pointer ivars

class vtkGeoImageNode;
class vtkGeoTreeNode;

class VTKGEOVISCORE_EXPORT vtkGeoFileImageSource : public vtkGeoSource
{
public:
  static vtkGeoFileImageSource *New();
  vtkTypeMacro(vtkGeoFileImageSource,vtkGeoSource);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  vtkGeoFileImageSource();
  ~vtkGeoFileImageSource() VTK_OVERRIDE;

  /**
   * Fetches the root image representing the whole globe.
   */
  bool FetchRoot(vtkGeoTreeNode* root) VTK_OVERRIDE;

  /**
   * Fetches the child image of a parent from disk.
   */
  bool FetchChild(vtkGeoTreeNode* node, int index, vtkGeoTreeNode* child) VTK_OVERRIDE;

  //@{
  /**
   * The path the tiled image database.
   */
  vtkSetStringMacro(Path);
  vtkGetStringMacro(Path);
  //@}

protected:

  bool ReadImage(int level, int id, vtkGeoImageNode* node);

private:
  vtkGeoFileImageSource(const vtkGeoFileImageSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoFileImageSource&) VTK_DELETE_FUNCTION;

  char* Path;
};

#endif
