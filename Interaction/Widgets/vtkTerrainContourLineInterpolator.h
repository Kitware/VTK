/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTerrainContourLineInterpolator - Contour interpolator for DEM data.
//
// .SECTION Description 
// vtkTerrainContourLineInterpolator interpolates nodes on height field data.
// The class is meant to be used in conjunciton with a vtkContourWidget, 
// enabling you to draw paths on terrain data. The class internally uses a 
// vtkProjectedTerrainPath. Users can set kind of interpolation
// desired between two node points by setting the modes of the this filter.
// For instance:
//
// \code
// contourRepresentation->SetLineInterpolator(interpolator);
// interpolator->SetImageData( demDataFile );
// interpolator->GetProjector()->SetProjectionModeToHug();
// interpolator->SetHeightOffset(25.0);
// \endcode
//
// You are required to set the ImageData to this class as the height-field 
// image.
//
// .SECTION See Also
// vtkTerrainDataPointPlacer vtkProjectedTerrainPath

#ifndef __vtkTerrainContourLineInterpolator_h
#define __vtkTerrainContourLineInterpolator_h

#include "vtkContourLineInterpolator.h"

class vtkImageData;
class vtkProjectedTerrainPath;

class VTK_WIDGETS_EXPORT vtkTerrainContourLineInterpolator 
                       : public vtkContourLineInterpolator
{
public:
  // Description:
  // Instantiate this class.
  static vtkTerrainContourLineInterpolator *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkTerrainContourLineInterpolator,
                              vtkContourLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Interpolate to create lines between contour nodes idx1 and idx2.
  // Depending on the projection mode, the interpolated line may either 
  // hug the terrain, just connect the two points with a straight line or 
  // a non-occluded interpolation. 
  // Used internally by vtkContourRepresentation.
  virtual int InterpolateLine( vtkRenderer *ren, 
                               vtkContourRepresentation *rep,
                               int idx1, int idx2 );
  
  // Description:
  // The interpolator is given a chance to update the node.
  // Used internally by vtkContourRepresentation
  // Returns 0 if the node (world position) is unchanged.
  virtual int UpdateNode( vtkRenderer *, 
                          vtkContourRepresentation *,
                          double * vtkNotUsed(node), int vtkNotUsed(idx) );

  // Description:
  // Set the height field data. The height field data is a 2D image. The
  // scalars in the image represent the height field. This must be set.
  virtual void SetImageData(vtkImageData *);
  vtkGetObjectMacro(ImageData, vtkImageData);

  // Description:
  // Get the vtkProjectedTerrainPath operator used to project the terrain
  // onto the data. This operator has several modes, See the documentation
  // of vtkProjectedTerrainPath. The default mode is to hug the terrain
  // data at 0 height offset.
  vtkGetObjectMacro(Projector, vtkProjectedTerrainPath);

protected:
  vtkTerrainContourLineInterpolator();
  ~vtkTerrainContourLineInterpolator();

  vtkImageData              *ImageData; // height field data
  vtkProjectedTerrainPath   *Projector; 

private:
  vtkTerrainContourLineInterpolator(const vtkTerrainContourLineInterpolator&);  //Not implemented
  void operator=(const vtkTerrainContourLineInterpolator&);  //Not implemented
};

#endif
