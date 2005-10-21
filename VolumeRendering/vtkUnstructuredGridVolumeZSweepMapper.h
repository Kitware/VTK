/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeZSweepMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridVolumeZSweepMapper - Unstructured grid volume mapper based the ZSweep Algorithm

// .SECTION Description
// This is a volume mapper for unstructured grid implemented with the ZSweep
// algorithm. This is a software projective method.

// .SECTION see also
// vtkVolumetMapper

// .SECTION Background
// The algorithm is described in the following paper:
// Ricardo Farias, Joseph S. B. Mitchell and Claudio T. Silva.
// ZSWEEP: An Efficient and Exact Projection Algorithm for Unstructured Volume
// Rendering. In 2000 Volume Visualization Symposium, pages 91--99.
// October 2000.
// http://www.cse.ogi.edu/~csilva/papers/volvis2000.pdf

#ifndef __vtkUnstructuredGridVolumeZSweepMapper_h
#define __vtkUnstructuredGridVolumeZSweepMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkRenderer;
class vtkVolume;
class vtkRayCastImageDisplayHelper;
class vtkCell;
class vtkGenericCell;
class vtkIdList;
class vtkPriorityQueue;
class vtkTransform;
class vtkMatrix4x4;
class vtkVolumeProperty;
class vtkDoubleArray;
class vtkUnstructuredGridVolumeRayIntegrator;
class vtkRenderWindow;

// Internal classes
class vtkScreenEdge;
class vtkSpan;
class vtkPixelListFrame;
class vtkUseSet;
class vtkVertices;
class vtkSimpleScreenEdge;
class vtkDoubleScreenEdge;
class vtkVertexEntry;
class vtkPixelListEntryMemory;

class VTK_VOLUMERENDERING_EXPORT vtkUnstructuredGridVolumeZSweepMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeRevisionMacro(vtkUnstructuredGridVolumeZSweepMapper,vtkUnstructuredGridVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );
  
  // Description:
  // Set MaxPixelListSize to 32.
  static vtkUnstructuredGridVolumeZSweepMapper *New();
  
  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (ScalarModeToDefault), the filter will use point data,
  // and if no point data is available, then cell data is used. Alternatively
  // you can explicitly set the filter to use point data
  // (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
  // You can also choose to get the scalars from an array in point field
  // data (ScalarModeToUsePointFieldData) or cell field data
  // (ScalarModeToUseCellFieldData).  If scalars are coming from a field
  // data array, you must call SelectColorArray before you call
  // GetColors.
  vtkSetMacro(ScalarMode,int);
  vtkGetMacro(ScalarMode,int);
  void SetScalarModeToDefault() {
    this->SetScalarMode(VTK_SCALAR_MODE_DEFAULT);};
  void SetScalarModeToUsePointData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA);};
  void SetScalarModeToUseCellData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_DATA);};
  void SetScalarModeToUsePointFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);};
  void SetScalarModeToUseCellFieldData() {
    this->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);};
  
  // Description:
  // When ScalarMode is set to UsePointFileData or UseCellFieldData,
  // you can specify which array to use for coloring using these methods.
  // The transfer function in the vtkVolumeProperty (attached to the calling
  // vtkVolume) will decide how to convert vectors to colors.
  virtual void SelectScalarArray(int arrayNum); 
  virtual void SelectScalarArray(const char* arrayName); 
  
  // Description:
  // Get the array name or number and component to color by.
  virtual char* GetArrayName() { return this->ArrayName; }
  virtual int GetArrayId() { return this->ArrayId; }
  virtual int GetArrayAccessMode() { return this->ArrayAccessMode; }

  // Description:
  // Return the method for obtaining scalar data.
  const char *GetScalarModeAsString();

  // Description:
  // Sampling distance in the XY image dimensions. Default value of 1 meaning
  // 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
  // set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels.
  vtkSetClampMacro( ImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( ImageSampleDistance, float );

  // Description:
  // This is the minimum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MinimumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MinimumImageSampleDistance, float );

  // Description:
  // This is the maximum image sample distance allow when the image
  // sample distance is being automatically adjusted
  vtkSetClampMacro( MaximumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MaximumImageSampleDistance, float );

  // Description:
  // If AutoAdjustSampleDistances is on, the the ImageSampleDistance
  // will be varied to achieve the allocated render time of this 
  // prop (controlled by the desired update rate and any culling in
  // use). 
  vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
  vtkGetMacro( AutoAdjustSampleDistances, int );
  vtkBooleanMacro( AutoAdjustSampleDistances, int );
  
  // Description:
  // If IntermixIntersectingGeometry is turned on, the zbuffer will be
  // captured and used to limit the traversal of the rays.
  vtkSetClampMacro( IntermixIntersectingGeometry, int, 0, 1 );
  vtkGetMacro( IntermixIntersectingGeometry, int );
  vtkBooleanMacro( IntermixIntersectingGeometry, int );

  // Description:
  // Maximum size allowed for a pixel list. Default is 32.
  // During the rendering, if a list of pixel is full, incremental compositing
  // is performed. Even if it is a user setting, it is an advanced parameter.
  // You have to understand how the algorithm works to change this value.
  int GetMaxPixelListSize();
  
  // Description:
  // Change the maximum size allowed for a pixel list. It is an advanced
  // parameter.
  // \pre positive_size: size>0
  void SetMaxPixelListSize(int size);
  
  // Description:
  // Set/Get the helper class for integrating rays.  If set to NULL, a
  // default integrator will be assigned.
  virtual void SetRayIntegrator(vtkUnstructuredGridVolumeRayIntegrator *ri);
  vtkGetObjectMacro(RayIntegrator, vtkUnstructuredGridVolumeRayIntegrator);
  
//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  void Render(vtkRenderer *ren,
              vtkVolume *vol);
  
  vtkGetVectorMacro( ImageInUseSize, int, 2 );
  vtkGetVectorMacro( ImageOrigin, int, 2 );
  vtkGetVectorMacro( ImageViewportSize, int , 2 );
