// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageSeedConnectivity
 * @brief   SeedConnectivity with user defined seeds.
 *
 * vtkImageSeedConnectivity marks pixels connected to user supplied seeds.
 * The input must be unsigned char, and the output is also unsigned char.  If
 * a seed supplied by the user does not have pixel value "InputTrueValue",
 * then the image is scanned +x, +y, +z until a pixel is encountered with
 * value "InputTrueValue".  This new pixel is used as the seed .  Any pixel
 * with out value "InputTrueValue" is consider off.  The output pixels values
 * are 0 for any off pixel in input, "OutputTrueValue" for any pixels
 * connected to seeds, and "OutputUnconnectedValue" for any on pixels not
 * connected to seeds.  The same seeds are used for all images in the image
 * set.
 */

#ifndef vtkImageSeedConnectivity_h
#define vtkImageSeedConnectivity_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingMorphologicalModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageConnector;
class vtkImageConnectorSeed;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageSeedConnectivity : public vtkImageAlgorithm
{
public:
  static vtkImageSeedConnectivity* New();
  vtkTypeMacro(vtkImageSeedConnectivity, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods for manipulating the seed pixels.
   */
  void RemoveAllSeeds();
  void AddSeed(int num, int* index);
  void AddSeed(int i0, int i1, int i2);
  void AddSeed(int i0, int i1);
  ///@}

  ///@{
  /**
   * Set/Get what value is considered as connecting pixels.
   */
  vtkSetMacro(InputConnectValue, unsigned char);
  vtkGetMacro(InputConnectValue, unsigned char);
  ///@}

  ///@{
  /**
   * Set/Get the value to set connected pixels to.
   */
  vtkSetMacro(OutputConnectedValue, unsigned char);
  vtkGetMacro(OutputConnectedValue, unsigned char);
  ///@}

  ///@{
  /**
   * Set/Get the value to set unconnected pixels to.
   */
  vtkSetMacro(OutputUnconnectedValue, unsigned char);
  vtkGetMacro(OutputUnconnectedValue, unsigned char);
  ///@}

  ///@{
  /**
   * Get the vtkImageCOnnector used by this filter.
   */
  vtkGetObjectMacro(Connector, vtkImageConnector);
  ///@}

  ///@{
  /**
   * Set the number of axes to use in connectivity.
   */
  vtkSetMacro(Dimensionality, int);
  vtkGetMacro(Dimensionality, int);
  ///@}

protected:
  vtkImageSeedConnectivity();
  ~vtkImageSeedConnectivity() override;

  unsigned char InputConnectValue;
  unsigned char OutputConnectedValue;
  unsigned char OutputUnconnectedValue;
  vtkImageConnectorSeed* Seeds;
  vtkImageConnector* Connector;
  int Dimensionality;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageSeedConnectivity(const vtkImageSeedConnectivity&) = delete;
  void operator=(const vtkImageSeedConnectivity&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
