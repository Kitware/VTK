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
/**
 * @class   vtkTerrainDataPointPlacer
 * @brief   Place points on terrain data
 *
 *
 * vtkTerrainDataPointPlacer dictates the placement of points on height field
 * data. The class takes as input the list of props that represent the terrain
 * in a rendered scene. A height offset can be specified to dicatate the
 * placement of points at a certain height above the surface.
 *
 * @par Usage:
 * A typical usage of this class is as follows:
 * \code
 * pointPlacer->AddProp(demActor);    // the actor(s) containing the terrain.
 * rep->SetPointPlacer(pointPlacer);
 * pointPlacer->SetHeightOffset( 100 );
 * \endcode
 *
 * @sa
 * vtkPointPlacer vtkTerrainContourLineInterpolator
*/

#ifndef vtkTerrainDataPointPlacer_h
#define vtkTerrainDataPointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkPropCollection;
class vtkProp;
class vtkPropPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkTerrainDataPointPlacer : public vtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static vtkTerrainDataPointPlacer *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkTerrainDataPointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  // Descuription:
  // Add an actor (that represents a terrain in a rendererd scene) to the
  // list. Only props in this list are considered by the PointPlacer
  virtual void AddProp( vtkProp * );
  virtual void RemoveAllProps();

  //@{
  /**
   * This is the height above (or below) the terrain that the dictated
   * point should be placed. Positive values indicate distances above the
   * terrain; negative values indicate distances below the terrain. The
   * default is 0.0.
   */
  vtkSetMacro(HeightOffset,double);
  vtkGetMacro(HeightOffset,double);
  //@}

  /**
   * Given a renderer and a display position in pixel coordinates,
   * compute the world position and orientation where this point
   * will be placed. This method is typically used by the
   * representation to place the point initially.
   * For the Terrain point placer this computes world points that
   * lie at the specified height above the terrain.
   */
  int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double worldPos[3],
                                    double worldOrient[9] ) VTK_OVERRIDE;

  /**
   * Given a renderer, a display position, and a reference world
   * position, compute the new world position and orientation
   * of this point. This method is typically used by the
   * representation to move the point.
   */
  int ComputeWorldPosition( vtkRenderer *ren,
                                    double displayPos[2],
                                    double refWorldPos[3],
                                    double worldPos[3],
                                    double worldOrient[9] ) VTK_OVERRIDE;

  /**
   * Given a world position check the validity of this
   * position according to the constraints of the placer
   */
  int ValidateWorldPosition( double worldPos[3] ) VTK_OVERRIDE;

  /**
   * Given a display position, check the validity of this position.
   */
  int ValidateDisplayPosition( vtkRenderer *, double displayPos[2] ) VTK_OVERRIDE;

  /**
   * Given a world position and a world orientation,
   * validate it according to the constraints of the placer.
   */
  int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] ) VTK_OVERRIDE;

  //@{
  /**
   * Get the Prop picker.
   */
  vtkGetObjectMacro( PropPicker, vtkPropPicker );
  //@}

protected:
  vtkTerrainDataPointPlacer();
  ~vtkTerrainDataPointPlacer() VTK_OVERRIDE;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  vtkPropCollection  *TerrainProps;
  vtkPropPicker      *PropPicker;
  double              HeightOffset;

private:
  vtkTerrainDataPointPlacer(const vtkTerrainDataPointPlacer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTerrainDataPointPlacer&) VTK_DELETE_FUNCTION;
};

#endif
