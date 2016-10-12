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
/**
 * @class   vtkGeoAlignedImageSource
 * @brief   Splits hi-res image into tiles.
 *
 *
 * vtkGeoAlignedImageSource uses a high resolution image to generate tiles
 * at multiple resolutions in a hierarchy. It should be used as a source in
 * vtkGeoAlignedImageRepresentation.
 *
 * @sa
 * vtkGeoAlignedImageRepresentation vtkGeoView vtkGeoView2D
*/

#ifndef vtkGeoAlignedImageSource_h
#define vtkGeoAlignedImageSource_h

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

  /**
   * Fetch the root image.
   */
  virtual bool FetchRoot(vtkGeoTreeNode* node);

  /**
   * Fetch a child image.
   */
  virtual bool FetchChild(vtkGeoTreeNode* parent, int index, vtkGeoTreeNode* child);

  //@{
  /**
   * The high-resolution image to be used to cover the globe.
   */
  vtkGetObjectMacro(Image, vtkImageData);
  virtual void SetImage(vtkImageData* image);
  //@}

  //@{
  /**
   * The range of the input hi-res image.
   */
  vtkSetVector2Macro(LatitudeRange, double);
  vtkGetVector2Macro(LatitudeRange, double);
  vtkSetVector2Macro(LongitudeRange, double);
  vtkGetVector2Macro(LongitudeRange, double);
  //@}

  //@{
  /**
   * The overlap of adjacent tiles.
   */
  vtkSetClampMacro(Overlap, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Overlap, double);
  //@}

  //@{
  /**
   * Whether to force image sizes to a power of two.
   */
  vtkSetMacro(PowerOfTwoSize, bool);
  vtkGetMacro(PowerOfTwoSize, bool);
  vtkBooleanMacro(PowerOfTwoSize, bool);
  //@}

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

  class vtkProgressObserver;
  vtkProgressObserver* ProgressObserver;

private:
  vtkGeoAlignedImageSource(const vtkGeoAlignedImageSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoAlignedImageSource&) VTK_DELETE_FUNCTION;
};

#endif
