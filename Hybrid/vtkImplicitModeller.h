/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitModeller.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImplicitModeller - compute distance from input geometry on structured point dataset
// .SECTION Description
// vtkImplicitModeller is a filter that computes the distance from the input
// geometry to the points of an output structured point set. This distance
// function can then be "contoured" to generate new, offset surfaces from
// the original geometry. An important feature of this object is
// "capping". If capping is turned on, after the implicit model is created,
// the values on the boundary of the structured points dataset are set to
// the cap value. This is used to force closure of the resulting contoured
// surface. Note, however, that large cap values can generate weird surface
// normals in those cells adjacent to the boundary of the dataset. Using
// smaller cap value will reduce this effect.
//<P>
// Another important ivar is MaximumDistance. This controls how far into the
// volume the distance function is computed from the input geometry.  Small
// values give significant increases in performance. However, there can
// strange sampling effects at the extreme range of the MaximumDistance.
//<P>
// In order to properly execute and sample the input data, a rectangular
// region in space must be defined (this is the ivar ModelBounds).  If not
// explicitly defined, the model bounds will be computed. Note that to avoid
// boundary effects, it is possible to adjust the model bounds (i.e., using
// the AdjustBounds and AdjustDistance ivars) to strictly contain the
// sampled data.
//<P>
// This filter has one other unusual capability: it is possible to append
// data in a sequence of operations to generate a single output. This is
// useful when you have multiple datasets and want to create a
// conglomeration of all the data.  However, the user must be careful to
// either specify the ModelBounds or specify the first item such that its
// bounds completely contain all other items.  This is because the 
// rectangular region of the output can not be changed after the 1st Append.
//<P>
// The ProcessMode ivar controls the method used within the Append function
// (where the actual work is done regardless if the Append function is
// explicitly called) to compute the implicit model.  If set to work in voxel
// mode, each voxel is visited once.  If set to cell mode, each cell is visited
// once.  Tests have shown once per voxel to be faster when there are a 
// lot of cells (at least a thousand?); relative performance improvement 
// increases with addition cells. Primitives should not be stripped for best
// performance of the voxel mode.  Also, if explicitly using the Append feature
// many times, the cell mode will probably be better because each voxel will be
// visited each Append.  Append the data before input if possible when using
// the voxel mode.
//<P>
// Further performance improvement is now possible using the PerVoxel process
// mode on multi-processor machines (the mode is now multithreaded).  Each
// thread processes a different "slab" of the output.  Also, if the input is 
// vtkPolyData, it is appropriately clipped for each thread; that is, each 
// thread only considers the input which could affect its slab of the output.

// .SECTION See Also
// vtkSampleFunction vtkContourFilter

#ifndef __vtkImplicitModeller_h
#define __vtkImplicitModeller_h

#include "vtkDataSetToStructuredPointsFilter.h"

#define VTK_VOXEL_MODE   0
#define VTK_CELL_MODE    1

class vtkMultiThreader;
class vtkExtractGeometry;

class VTK_EXPORT vtkImplicitModeller : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkTypeMacro(vtkImplicitModeller,vtkDataSetToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with sample dimensions=(50,50,50), and so that model bounds are
  // automatically computed from the input. Capping is turned on with CapValue
  // equal to a large positive number.
  static vtkImplicitModeller *New();

  // Description:
  // Compute ModelBounds from input geometry. If input is not specified, the input
  // of the filter will be used.
  float ComputeModelBounds(vtkDataSet *input = NULL);

  // Description:
  // Set/Get the i-j-k dimensions on which to sample distance function.
  vtkGetVectorMacro(SampleDimensions,int,3);
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);

  // Description:
  // Set / get the distance away from surface of input geometry to
  // sample. Smaller values make large increases in performance.
  vtkSetClampMacro(MaximumDistance,float,0.0,1.0);
  vtkGetMacro(MaximumDistance,float);

  // Description:
  // Set / get the region in space in which to perform the sampling. If
  // not specified, it will be computed automatically.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Control how the model bounds are computed. If the ivar AdjustBounds
  // is set, then the bounds specified (or computed automatically) is modified
  // by the fraction given by AdjustDistance. This means that the model
  // bounds is expanded in each of the x-y-z directions.
  vtkSetMacro(AdjustBounds,int);
  vtkGetMacro(AdjustBounds,int);
  vtkBooleanMacro(AdjustBounds,int);
  
  // Description:
  // Specify the amount to grow the model bounds (if the ivar AdjustBounds
  // is set). The value is a fraction of the maximum length of the sides
  // of the box specified by the model bounds.
  vtkSetClampMacro(AdjustDistance,float,-1.0,1.0);
  vtkGetMacro(AdjustDistance,float);

  // Description:
  // The outer boundary of the structured point set can be assigned a 
  // particular value. This can be used to close or "cap" all surfaces.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Specify the capping value to use. The CapValue is also used as an
  // initial distance value at each point in the dataset.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

  // Description:
  // Specify whether to visit each cell once per append or each voxel once
  // per append.  Some tests have shown once per voxel to be faster
  // when there are a lot of cells (at least a thousand?); relative
  // performance improvement increases with addition cells.  Primitives
  // should not be stripped for best performance of the voxel mode.
  vtkSetClampMacro(ProcessMode, int, 0, 1);
  vtkGetMacro(ProcessMode, int);
  void SetProcessModeToPerVoxel() {this->SetProcessMode(VTK_VOXEL_MODE);}
  void SetProcessModeToPerCell()  {this->SetProcessMode(VTK_CELL_MODE);}
  const char *GetProcessModeAsString(void);

  // Description:
  // Specify the level of the locator to use when using the per voxel
  // process mode.
  vtkSetMacro(LocatorMaxLevel,int);
  vtkGetMacro(LocatorMaxLevel,int);

  // Description:
  // Set / Get the number of threads used during Per-Voxel processing mode
  vtkSetMacro( NumberOfThreads, int );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Initialize the filter for appending data. You must invoke the
  // StartAppend() method before doing successive Appends(). It's also a
  // good idea to manually specify the model bounds; otherwise the input
  // bounds for the data will be used.
  void StartAppend();

  // Description:
  // Append a data set to the existing output. To use this function,
  // you'll have to invoke the StartAppend() method before doing
  // successive appends. It's also a good idea to specify the model
  // bounds; otherwise the input model bounds is used. When you've
  // finished appending, use the EndAppend() method.
  void Append(vtkDataSet *input);

  // Description:
  // Method completes the append process.
  void EndAppend();

  // Description:
  virtual void UpdateData(vtkDataObject *output);

protected:
  vtkImplicitModeller();
  ~vtkImplicitModeller();
  vtkImplicitModeller(const vtkImplicitModeller&) {};
  void operator=(const vtkImplicitModeller&) {};

  void Execute();
  void ExecuteInformation();
  
  void Cap(vtkDataArray *s);

  vtkMultiThreader *Threader;
  int              NumberOfThreads;

  int SampleDimensions[3];
  float MaximumDistance;
  float ModelBounds[6];
  int Capping;
  float CapValue;
  int DataAppended;
  int AdjustBounds;
  float AdjustDistance;
  int ProcessMode;
  int LocatorMaxLevel;

  int BoundsComputed; // flag to limit to one ComputeModelBounds per StartAppend
  float InternalMaxDistance; // the max distance computed during that one call
};

#endif


