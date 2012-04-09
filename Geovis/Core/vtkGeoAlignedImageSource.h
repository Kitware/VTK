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
// .NAME vtkGeoAlignedImageSource - Splits hi-res image into tiles.
//
// .SECTION Description
// vtkGeoAlignedImageSource uses a high resolution image to generate tiles
// at multiple resolutions in a hierarchy. It should be used as a source in
// vtkGeoAlignedImageRepresentation.

// .SECTION See Also
// vtkGeoAlignedImageRepresentation vtkGeoView vtkGeoView2D

#ifndef __vtkGeoAlignedImageSource_h
#define __vtkGeoAlignedImageSource_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkGeoSource.h"

class vtkGeoImageNode;
class vtkImageData;
class vtkMultiBlockDataSet;

class VTKGEOVISCORE_EXPORT vtkGeoAlignedImageSource : public vtkGeoSource
{
public:
  static vtkGeoAlignedImageSource *New();
  vtkTypeMacro(vtkGeoAlignedImageSource, vtkGeoSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Fetch the root image.
  virtual bool FetchRoot(vtkGeoTreeNode* node);

  // Description:
  // Fetch a child image.
  virtual bool FetchChild(vtkGeoTreeNode* parent, int index, vtkGeoTreeNode* child);

  // Description:
  // The high-resolution image to be used to cover the globe.
  vtkGetObjectMacro(Image, vtkImageData);
  virtual void SetImage(vtkImageData* image);

  // Description:
  // The range of the input hi-res image.
  vtkSetVector2Macro(LatitudeRange, double);
  vtkGetVector2Macro(LatitudeRange, double);
  vtkSetVector2Macro(LongitudeRange, double);
  vtkGetVector2Macro(LongitudeRange, double);

  // Description:
  // The overlap of adjacent tiles.
  vtkSetClampMacro(Overlap, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Overlap, double);

  // Description:
  // Whether to force image sizes to a power of two.
  vtkSetMacro(PowerOfTwoSize, bool);
  vtkGetMacro(PowerOfTwoSize, bool);
  vtkBooleanMacro(PowerOfTwoSize, bool);

protected:
  vtkGeoAlignedImageSource();
  ~vtkGeoAlignedImageSource();

  void CropImageForNode(vtkGeoImageNode* node, vtkImageData* image);
  int PowerOfTwo(int val);

  vtkImageData* Image;
  vtkMultiBlockDataSet* LevelImages;
  double LatitudeRange[2];
  double LongitudeRange[2];
  double Overlap;
  bool PowerOfTwoSize;

  //BTX
  class vtkProgressObserver;
  vtkProgressObserver* ProgressObserver;
  //ETX

private:
  vtkGeoAlignedImageSource(const vtkGeoAlignedImageSource&);  // Not implemented.
  void operator=(const vtkGeoAlignedImageSource&);  // Not implemented.
};

#endif
