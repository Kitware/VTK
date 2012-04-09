/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTerrainDataPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTerrainDataPointPlacer - Place points on terrain data
//
// .SECTION Description
// vtkTerrainDataPointPlacer dictates the placement of points on height field
// data. The class takes as input the list of props that represent the terrain
// in a rendered scene. A height offset can be specified to dicatate the
// placement of points at a certain height above the surface.
//
// .SECTION Usage
// A typical usage of this class is as follows:
// \code
// pointPlacer->AddProp(demActor);    // the actor(s) containing the terrain.
// rep->SetPointPlacer(pointPlacer);
// pointPlacer->SetHeightOffset( 100 );
// \endcode
//
// .SECTION See Also
// vtkPointPlacer vtkTerrainContourLineInterpolator

#ifndef __vtkTerrainDataPointPlacer_h
#define __vtkTerrainDataPointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkPropCollection;
class vtkProp;
class vtkPropPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkTerrainDataPointPlacer : public vtkPointPlacer
{
public:
  // Description:
  // Instantiate this class.
  static vtkTerrainDataPointPlacer *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeMacro(vtkTerrainDataPointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descuription:
  // Add an actor (that represents a terrain in a rendererd scene) to the
  // list. Only props in this list are considered by the PointPlacer
  virtual void AddProp( vtkProp * );
  virtual void RemoveAllProps();

  // Description:
  // This is the height above (or below) the terrain that the dictated
  // point should be placed. Positive values indicate distances above the
  // terrain; negative values indicate distances below the terrain. The
  // default is 0.0.
  vtkSetMacro(HeightOffset,double);
  vtkGetMacro(HeightOffset,double);

  // Description:
  // Given a renderer and a display position in pixel coordinates,
  // compute the world position and orientation where this point
  // will be placed. This method is typically used by the
  // representation to place the point initially.
  // For the Terrain point placer this computes world points that
  // lie at the specified height above the terrain.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double worldPos[3],
                                    double worldOrient[9] );

  // Description:
  // Given a renderer, a display position, and a reference world
  // position, compute the new world position and orientation
  // of this point. This method is typically used by the
  // representation to move the point.
  virtual int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double refWorldPos[3],
                                    double worldPos[3],
                                    double worldOrient[9] );

  // Description:
  // Given a world position check the validity of this
  // position according to the constraints of the placer
  virtual int ValidateWorldPosition( double worldPos[3] );

  // Description:
  // Given a display position, check the validity of this position.
  virtual int ValidateDisplayPosition( vtkRenderer *, double displayPos[2] );

  // Description:
  // Given a world position and a world orientation,
  // validate it according to the constraints of the placer.
  virtual int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] );

  // Description:
  // Get the Prop picker.
  vtkGetObjectMacro( PropPicker, vtkPropPicker );

protected:
  vtkTerrainDataPointPlacer();
  ~vtkTerrainDataPointPlacer();

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  vtkPropCollection  *TerrainProps;
  vtkPropPicker      *PropPicker;
  double              HeightOffset;

private:
  vtkTerrainDataPointPlacer(const vtkTerrainDataPointPlacer&);  //Not implemented
  void operator=(const vtkTerrainDataPointPlacer&);  //Not implemented
};

#endif
