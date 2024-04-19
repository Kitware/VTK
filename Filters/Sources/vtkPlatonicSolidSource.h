// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlatonicSolidSource
 * @brief   produce polygonal Platonic solids
 *
 * vtkPlatonicSolidSource can generate each of the five Platonic solids:
 * tetrahedron, cube, octahedron, icosahedron, and dodecahedron. Each of the
 * solids is placed inside a sphere centered at the origin with radius 1.0.
 * To use this class, simply specify the solid to create. Note that this
 * source object creates cell scalars that are (integral value) face numbers.
 */

#ifndef vtkPlatonicSolidSource_h
#define vtkPlatonicSolidSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_SOLID_TETRAHEDRON 0
#define VTK_SOLID_CUBE 1
#define VTK_SOLID_OCTAHEDRON 2
#define VTK_SOLID_ICOSAHEDRON 3
#define VTK_SOLID_DODECAHEDRON 4

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkPlatonicSolidSource : public vtkPolyDataAlgorithm
{
public:
  static vtkPlatonicSolidSource* New();
  vtkTypeMacro(vtkPlatonicSolidSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the type of PlatonicSolid solid to create.
   */
  vtkSetClampMacro(SolidType, int, VTK_SOLID_TETRAHEDRON, VTK_SOLID_DODECAHEDRON);
  vtkGetMacro(SolidType, int);
  void SetSolidTypeToTetrahedron() { this->SetSolidType(VTK_SOLID_TETRAHEDRON); }
  void SetSolidTypeToCube() { this->SetSolidType(VTK_SOLID_CUBE); }
  void SetSolidTypeToOctahedron() { this->SetSolidType(VTK_SOLID_OCTAHEDRON); }
  void SetSolidTypeToIcosahedron() { this->SetSolidType(VTK_SOLID_ICOSAHEDRON); }
  void SetSolidTypeToDodecahedron() { this->SetSolidType(VTK_SOLID_DODECAHEDRON); }
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkPlatonicSolidSource();
  ~vtkPlatonicSolidSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int SolidType;
  int OutputPointsPrecision;

private:
  vtkPlatonicSolidSource(const vtkPlatonicSolidSource&) = delete;
  void operator=(const vtkPlatonicSolidSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
