/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphereTree.h"

#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkPlane.h"
#include "vtkSphere.h"
#include "vtkDebugLeaks.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocal.h"


vtkStandardNewMacro(vtkSphereTree);
vtkCxxSetObjectMacro(vtkSphereTree,DataSet,vtkDataSet);

// Implementation notes:
// Currently only two levels of the sphere tree are being built: the leaf
// spheres (one sphere per cell) and then the next level groupings of the
// leaf spheres. This is done because it is easier to thread, and the
// benefits of additional sphere tree hierarchy diminish quickly in a
// threaded environment. Future work may want to revisit this. In particular,
// huge datasets probably would benefit from more levels.
//
// Further room for improvement: while the leaf spheres are built in
// parallel, the hierarchy is built serially. The hierarchy could also
// be built in parallel.
//
// Note the sphere generation uses Ritter's algorithm. While fast, it can
// overestimate the sphere size by 5-20%. Tighter spheres would improve
// performance.


// Type of sphere tree hierarchy generated
#define VTK_SPHERE_TREE_HIERARCHY_NONE 0
#define VTK_SPHERE_TREE_HIERARCHY_STRUCTURED 1
#define VTK_SPHERE_TREE_HIERARCHY_UNSTRUCTURED 2


// Different types of sphere tree hierarchies can be created. These are
// basically data structures for different types of dataset (structured
// and unstructured)
struct vtkSphereTreeHierarchy
{
  virtual ~vtkSphereTreeHierarchy() {}
};

struct vtkStructuredHierarchy : public vtkSphereTreeHierarchy
{
  vtkIdType NumCells;
  vtkDoubleArray *H;
  vtkIdType Dims[3];
  int Resolution;

  vtkIdType GridSize;
  vtkIdType GridDims[3];
  double *GridSpheres;

  vtkStructuredHierarchy(vtkIdType numCells, vtkIdType size) :
    NumCells(numCells)
  {
    this->Resolution = 0;
    this->Dims[0] = this->Dims[1] = this->Dims[2] = 0;
    this->GridSize = 0;
    this->GridDims[0] = this->GridDims[1] = this->GridDims[2] = 0;
    this->GridSpheres = nullptr;
    this->H = vtkDoubleArray::New();
    this->H->SetNumberOfComponents(1);
    this->H->SetNumberOfTuples(size);
  }

  virtual ~vtkStructuredHierarchy()
  {
    this->H->Delete();
    this->H = nullptr;
  }
};

// Currently the unstructured hierarchy is one level deep (to keep it
// simple). In the future a full blown hierarchy could be created. Note that
// there is significant cost to memory allocation/deletion etc. so the
// benefits run out quickly.
struct vtkUnstructuredHierarchy : public vtkSphereTreeHierarchy
{
  vtkIdType NumCells;
  int Dims[3];
  double Bounds[6],Spacing[3];
  vtkIdType GridSize;
  vtkIdType *NumSpheres;
  vtkIdType *Offsets;
  vtkIdType *CellLoc;
  vtkIdType *CellMap;
  double    *GridSpheres;

  vtkUnstructuredHierarchy(int dims[3], double bounds[6], double spacing[3],
                           vtkIdType numCells) :
    NumCells(numCells), NumSpheres(nullptr), Offsets(nullptr),
    CellLoc(nullptr), CellMap(nullptr), GridSpheres(nullptr)
  {
    this->GridSize = static_cast<vtkIdType>(dims[0])*dims[1]*dims[2];
    for (int i=0; i<3; ++i)
      {
      this->Dims[i] = dims[i];
      this->Spacing[i] = spacing[i];
      this->Bounds[2*i] = bounds[2*i];
      this->Bounds[2*i+1] = bounds[2*i+1];
      }

    // Create high-level meta structure that points to grid cells
    this->NumSpheres = new vtkIdType [this->GridSize];
    this->Offsets = new vtkIdType [this->GridSize+1];
    std::fill_n(this->NumSpheres, this->GridSize, 0);
    this->CellLoc = new vtkIdType [numCells];
    this->CellMap = new vtkIdType [numCells];
  }
  virtual ~vtkUnstructuredHierarchy()
  {
    delete [] this->NumSpheres;
    this->NumSpheres = nullptr;
    delete [] this->Offsets;
    this->Offsets = nullptr;
    delete [] this->CellLoc;
    this->CellLoc = nullptr;
    delete [] this->CellMap;
    this->CellMap = nullptr;
    delete [] this->GridSpheres;
    this->GridSpheres = nullptr;
  }
};

// Threaded helper functions placed in anonymous namespace
//----------------------------------------------------------------------------
namespace {

  //----------------------------------------------------------------------------
  // Compute bounds for each cell in any type of dataset
  struct DataSetSpheres
  {
    vtkDataSet *DataSet;
    double *Spheres;
    double AverageRadius;
    double Bounds[6];
    vtkSMPThreadLocal<double> Radius;
    vtkSMPThreadLocal<vtkIdType> Count;
    vtkSMPThreadLocal<double> XMin;
    vtkSMPThreadLocal<double> XMax;
    vtkSMPThreadLocal<double> YMin;
    vtkSMPThreadLocal<double> YMax;
    vtkSMPThreadLocal<double> ZMin;
    vtkSMPThreadLocal<double> ZMax;

    DataSetSpheres(vtkDataSet *ds, double *s) :
      DataSet(ds), Spheres(s), AverageRadius(0.0)
    {
      this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
      this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 0.0;
    }

    void Initialize()
    {
      double& radius = this->Radius.Local();
      radius = 0.0;

      vtkIdType& count = this->Count.Local();
      count = 0;

      double& xmin = this->XMin.Local();
      xmin = VTK_DOUBLE_MAX;
      double& ymin = this->YMin.Local();
      ymin = VTK_DOUBLE_MAX;
      double& zmin = this->ZMin.Local();
      zmin = VTK_DOUBLE_MAX;

      double& xmax = this->XMax.Local();
      xmax = VTK_DOUBLE_MIN;
      double& ymax = this->YMax.Local();
      ymax = VTK_DOUBLE_MIN;
      double& zmax = this->ZMax.Local();
      zmax = VTK_DOUBLE_MIN;
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      vtkDataSet *ds = this->DataSet;
      double *sphere = this->Spheres + 4*cellId;
      double r, bounds[6];
      double& radius = this->Radius.Local();
      vtkIdType& count = this->Count.Local();
      double& xmin = this->XMin.Local();
      double& ymin = this->YMin.Local();
      double& zmin = this->ZMin.Local();
      double& xmax = this->XMax.Local();
      double& ymax = this->YMax.Local();
      double& zmax = this->ZMax.Local();

      for ( ; cellId < endCellId; ++cellId, sphere+=4)
      {
        ds->GetCellBounds(cellId,bounds);
        sphere[0] = (bounds[0]+bounds[1])/2.0;
        sphere[1] = (bounds[2]+bounds[3])/2.0;
        sphere[2] = (bounds[4]+bounds[5])/2.0;
        sphere[3] = sqrt( (bounds[1]-sphere[0])*(bounds[1]-sphere[0]) +
                          (bounds[3]-sphere[1])*(bounds[3]-sphere[1]) +
                          (bounds[5]-sphere[2])*(bounds[5]-sphere[2]) );

        // Keep a bounds for the dataset
        r = sphere[3];
        xmin = ((sphere[0]-r) < xmin ? (sphere[0]-r) : xmin);
        xmax = ((sphere[0]+r) > xmax ? (sphere[0]+r) : xmax);
        ymin = ((sphere[1]-r) < ymin ? (sphere[1]-r) : ymin);
        ymax = ((sphere[1]+r) > ymax ? (sphere[1]+r) : ymax);
        zmin = ((sphere[2]-r) < zmin ? (sphere[2]-r) : zmin);
        zmax = ((sphere[2]+r) > zmax ? (sphere[2]+r) : zmax);

        // Keep a running average of the radius
        count++;
        radius = radius + (r - radius) / static_cast<double>(count);;
      }
    }

