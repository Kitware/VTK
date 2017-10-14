/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCentersPointPlacer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellCentersPointPlacer
 * @brief   Snaps points at the center of a cell
 *
 *
 * vtkCellCentersPointPlacer is a class to snap points on the center of cells.
 * The class has 3 modes. In the ParametricCenter mode, it snaps points
 * to the parametric center of the cell (see vtkCell). In CellPointsMean
 * mode, points are snapped to the mean of the points in the cell.
 * In 'None' mode, no snapping is performed. The computed world position
 * is the picked position within the cell.
 *
 * @par Usage:
 * The actors that render data and wish to be considered for placement
 * by this placer are added to the list as
 * \code
 * placer->AddProp( actor );
 * \endcode
 *
 * @sa
 * vtkPointPlacer
*/

#ifndef vtkCellCentersPointPlacer_h
#define vtkCellCentersPointPlacer_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPointPlacer.h"

class vtkRenderer;
class vtkPropCollection;
class vtkProp;
class vtkCellPicker;

class VTKINTERACTIONWIDGETS_EXPORT vtkCellCentersPointPlacer : public vtkPointPlacer
{
public:
  /**
   * Instantiate this class.
   */
  static vtkCellCentersPointPlacer *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkCellCentersPointPlacer,vtkPointPlacer);
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
  vtkGetObjectMacro( CellPicker, vtkCellPicker );
  //@}

  //@{
  /**
   * Modes to change the point placement. Parametric center picks
   * the parametric center within the cell. CellPointsMean picks
   * the average of all points in the cell. When the mode is None,
   * the input point is passed through unmodified. Default is CellPointsMean.
   */
  vtkSetMacro( Mode, int );
  vtkGetMacro( Mode, int );
  //@}

  enum
  {
    ParametricCenter = 0,
    CellPointsMean,
    None
  };

protected:
  vtkCellCentersPointPlacer();
  ~vtkCellCentersPointPlacer() override;

  // The props that represents the terrain data (one or more) in a rendered
  // scene
  vtkPropCollection  *PickProps;
  vtkCellPicker      *CellPicker;
  int                 Mode;

private:
  vtkCellCentersPointPlacer(const vtkCellCentersPointPlacer&) = delete;
  void operator=(const vtkCellCentersPointPlacer&) = delete;
};

#endif
