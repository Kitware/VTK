/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHAVSVolumeMapper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/* Copyright 2005, 2006 by University of Utah. */

// .NAME vtkHAVSVolumeMapper - Hardware-Assisted Visibility Sorting unstructured grid mapper
//
// .SECTION Description
//
// vtkHAVSVolumeMapper is a class that renders polygonal data
// (represented as an unstructured grid) using the Hardware-Assisted
// Visibility Sorting (HAVS) algorithm.  First the unique triangles are sorted
// in object space, then they are sorted in image space using a fixed size
// A-buffer implemented on the GPU called the k-buffer.  The HAVS algorithm
// excels at rendering large datasets quickly.  The trade-off is that the
// algorithm may produce some rendering artifacts due to an insufficient k
// size (currently 2 or 6 is supported) or read/write race conditions.
//
// A built in level-of-detail (LOD) approach samples the geometry using one of
// two heuristics (field or area).  If LOD is enabled, the amount of geometry
// that is sampled and rendered changes dynamically to stay within the target
// frame rate.  The field sampling method generally works best for datasets
// with cell sizes that don't vary much in size.  On the contrary, the area
// sampling approach gives better approximations when the volume has a lot of
// variation in cell size.
//
// The HAVS algorithm uses several advanced features on graphics hardware.
// The k-buffer sorting network is implemented using framebuffer objects
// (FBOs) with multiple render targets (MRTs).  Therefore, only cards that
// support these features can run the algorithm (at least an ATI 9500 or an
// NVidia NV40 (6600)).
//
// .SECTION Notes
//
// Several issues had to be addressed to get the HAVS algorithm working within
// the vtk framework.  These additions forced the code to forsake speed for
// the sake of compliance and robustness.
//
// The HAVS algorithm operates on the triangles that compose the mesh.
// Therefore, before rendering, the cells are decomposed into unique triangles
// and stored on the GPU for efficient rendering.  The use of GPU data
// structures is only recommended if the entire geometry can fit in graphics
// memory.  Otherwise this feature should be disabled.
//
// Another new feature is the handling of mixed data types (eg., polygonal
// data with volume data).  This is handled by reading the z-buffer from the
// current window and copying it into the framebuffer object for off-screen
// rendering.  The depth test is then enabled so that the volume only appears
// over the opaque geometry.  Finally, the results of the off-screen rendering
// are blended into the framebuffer as a transparent, view-aligned texture.
//
// Instead of using a preintegrated 3D lookup table for storing the ray
// integral, this implementation uses partial pre-integration.  This improves
// the performance of dynamic transfer function updates by avoiding a costly
// preprocess of the table.
//
// A final change to the original algorithm is the handling of non-convexities
// in the mesh.  Due to read/write hazards that may create undesired artifacts
// with non-convexities when using a inside/outside toggle in the fragment
// program, another approach was employed.  To handle non-convexities, the
// fragment shader determines if a ray-gap is larger than the max cell size
// and kill the fragment if so.  This approximation performs rather well in
// practice but may miss small non-convexities.
//
// For more information on the HAVS algorithm see:
//
//  "Hardware-Assisted Visibility Sorting for Unstructured Volume
// Rendering" by S. P. Callahan, M. Ikits, J. L. D. Comba, and C. T. Silva,
// IEEE Transactions of Visualization and Computer Graphics; May/June 2005.
//
// For more information on the Level-of-Detail algorithm, see:
//
// "Interactive Rendering of Large Unstructured Grids Using Dynamic
// Level-of-Detail" by S. P. Callahan, J. L. D. Comba, P. Shirley, and
// C. T. Silva, Proceedings of IEEE Visualization '05, Oct. 2005.
//
// .SECTION Acknowledgments
//
// This code was developed by Steven P. Callahan under the supervision
// of Prof. Claudio T. Silva. The code also contains contributions
// from Milan Ikits, Linh Ha, Huy T. Vo, Carlos E. Scheidegger, and
// Joao L. D. Comba.
//
// The work was supported by grants, contracts, and gifts from the
// National Science Foundation, the Department of Energy, the Army
// Research Office, and IBM.
//
// The port of HAVS to VTK and ParaView has been primarily supported
// by Sandia National Labs.
//

#ifndef __vtkHAVSVolumeMapper_h
#define __vtkHAVSVolumeMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeMapper.h"

#define VTK_KBUFFER_SIZE_2 0
#define VTK_KBUFFER_SIZE_6 1
#define VTK_FIELD_LEVEL_OF_DETAIL 0
#define VTK_AREA_LEVEL_OF_DETAIL 1


class vtkUnstructuredGrid;
class vtkDepthRadixSortUnstructuredGrid;
class vtkHAVSSortedFace;

