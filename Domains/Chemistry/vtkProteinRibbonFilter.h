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

#ifndef _vtkProteinRibbonFilter_h
#define _vtkProteinRibbonFilter_h

// .NAME vtkProteinRibbonFilter - generates protein ribbons
// .SECTION Description
// vtkProteinRibbonFilter is a polydata algorithm that generates protein
// ribbons.

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
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkProteinRibbonFilter* New();

  vtkGetMacro(CoilWidth, float);
  vtkSetMacro(CoilWidth, float);

  vtkGetMacro(HelixWidth, float);
  vtkSetMacro(HelixWidth, float);

  vtkGetMacro(SphereResolution, int);
  vtkSetMacro(SphereResolution, int);

  vtkGetMacro(SubdivideFactor, int);
  vtkSetMacro(SubdivideFactor, int);

protected:
  vtkProteinRibbonFilter();
  ~vtkProteinRibbonFilter();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

  void CreateThinStrip(vtkPolyData* poly, vtkUnsignedCharArray *pointsColors,
                       vtkPoints* p, std::vector<std::pair<vtkVector3f, bool> >& p1,
                       std::vector<std::pair<vtkVector3f, bool> >& p2,
                       std::vector<vtkColor3ub> &colors);

  void CreateAtomAsSphere(vtkPolyData* poly, vtkUnsignedCharArray *pointsColors,
                          double *pos, const vtkColor3ub& color, float radius,
                          float scale);

  static std::vector<vtkVector3f>* Subdivide(std::vector<std::pair<vtkVector3f, bool> >& p,
                                             int div);

  void SetColorByAtom( std::vector<vtkColor3ub>& colors, vtkStringArray* atomTypes);

  void SetColorByStructure(std::vector<vtkColor3ub>& colors,
                           vtkStringArray* atomTypes, vtkUnsignedCharArray* ss,
                           const vtkColor3ub& helixColor,
                           const vtkColor3ub& sheetColor);

  std::map<std::string, vtkColor3ub> ElementColors;

  float CoilWidth;
  float HelixWidth;
  int SphereResolution;
  int SubdivideFactor;

private:
  vtkProteinRibbonFilter(const vtkProteinRibbonFilter&);  // Not implemented.
  void operator=(const vtkProteinRibbonFilter&);  // Not implemented.
};

#endif // _vtkProteinRibbonFilter_h
