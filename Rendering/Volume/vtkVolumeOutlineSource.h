/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeOutlineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeOutlineSource
 * @brief   outline of volume cropping region
 *
 * vtkVolumeOutlineSource generates a wireframe outline that corresponds
 * to the cropping region of a vtkVolumeMapper.  It requires a
 * vtkVolumeMapper as input.  The GenerateFaces option turns on the
 * solid faces of the outline, and the GenerateScalars option generates
 * color scalars.  When GenerateScalars is on, it is possible to set
 * an "ActivePlaneId" value in the range [0..6] to highlight one of the
 * six cropping planes.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
*/

#ifndef vtkVolumeOutlineSource_h
#define vtkVolumeOutlineSource_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkVolumeMapper;

class VTKRENDERINGVOLUME_EXPORT vtkVolumeOutlineSource : public vtkPolyDataAlgorithm
{
public:
  static vtkVolumeOutlineSource *New();
  vtkTypeMacro(vtkVolumeOutlineSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the mapper that has the cropping region that the outline will
   * be generated for.  The mapper must have an input, because the
   * bounds of the data must be computed in order to generate the
   * outline.
   */
  virtual void SetVolumeMapper(vtkVolumeMapper *mapper);
  vtkVolumeMapper *GetVolumeMapper() { return this->VolumeMapper; };
  //@}

  //@{
  /**
   * Set whether to generate color scalars for the output.  By default,
   * the output has no scalars and the color must be set in the
   * property of the actor.
   */
  vtkSetMacro(GenerateScalars, int);
  vtkBooleanMacro(GenerateScalars, int);
  vtkGetMacro(GenerateScalars, int);
  //@}

  //@{
  /**
   * Set whether to generate an outline wherever an input face was
   * cut by a plane.  This is on by default.
   */
  vtkSetMacro(GenerateOutline, int);
  vtkBooleanMacro(GenerateOutline, int);
  vtkGetMacro(GenerateOutline, int);
  //@}

  //@{
  /**
   * Set whether to generate polygonal faces for the output.  By default,
   * only lines are generated.  The faces will form a closed, watertight
   * surface.
   */
  vtkSetMacro(GenerateFaces, int);
  vtkBooleanMacro(GenerateFaces, int);
  vtkGetMacro(GenerateFaces, int);
  //@}

  //@{
  /**
   * Set the color of the outline.  This has no effect unless GenerateScalars
   * is On.  The default color is red.
   */
  vtkSetVector3Macro(Color, double);
  vtkGetVector3Macro(Color, double);
  //@}

  //@{
  /**
   * Set the active plane, e.g. to display which plane is currently being
   * modified by an interaction.  Set this to -1 if there is no active plane.
   * The default value is -1.
   */
  vtkSetMacro(ActivePlaneId, int);
  vtkGetMacro(ActivePlaneId, int);
  //@}

  //@{
  /**
   * Set the color of the active cropping plane.  This has no effect unless
   * GenerateScalars is On and ActivePlaneId is non-negative.  The default
   * color is yellow.
   */
  vtkSetVector3Macro(ActivePlaneColor, double);
  vtkGetVector3Macro(ActivePlaneColor, double);
  //@}

protected:
  vtkVolumeOutlineSource();
  ~vtkVolumeOutlineSource() VTK_OVERRIDE;

  vtkVolumeMapper *VolumeMapper;
  int GenerateScalars;
  int GenerateOutline;
  int GenerateFaces;
  int ActivePlaneId;
  double Color[3];
  double ActivePlaneColor[3];

  int Cropping;
  int CroppingRegionFlags;
  double Bounds[6];
  double CroppingRegionPlanes[6];

  static int ComputeCubePlanes(double planes[3][4],
                               double croppingPlanes[6],
                               double bounds[6]);

  static void GeneratePolys(vtkCellArray *polys,
                            vtkUnsignedCharArray *scalars,
                            unsigned char colors[2][3],
                            int activePlane,
                            int flags,
                            int tolPtId[3][4]);

  static void GenerateLines(vtkCellArray *lines,
                            vtkUnsignedCharArray *scalars,
                            unsigned char colors[2][3],
                            int activePlane,
                            int flags,
                            int tolPtId[3][4]);

  static void GeneratePoints(vtkPoints *points,
                             vtkCellArray *lines,
                             vtkCellArray *polys,
                             double planes[3][4],
                             double tol);

  static void NudgeCropPlanesToBounds(int tolPtId[3][4],
                                      double planes[3][4],
                                      double tol);

  static void CreateColorValues(unsigned char colors[2][3],
                                double color1[3], double color2[3]);

  int ComputePipelineMTime(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector,
                                   int requestFromOutputPort,
                                   vtkMTimeType* mtime) VTK_OVERRIDE;

  int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector) VTK_OVERRIDE;

  int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  vtkVolumeOutlineSource(const vtkVolumeOutlineSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeOutlineSource&) VTK_DELETE_FUNCTION;
};

#endif