class VTKRENDERINGVOLUME_EXPORT vtkHAVSVolumeMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  static vtkHAVSVolumeMapper *New();
  vtkTypeMacro(vtkHAVSVolumeMapper,
                       vtkUnstructuredGridVolumeMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description: Set/get whether or not to attempt to handle non convex
  // regions by removing ray segments larger than the max cell size.
  vtkSetMacro(PartiallyRemoveNonConvexities, bool);
  vtkGetMacro(PartiallyRemoveNonConvexities, bool);

  // Description:
  // Set/get the desired level of detail target time measured in frames/sec.
  vtkSetMacro(LevelOfDetailTargetTime, float);
  vtkGetMacro(LevelOfDetailTargetTime, float);

  // Description:
  // Turn on/off level-of-detail volume rendering
  vtkSetMacro(LevelOfDetail, bool);
  vtkGetMacro(LevelOfDetail, bool);

  // Description:
  // Set/get the current level-of-detail method
  void SetLevelOfDetailMethod(int);
  vtkGetMacro(LevelOfDetailMethod, int);
  void SetLevelOfDetailMethodField()
  {this->SetLevelOfDetailMethod(VTK_FIELD_LEVEL_OF_DETAIL);}
  void SetLevelOfDetailMethodArea()
  {this->SetLevelOfDetailMethod(VTK_AREA_LEVEL_OF_DETAIL);}

  // Description:
  // Set the kbuffer size
  vtkSetMacro(KBufferSize,int);
  vtkGetMacro(KBufferSize,int);
  void SetKBufferSizeTo2()
  {this->SetKBufferSize(VTK_KBUFFER_SIZE_2);}
  void SetKBufferSizeTo6()
  {this->SetKBufferSize(VTK_KBUFFER_SIZE_6);}

  // Description:
  // Check hardware support for the HAVS algorithm.  Necessary
  // features include off-screen rendering, 32-bit fp textures, multiple
  // render targets, and framebuffer objects.
  // Subclasses must override this method to indicate if supported by Hardware.
  virtual bool SupportedByHardware(vtkRenderer *vtkNotUsed(r))
    {return false; }

  // Description:
  // Set/get whether or not the data structures should be stored on the GPU
  // for better peformance.
  virtual void SetGPUDataStructures(bool) = 0;
  vtkGetMacro(GPUDataStructures, bool);

protected:
  vtkHAVSVolumeMapper();
  ~vtkHAVSVolumeMapper();

//BTX
  virtual void Initialize(vtkRenderer *ren, vtkVolume *vol) = 0;
  void InitializePrimitives(vtkVolume *vol);
  void InitializeScalars();
  void InitializeLevelOfDetail();
  void InitializeLookupTables(vtkVolume *vol);

  void FRadixSort(vtkHAVSSortedFace *array, vtkHAVSSortedFace *temp, int lo, int up);
  void FRadix(int byte, int len, vtkHAVSSortedFace *source, vtkHAVSSortedFace *dest, int *count);

  void UpdateLevelOfDetail(float targetTime);
  void PartialVisibilitySort(float *eye);
  bool CheckInitializationError();

  enum
  {
    NO_INIT_ERROR=0,
    NON_TETRAHEDRA=1,
    UNSUPPORTED_EXTENSIONS=2,
    NO_SCALARS=3,
    CELL_DATA=4,
    NO_CELLS=5
  };

  // Mesh
  float *Vertices;
  float *Scalars;
  double ScalarRange[2];
  unsigned int *Triangles;
  unsigned int *OrderedTriangles;
  vtkHAVSSortedFace *SortedFaces;
  vtkHAVSSortedFace *RadixTemp;
  float *Centers;
  unsigned int NumberOfVertices;
  unsigned int NumberOfCells;
  unsigned int NumberOfScalars;
  unsigned int NumberOfTriangles;

  // Level-Of-Detail
  unsigned int NumberOfBoundaryTriangles;
  unsigned int NumberOfInternalTriangles;
  unsigned int *BoundaryTriangles;
  unsigned int *InternalTriangles;
  unsigned int LevelOfDetailTriangleCount;
  float CurrentLevelOfDetail;
  float LevelOfDetailTargetTime;
  bool LevelOfDetail;
  int LevelOfDetailMethod;

  // K-Buffer
  int KBufferState;
  float MaxEdgeLength;
  float LevelOfDetailMaxEdgeLength;
  float UnitDistance;
  bool GPUDataStructures;
  float Diagonal;
  bool PartiallyRemoveNonConvexities;
  int KBufferSize;

  // Lookup Tables
  float *TransferFunction;
  int TransferFunctionSize;

  // State and Timing Stats
  bool Initialized;
  int InitializationError;
  int FrameNumber;
  float TotalRenderTime;
  vtkTimeStamp ColorTransferFunctionMTime;
  vtkTimeStamp AlphaTransferFunctionMTime;
  vtkTimeStamp UnstructuredGridMTime;
  vtkTimeStamp ScalarsMTime;
  vtkVolume *LastVolume;
//ETX

private:
  vtkHAVSVolumeMapper(const vtkHAVSVolumeMapper&);  // Not implemented.
  void operator=(const vtkHAVSVolumeMapper&);  // Not implemented.
};
#endif