    // Compute approximation to the average radius, compute bounds
    void Reduce()
    {
      vtkSMPThreadLocal<double>::iterator iter;
      vtkSMPThreadLocal<double>::iterator end=this->Radius.end();

      double aveRadius=0.0;
      int numThreads=0;
      for ( iter = this->Radius.begin(); iter != end; ++iter)
      {
        numThreads++;
        aveRadius += *iter;
      }
      if ( numThreads < 1 )
      {
        this->AverageRadius = 1.0;
      }
      else
      {
        this->AverageRadius = aveRadius / static_cast<double>(numThreads);
      }

      // Reduce bounds from all threads
      double xmin = VTK_DOUBLE_MAX;
      for ( iter = this->XMin.begin(); iter != this->XMin.end(); ++iter)
      {
        xmin = ( *iter < xmin ? *iter : xmin );
      }
      double ymin = VTK_DOUBLE_MAX;
      for ( iter = this->YMin.begin(); iter != this->YMin.end(); ++iter)
      {
        ymin = ( *iter < ymin ? *iter : ymin );
      }
      double zmin = VTK_DOUBLE_MAX;
      for ( iter = this->ZMin.begin(); iter != this->ZMin.end(); ++iter)
      {
        zmin = ( *iter < zmin ? *iter : zmin );
      }

      double xmax = VTK_DOUBLE_MIN;
      for ( iter = this->XMax.begin(); iter != this->XMax.end(); ++iter)
      {
        xmax = ( *iter > xmax ? *iter : xmax );
      }
      double ymax = VTK_DOUBLE_MIN;
      for ( iter = this->YMax.begin(); iter != this->YMax.end(); ++iter)
      {
        ymax = ( *iter > ymax ? *iter : ymax );
      }
      double zmax = VTK_DOUBLE_MIN;
      for ( iter = this->ZMax.begin(); iter != this->ZMax.end(); ++iter)
      {
        zmax = ( *iter > zmax ? *iter : zmax );
      }

      this->Bounds[0] = xmin;
      this->Bounds[1] = xmax;
      this->Bounds[2] = ymin;
      this->Bounds[3] = ymax;
      this->Bounds[4] = zmin;
      this->Bounds[5] = zmax;
    }

    void GetBounds(double bounds[6])
    {
      bounds[0] = this->Bounds[0];
      bounds[1] = this->Bounds[1];
      bounds[2] = this->Bounds[2];
      bounds[3] = this->Bounds[3];
      bounds[4] = this->Bounds[4];
      bounds[5] = this->Bounds[5];
    }

    static void Execute(vtkIdType numCells, vtkDataSet *ds,
                        double *s, double& aveRadius, double sphereBounds[6])
    {
      DataSetSpheres spheres(ds, s);
      vtkSMPTools::For(0, numCells, spheres);
      aveRadius = spheres.AverageRadius;
      spheres.GetBounds(sphereBounds);
    }

  };

  //----------------------------------------------------------------------------
  // Compute bounds for each cell in an unstructured grid
  struct UnstructuredSpheres : public DataSetSpheres
  {
    UnstructuredSpheres(vtkUnstructuredGrid *grid, double *s) :
      DataSetSpheres(grid, s)
    {
    }

    void Initialize()
    {
      DataSetSpheres::Initialize();
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      double *sphere = this->Spheres + 4*cellId;
      vtkUnstructuredGrid *grid = static_cast<vtkUnstructuredGrid*>(this->DataSet);
      double cellPts[120], *p, r;
      vtkIdType ptNum, *cellIds, numCellPts;
      double& radius = this->Radius.Local();
      vtkIdType& count = this->Count.Local();
      double& xmin = this->XMin.Local();
      double& ymin = this->YMin.Local();
      double& zmin = this->ZMin.Local();
      double& xmax = this->XMax.Local();
      double& ymax = this->YMax.Local();
      double& zmax = this->ZMax.Local();

      for ( ; cellId < endCellId; ++cellId, sphere+=4)
      {
        grid->GetCellPoints(cellId, numCellPts, cellIds);
        numCellPts = ( numCellPts < 40 ? numCellPts : 40);
        p = cellPts;
        for (ptNum=0; ptNum < numCellPts; ++ptNum, p+=3)
        {
          grid->GetPoint(cellIds[ptNum], p);
        }
        vtkSphere::ComputeBoundingSphere(cellPts, numCellPts, sphere, nullptr);

        // Keep a bounds for the grid
        r = sphere[3];
        xmin = ((sphere[0]-r) < xmin ? (sphere[0]-r) : xmin);
        xmax = ((sphere[0]+r) > xmax ? (sphere[0]+r) : xmax);
        ymin = ((sphere[1]-r) < ymin ? (sphere[1]-r) : ymin);
        ymax = ((sphere[1]+r) > ymax ? (sphere[1]+r) : ymax);
        zmin = ((sphere[2]-r) < zmin ? (sphere[2]-r) : zmin);
        zmax = ((sphere[2]+r) > zmax ? (sphere[2]+r) : zmax);

        // Keep a running average of the radius
        count++;
        radius = radius + (r - radius) / static_cast<double>(count);;
      }
    }

    void Reduce()
    {
      DataSetSpheres::Reduce();
    }

    static void Execute(vtkIdType numCells, vtkUnstructuredGrid *grid,
                        double *s, double& aveRadius, double sphereBounds[6])
    {
      UnstructuredSpheres spheres(grid, s);
      vtkSMPTools::For(0, numCells, spheres);
      aveRadius = spheres.AverageRadius;
      spheres.GetBounds(sphereBounds);
    }

  };


  //----------------------------------------------------------------------------
  // Compute bounds for each cell in a structured grid
  struct StructuredSpheres : public DataSetSpheres
  {
    int Dims[3];
    vtkPoints *Points;

    StructuredSpheres(vtkStructuredGrid *grid, double *s) :
      DataSetSpheres(grid, s)
    {
      grid->GetDimensions(this->Dims);
      this->Points = grid->GetPoints();
    }

    void Initialize()
    {
      DataSetSpheres::Initialize();
    }

