/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataPointPlacer
 * @brief   Base class to place points given constraints on polygonal data
 *
 *
 * vtkPolyDataPointPlacer is a base class to place points on the surface of
 * polygonal data.
 *
 * @par Usage:
 * The actors that render polygonal data and wish to be considered
 * for placement by this placer are added to the list as
 * \code
 * placer->AddProp( polyDataActor );
 * \endcode
 *
 * @sa
 * vtkPolygonalSurfacePointPlacer
*/

#ifndef vtkPolyDataPointPlacer_h
#define vtkPolyDataPointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkRenderer;
class vtkPropCollection;
class vtkProp;
class vtkPropPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkPolyDataPointPlacer : public vtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static vtkPolyDataPointPlacer *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkPolyDataPointPlacer,vtkPointPlacer);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  // Descuription:
  // Add an actor (that represents a terrain in a rendererd scene) to the
  // list. Only props in this list are considered by the PointPlacer
  virtual void AddProp( vtkProp * );
  virtual void RemoveViewProp(vtkProp *prop);
  virtual void RemoveAllProps();
  int          HasProp( vtkProp * );
  int          GetNumberOfProps();

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
                                    double worldOrient[9] ) override;

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
                                    double worldOrient[9] ) override;

  /**
   * Given a world position check the validity of this
   * position according to the constraints of the placer
   */
  int ValidateWorldPosition( double worldPos[3] ) override;

  /**
   * Given a display position, check the validity of this position.
   */
  int ValidateDisplayPosition( vtkRenderer *, double displayPos[2] ) override;

  /**
   * Given a world position and a world orientation,
   * validate it according to the constraints of the placer.
   */
  int ValidateWorldPosition( double worldPos[3],
                                     double worldOrient[9] ) override;

  //@{
  /**
   * Get the Prop picker.
   */
  vtkGetObjectMacro( PropPicker, vtkPropPicker );
  //@}

protected:
  vtkPolyDataPointPlacer();
  ~vtkPolyDataPointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  vtkPropCollection  *SurfaceProps;
  vtkPropPicker      *PropPicker;

private:
  vtkPolyDataPointPlacer(const vtkPolyDataPointPlacer&) = delete;
  void operator=(const vtkPolyDataPointPlacer&) = delete;
};

#endif

