/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenQubeElectronicData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenQubeElectronicData
 * @brief   Provides access to and storage of
 * electronic data calculated by OpenQube.
*/

#ifndef vtkOpenQubeElectronicData_h
#define vtkOpenQubeElectronicData_h

#include "vtkDomainsChemistryModule.h" // For export macro
#include "vtkAbstractElectronicData.h"
#include "vtkNew.h" // for vtkNew

namespace OpenQube {
  class BasisSet;
  class Cube;
}

class vtkImageData;
class vtkDataSetCollection;

class VTKDOMAINSCHEMISTRY_EXPORT vtkOpenQubeElectronicData
    : public vtkAbstractElectronicData
{
public:
  static vtkOpenQubeElectronicData *New();
  vtkTypeMacro(vtkOpenQubeElectronicData,vtkAbstractElectronicData);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Returns the number of molecular orbitals in the OpenQube::BasisSet.
   */
  vtkIdType GetNumberOfMOs();

  /**
   * Returns the number of electrons in the molecule.
   */
  unsigned int GetNumberOfElectrons();

  /**
   * Returns the vtkImageData for the requested molecular orbital. The data
   * will be calculated when first requested, and cached for later requests.
   */
  vtkImageData * GetMO(vtkIdType orbitalNumber);

  /**
   * Returns vtkImageData for the molecule's electron density. The data
   * will be calculated when first requested, and cached for later requests.
   */
  vtkImageData * GetElectronDensity();

  //@{
  /**
   * Set/Get the OpenQube::BasisSet object used to generate the image data
   */
  vtkSetMacro(BasisSet, OpenQube::BasisSet*);
  vtkGetMacro(BasisSet, OpenQube::BasisSet*);
  //@}

  //@{
  /**
   * Set/Get the padding around the molecule used in determining the image
   * limits. Default: 2.0
   */
  vtkSetMacro(Padding, double);
  vtkGetMacro(Padding, double);
  //@}

  //@{
  /**
   * Set/Get the interval distance between grid points. Default: 0.1
   */
  vtkSetMacro(Spacing, double);
  vtkGetMacro(Spacing, double);
  //@}

  //@{
  /**
   * Get the collection of cached images
   */
  vtkGetNewMacro(Images, vtkDataSetCollection);
  //@}

  /**
   * Deep copies the data object into this.
   */
  virtual void DeepCopy(vtkDataObject *obj);

protected:
  vtkOpenQubeElectronicData();
  ~vtkOpenQubeElectronicData();

  //@{
  /**
   * Calculates and returns the requested vtkImageData. The data is added to
   * the cache, but the cache is not searched in this function.
   */
  vtkImageData * CalculateMO(vtkIdType orbitalNumber);
  vtkImageData * CalculateElectronDensity();
  //@}

  /**
   * Converts an OpenQube::Cube object into vtkImageData.
   */
  void FillImageDataFromQube(OpenQube::Cube *qube,
                             vtkImageData *image);

  /**
   * Cache of calculated image data.
   */
  vtkNew<vtkDataSetCollection> Images;

  /**
   * The OpenQube::BasisSet object used to calculate the images.
   */
  OpenQube::BasisSet *BasisSet;

  /**
   * Used to determine the spacing of the image data.
   */
  double Spacing;

private:
  vtkOpenQubeElectronicData(const vtkOpenQubeElectronicData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenQubeElectronicData&) VTK_DELETE_FUNCTION;
};

#endif
