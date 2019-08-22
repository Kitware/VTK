/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSubsetWithSeed.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkExtractSubsetWithSeed
 * @brief extract a line or plane in the ijk space starting with a seed
 *
 * vtkExtractSubsetWithSeed is a filter that can extract a line or a plane
 * in the i-j-k space starting with a seed point. The filter supports cases
 * where the structured grid is split up into multiple blocks (across multiple
 * ranks). It also handles cases were the ijk origin for each the blocks is not
 * aligned.
 *
 * The implementation starts with the seed point and then extracts a line
 * in the chosen direction. Then, using the face center for the terminal
 * faces as the new seeds it continues seeding and extracting until a seed can
 * no longer extract a new grid. The same principle holds when extracting a
 * plane, except in that case multiple seeds are generated using face centers
 * for each face alone the plane edges.
 */

#ifndef vtkExtractSubsetWithSeed_h
#define vtkExtractSubsetWithSeed_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // for export macros

class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkExtractSubsetWithSeed : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractSubsetWithSeed* New();
  vtkTypeMacro(vtkExtractSubsetWithSeed, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the extraction seed point. This is specified in world coordinates
   * i.e. x-y-z space.
   */
  vtkSetVector3Macro(Seed, double);
  vtkGetVector3Macro(Seed, double);
  //@}

  enum
  {
    LINE_I = 0,
    LINE_J,
    LINE_K,
    PLANE_IJ,
    PLANE_JK,
    PLANE_KI,
  };
  //@{
  /**
   * Get/Set the directions in the ijk spaced to extract starting with the
   * seed.
   */
  vtkSetClampMacro(Direction, int, LINE_I, PLANE_KI);
  vtkGetMacro(Direction, int);
  void SetDirectionToLineI() { this->SetDirection(LINE_I); }
  void SetDirectionToLineJ() { this->SetDirection(LINE_J); }
  void SetDirectionToLineK() { this->SetDirection(LINE_K); }
  void SetDirectionToPlaneIJ() { this->SetDirection(PLANE_IJ); }
  void SetDirectionToPlaneJK() { this->SetDirection(PLANE_JK); }
  void SetDirectionToPlaneKI() { this->SetDirection(PLANE_KI); }
  //@}

  //@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}
protected:
  vtkExtractSubsetWithSeed();
  ~vtkExtractSubsetWithSeed() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkExtractSubsetWithSeed(const vtkExtractSubsetWithSeed&) = delete;
  void operator=(const vtkExtractSubsetWithSeed&) = delete;

  double Seed[3] = { 0, 0, 0 };
  int Direction = LINE_I;
  vtkMultiProcessController* Controller = nullptr;
};

#endif
