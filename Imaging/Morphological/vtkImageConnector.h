// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageConnector
 * @brief   Create a binary image of a sphere.
 *
 * vtkImageConnector is a helper class for connectivity filters.
 * It is not meant to be used directly.
 * It implements a stack and breadth first search necessary for
 * some connectivity filters.  Filtered axes sets the dimensionality
 * of the neighbor comparison, and
 * cannot be more than three dimensions.
 * As implemented, only voxels which share faces are considered
 * neighbors.
 */

#ifndef vtkImageConnector_h
#define vtkImageConnector_h

#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

//
// Special classes for manipulating data
//
// For the breadth first search
class vtkImageConnectorSeed
{ //;prevent man page generation
public:
  static vtkImageConnectorSeed* New() { return new vtkImageConnectorSeed; }
  void* Pointer;
  int Index[3];
  vtkImageConnectorSeed* Next;
};

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageConnector : public vtkObject
{
public:
  static vtkImageConnector* New();

  vtkTypeMacro(vtkImageConnector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkImageConnectorSeed* NewSeed(int index[3], void* ptr);
  void AddSeed(vtkImageConnectorSeed* seed);
  void AddSeedToEnd(vtkImageConnectorSeed* seed);

  void RemoveAllSeeds();

  ///@{
  /**
   * Values used by the MarkRegion method
   */
  vtkSetMacro(ConnectedValue, unsigned char);
  vtkGetMacro(ConnectedValue, unsigned char);
  vtkSetMacro(UnconnectedValue, unsigned char);
  vtkGetMacro(UnconnectedValue, unsigned char);
  ///@}

  /**
   * Input a data of 0's and "UnconnectedValue"s. Seeds of this object are
   * used to find connected pixels.  All pixels connected to seeds are set to
   * ConnectedValue.  The data has to be unsigned char.
   */
  void MarkData(vtkImageData* data, int dimensionality, int ext[6]);

protected:
  vtkImageConnector();
  ~vtkImageConnector() override;

  unsigned char ConnectedValue;
  unsigned char UnconnectedValue;

  vtkImageConnectorSeed* PopSeed();

  vtkImageConnectorSeed* Seeds;
  vtkImageConnectorSeed* LastSeed;

private:
  vtkImageConnector(const vtkImageConnector&) = delete;
  void operator=(const vtkImageConnector&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
