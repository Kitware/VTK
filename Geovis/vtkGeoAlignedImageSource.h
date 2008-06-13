/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageSource.h

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


// .NAME vtkGeoAlignedImageSource - 
// .SECTION Description

// .SECTION See Also
   
#ifndef __vtkGeoAlignedImageSource_h
#define __vtkGeoAlignedImageSource_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // for SP
#include "vtkGeoImageNode.h" // for SP

class vtkImageData;

class VTK_GEOVIS_EXPORT vtkGeoAlignedImageSource : public vtkObject
{
public:
  static vtkGeoAlignedImageSource *New();
  vtkTypeRevisionMacro(vtkGeoAlignedImageSource, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load an image from a file with the default extent of the full globe.
  // If dbLocation is specified, output all tiles to that location.
  void LoadAnImage(const char* fileName, const char* dbLocation = 0)
    {
    double ext[4] = {-180.0, 180.0, -90.0, 90.0};
    this->LoadAnImage(fileName, ext, dbLocation);
    }
  void LoadAnImage(vtkImageData* data, const char* dbLocation = 0)
    {
    double ext[4] = {-180.0, 180.0, -90.0, 90.0};
    this->LoadAnImage(data, ext, dbLocation);
    }

  // Description:
  // Load an image from a file which covers a certain latitute/longitude extent.
  // longLatExtent has the format {long min, long max, lat min, lat max}.
  // If dbLocation is specified, output all tiles to that location.
  void LoadAnImage(
    const char* fileName,
    double longLatExtent[4],
    const char* dbLocation = 0);
  void LoadAnImage(
    vtkImageData* data,
    double longLatExtent[4],
    const char* dbLocation = 0);

  // Description:
  // Load tiles from a database of files generated from a LoadAnImage.
  void LoadTiles(const char* loc, vtkGeoImageNode* n = 0);

  // Temporarily public until we get an API to the source.

  // A tree to save the images converted into to tiles.
//BTX
  vtkSmartPointer<vtkGeoImageNode> WesternHemisphere;
  vtkSmartPointer<vtkGeoImageNode> EasternHemisphere;
//ETX

  // Description:
  // Whether this source uses a database of patch files.
  vtkSetMacro(UseTileDatabase, bool);
  vtkGetMacro(UseTileDatabase, bool);
  vtkBooleanMacro(UseTileDatabase, bool);

  // Description:
  // The location of the tile databse.
  vtkSetStringMacro(TileDatabaseLocation);
  vtkGetStringMacro(TileDatabaseLocation);
  
  // Description:
  // The number of levels in the tile database.
  vtkSetMacro(TileDatabaseDepth, int);
  vtkGetMacro(TileDatabaseDepth, int);

protected:
  vtkGeoAlignedImageSource();
  ~vtkGeoAlignedImageSource();

  void AddImageToTree(vtkGeoImageNode* branch,
    vtkImageData* image, double imageLonLatExt[4], const char* dbLocation);

  bool UseTileDatabase;
  char* TileDatabaseLocation;
  int TileDatabaseDepth;

private:
  vtkGeoAlignedImageSource(const vtkGeoAlignedImageSource&);  // Not implemented.
  void operator=(const vtkGeoAlignedImageSource&);  // Not implemented.

//BTX
  class vtkProgressObserver;
  vtkProgressObserver* ProgressObserver;
//ETX
};

#endif
