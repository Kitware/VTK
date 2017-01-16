/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProteinRibbonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkProteinRibbonFilter_h
#define vtkProteinRibbonFilter_h

/**
 * @class   vtkProteinRibbonFilter
 * @brief   generates protein ribbons
 *
 * vtkProteinRibbonFilter is a polydata algorithm that generates protein
 * ribbons.
*/

#include "vtkDomainsChemistryModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkColor.h" // For vtkColor3ub.
#include <map> // For element to color map.

class vtkVector3f;
class vtkStringArray;

class VTKDOMAINSCHEMISTRY_EXPORT vtkProteinRibbonFilter
  : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkProteinRibbonFilter, vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkProteinRibbonFilter* New();

  //@{
  /**
   * Width of the ribbon coil. Default is 0.3.
   */
  vtkGetMacro(CoilWidth, float);
  vtkSetMacro(CoilWidth, float);
  //@}

  //@{
  /**
   * Width of the helix part of the ribbon. Default is 1.3.
   */
  vtkGetMacro(HelixWidth, float);
  vtkSetMacro(HelixWidth, float);
  //@}

  //@{
  /**
   * Smoothing factor of the ribbon. Default is 20.
   */
  vtkGetMacro(SubdivideFactor, int);
  vtkSetMacro(SubdivideFactor, int);
  //@}

  //@{
  /**
   * If enabled, small molecules (HETATMs) are drawn as spheres. Default is true.
   */
  vtkGetMacro(DrawSmallMoleculesAsSpheres, bool);
  vtkSetMacro(DrawSmallMoleculesAsSpheres, bool);
  //@}

  //@{
  /**
   * Resolution of the spheres for small molecules. Default is 20.
   */
  vtkGetMacro(SphereResolution, int);
  vtkSetMacro(SphereResolution, int);
  //@}

protected:
  vtkProteinRibbonFilter();
  ~vtkProteinRibbonFilter() VTK_OVERRIDE;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  void CreateThinStrip(vtkPolyData* poly, vtkUnsignedCharArray *pointsColors,
                       vtkPoints* p, std::vector<std::pair<vtkVector3f, bool> >& p1,
                       std::vector<std::pair<vtkVector3f, bool> >& p2,
                       std::vector<vtkColor3ub> &colors);

  void CreateAtomAsSphere(vtkPolyData* poly, vtkUnsignedCharArray *pointsColors,
                          double *pos, const vtkColor3ub& color, float radius,
                          float scale);

  static std::vector<vtkVector3f>* Subdivide(std::vector<std::pair<vtkVector3f, bool> >& p,
                                             int div);

  void SetColorByAtom(std::vector<vtkColor3ub>& colors, vtkStringArray* atomTypes);

  void SetColorByStructure(std::vector<vtkColor3ub>& colors,
                           vtkStringArray* atomTypes, vtkUnsignedCharArray* ss,
                           const vtkColor3ub& helixColor,
                           const vtkColor3ub& sheetColor);

  std::map<std::string, vtkColor3ub> ElementColors;

  float CoilWidth;
  float HelixWidth;
  int SphereResolution;
  int SubdivideFactor;
  bool DrawSmallMoleculesAsSpheres;

private:
  vtkProteinRibbonFilter(const vtkProteinRibbonFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkProteinRibbonFilter&) VTK_DELETE_FUNCTION;
};

#endif // vtkProteinRibbonFilter_h