    void operator() (vtkIdType slice, vtkIdType endSlice)
    {
      double *p, cellPts[24], r;
      vtkIdType cellIds[8], ptId, idx, i, j, jOffset, kOffset, sliceOffset, hint[2];
      hint[0]=0; hint[1]=6;
      int *dims = this->Dims;
      sliceOffset = static_cast<vtkIdType>(dims[0])*dims[1];
      vtkPoints *inPts=this->Points;
      double *sphere = this->Spheres + slice*4*(dims[0]-1)*(dims[1]-1);
      double& radius = this->Radius.Local();
      vtkIdType& count = this->Count.Local();
      double& xmin = this->XMin.Local();
      double& ymin = this->YMin.Local();
      double& zmin = this->ZMin.Local();
      double& xmax = this->XMax.Local();
      double& ymax = this->YMax.Local();
      double& zmax = this->ZMax.Local();

      for ( ; slice < endSlice; ++slice)
      {
        kOffset = slice*sliceOffset;
        for ( j=0; j < (dims[1]-1); ++j )
        {
          jOffset = j*dims[0];
          for ( i=0; i < (dims[0]-1); ++i )
          {
            ptId = i + jOffset + kOffset;
            cellIds[0] = ptId;
            cellIds[1] = ptId + 1;
            cellIds[2] = ptId + 1 + dims[0];
            cellIds[3] = ptId + dims[0];
            cellIds[4] = ptId + sliceOffset;
            cellIds[5] = ptId + 1 + sliceOffset;
            cellIds[6] = ptId + 1 + dims[0] + sliceOffset;
            cellIds[7] = ptId + dims[0] + sliceOffset;

            p = cellPts;
            for (idx=0; idx<8; ++idx, p+=3)
            {
              inPts->GetPoint(cellIds[idx], p);
            }

            vtkSphere::ComputeBoundingSphere(cellPts, 8, sphere, hint);

            // Keep a bounds for the grid
            r = sphere[3];
            xmin = ((sphere[0]-r) < xmin ? (sphere[0]-r) : xmin);
            xmax = ((sphere[0]+r) > xmax ? (sphere[0]+r) : xmax);
            ymin = ((sphere[1]-r) < ymin ? (sphere[1]-r) : ymin);
            ymax = ((sphere[1]+r) > ymax ? (sphere[1]+r) : ymax);
            zmin = ((sphere[2]-r) < zmin ? (sphere[2]-r) : zmin);
            zmax = ((sphere[2]+r) > zmax ? (sphere[2]+r) : zmax);

            // Keep a running average of the radius
            count++;
            radius = radius + (r - radius) / static_cast<double>(count);;

            sphere += 4;
          }//i
        }//j
      }//slices
    }

    void Reduce()
    {
      DataSetSpheres::Reduce();
    }

    static void Execute(vtkStructuredGrid *grid, double *s)
    {
      StructuredSpheres spheres(grid, s);
      vtkSMPTools::For(0, spheres.Dims[2]-1, spheres);
    }
  };

  //----------------------------------------------------------------------------
  // Base class for selection of cells via geometric operations
  struct BaseCellSelect
  {
    vtkIdType NumberOfCells;
    vtkIdType NumberOfCellsSelected;
    vtkSMPThreadLocal<vtkIdType> NumberSelected;
    unsigned char *Selected;
    double *Spheres;
    double Point[3];

    BaseCellSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                   double p[3]) :
      NumberOfCells(numCells), NumberOfCellsSelected(0),
      Selected(select), Spheres(spheres)
    {
      for (int i=0; i < 3; ++i)
      {
        this->Point[i] = p[i];
      }
      std::fill_n(this->Selected, numCells, 0);
    }

    void Initialize()
    {
      this->NumberOfCellsSelected = 0;
      vtkIdType& numSelected = this->NumberSelected.Local();
      numSelected = 0;
    }

