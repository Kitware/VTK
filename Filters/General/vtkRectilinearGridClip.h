/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearGridClip
 * @brief   Reduces the image extent of the input.
 *
 * vtkRectilinearGridClip  will make an image smaller.  The output must have
 * an image extent which is the subset of the input.  The filter has two
 * modes of operation:
 * 1: By default, the data is not copied in this filter.
 * Only the whole extent is modified.
 * 2: If ClipDataOn is set, then you will get no more that the clipped
 * extent.
*/

#ifndef vtkRectilinearGridClip_h
#define vtkRectilinearGridClip_h

// I did not make this a subclass of in place filter because
// the references on the data do not matter. I make no modifications
// to the data.
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkRectilinearGridAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkRectilinearGridClip : public vtkRectilinearGridAlgorithm
{
public:
  static vtkRectilinearGridClip *New();
  vtkTypeMacro(vtkRectilinearGridClip,vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The whole extent of the output has to be set explicitly.
   */
  void SetOutputWholeExtent(int extent[6], vtkInformation *outInfo=0);
  void SetOutputWholeExtent(int minX, int maxX, int minY, int maxY,
                            int minZ, int maxZ);
  void GetOutputWholeExtent(int extent[6]);
  int *GetOutputWholeExtent() {return this->OutputWholeExtent;}
  //@}

  void ResetOutputWholeExtent();

  //@{
  /**
   * By default, ClipData is off, and only the WholeExtent is modified.
   * the data's extent may actually be larger.  When this flag is on,
   * the data extent will be no more than the OutputWholeExtent.
   */
  vtkSetMacro(ClipData, int);
  vtkGetMacro(ClipData, int);
  vtkBooleanMacro(ClipData, int);
  //@}

protected:
  vtkRectilinearGridClip();
  ~vtkRectilinearGridClip() VTK_OVERRIDE {}

  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputWholeExtent[6];

  int ClipData;

  int RequestInformation (vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  void CopyData(vtkRectilinearGrid *inData, vtkRectilinearGrid *outData, int *ext);

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkRectilinearGridClip(const vtkRectilinearGridClip&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRectilinearGridClip&) VTK_DELETE_FUNCTION;
};



#endif



