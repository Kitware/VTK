/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDefaultPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDefaultPainter
 * @brief   sets up a default chain of painters.
 *
 *
 * This painter does not do any actual rendering.
 * Sets up a default pipeline of painters to mimick the behaiour of
 * old vtkPolyDataMapper. The chain is as follows:
 * input--> vtkScalarsToColorsPainter --> vtkClipPlanesPainter -->
 * vtkDisplayListPainter --> vtkCompositePainter -->
 * vtkCoincidentTopologyResolutionPainter -->
 * vtkLightingPainter --> vtkRepresentationPainter -->
 * \<Delegate of vtkDefaultPainter\>.
 * Typically, the delegate of the default painter be one that is capable of r
 * rendering graphics primitives or a vtkChooserPainter which can select appropriate
 * painters to do the rendering.
*/

#ifndef vtkDefaultPainter_h
#define vtkDefaultPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPainter.h"

class vtkClipPlanesPainter;
class vtkCoincidentTopologyResolutionPainter;
class vtkCompositePainter;
class vtkDisplayListPainter;
class vtkLightingPainter;
class vtkRepresentationPainter;
class vtkScalarsToColorsPainter;

class VTKRENDERINGOPENGL_EXPORT vtkDefaultPainter : public vtkPainter
{
public:
  static vtkDefaultPainter *New();
  vtkTypeMacro(vtkDefaultPainter, vtkPainter);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the painter that maps scalars to colors.
   */
  void SetScalarsToColorsPainter(vtkScalarsToColorsPainter*);
  vtkGetObjectMacro(ScalarsToColorsPainter, vtkScalarsToColorsPainter);
  //@}

  //@{
  /**
   * Get/Set the painter that handles clipping.
   */
  void SetClipPlanesPainter(vtkClipPlanesPainter*);
  vtkGetObjectMacro(ClipPlanesPainter, vtkClipPlanesPainter);
  //@}

  //@{
  /**
   * Get/Set the painter that builds display lists.
   */
  void SetDisplayListPainter(vtkDisplayListPainter*);
  vtkGetObjectMacro(DisplayListPainter, vtkDisplayListPainter);
  //@}

  //@{
  /**
   * Get/Set the painter used to handle composite datasets.
   */
  void SetCompositePainter(vtkCompositePainter*);
  vtkGetObjectMacro(CompositePainter, vtkCompositePainter);
  //@}

  //@{
  /**
   * Painter used to resolve coincident topology.
   */
  void SetCoincidentTopologyResolutionPainter(
    vtkCoincidentTopologyResolutionPainter*);
  vtkGetObjectMacro(CoincidentTopologyResolutionPainter,
    vtkCoincidentTopologyResolutionPainter);
  //@}

  //@{
  /**
   * Get/Set the painter that controls lighting.
   */
  void SetLightingPainter(vtkLightingPainter*);
  vtkGetObjectMacro(LightingPainter, vtkLightingPainter);
  //@}

  //@{
  /**
   * Painter used to convert polydata to Wireframe/Points representation.
   */
  void SetRepresentationPainter(vtkRepresentationPainter*);
  vtkGetObjectMacro(RepresentationPainter, vtkRepresentationPainter);
  //@}

  /**
   * Set/Get the painter to which this painter should propagare its draw calls.
   * These methods are overridden so that the delegate is set
   * to the end of the Painter Chain.
   */
  void SetDelegatePainter(vtkPainter*) VTK_OVERRIDE;
  vtkPainter* GetDelegatePainter() VTK_OVERRIDE { return this->DefaultPainterDelegate; }

  /**
   * Overridden to setup the chain of painter depending on the
   * actor representation. The chain is rebuilt if
   * this->MTime has changed
   * since last BuildPainterChain();
   * Building of the chain does not depend on input polydata,
   * hence it does not check if the input has changed at all.
   */
  void Render(vtkRenderer* renderer, vtkActor* actor,
                      unsigned long typeflags, bool forceCompileOnly) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this painter.
   * The parameter window could be used to determine which graphic
   * resources to release.
   * The call is propagated to the delegate painter, if any.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Expand or shrink the estimated bounds based on the geometric
   * transformations applied in the painter. The bounds are left unchanged
   * if the painter does not change the geometry.
   */
  void UpdateBounds(double bounds[6]) VTK_OVERRIDE;

protected:
  vtkDefaultPainter();
  ~vtkDefaultPainter() VTK_OVERRIDE;

  /**
   * Setups the the painter chain.
   */
  virtual void BuildPainterChain();

  /**
   * Take part in garbage collection.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

  vtkScalarsToColorsPainter* ScalarsToColorsPainter;
  vtkClipPlanesPainter* ClipPlanesPainter;
  vtkDisplayListPainter* DisplayListPainter;
  vtkCompositePainter* CompositePainter;
  vtkCoincidentTopologyResolutionPainter* CoincidentTopologyResolutionPainter;
  vtkLightingPainter* LightingPainter;
  vtkRepresentationPainter* RepresentationPainter;
  vtkTimeStamp ChainBuildTime;

  vtkPainter* DefaultPainterDelegate;
  void  SetDefaultPainterDelegate(vtkPainter*);

private:
  vtkDefaultPainter(const vtkDefaultPainter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkDefaultPainter &) VTK_DELETE_FUNCTION;
};

#endif //_vtkDefaultPainter_h
