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

#include "vtkPolyDataAlgorithm.h"
class vtkPlane;
class vtkDataArray;
class vtkDoubleArray;
class vtkRectilinearGrid;

class vtkExtractCTHPartInternal;
class vtkMultiGroupDataSet;
class vtkPolyData;
class vtkUniformGrid;
class vtkImageData;
class vtkDataSet;

class vtkContourFilter;
class vtkAppendPolyData;
class vtkDataSetSurfaceFilter;
class vtkClipPolyData;
class vtkCutter;

class vtkMultiProcessController;

//#define EXTRACT_USE_IMAGE_DATA 1

class VTK_PARALLEL_EXPORT vtkExtractCTHPart : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractCTHPart,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // key to record the bounds of the hierarchical dataset.
  static vtkInformationDoubleVectorKey *BOUNDS();
  
  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkExtractCTHPart *New();

  // Description:
  // Names of cell volume fraction arrays to extract.
  void RemoveAllVolumeArrayNames();
  void AddVolumeArrayName(char* arrayName);
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);

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
  
  // Return the controller used to coordinate parallel processing. By default,
  // it is the global controller.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  
protected:
  vtkExtractCTHPart();
  ~vtkExtractCTHPart();

  void SetOutputData(int idx, vtkPolyData* d);
  
  int RequestInformation(vtkInformation *request,
                         vtkInformationVector **inputVector,
                         vtkInformationVector *outputVector);
  
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  
  // Description:
  // the input is a hierarchy of vtkUniformGrid or one level of
  // vtkRectilinearGrid. The output is a hierarchy of vtkPolyData.
  
  
  // Description:
  // Compute the bounds over the composite dataset, some sub-dataset
  // can be on other processors.
  void ComputeBounds(vtkMultiGroupDataSet *input,
                     int processNumber,
                     int numProcessors);
  
  // Description:
  // The processors are views as a heap tree. The root is the processor of
  // id 0.
  int GetParentProcessor(int proc);
  int GetLeftChildProcessor(int proc);
  
  void ExecutePart(const char *arrayName,
                   vtkMultiGroupDataSet *input,
                   vtkAppendPolyData *appendSurface,
                   vtkAppendPolyData *append);
  
  void ExecutePartOnUniformGrid(const char *arrayName,
#ifdef EXTRACT_USE_IMAGE_DATA
                                vtkImageData *input,
#else
                                vtkUniformGrid *input,
#endif
                                vtkAppendPolyData *appendSurface,
                                vtkAppendPolyData *append);
  
  void ExecutePartOnRectilinearGrid( const char *arrayName,
                                     vtkRectilinearGrid *input,
                                     vtkAppendPolyData *appendSurface,
                                     vtkAppendPolyData *append);
  
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkDoubleArray *pointVolumeFraction,
                                  int *dims);
  
  int FillInputPortInformation(int port,
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
  vtkPolyData *RPolyData;
  vtkPolyData *SurfacePolyData;
  
  vtkRectilinearGrid *RData;
  vtkContourFilter *RContour;
  vtkAppendPolyData *RAppend2;
  vtkClipPolyData *RClip1;
  vtkCutter *RCut;
  vtkClipPolyData *RClip2;

  void EvaluateVolumeFractionType(vtkRectilinearGrid* rg, vtkMultiGroupDataSet* input);
  int VolumeFractionType;
  double VolumeFractionSurfaceValue;
  
  double Bounds[6]; // Whole bounds (dataset over all the processors)
  
  vtkMultiProcessController *Controller;
private:
  vtkExtractCTHPart(const vtkExtractCTHPart&);  // Not implemented.
  void operator=(const vtkExtractCTHPart&);  // Not implemented.
};
#endif