//ETX
  
protected:
  vtkUnstructuredGridVolumeZSweepMapper();
  ~vtkUnstructuredGridVolumeZSweepMapper();
  
  // Description:
  // For each vertex, find the list of incident faces.
  void BuildUseSets();
  
  // Description:
  // Reorder vertices `v' in increasing order in `w'. Return if the orientation
  // has changed.
  int ReorderTriangle(vtkIdType v[3],
                      vtkIdType w[3]);

  // Description:
  // Project and sort the vertices by z-coordinates in view space in the
  // "event list" (an heap).
  // \pre empty_list: this->EventList->GetNumberOfItems()==0
  void ProjectAndSortVertices(vtkRenderer *ren,
                              vtkVolume *vol);
  
  // Description:
  // Create an empty "pixel list" for each pixel of the screen.
  void CreateAndCleanPixelList();
  
  // Description:
  // MainLoop of the Zsweep algorithm.
  // \post empty_list: this->EventList->GetNumberOfItems()==0
  void MainLoop(vtkRenderWindow *renWin);
  
  // Description:
  // Do delayed compositing from back to front, stopping at zTarget for each
  // pixel inside the bounding box.
  void CompositeFunction(double zTarget);
  
  // Description:
  // Convert and clamp a float color component into a unsigned char.
  unsigned char ColorComponentRealToByte(float color);
  
  // Description:
  // Perform scan conversion of a triangle face.
  void RasterizeFace(vtkIdType faceIds[3]);
  
  // Description:
  // Perform scan conversion of a triangle defined by its vertices.
  // \pre ve0_exists: ve0!=0
  // \pre ve1_exists: ve1!=0
  // \pre ve2_exists: ve2!=0
  void RasterizeTriangle(vtkVertexEntry *ve0,vtkVertexEntry *ve1,
                         vtkVertexEntry *ve2);
  
  // Description:
  // Perform scan conversion of an horizontal span from left ro right at line
  // y.
  // \pre left_exists: left!=0
  // \pre right_exists: right!=0
  void RasterizeSpan(int y,
                     vtkScreenEdge *left,
                     vtkScreenEdge *right);
  
  // Description:
  // Scan conversion of a straight line defined by endpoints v0 and v1.
  // \pre v0_exists: v0!=0
  // \pre v1_exists: v1!=0
  // \pre y_ordered v0->GetScreenY()<=v1->GetScreenY()
  void RasterizeLine(vtkVertexEntry *v0,
                     vtkVertexEntry *v1);
  
  void StoreRenderTime(vtkRenderer *ren,
                       vtkVolume *vol,
                       float t);
  
  float RetrieveRenderTime(vtkRenderer *ren,
                           vtkVolume *vol);
  
  // Description:
  // Return the value of the z-buffer at screen coordinates (x,y).
  double GetZBufferValue(int x,
                         int y);
  
  double GetMinimumBoundsDepth(vtkRenderer *ren,
                               vtkVolume *vol);
  
  // Description:
  // Allocate an array of usesets of size `size' only if the current one is not
  // large enough. Otherwise clear each use set of each vertex.
  void AllocateUseSet(vtkIdType size);
  
  // Description:
  // Allocate a vertex array of size `size' only if the current one is not
  // large enough.
  void AllocateVertices(vtkIdType size);
  
  // Description:
  // For debugging purpose, save the pixel list frame as a dataset.
  void SavePixelListFrame();
  
  int MaxPixelListSize;
  
  float ImageSampleDistance;
  float MinimumImageSampleDistance;
  float MaximumImageSampleDistance;
  int AutoAdjustSampleDistances;
  
  vtkRayCastImageDisplayHelper *ImageDisplayHelper;
  
  // This is how big the image would be if it covered the entire viewport
  int ImageViewportSize[2];
  
  // This is how big the allocated memory for image is. This may be bigger
  // or smaller than ImageFullSize - it will be bigger if necessary to 
  // ensure a power of 2, it will be smaller if the volume only covers a
  // small region of the viewport
  int ImageMemorySize[2];
  
  // This is the size of subregion in ImageSize image that we are using for
  // the current image. Since ImageSize is a power of 2, there is likely
  // wasted space in it. This number will be used for things such as clearing
  // the image if necessary.
  int ImageInUseSize[2];
  
  // This is the location in ImageFullSize image where our ImageSize image
  // is located.
  int ImageOrigin[2];
  
  // This is the allocated image
  unsigned char *Image;
  
  // This is the accumulating double RGBA image
  float *RealRGBAImage;
  
  float *RenderTimeTable;
  vtkVolume **RenderVolumeTable;
  vtkRenderer **RenderRendererTable;
  int RenderTableSize;
  int RenderTableEntries;
  
  int IntermixIntersectingGeometry;

  float *ZBuffer;
  int ZBufferSize[2];
  int ZBufferOrigin[2];
  
  int ScalarMode;
  char *ArrayName;
  int ArrayId;
  int ArrayAccessMode;
  
  vtkDataArray *Scalars;
  int CellScalars;
  
  // if use CellScalars, we need to keep track of the
  // values on each side of the face and figure out
  // if the face is used by two cells (twosided) or one cell.
  double FaceScalars[2];
  int FaceSide;
  
  vtkSpan *Span;
  vtkPixelListFrame *PixelListFrame;
  
  // Used by BuildUseSets().
  vtkGenericCell *Cell;
  
  vtkUseSet *UseSet;
  
  vtkPriorityQueue *EventList;
  vtkVertices *Vertices;
  
  vtkTransform *PerspectiveTransform;
  vtkMatrix4x4 *PerspectiveMatrix;
  
  // Used by the main loop
  int MaxPixelListSizeReached;
  int XBounds[2];
  int YBounds[2];
  
  vtkSimpleScreenEdge *SimpleEdge;
  vtkDoubleScreenEdge *DoubleEdge;
  
  vtkUnstructuredGridVolumeRayIntegrator *RayIntegrator;
  vtkUnstructuredGridVolumeRayIntegrator *RealRayIntegrator;
  
  vtkTimeStamp SavedTriangleListMTime;
  
  // Used during compositing
  vtkDoubleArray *IntersectionLengths;
  vtkDoubleArray *NearIntersections;
  vtkDoubleArray *FarIntersections;
  
  // Benchmark
  vtkIdType MaxRecordedPixelListSize;
  
  
  vtkPixelListEntryMemory *MemoryManager;
private:
  vtkUnstructuredGridVolumeZSweepMapper(const vtkUnstructuredGridVolumeZSweepMapper&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeZSweepMapper&);  // Not implemented.
};

#endif