    void Reduce()
    {
      vtkSMPThreadLocal<vtkIdType>::iterator iter;
      vtkSMPThreadLocal<vtkIdType>::iterator end=this->NumberSelected.end();
      this->NumberOfCellsSelected = 0;
      for ( iter = this->NumberSelected.begin(); iter != end; ++iter)
      {
        this->NumberOfCellsSelected += *iter;
      }
    }
  };

  //----------------------------------------------------------------------------
  // Select cells from point based on leaf-level spheres (default)
  struct DefaultPointSelect : public BaseCellSelect
  {
    DefaultPointSelect(vtkIdType numCells, unsigned char *select,
                       double *spheres,double p[3]) :
      BaseCellSelect(numCells, select, spheres, p)
    {
    }

    void Initialize()
    {
      BaseCellSelect::Initialize();
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      double *sphere = this->Spheres + 4*cellId;
      double *p=this->Point;
      unsigned char *s = this->Selected + cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      for ( ; cellId < endCellId; ++cellId, sphere+=4, ++s)
      {
        if ( vtkMath::Distance2BetweenPoints(sphere,p) <= (sphere[3]*sphere[3]) )
        {
          *s = 1;
          ++numSelected;
        }
      }//for cells
    }

    void Reduce()
    {
      BaseCellSelect::Reduce();
    }

  };

  // Select cells with point from unstructured hierarchy
  struct UnstructuredPointSelect : public DefaultPointSelect
  {
    vtkUnstructuredHierarchy *H;

    UnstructuredPointSelect(vtkIdType numCells, unsigned char *select,
                            double *spheres, double p[3],
                            vtkUnstructuredHierarchy *h) :
      DefaultPointSelect(numCells, select, spheres, p), H(h)
    {
    }

    void Initialize()
    {
      DefaultPointSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *spheres = this->Spheres;
      double *sph;
      double *gs = this->H->GridSpheres + 4*gridId;
      double *p=this->Point;
      unsigned char *s = this->Selected;
      const vtkIdType *cellMap=this->H->CellMap;
      const vtkIdType *offsets=this->H->Offsets;
      vtkIdType ii, numSph, cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkMath::Distance2BetweenPoints(gs,p) <= (gs[3]*gs[3]) )
        {
          numSph = offsets[gridId+1] - offsets[gridId];
          for (ii=0; ii<numSph; ++ii)
          {
            cellId = *(cellMap + offsets[gridId] + ii);
            sph = spheres + 4*cellId;
            if ( vtkMath::Distance2BetweenPoints(sph,p) <= (sph[3]*sph[3]) )
            {
              s[cellId] = 1;
              ++numSelected;
            }
          }//for cells in bucket
        }//if bucket sphere intersects point
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultPointSelect::Reduce();
    }
  };

  // Select cells from unstructured hierarchy
  struct StructuredPointSelect : public DefaultPointSelect
  {
    vtkStructuredHierarchy *H;

    StructuredPointSelect(vtkIdType numCells, unsigned char *select,
                          double *spheres, double p[3], vtkStructuredHierarchy *h):
      DefaultPointSelect(numCells, select, spheres, p), H(h)
    {
    }

    void Initialize()
    {
      DefaultPointSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *p=this->Point;
      unsigned char *s = this->Selected;
      double *spheres = this->Spheres;
      double *sph, *gs = this->H->GridSpheres + 4*gridId;
      const vtkIdType *gridDims = this->H->GridDims;
      int gridSliceOffset = gridDims[0]*gridDims[1];
      const vtkIdType *dims = this->H->Dims;
      vtkIdType i0, j0, k0, i, j, k, jOffset, kOffset, sliceOffset=dims[0]*dims[1];
      vtkIdType iEnd, jEnd, kEnd, cellId;
      int resolution = this->H->Resolution;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect the point are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkMath::Distance2BetweenPoints(gs,p) <= (gs[3]*gs[3]) )
        {
          // i-j-k coordinates in grid space
          i0 = (gridId % gridDims[0]) * resolution;
          j0 = ((gridId / gridDims[0]) % gridDims[1]) * resolution;
          k0 = (gridId / gridSliceOffset) * resolution;

          iEnd = ((i0 + resolution) < dims[0] ? i0 + resolution : dims[0]);
          jEnd = ((j0 + resolution) < dims[1] ? j0 + resolution : dims[1]);
          kEnd = ((k0 + resolution) < dims[2] ? k0 + resolution : dims[2]);

          // Now loop over resolution*resolution*resolution block of leaf cells
          for (k=k0; k<kEnd; ++k)
          {
            kOffset = k * sliceOffset;
            for (j=j0; j<jEnd; ++j)
            {
              jOffset = j * dims[0];
              for (i=i0; i<iEnd; ++i)
              {
                cellId = (i + jOffset + kOffset);
                sph = spheres + 4*cellId;
                if ( vtkMath::Distance2BetweenPoints(sph,p) <= (sph[3]*sph[3]) )
                {
                  s[cellId] = 1; //mark as candidate
                  ++numSelected;
                }
              }
            }
          }

        }//if bucket sphere contains point
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultPointSelect::Reduce();
    }
  };

  //----------------------------------------------------------------------------
  // Select cells from line based on leaf-level spheres (default)
  struct DefaultLineSelect : public BaseCellSelect
  {
    double P1[3];

    DefaultLineSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                      double p[3], double ray[3]) :
      BaseCellSelect(numCells, select, spheres, p)
    {
      for (int i=0; i < 3; ++i)
      {
        this->P1[i] = this->Point[i] + ray[i];
      }
    }

    void Initialize()
    {
      BaseCellSelect::Initialize();
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      double *sph = this->Spheres + 4*cellId;
      double *p0=this->Point, *p1=this->P1;
      unsigned char *s = this->Selected + cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      for ( ; cellId < endCellId; ++cellId, sph+=4, ++s)
      {
        if ( vtkLine::DistanceToLine(sph,p0,p1) <= (sph[3]*sph[3]) )
        {
          *s = 1;
          ++numSelected;
        }
      }//for cells
    }

    void Reduce()
    {
      BaseCellSelect::Reduce();
    }
  };

  // Select cells with line from unstructured hierarchy
  struct UnstructuredLineSelect : public DefaultLineSelect
  {
    vtkUnstructuredHierarchy *H;

    UnstructuredLineSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                            vtkUnstructuredHierarchy *h, double o[3], double ray[3]) :
      DefaultLineSelect(numCells, select, spheres, o, ray), H(h)
    {
    }

    void Initialize()
    {
      DefaultLineSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *spheres = this->Spheres;
      double *sph;
      double *gs = this->H->GridSpheres + 4*gridId;
      double *p0=this->Point, *p1=this->P1;
      unsigned char *s = this->Selected;
      const vtkIdType *cellMap=this->H->CellMap;
      const vtkIdType *offsets=this->H->Offsets;
      vtkIdType ii, numSph, cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkLine::DistanceToLine(gs,p0,p1) <= gs[3] )
        {
          numSph = offsets[gridId+1] - offsets[gridId];
          for (ii=0; ii<numSph; ++ii)
          {
            cellId = *(cellMap + offsets[gridId] + ii);
            sph = spheres + 4*cellId;
            if ( vtkLine::DistanceToLine(sph,p0,p1) <= (sph[3]*sph[3]) )
            {
              s[cellId] = 1;
              ++numSelected;
            }
          }//for cells in bucket
        }//if bucket sphere intersects line
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultLineSelect::Reduce();
    }
  };

  // Select cells from unstructured hierarchy
  struct StructuredLineSelect : public DefaultLineSelect
  {
    vtkStructuredHierarchy *H;

    StructuredLineSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                          vtkStructuredHierarchy *h, double o[3], double ray[3]) :
      DefaultLineSelect(numCells, select, spheres, o, ray), H(h)
    {
    }

    void Initialize()
    {
      DefaultLineSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *p0=this->Point, *p1=this->P1;
      unsigned char *s = this->Selected;
      double *spheres = this->Spheres;
      double *sph, *gs = this->H->GridSpheres + 4*gridId;
      const vtkIdType *gridDims = this->H->GridDims;
      int gridSliceOffset = gridDims[0]*gridDims[1];
      const vtkIdType *dims = this->H->Dims;
      vtkIdType i0, j0, k0, i, j, k, jOffset, kOffset, sliceOffset=dims[0]*dims[1];
      vtkIdType iEnd, jEnd, kEnd, cellId;
      int resolution = this->H->Resolution;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect the line are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkLine::DistanceToLine(gs,p0,p1) <= gs[3] )
        {
          // i-j-k coordinates in grid space
          i0 = (gridId % gridDims[0]) * resolution;
          j0 = ((gridId / gridDims[0]) % gridDims[1]) * resolution;
          k0 = (gridId / gridSliceOffset) * resolution;

          iEnd = ((i0 + resolution) < dims[0] ? i0 + resolution : dims[0]);
          jEnd = ((j0 + resolution) < dims[1] ? j0 + resolution : dims[1]);
          kEnd = ((k0 + resolution) < dims[2] ? k0 + resolution : dims[2]);

          // Now loop over resolution*resolution*resolution block of leaf cells
          for (k=k0; k<kEnd; ++k)
          {
            kOffset = k * sliceOffset;
            for (j=j0; j<jEnd; ++j)
            {
              jOffset = j * dims[0];
              for (i=i0; i<iEnd; ++i)
              {
                cellId = (i + jOffset + kOffset);
                sph = spheres + 4*cellId;
                if ( vtkLine::DistanceToLine(sph,p0,p1) <= (sph[3]*sph[3]) )
                {
                  s[cellId] = 1; //mark as candidate
                  ++numSelected;
                }
              }
            }
          }

        }//if bucket sphere intersects line
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultLineSelect::Reduce();
    }
  };

  //----------------------------------------------------------------------------
  // Select cells from plane based on leaf-level spheres (default)
  struct DefaultPlaneSelect : public BaseCellSelect
  {
    double Normal[3];

    DefaultPlaneSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                       double o[3], double n[3]) :
      BaseCellSelect(numCells, select, spheres, o)
    {
      for (int i=0; i < 3; ++i)
      {
        this->Normal[i] = n[i];
      }
      vtkMath::Normalize(this->Normal);
    }

    void Initialize()
    {
      BaseCellSelect::Initialize();
    }

    void operator() (vtkIdType cellId, vtkIdType endCellId)
    {
      double *sphere = this->Spheres + 4*cellId;
      double *o=this->Point, *n=this->Normal;
      unsigned char *s = this->Selected + cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      for ( ; cellId < endCellId; ++cellId, sphere+=4, ++s)
      {
        if ( vtkPlane::DistanceToPlane(sphere,n,o) <= sphere[3] )
        {
          *s = 1;
          ++numSelected;
        }
      }//for cells
    }

    void Reduce()
    {
      BaseCellSelect::Reduce();
    }
  };

  // Select cells with plane from unstructured hierarchy
  struct UnstructuredPlaneSelect : public DefaultPlaneSelect
  {
    vtkUnstructuredHierarchy *H;

    UnstructuredPlaneSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                            vtkUnstructuredHierarchy *h, double o[3], double n[3]) :
      DefaultPlaneSelect(numCells, select, spheres, o, n), H(h)
    {
    }

    void Initialize()
    {
      DefaultPlaneSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *spheres = this->Spheres;
      double *sph;
      double *gs = this->H->GridSpheres + 4*gridId;
      double *o=this->Point, *n=this->Normal;
      unsigned char *s = this->Selected;
      const vtkIdType *cellMap=this->H->CellMap;
      const vtkIdType *offsets=this->H->Offsets;
      vtkIdType ii, numSph, cellId;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkPlane::DistanceToPlane(gs,n,o) <= gs[3] )
        {
          numSph = offsets[gridId+1] - offsets[gridId];
          for (ii=0; ii<numSph; ++ii)
          {
            cellId = *(cellMap + offsets[gridId] + ii);
            sph = spheres + 4*cellId;
            if ( vtkPlane::DistanceToPlane(sph,n,o) <= sph[3] )
            {
              s[cellId] = 1;
              ++numSelected;
            }
          }//for cells in bucket
        }//if bucket sphere intersects plane
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultPlaneSelect::Reduce();
    }
  };

  // Select cells from unstructured hierarchy
  struct StructuredPlaneSelect : public DefaultPlaneSelect
  {
    vtkStructuredHierarchy *H;

    StructuredPlaneSelect(vtkIdType numCells, unsigned char *select, double *spheres,
                          vtkStructuredHierarchy *h, double o[3], double n[3]) :
      DefaultPlaneSelect(numCells, select, spheres, o, n), H(h)
    {
    }

    void Initialize()
    {
      DefaultPlaneSelect::Initialize();
    }

    void operator() (vtkIdType gridId, vtkIdType endGridId)
    {
      double *o=this->Point, *n=this->Normal;
      unsigned char *s = this->Selected;
      double *spheres = this->Spheres;
      double *sph, *gs = this->H->GridSpheres + 4*gridId;
      const vtkIdType *gridDims = this->H->GridDims;
      int gridSliceOffset = gridDims[0]*gridDims[1];
      const vtkIdType *dims = this->H->Dims;
      vtkIdType i0, j0, k0, i, j, k, jOffset, kOffset, sliceOffset=dims[0]*dims[1];
      vtkIdType iEnd, jEnd, kEnd, cellId;
      int resolution = this->H->Resolution;
      vtkIdType& numSelected = this->NumberSelected.Local();

      // Loop over grid buckets. The cell spheres that are located in buckets
      // that intersect the plane are processed further.
      for ( ; gridId < endGridId; ++gridId, gs+=4 )
      {
        if ( vtkPlane::DistanceToPlane(gs,n,o) <= gs[3] )
        {
          // i-j-k coordinates in grid space
          i0 = (gridId % gridDims[0]) * resolution;
          j0 = ((gridId / gridDims[0]) % gridDims[1]) * resolution;
          k0 = (gridId / gridSliceOffset) * resolution;

          iEnd = ((i0 + resolution) < dims[0] ? i0 + resolution : dims[0]);
          jEnd = ((j0 + resolution) < dims[1] ? j0 + resolution : dims[1]);
          kEnd = ((k0 + resolution) < dims[2] ? k0 + resolution : dims[2]);

          // Now loop over resolution*resolution*resolution block of leaf cells
          for (k=k0; k<kEnd; ++k)
          {
            kOffset = k * sliceOffset;
            for (j=j0; j<jEnd; ++j)
            {
              jOffset = j * dims[0];
              for (i=i0; i<iEnd; ++i)
              {
                cellId = (i + jOffset + kOffset);
                sph = spheres + 4*cellId;
                if ( vtkPlane::DistanceToPlane(sph,n,o) <= sph[3] )
                {
                  s[cellId] = 1; //mark as candidate
                  ++numSelected;
                }
              }
            }
          }

        }//if bucket sphere intersects plane
      }//for grid buckets
    }

    void Reduce()
    {
      DefaultPlaneSelect::Reduce();
    }
  };

}//anonymous namespace



