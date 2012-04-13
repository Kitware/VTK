/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractCTHPart - Generates surface of an CTH volume fraction.
// .SECTION Description
// vtkExtractCTHPart is a filter that is specialized for creating
// visualization of a CTH simulation.  First it converts the cell data
// to point data.  It contours the selected volume fraction at a value
// of 0.5.  The user has the option of clipping the part with a plane.
// Clipped surfaces of the part are generated.

#ifndef __vtkExtractCTHPart_h
#define __vtkExtractCTHPart_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkAppendPolyData;
class vtkBoundingBox;
class vtkClipPolyData;
class vtkContourFilter;
class vtkCutter;
class vtkDataArray;
class vtkDataSet;
class vtkDataSetSurfaceFilter;
class vtkDoubleArray;
class vtkExtractCTHPartInternal;
class vtkImageData;
class vtkInformationDoubleVectorKey;
class vtkCompositeDataSet;
class vtkMultiProcessController;
class vtkPlane;
class vtkPolyData;
class vtkRectilinearGrid;
class vtkUniformGrid;
class vtkUnsignedCharArray;

//#define EXTRACT_USE_IMAGE_DATA 1

class VTKFILTERSPARALLEL_EXPORT vtkExtractCTHPart : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkExtractCTHPart,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkExtractCTHPart *New();

  // Description:
  // Names of cell volume fraction arrays to extract.
  void RemoveDoubleVolumeArrayNames();
  void RemoveFloatVolumeArrayNames();
  void RemoveUnsignedCharVolumeArrayNames();
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);
  // for backwards compatibility
  void RemoveAllVolumeArrayNames();

  // Description
  // Names of cell volume fraction arrays to extract.
  // Each of the volume fraction arrays must be of the same type.
  // These three methods enforce that on input, removing any prior arrays
  // of the wrong type whenever a new array is added.
  void AddDoubleVolumeArrayName(char* arrayName);
  void AddFloatVolumeArrayName(char* arrayName);
  void AddUnsignedCharVolumeArrayName(char* arrayName);
  //for backwards compatibility
  void AddVolumeArrayName(char* arrayName);

  // Description:
  // Set, get or maninpulate the implicit clipping plane.
  void SetClipPlane(vtkPlane *clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);

  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();

  // Description:
  // Set the controller used to coordinate parallel processing.
  void SetController(vtkMultiProcessController* controller);

  // Description:
  // Return the controller used to coordinate parallel processing. By default,
  // it is the global controller.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);

  // Description:
  // Set and get the volume fraction surface value. This value should be
  // between 0 and 1
  vtkSetClampMacro(VolumeFractionSurfaceValue, double, 0.0, 1.0);
  vtkGetMacro(VolumeFractionSurfaceValue, double);

protected:
  vtkExtractCTHPart();
  ~vtkExtractCTHPart();

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  // Description:
  // the input is a hierarchy of vtkUniformGrid or one level of
  // vtkRectilinearGrid. The output is a hierarchy of vtkPolyData.


  // Description:
  // Compute the bounds over the composite dataset, some sub-dataset
  // can be on other processors.
  void ComputeBounds(vtkCompositeDataSet *input,
                     int processNumber,
                     int numProcessors);

  void ExecutePart(const char *arrayName,
                   vtkCompositeDataSet *input,
                   vtkAppendPolyData *appendSurface,
                   vtkAppendPolyData *append,
                   float minProgress,
                   float maxProgress);

  void ExecutePartOnUniformGrid(const char *arrayName,
#ifdef EXTRACT_USE_IMAGE_DATA
                                vtkImageData *input,
#else
                                vtkUniformGrid *input,
#endif
                                vtkAppendPolyData *appendSurface,
                                vtkAppendPolyData *append,
                                float minProgress,
                                float maxProgress);

  void ExecutePartOnRectilinearGrid(const char *arrayName,
                                    vtkRectilinearGrid *input,
                                    vtkAppendPolyData *appendSurface,
                                    vtkAppendPolyData *append,
                                    float minProgress,
                                    float maxProgress);

  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction,
                                  vtkDoubleArray *pointVolumeFraction,
                                  int *dims,
                                  float minProgress,
                                  float maxProgress,
                                  int reportProgress);

  virtual int FillInputPortInformation(int port,
                                       vtkInformation *info);

  void CreateInternalPipeline();
  void DeleteInternalPipeline();

  // Description:
  // Append quads for faces of the block that actually on the bounds
  // of the hierarchical dataset. Deals with ghost cells.
  // Return true if the output is not empty.
  int ExtractUniformGridSurface(
#ifdef EXTRACT_USE_IMAGE_DATA
    vtkImageData *input,
#else
    vtkUniformGrid *input,
#endif
    vtkPolyData *output);

  // Description:
  // Append quads for faces of the block that actually on the bounds
  // of the hierarchical dataset. Deals with ghost cells.
  // Return true if the output is not empty.
  int ExtractRectilinearGridSurface(vtkRectilinearGrid *input,
                                    vtkPolyData *output);

  void ExecuteFaceQuads(vtkDataSet *input,
                        vtkPolyData *output,
                        int maxFlag,
                        int originExtents[3],
                        int ext[6],
                        int aAxis,
                        int bAxis,
                        int cAxis);

  // Description:
  // Is block face on axis0 (either min or max depending on the maxFlag)
  // composed of only ghost cells?
  // \pre valid_axis0: axis0>=0 && axis0<=2
  int IsGhostFace(int axis0,
                  int maxFlag,
                  int dims[3],
                  vtkUnsignedCharArray *ghostArray);

  vtkPlane *ClipPlane;
  vtkExtractCTHPartInternal* Internals;

  // Internal Pipeline elements
  vtkDoubleArray *PointVolumeFraction;

#ifdef EXTRACT_USE_IMAGE_DATA
  vtkImageData *Data;
#else
  vtkUniformGrid *Data;
#endif

  vtkContourFilter *Contour;
  vtkAppendPolyData *Append2;
  vtkClipPolyData *Clip1;
  vtkCutter *Cut;
  vtkClipPolyData *Clip2;

  vtkPolyData *PolyData;
  vtkAlgorithm *PolyDataProducer;
  vtkPolyData *RPolyData;
  vtkAlgorithm *RPolyDataProducer;
  vtkPolyData *SurfacePolyData;

  vtkRectilinearGrid *RData;
  vtkContourFilter *RContour;
  vtkAppendPolyData *RAppend2;
  vtkClipPolyData *RClip1;
  vtkCutter *RCut;
  vtkClipPolyData *RClip2;

  void EvaluateVolumeFractionType(vtkRectilinearGrid* rg,
                                  vtkCompositeDataSet* input);
  int VolumeFractionType;
  double VolumeFractionSurfaceValue;
  double VolumeFractionSurfaceValueInternal;
  int OverwriteVolumeFractionSurfaceValue;

  vtkBoundingBox *Bounds; // Whole bounds (dataset over all the processors)

  vtkMultiProcessController *Controller;
private:
  vtkExtractCTHPart(const vtkExtractCTHPart&);  // Not implemented.
  void operator=(const vtkExtractCTHPart&);  // Not implemented.
};
#endif
