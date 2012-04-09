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

//BTX
// Internal classes
namespace vtkUnstructuredGridVolumeZSweepMapperNamespace
{
  class vtkScreenEdge;
  class vtkSpan;
  class vtkPixelListFrame;
  class vtkUseSet;
  class vtkVertices;
  class vtkSimpleScreenEdge;
  class vtkDoubleScreenEdge;
  class vtkVertexEntry;
  class vtkPixelListEntryMemory;
};
//ETX

class VTK_VOLUMERENDERING_EXPORT vtkUnstructuredGridVolumeZSweepMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeZSweepMapper,vtkUnstructuredGridVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );
  
  // Description:
  // Set MaxPixelListSize to 32.
  static vtkUnstructuredGridVolumeZSweepMapper *New();
  
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
  // \pre positive_size: size>1
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
  void RasterizeFace(vtkIdType faceIds[3], int externalSide);

//BTX
  // Description:
  // Perform scan conversion of a triangle defined by its vertices.
  // \pre ve0_exists: ve0!=0
  // \pre ve1_exists: ve1!=0
  // \pre ve2_exists: ve2!=0
  void RasterizeTriangle(
            vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertexEntry *ve0,
            vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertexEntry *ve1,
            vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertexEntry *ve2,
            bool exitFace);
  
  // Description:
  // Perform scan conversion of an horizontal span from left ro right at line
  // y.
  // \pre left_exists: left!=0
  // \pre right_exists: right!=0
  void RasterizeSpan(int y,
           vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkScreenEdge *left,
           vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkScreenEdge *right,
           bool exitFace);
  
  // Description:
  // Scan conversion of a straight line defined by endpoints v0 and v1.
  // \pre v0_exists: v0!=0
  // \pre v1_exists: v1!=0
  // \pre y_ordered v0->GetScreenY()<=v1->GetScreenY()
  void RasterizeLine(
             vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertexEntry *v0,
             vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertexEntry *v1,
             bool exitFace);
//ETX
  
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
  
  vtkDataArray *Scalars;
  int CellScalars;
  
  // if use CellScalars, we need to keep track of the
  // values on each side of the face and figure out
  // if the face is used by two cells (twosided) or one cell.
  double FaceScalars[2];
  int FaceSide;

//BTX
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkSpan *Span;
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkPixelListFrame *PixelListFrame;
  
  // Used by BuildUseSets().
  vtkGenericCell *Cell;
  
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkUseSet *UseSet;
  
  vtkPriorityQueue *EventList;
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkVertices *Vertices;
  
  vtkTransform *PerspectiveTransform;
  vtkMatrix4x4 *PerspectiveMatrix;
  
  // Used by the main loop
  int MaxPixelListSizeReached;
  int XBounds[2];
  int YBounds[2];
  
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkSimpleScreenEdge *SimpleEdge;
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkDoubleScreenEdge *DoubleEdge;
  
  vtkUnstructuredGridVolumeRayIntegrator *RayIntegrator;
  vtkUnstructuredGridVolumeRayIntegrator *RealRayIntegrator;
  
  vtkTimeStamp SavedTriangleListMTime;
  
  // Used during compositing
  vtkDoubleArray *IntersectionLengths;
  vtkDoubleArray *NearIntersections;
  vtkDoubleArray *FarIntersections;
  
  // Benchmark
  vtkIdType MaxRecordedPixelListSize;

  
  vtkUnstructuredGridVolumeZSweepMapperNamespace::vtkPixelListEntryMemory *MemoryManager;
//ETX
private:
  vtkUnstructuredGridVolumeZSweepMapper(const vtkUnstructuredGridVolumeZSweepMapper&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeZSweepMapper&);  // Not implemented.
};

#endif