//================================Sphere Tree class proper===================
//----------------------------------------------------------------------------
// Construct object.
vtkSphereTree::vtkSphereTree()
{
  this->DataSet = nullptr;
  this->Selected = nullptr;
  this->Resolution = 3;
  this->MaxLevel = 10;
  this->NumberOfLevels = 0;
  this->Tree = nullptr;
  this->Hierarchy = nullptr;
  this->BuildHierarchy = 1;
  this->SphereTreeType = VTK_SPHERE_TREE_HIERARCHY_NONE;
  this->AverageRadius = 0.0;
  this->SphereBounds[0] = this->SphereBounds[1] = this->SphereBounds[2] = 0.0;
  this->SphereBounds[3] = this->SphereBounds[4] = this->SphereBounds[5] = 0.0;
}

//----------------------------------------------------------------------------
// Destroy object.
vtkSphereTree::~vtkSphereTree()
{
  this->SetDataSet(nullptr);
  if ( this->Selected )
  {
    delete [] this->Selected;
    this->Selected = nullptr;
  }
  if ( this->Tree )
    {
    this->Tree->Delete();
    this->Tree = nullptr;
    }
  if ( this->Hierarchy )
    {
    delete this->Hierarchy;
    this->Hierarchy = nullptr;
    }
}

//================General tree methods========================================
//----------------------------------------------------------------------------
void vtkSphereTree::Build()
{
  if ( ! this->DataSet )
  {
    return;
  }
  else
  {
    this->Build(this->DataSet);
  }
}

