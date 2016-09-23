/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMoleculeToAtomBallFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMoleculeToAtomBallFilter
 * @brief   Generate polydata with spheres
 * representing atoms
 *
 *
 * This filter is used to generate one sphere for each atom in the
 * input vtkMolecule. Each sphere is centered at the atom center and
 * can be scaled using either covalent or van der Waals radii. The
 * point scalars of the output vtkPolyData contains the atomic number
 * of the appropriate atom for color mapping.
 *
 * \note Consider using the faster, simpler vtkMoleculeMapper class,
 * rather than generating polydata manually via these filters.
 *
 * @sa
 * vtkMoleculeMapper vtkMoleculeToBondStickFilter
*/

#ifndef vtkMoleculeToAtomBallFilter_h
#define vtkMoleculeToAtomBallFilter_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkMoleculeToPolyDataFilter.h"

class vtkMolecule;

class VTKDOMAINSCHEMISTRY_EXPORT vtkMoleculeToAtomBallFilter
  : public vtkMoleculeToPolyDataFilter
{
 public:
  vtkTypeMacro(vtkMoleculeToAtomBallFilter,vtkMoleculeToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkMoleculeToAtomBallFilter *New();

  enum {
    CovalentRadius = 0,
    VDWRadius,
    UnitRadius
  }; // TODO Custom radii from array/fieldData

  vtkGetMacro(RadiusSource, int);
  vtkSetMacro(RadiusSource, int);

  vtkGetMacro(Resolution, int);
  vtkSetMacro(Resolution, int);

  vtkGetMacro(RadiusScale, double);
  vtkSetMacro(RadiusScale, double);

protected:
  vtkMoleculeToAtomBallFilter();
  ~vtkMoleculeToAtomBallFilter();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  int Resolution;
  double RadiusScale;
  int RadiusSource;

private:
  vtkMoleculeToAtomBallFilter(const vtkMoleculeToAtomBallFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMoleculeToAtomBallFilter&) VTK_DELETE_FUNCTION;
};

#endif
