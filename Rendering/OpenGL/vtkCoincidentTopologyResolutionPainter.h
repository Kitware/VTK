/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoincidentTopologyResolutionPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCoincidentTopologyResolutionPainter
 * @brief   painter that resolves
 * conicident topology.
 *
 * Provides the ability to shift the z-buffer to resolve coincident topology.
 * For example, if you'd like to draw a mesh with some edges a different color,
 * and the edges lie on the mesh, this feature can be useful to get nice
 * looking lines.
*/

#ifndef vtkCoincidentTopologyResolutionPainter_h
#define vtkCoincidentTopologyResolutionPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class vtkInformationIntegerKey;
class vtkInformationDoubleKey;
class vtkInformationDoubleVectorKey;

class VTKRENDERINGOPENGL_EXPORT vtkCoincidentTopologyResolutionPainter :
  public vtkPolyDataPainter
{
public:
  static vtkCoincidentTopologyResolutionPainter* New();
  vtkTypeMacro(vtkCoincidentTopologyResolutionPainter,
    vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Set/Get a global flag that controls whether coincident topology (e.g., a
   * line on top of a polygon) is shifted to avoid z-buffer resolution (and
   * hence rendering problems). If not off, there are two methods to choose
   * from. PolygonOffset uses graphics systems calls to shift polygons, but
   * does not distinguish vertices and lines from one another. ShiftZBuffer
   * remaps the z-buffer to distinguish vertices, lines, and polygons, but
   * does not always produce acceptable results. If you use the ShiftZBuffer
   * approach, you may also want to set the ResolveCoincidentTopologyZShift
   * value. (Note: not all mappers/graphics systems implement this
   * functionality.)
   */
  static vtkInformationIntegerKey* RESOLVE_COINCIDENT_TOPOLOGY();

  /**
   * Used to set the z-shift if ResolveCoincidentTopology is set to
   * ShiftZBuffer.
   */
  static vtkInformationDoubleKey* Z_SHIFT();

  /**
   * Used to set the polygon offset scale factor and units. Used when
   * ResolveCoincidentTopology is set to PolygonOffset.
   */
  static vtkInformationDoubleVectorKey* POLYGON_OFFSET_PARAMETERS();

  /**
   * When set and when RESOLVE_COINCIDENT_TOPOLOGY is set to use polygon offset,
   * solid polygonal faces will be offsetted, otherwise lines/vertices will be
   * offsetted.
   */
  static vtkInformationIntegerKey* POLYGON_OFFSET_FACES();

protected:
  vtkCoincidentTopologyResolutionPainter();
  ~vtkCoincidentTopologyResolutionPainter();

  /**
   * Called before RenderInternal() if the Information has been changed
   * since the last time this method was called.
   */
  virtual void ProcessInformation(vtkInformation*);

  // These are method to set ivars. These are purpisefully protected.
  // The only means to affect these values is thru information object.
  vtkSetMacro(ResolveCoincidentTopology, int);
  vtkSetMacro(ZShift, double);
  vtkSetMacro(OffsetFaces, int);
  void SetPolygonOffsetParameters(double factor, double units)
  {
    if (this->PolygonOffsetFactor != factor ||
      this->PolygonOffsetUnits != units)
    {
      this->PolygonOffsetFactor = factor;
      this->PolygonOffsetUnits = units;
      this->Modified();
    }
  }

  int ResolveCoincidentTopology;
  double PolygonOffsetFactor;
  double PolygonOffsetUnits;
  double ZShift;
  int OffsetFaces;

private:
  vtkCoincidentTopologyResolutionPainter(const vtkCoincidentTopologyResolutionPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCoincidentTopologyResolutionPainter&) VTK_DELETE_FUNCTION;
};


#endif
