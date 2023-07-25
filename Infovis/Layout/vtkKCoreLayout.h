// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkKCoreLayout
 * @brief   Produces a layout for a graph labeled with K-Core
 *                        information.
 *
 *
 * vtkKCoreLayout creates coordinates for each vertex that can be used to
 * perform a layout for a k-core view.
 * Prerequisite:  Vertices must have an attribute array containing their
 *                k-core number.  This layout is based on the algorithm
 *                described in the paper: "k-core decomposition: a tool
 *                for the visualization of large scale networks", by
 *                Ignacio Alvarez-Hamelin et. al.
 *
 *                This graph algorithm appends either polar coordinates or cartesian coordinates
 *                as vertex attributes to the graph giving the position of the vertex in a layout.
 *                Input graphs must have the k-core number assigned to each vertex (default
 *                attribute array storing kcore numbers is "kcore").
 *
 *                Epsilon - this factor is used to adjust the amount vertices are 'pulled' out of
 *                          their default ring radius based on the number of neighbors in higher
 *                          cores.  Default=0.2
 *                UnitRadius - Tweaks the base unit radius value.  Default=1.0
 *
 *                Still TODO: Still need to work on the connected-components within each shell and
 *                            associated layout issues with that.
 *
 * Input port 0: graph
 *
 * @par Thanks:
 * Thanks to William McLendon from Sandia National Laboratories for providing this
 * implementation.
 */

#ifndef vtkKCoreLayout_h
#define vtkKCoreLayout_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisLayoutModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkKCoreLayout : public vtkGraphAlgorithm
{
public:
  static vtkKCoreLayout* New();
  vtkTypeMacro(vtkKCoreLayout, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Convenience function provided for setting the graph input.
  void SetGraphConnection(vtkAlgorithmOutput*);

  vtkKCoreLayout();
  ~vtkKCoreLayout() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  ///@{
  /**
   * Set the name of the vertex attribute array storing k-core labels.
   * Default: kcore
   */
  vtkSetStringMacro(KCoreLabelArrayName);
  ///@}

  ///@{
  /**
   * Output polar coordinates for vertices if True.  Default column names are
   * coord_radius, coord_angle.
   * Default: False
   */
  vtkGetMacro(Polar, bool);
  vtkSetMacro(Polar, bool);
  vtkBooleanMacro(Polar, bool);
  ///@}

  ///@{
  /**
   * Set whether or not to convert output to cartesian coordinates.  If false, coordinates
   * will be returned in polar coordinates (radius, angle).
   * Default: True
   */
  vtkGetMacro(Cartesian, bool);
  vtkSetMacro(Cartesian, bool);
  vtkBooleanMacro(Cartesian, bool);
  ///@}

  ///@{
  /**
   * Polar coordinates array name for radius values.
   * This is only used if OutputCartesianCoordinates is False.
   * Default: coord_radius
   */
  vtkSetStringMacro(PolarCoordsRadiusArrayName);
  vtkGetStringMacro(PolarCoordsRadiusArrayName);
  ///@}

  ///@{
  /**
   * Polar coordinates array name for angle values in radians.
   * This is only used if OutputCartesianCoordinates is False.
   * Default: coord_angle
   */
  vtkSetStringMacro(PolarCoordsAngleArrayName);
  vtkGetStringMacro(PolarCoordsAngleArrayName);
  ///@}

  ///@{
  /**
   * Cartesian coordinates array name for the X coordinates.
   * This is only used if OutputCartesianCoordinates is True.
   * Default: coord_x
   */
  vtkSetStringMacro(CartesianCoordsXArrayName);
  vtkGetStringMacro(CartesianCoordsXArrayName);
  ///@}

  ///@{
  /**
   * Cartesian coordinates array name for the Y coordinates.
   * This is only used if OutputCartesianCoordinates is True.
   * Default: coord_y
   */
  vtkSetStringMacro(CartesianCoordsYArrayName);
  vtkGetStringMacro(CartesianCoordsYArrayName);
  ///@}

  ///@{
  /**
   * Epsilon value used in the algorithm.
   * Default = 0.2
   */
  vtkSetMacro(Epsilon, float);
  vtkGetMacro(Epsilon, float);
  ///@}

  ///@{
  /**
   * Unit Radius value used in the algorithm.
   * Default = 1.0
   */
  vtkSetMacro(UnitRadius, float);
  vtkGetMacro(UnitRadius, float);
  ///@}

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  char* KCoreLabelArrayName;

  char* PolarCoordsRadiusArrayName;
  char* PolarCoordsAngleArrayName;

  char* CartesianCoordsXArrayName;
  char* CartesianCoordsYArrayName;

  bool Cartesian;
  bool Polar;

  float Epsilon;
  float UnitRadius;

private:
  vtkKCoreLayout(const vtkKCoreLayout&) = delete;
  void operator=(const vtkKCoreLayout&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
