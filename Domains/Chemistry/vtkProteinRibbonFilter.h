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
// vtkProteinRibbonFilter is an poly data algorithm which generates
// protein ribbons.

#include "vtkDomainsChemistryModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

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

  void CreateThinStrip(vtkPolyData* poly, vtkUnsignedCharArray *faceColors,
                       vtkPoints* p, std::vector<std::pair<vtkVector3f, bool> >& p1,
                       std::vector<std::pair<vtkVector3f, bool> >& p2,
                       std::vector<unsigned int> &colors);

  void CreateAtomAsSphere(vtkPolyData* poly, vtkUnsignedCharArray *faceColors,
                          double *pos, unsigned int colors, float radius,
                          float scale);

  static std::vector<vtkVector3f>* Subdivide(std::vector<std::pair<vtkVector3f, bool> >& p,
                                             int div);

  void SetColorByAtom( std::vector<unsigned int>& colors, vtkStringArray* atomTypes);

  void SetColorByStructure(std::vector<unsigned int>& colors,
                           vtkStringArray* atomTypes, vtkUnsignedCharArray* ss,
                           unsigned int helixColor = 0xFF0080,
                           unsigned int sheetColor = 0xFFC800);

  float CoilWidth;
  float HelixWidth;
  int SphereResolution;
  int SubdivideFactor;

private:
  vtkProteinRibbonFilter(const vtkProteinRibbonFilter&);  // Not implemented.
  void operator=(const vtkProteinRibbonFilter&);  // Not implemented.
};

#endif // _vtkProteinRibbonFilter_h