//----------------------------------------------------------------------------
void vtkSphereTree::Build(vtkDataSet *input)
{
  this->SetDataSet(input);

  if ( this->Tree != nullptr && this->Hierarchy != nullptr &&
       this->BuildTime > this->MTime &&
       (this->BuildTime > this->DataSet->GetMTime()) )
    {
    return;
    }

  this->SphereTreeType = VTK_SPHERE_TREE_HIERARCHY_NONE;
  this->BuildTreeSpheres(input);
  if ( this->BuildHierarchy )
    {
    this->BuildTreeHierarchy(input);
    }

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
// Compute the sphere tree leafs (i.e., spheres around each cell)
vtkDoubleArray *vtkSphereTree::BuildTreeSpheres(vtkDataSet *input)
{
  // See if anything has to be done
  if ( this->Tree != nullptr && this->BuildTime > this->MTime )
    {
    return this->Tree;
    }

  // Allocate
  //
  vtkIdType numCells = input->GetNumberOfCells();
  vtkDoubleArray *newScalars = vtkDoubleArray::New();
  newScalars->SetNumberOfComponents(4);
  newScalars->SetNumberOfTuples(input->GetNumberOfCells());
  this->Tree = newScalars;
  this->TreePtr = newScalars->GetPointer(0);

  this->Selected = new unsigned char [numCells];

  if (input->GetDataObjectType() == VTK_STRUCTURED_GRID)
  {
    StructuredSpheres::Execute(vtkStructuredGrid::SafeDownCast(input), this->TreePtr);
  }

  else if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
  {
    UnstructuredSpheres::Execute(numCells, vtkUnstructuredGrid::SafeDownCast(input),
                                 this->TreePtr, this->AverageRadius, this->SphereBounds);
  }

  else //default algorithm
  {
    DataSetSpheres::Execute(numCells, input, this->TreePtr,
                            this->AverageRadius, this->SphereBounds);
  }

  this->BuildTime.Modified();

  return newScalars;
}

//----------------------------------------------------------------------------
void vtkSphereTree::
BuildTreeHierarchy(vtkDataSet *input)
{
  // See if anything has to be done
  if ( this->Tree != nullptr && this->Hierarchy != nullptr &&
       this->BuildTime > this->MTime )
  {
      return;
  }

  if (input->GetDataObjectType() == VTK_STRUCTURED_GRID)
  {
    vtkSphereTree::
      BuildStructuredHierarchy(vtkStructuredGrid::SafeDownCast(input),
                               this->TreePtr);
  }

  else if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
  {
    vtkSphereTree::
      BuildUnstructuredHierarchy(vtkUnstructuredGrid::SafeDownCast(input),
                                 this->TreePtr);
  }

  else //default hierarchy
  {
    vtkSphereTree::BuildUnstructuredHierarchy(input, this->TreePtr);
  }

  this->BuildTime.Modified();
}


//================Specialized methods for structured grids====================
//----------------------------------------------------------------------------
// From the leaf spheres, build a sphere tree. Use the structure of the grid
// to control how the sphere tree hierarchy is constructed.
void vtkSphereTree::
BuildStructuredHierarchy(vtkStructuredGrid *input, double *tree)
{
  this->SphereTreeType = VTK_SPHERE_TREE_HIERARCHY_STRUCTURED;

  // Determine the lay of the land. Note that the code below can build more than
  // the two levels, but for now we clamp to just two levels (the tree leaf
  // spheres plus one level up).
  //  vtkIdType numLevels = this->NumberOfLevels = this->MaxLevel;
  vtkIdType numLevels = this->NumberOfLevels = 2;
  vtkIdType i;
  int lDims[VTK_MAX_SPHERE_TREE_LEVELS][3], size[VTK_MAX_SPHERE_TREE_LEVELS];
  int resolution = this->Resolution;

  // Configure the various levels
  int curLevel = numLevels - 1;
  input->GetDimensions(lDims[curLevel]);
  lDims[curLevel][0] -= 1;
  lDims[curLevel][1] -= 1;
  lDims[curLevel][2] -= 1;
  size[curLevel] = lDims[curLevel][0]*lDims[curLevel][1]*lDims[curLevel][2];
  vtkIdType totalSize=0;
  for (i=numLevels-2; i >= 0; --i)
  {
    lDims[i][0] = (lDims[i+1][0]-1)/resolution + 1;
    lDims[i][1] = (lDims[i+1][1]-1)/resolution + 1;
    lDims[i][2] = (lDims[i+1][2]-1)/resolution + 1;
    size[i] = lDims[i][0]*lDims[i][1]*lDims[i][2];
    totalSize += size[i];
  }

  // Allocate space and set up storage.
  delete this->Hierarchy; //cleanup if necessary
  vtkStructuredHierarchy *sH =
    new vtkStructuredHierarchy(input->GetNumberOfCells(),4*totalSize+2);
  this->Hierarchy = sH;

  double *sphere, *spheres[VTK_MAX_SPHERE_TREE_LEVELS];
  spheres[0] = sH->H->GetPointer(0);
  spheres[0][0] = numLevels;
  spheres[0][1] = resolution;
  spheres[0] += 2;
  // As long as numLevels=2; then this is dead code and hence commented out.
  // for (i=1; i < numLevels-1; ++i)
  // {
  //   spheres[i] = spheres[i-1] + 4*size[i-1];
  // }
  spheres[curLevel] = tree;

  // For now, we are going to do something really simple stupid. That is,
  // cull based on blocks of cells one level up from leaf spheres. In the
  // future this will be optimized.
  sH->Dims[0] = lDims[curLevel][0];
  sH->Dims[1] = lDims[curLevel][1];
  sH->Dims[2] = lDims[curLevel][2];
  sH->Resolution = resolution;
  sH->GridSize = size[curLevel-1];
  sH->GridDims[0] = lDims[curLevel-1][0];
  sH->GridDims[1] = lDims[curLevel-1][2];
  sH->GridDims[2] = lDims[curLevel-1][2];
  sH->GridSpheres = spheres[curLevel-1];

  // Loop over all levels, from the bottom up, determining sphere tree from level below
  vtkIdType level, j, k, jOffset, kOffset, sliceOffset;
  vtkIdType ii, jj, kk, iStart, iEnd, jStart, jEnd, kStart, kEnd;
  vtkIdType jjOffset, kkOffset, blockSliceOffset, hints[2], numSpheres;
  double *blockSpheres[VTK_MAX_SPHERE_TREE_RESOLUTION *
                       VTK_MAX_SPHERE_TREE_RESOLUTION *
                       VTK_MAX_SPHERE_TREE_RESOLUTION];
  hints[0] = 0;

  for ( level=numLevels-2; level >= 0; --level)
  {
    sliceOffset = static_cast<vtkIdType>(lDims[level][0])*lDims[level][1];
    for (k=0; k < lDims[level][2]; ++k)
    {
      kOffset = k*sliceOffset;
      kStart = k * resolution;
      kEnd = ( kStart + resolution < lDims[level+1][2] ?
               kStart+resolution : lDims[level+1][2]);
      for (j=0; j < lDims[level][1]; ++j)
      {
        jOffset = j*lDims[level][0];
        jStart = j * resolution;
        jEnd = ( jStart + resolution < lDims[level+1][1] ?
                 jStart+resolution : lDims[level+1][1]);
        for (i=0; i < lDims[level][0]; ++i)
        {
          iStart = i * resolution;
          iEnd = ( iStart + resolution < lDims[level+1][0] ?
                   iStart+resolution : lDims[level+1][0]);
          sphere = spheres[level] + 4*(i + jOffset + kOffset);
          numSpheres = 0;

          // Now compute bounding sphere for this block of spheres
          hints[1] = (iEnd-iStart)*(jEnd-jStart)*(kEnd-kStart) - 1;

          blockSliceOffset = static_cast<vtkIdType>(lDims[level+1][0])*lDims[level+1][1];
          for ( kk=kStart; kk < kEnd; ++kk)
          {
            kkOffset = kk * blockSliceOffset;
            for ( jj=jStart; jj < jEnd; ++jj)
            {
              jjOffset = jj * lDims[level+1][0];
              for ( ii=iStart; ii < iEnd; ++ii)
              {
                blockSpheres[numSpheres++] = spheres[level+1] + 4*(ii + jjOffset + kkOffset);
              }//for sub-block ii
            }//for sub-block jj
          }//for sub-block kk
          vtkSphere::ComputeBoundingSphere(blockSpheres,numSpheres, sphere, hints);
        }//for i
      }//for j
    }//for k
  }//for all levels
}

//================Specialized methods for unstructured grids====================
// Here we create a pointerless binary sphere tree. The order of the spheres
// is implicit with the ordering of the cells. Note that the statistics
// gathered in the previous step are used to organize the grid. The average
// radius controls whether to create lots of spheres or less. Too many
// spheres is wasteful; too few and the computational benefit of the sphere
// tree is reduced.
//
// Based on the average radius and bounds, we'll grid a regular grid
// subdivided n x m x o in the x-y-z directions. We will attempt to
// make the grid buckets cubical. Once the grid is formed, cell
// spheres will be assigned to the grid buckets based on where the
// sphere's center is located. Finally, spheres will be associated
// with each grid bucket (which bound all spheres contained within the
// grid bucket).
void vtkSphereTree::
BuildUnstructuredHierarchy(vtkDataSet *input, double *tree)
{
  this->SphereTreeType = VTK_SPHERE_TREE_HIERARCHY_UNSTRUCTURED;

  // Make sure we have something to do.
  vtkIdType numCells = input->GetNumberOfCells();
  if ( this->AverageRadius <= 0.0 || numCells <= 0 )
  {
    delete this->Hierarchy;
    this->Hierarchy = nullptr;
  }

  // Currently only two levels are being built (see implementation notes).
  this->NumberOfLevels = 2;

  // Compute the grid resolution in the x-y-z directions. Assume that
  // a grid cell should be this->Resolution times bigger than the average
  // radius (in each direction).
  double spacing[3], r=this->AverageRadius, *bds=this->SphereBounds;
  int dims[3], res=this->Resolution;
  for (int i=0; i<3; ++i)
  {
    dims[i] = static_cast<int>( (bds[2*i+1]-bds[2*i])/(res*r) );
    dims[i] = ( dims[i] < 1 ? 1 : dims[i] );
    spacing[i] = (bds[2*i+1]-bds[2*i]) / dims[i];
  }

  // We are ready to create the hierarchy
  vtkUnstructuredHierarchy *h;
  this->Hierarchy = h =
    new vtkUnstructuredHierarchy(dims,bds,spacing,numCells);
  vtkIdType *cellLoc=h->CellLoc, *cellMap=h->CellMap;
  vtkIdType *numSpheres=h->NumSpheres, *offsets=h->Offsets;
  vtkIdType gridSize=h->GridSize;

  // Okay loop over all cell spheres and assign them to the grid cells.
  vtkIdType cellId, i, j, k, ii, idx, sliceOffset=static_cast<vtkIdType>(dims[0])*dims[1];
  double *sphere=tree;
  for ( cellId=0; cellId < numCells; ++cellId, sphere+=4 )
  {
    i = static_cast<int>( dims[0] * (sphere[0] - bds[0]) / (bds[1]-bds[0]) );
    j = static_cast<int>( dims[1] * (sphere[1] - bds[2]) / (bds[3]-bds[2]) );
    k = static_cast<int>( dims[2] * (sphere[2] - bds[4]) / (bds[5]-bds[4]) );
    idx = i + j*dims[0] + k*sliceOffset;
    cellLoc[cellId] = idx;
    numSpheres[idx]++;
  }

  // Compute offsets into linear array. Also remember the max number of spheres
  // in any given bucket (for subsequent memory allocation).
  vtkIdType maxNumSpheres=numSpheres[0];
  offsets[0] = 0;
  for ( idx=1; idx < gridSize; ++idx )
  {
    offsets[idx] = offsets[idx-1] + numSpheres[idx-1];
    maxNumSpheres = (numSpheres[idx] > maxNumSpheres ?
                     numSpheres[idx] : maxNumSpheres);
  }
  offsets[gridSize] = numCells;

  // Now associate cells with appropriate grid buckets.
  for ( cellId=0; cellId < numCells; ++cellId )
  {
    idx = cellLoc[cellId];
    *(cellMap + offsets[idx] + numSpheres[idx] - 1) = cellId;
    numSpheres[idx]--; //counting down towards offset
  }

  // Free extra data. What we have left is a grid with cells associated
  // with each bucket.
  delete [] h->NumSpheres; h->NumSpheres=nullptr;
  delete [] h->CellLoc; h->CellLoc=nullptr;

  // Now it's time to create a sphere per bucket, and adjust the spheres
  // to fit all of the cell spheres contained within it.
  double **tmpSpheres = new double* [maxNumSpheres];
  h->GridSpheres = new double [4*gridSize];
  double *gs=h->GridSpheres;
  vtkIdType nSph;

  for ( k=0; k<dims[2]; ++k)
  {
    for ( j=0; j<dims[1]; ++j)
    {
      for ( i=0; i<dims[0]; ++i)
      {
        idx = i + j*dims[0] + k*sliceOffset;
        nSph = offsets[idx+1] - offsets[idx];
        for (ii=0; ii<nSph; ++ii)
        {
          cellId = *(cellMap + offsets[idx] + ii);
          tmpSpheres[ii] = tree + 4*cellId;
        }
        vtkSphere::ComputeBoundingSphere(tmpSpheres,nSph,gs,nullptr);
        gs += 4;
      }//i
    }//j
  }//k

  // Cleanup
  delete [] tmpSpheres;
}

//----------------------------------------------------------------------------
// Note that there is a long story behind these crude methods for selecting
// cells based on a sphere tree. Initially there was a complex hierarchy of
// iterators for different dataset types and geometric intersection entities
// (e.g., point, line or plane). However the performance of this approach was
// really poor and the code was excessively complex. To do it right requires
// extensive templating etc. Maybe someday.... In the mean time this approach
// (using a selection mask) is really simple and performs pretty well. It
// also suggests future approaches which use cell locators (and other
// classes) to produce selection masks as well.
const unsigned char* vtkSphereTree::
SelectPoint(double x[3], vtkIdType &numSelected)
{
  // Check input
  if ( this->DataSet == nullptr )
  {
    return nullptr;
  }

  vtkIdType numCells = this->DataSet->GetNumberOfCells();

  // Specialized for structured grids
  if ( this->Hierarchy &&
       this->DataSet->GetDataObjectType() == VTK_STRUCTURED_GRID )
  {
    vtkStructuredHierarchy *h =
      static_cast<vtkStructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    StructuredPointSelect sPointSelect(numCells, this->Selected, this->TreePtr, x, h);
    vtkSMPTools::For(0,gridSize,sPointSelect);
    numSelected = sPointSelect.NumberOfCellsSelected;
  }

  // Specialized for unstructured grids
  else if ( this->Hierarchy &&
            this->DataSet->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
  {
    vtkUnstructuredHierarchy *h =
      static_cast<vtkUnstructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    UnstructuredPointSelect uPointSelect(numCells, this->Selected, this->TreePtr, x, h);
    vtkSMPTools::For(0,gridSize,uPointSelect);
    numSelected = uPointSelect.NumberOfCellsSelected;
  }

  // default, process leaf spheres without hierarchy
  else
  {
    DefaultPointSelect defaultPointSelect(numCells, this->Selected,
                                         this->TreePtr, x);
    vtkSMPTools::For(0,numCells,defaultPointSelect);
    numSelected = defaultPointSelect.NumberOfCellsSelected;
  }

  return this->Selected;
}

//----------------------------------------------------------------------------
// Create selection mask based on intersection with an infinite line.
const unsigned char* vtkSphereTree::
SelectLine(double origin[3], double ray[3], vtkIdType &numSelected)
{
  // Check input
  if ( this->DataSet == nullptr )
  {
    return nullptr;
  }

  vtkIdType numCells = this->DataSet->GetNumberOfCells();;

  // Specialized for structured grids
  if ( this->Hierarchy &&
       this->DataSet->GetDataObjectType() == VTK_STRUCTURED_GRID )
  {
    vtkStructuredHierarchy *h =
      static_cast<vtkStructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    StructuredLineSelect sLineSelect(numCells, this->Selected, this->TreePtr, h,
                                     origin, ray);
    vtkSMPTools::For(0,gridSize,sLineSelect);
    numSelected = sLineSelect.NumberOfCellsSelected;
  }

  // Specialized for unstructured grids
  else if ( this->Hierarchy &&
            this->DataSet->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
  {
    vtkUnstructuredHierarchy *h =
      static_cast<vtkUnstructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    UnstructuredLineSelect uLineSelect(numCells, this->Selected, this->TreePtr, h,
                                       origin, ray);
    vtkSMPTools::For(0,gridSize,uLineSelect);
    numSelected = uLineSelect.NumberOfCellsSelected;
  }

  // default, process leaf spheres without hierarchy
  else
  {
    DefaultLineSelect defaultLineSelect(numCells, this->Selected,
                                        this->TreePtr, origin, ray);
    vtkSMPTools::For(0,numCells,defaultLineSelect);
    numSelected = defaultLineSelect.NumberOfCellsSelected;
  }

  return this->Selected;
}

//----------------------------------------------------------------------------
// Create selection mask based on intersection with an infinite plane.
const unsigned char* vtkSphereTree::
SelectPlane(double origin[3], double normal[3], vtkIdType &numSelected)
{
  // Check input
  if ( this->DataSet == nullptr )
  {
    return nullptr;
  }

  vtkIdType numCells = this->DataSet->GetNumberOfCells();;

  // Specialized for structured grids
  if ( this->Hierarchy &&
       this->DataSet->GetDataObjectType() == VTK_STRUCTURED_GRID )
  {
    vtkStructuredHierarchy *h =
      static_cast<vtkStructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    StructuredPlaneSelect sPlaneSelect(numCells, this->Selected, this->TreePtr, h,
                                         origin, normal);
    vtkSMPTools::For(0,gridSize,sPlaneSelect);
    numSelected = sPlaneSelect.NumberOfCellsSelected;
  }

  // Specialized for unstructured grids
  else if ( this->Hierarchy &&
            this->DataSet->GetDataObjectType() == VTK_UNSTRUCTURED_GRID )
  {
    vtkUnstructuredHierarchy *h =
      static_cast<vtkUnstructuredHierarchy*>(this->Hierarchy);
    vtkIdType gridSize = h->GridSize;
    UnstructuredPlaneSelect uPlaneSelect(numCells, this->Selected, this->TreePtr, h,
                                         origin, normal);
    vtkSMPTools::For(0,gridSize,uPlaneSelect);
    numSelected = uPlaneSelect.NumberOfCellsSelected;
  }

  // default, process leaf spheres without hierarchy
  else
  {
    DefaultPlaneSelect defaultPlaneSelect(numCells, this->Selected,
                                          this->TreePtr, origin, normal);
    vtkSMPTools::For(0,numCells,defaultPlaneSelect);
    numSelected = defaultPlaneSelect.NumberOfCellsSelected;
  }

  return this->Selected;

}

//----------------------------------------------------------------------------
// Simply return the leaf spheres
const double* vtkSphereTree::GetCellSpheres()
{
  return this->TreePtr;
}

//----------------------------------------------------------------------------
// The number of levels is this->NumberOfLevels, with
// (this->NumberOfLevels-1) the cell (leaf) spheres, and level 0 the root
// level.
const double* vtkSphereTree::GetTreeSpheres(int level, vtkIdType& numSpheres)
{
  int numLevels = this->NumberOfLevels;

  // Check input for simple cases
  if ( level == (numLevels - 1) )
  {
    numSpheres = this->DataSet->GetNumberOfCells();
    return this->TreePtr; //just return leaf spheres
  }
  else if ( level < 0 || level >= numLevels ||
            this->DataSet == nullptr || this->Hierarchy == nullptr )
  {
    numSpheres = 0;
    return nullptr;
  }

  // Asking for spheres within tree hierarchy
  if ( this->SphereTreeType == VTK_SPHERE_TREE_HIERARCHY_STRUCTURED )
  {
    vtkStructuredHierarchy *h =
      static_cast<vtkStructuredHierarchy*>(this->Hierarchy);
    numSpheres = h->GridSize;
    return h->GridSpheres;
  }
  else if ( this->SphereTreeType == VTK_SPHERE_TREE_HIERARCHY_UNSTRUCTURED )
  {
    vtkUnstructuredHierarchy *h =
      static_cast<vtkUnstructuredHierarchy*>(this->Hierarchy);
    numSpheres = h->GridSize;
    return h->GridSpheres;
  }

  // worst case shouldn't happen
  numSpheres = 0;
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSphereTree::
SelectPoint(double point[3], vtkIdList *cellIds)
{
  vtkIdType numSelected;
  const unsigned char *selected = this->SelectPoint(point, numSelected);
  this->ExtractCellIds(selected,cellIds,numSelected);
}

//----------------------------------------------------------------------------
void vtkSphereTree::
SelectLine(double origin[3], double ray[3], vtkIdList *cellIds)
{
  vtkIdType numSelected;
  const unsigned char *selected = this->SelectLine(origin,ray,numSelected);
  this->ExtractCellIds(selected,cellIds,numSelected);
}

//----------------------------------------------------------------------------
void vtkSphereTree::
SelectPlane(double origin[3], double normal[3], vtkIdList *cellIds)
{
  vtkIdType numSelected;
  const unsigned char *selected = this->SelectPlane(origin,normal,numSelected);
  this->ExtractCellIds(selected,cellIds,numSelected);
}

//----------------------------------------------------------------------------
void vtkSphereTree::
ExtractCellIds(const unsigned char *selected, vtkIdList *cellIds,
               vtkIdType numSelected)
{
  if ( numSelected < 1 || selected == nullptr )
  {
    cellIds->Reset();
  }
  else
  {
    const unsigned char *s = selected;
    vtkIdType numCells = this->DataSet->GetNumberOfCells();
    vtkIdType numInserted=0;
    cellIds->SetNumberOfIds(numSelected);
    for (vtkIdType cellId=0; cellId < numCells; ++cellId)
    {
      if ( *s++ > 0 )
      {
        cellIds->SetId(numInserted,cellId);
        numInserted++;
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------
void vtkSphereTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Number Of Levels: " << this->NumberOfLevels << "\n";
  os << indent << "Maximum Number Of Levels: " << this->MaxLevel << "\n";
  os << indent << "Build Hierarchy: "
     << (this->BuildHierarchy ? "On\n" : "Off\n");
}
