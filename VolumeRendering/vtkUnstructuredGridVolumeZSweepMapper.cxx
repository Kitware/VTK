/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeZSweepMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridVolumeZSweepMapper.h"

#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRayCastImageDisplayHelper.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkGenericCell.h"
#include "vtkPriorityQueue.h"
#include "vtkIdList.h"
#include "vtkVolumeProperty.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkUnstructuredGridPreIntegration.h"
#include "vtkUnstructuredGridPartialPreIntegration.h"
#include "vtkUnstructuredGridHomogeneousRayIntegrator.h"
#include "vtkDoubleArray.h"
#include "vtkDataArray.h"

#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPointData.h"

#include <assert.h>
#include <string.h> // memset()
#include <vtkstd/vector>
#include <vtkstd/list>

// do not remove the following line:
//#define BACK_TO_FRONT

// Put the internal classes in a namespace to avoid potential naming conflicts.
namespace vtkUnstructuredGridVolumeZSweepMapperNamespace
{

enum
{
  VTK_VALUES_X_INDEX=0, //  world coordinate
  VTK_VALUES_Y_INDEX, //  world coordinate
  VTK_VALUES_Z_INDEX, //  world coordinate
  VTK_VALUES_SCALAR_INDEX,
  VTK_VALUES_SIZE // size of a value array
};

// Internal classes
//-----------------------------------------------------------------------------
// Store the result of the scan conversion at some pixel.
class vtkPixelListEntry
{
public:
  vtkPixelListEntry()
    {
    }
  
  void Init(double values[VTK_VALUES_SIZE],
            double zView,
            bool exitFace)
    {
      this->Zview=zView;
      int i=0;
      while(i<VTK_VALUES_SIZE)
        {
        this->Values[i]=values[i];
        ++i;
        }
      this->ExitFace = exitFace;
    }
  
  
  // Return the interpolated values at this pixel.
  inline double *GetValues() { return this->Values; }
  // Return the interpolated z coordinate in view space at this pixel.
  inline double GetZview() const { return this->Zview; }
  // Return whether the fragment comes from an external face.
  inline bool GetExitFace() const { return this->ExitFace; }
  
  vtkPixelListEntry *GetPrevious() { return this->Previous; }
  vtkPixelListEntry *GetNext() { return this->Next; }
  
  void SetPrevious(vtkPixelListEntry *e) { this->Previous=e; }
  void SetNext(vtkPixelListEntry *e) { this->Next=e; }
  
protected:
  double Values[VTK_VALUES_SIZE];
  double Zview;
  bool ExitFace;
  
  // List structure: both for the free block list (one-way) and any
  // pixel list (two-way)
  vtkPixelListEntry *Next;
  // List structure: only for the pixel list (two-way)
  vtkPixelListEntry *Previous;
private:
  vtkPixelListEntry(const vtkPixelListEntry &other);
  vtkPixelListEntry &operator=(const vtkPixelListEntry &other);
};

//-----------------------------------------------------------------------------
// Cache the projection of a vertex
class vtkVertexEntry
{
public:
  vtkVertexEntry() {}
  
  vtkVertexEntry(int screenX,
                 int screenY,
                 double xWorld,
                 double yWorld,
                 double zWorld,
                 double zView,
                 double scalar,
                 double invW)
    :ScreenX(screenX),ScreenY(screenY),Zview(zView),InvW(invW)
    { 
      this->Values[VTK_VALUES_X_INDEX]=xWorld;
      this->Values[VTK_VALUES_Y_INDEX]=yWorld;
      this->Values[VTK_VALUES_Z_INDEX]=zWorld;
      this->Values[VTK_VALUES_SCALAR_INDEX]=scalar;
    }
  
  void Set(int screenX,
           int screenY,
           double xWorld,
           double yWorld,
           double zWorld,
           double zView,
           double scalar,
           double invW)
    {
      this->ScreenX=screenX;
      this->ScreenY=screenY;
      this->Zview=zView;
      this->Values[VTK_VALUES_X_INDEX]=xWorld;
      this->Values[VTK_VALUES_Y_INDEX]=yWorld;
      this->Values[VTK_VALUES_Z_INDEX]=zWorld;
      this->Values[VTK_VALUES_SCALAR_INDEX]=scalar;
      this->InvW=invW;
    }
  
  int GetScreenX()
    {
      return this->ScreenX;
    }
  int GetScreenY()
    {
      return this->ScreenY;
    }
  double *GetValues()
    {
      return this->Values;
    }
  double GetZview()
    {
      return this->Zview;
    }
  double GetInvW()
    {
      return this->InvW;
    }
  
  vtkVertexEntry &operator=(const vtkVertexEntry &other) {
    ScreenX = other.ScreenX;
    ScreenY = other.ScreenY;
    memcpy(Values, other.Values, sizeof(double)*VTK_VALUES_SIZE);
    Zview = other.Zview;
    InvW = other.InvW;
    return *this;
  }
  vtkVertexEntry(const vtkVertexEntry &other) {
    if( this != &other)
      {
      *this = other;
      }
  }
  
protected:
  int ScreenX;
  int ScreenY;
  double Values[VTK_VALUES_SIZE];
  double Zview;
  double InvW;
};


//-----------------------------------------------------------------------------
// Abstract interface for an edge of a triangle in the screen space.
// Used during scan-conversion.
class vtkScreenEdge
{
public:
  // If the edge is a composite edge (top+bottom) switch to the bottom edge.
  // Otherwise, do nothing.
  virtual void OnBottom(int skipped, int y)
    {
      if(!skipped)
        {
        this->NextLine(y);
        }
    }
  // Increment edge state to the next line.
  virtual void NextLine(int y)=0;
  // Increment edge state to the next deltaY line.
  virtual void SkipLines(int deltaY,
                         int y)=0;
  // Return the abscissa for the current line.
  virtual int GetX()=0;
  // Return the projected values for the current line. They are linearly
  // incrementally interpolated in view space. The actual values are
  // given by projectedValue/InvW. This is the way the values in world space
  // are incrementally (not linearly) interpolated in view space.
  virtual double *GetPValues()=0;
  // Return 1/W, linearly interpolated in view space.
  virtual double GetInvW()=0;
  // Return Z in view coordinates, linearly interpolated in view space.
  virtual double GetZview()=0;
protected:
  // Destructor.
  virtual ~vtkScreenEdge() {}
};

//-----------------------------------------------------------------------------
// Do an incremental traversing of an edge based on an Y increment.
enum
{
  VTK_CASE_VERTICAL=0,
  VTK_CASE_MOSTLY_VERTICAL,
  VTK_CASE_DIAGONAL,
  VTK_CASE_HORIZONTAL_BEGIN,
  VTK_CASE_HORIZONTAL_END,
  VTK_CASE_HORIZONTAL_MS, // most significant pixel
  VTK_CASE_VERTICAL_IN_TO_OUT, // with edge equation
  VTK_CASE_VERTICAL_OUT_TO_IN,
  VTK_CASE_HORIZONTAL_IN_TO_OUT,
  VTK_CASE_HORIZONTAL_OUT_TO_IN
};

// We use an edge equation as described in:
// Juan Pineda
// A Parallel Algorithm for Polygon Rasterization
// In Computer Graphics, Volume 22, Number 4, August 1988
// SIGGRAPH'88, Atlanta, August 1-5, 1988.
// pages 17--20.

// or in:

// Marc Olano and Trey Greer
// Triangle Scan Conversion using 2D Homogeneous Coordinates
// In 1997 SIGGRAPH/Eurographics Workshop
// pages 89--95.
// http://www.cs.unc.edu/~olano/papers/2dh-tri/2dh-tri.pdf

#define MOST_SIGNIFICANT
#define EDGE_EQUATION
#define HORI_EDGE_EQUATION
//#define STRICTLY_INSIDE

class vtkSimpleScreenEdge
  : public vtkScreenEdge
{
public:
  // Initialize the edge by the vertices v0 and v2 (ordered in y)
  // `onRight' is true if the edge in on the right side of the triangle.
  void Init(vtkVertexEntry *v0,
            vtkVertexEntry *v2,
            int dx20,
            int dy20,
            int onRight)
    {
      double z0=v0->GetZview();
      double z2=v2->GetZview();
      
      double invW0=v0->GetInvW();
      double invW2=v2->GetInvW();
      
      double pv0[VTK_VALUES_SIZE];
      double pv2[VTK_VALUES_SIZE];
      int i=0;
      while(i<VTK_VALUES_SIZE)
        {
        pv0[i]=v0->GetValues()[i]*invW0;
        this->PValues[i]=pv0[i];
        pv2[i]=v2->GetValues()[i]*invW2;
        ++i;
        }
      
      this->InvW=invW0;
      this->Zview=z0;
      
      int x0=v0->GetScreenX();
      int x2=v2->GetScreenX();
     
      this->X0=x0;
      this->X2=x2;
      
      this->V2=v2;
      
      this->X=x0;
      
      if(dx20==0)
        {
        this->Case=VTK_CASE_VERTICAL;
        double invDy20=1.0/dy20;
        i=0;
        while(i<VTK_VALUES_SIZE)
          {
          this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
          ++i;
          }
        this->DinvW=(invW2-invW0)*invDy20;
        this->Dz=(z2-z0)*invDy20;
        }
      else
        {
        if(dx20>0)
          {
          this->IncX=1;
          if(dx20>dy20)
            {
            // Mostly horizontal
#ifdef HORI_EDGE_EQUATION
            if(onRight)
              {
              this->Case=VTK_CASE_HORIZONTAL_IN_TO_OUT;
              }
            else
              {
              this->Case=VTK_CASE_HORIZONTAL_OUT_TO_IN;
              }
            this->Error=0;
            this->SDy=dy20;
            this->XStep=dx20/dy20; // integral division
            this->Dx=dx20-this->XStep*this->SDy;
            
            double invDy20=1.0/dy20;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
              ++i;
              }
            this->DinvW=(invW2-invW0)*invDy20;
            this->Dz=(z2-z0)*invDy20;
#else
#ifdef MOST_SIGNIFICANT
            this->Case=VTK_CASE_HORIZONTAL_MS;
            this->XStep=dx20/dy20; // integral division
            this->Dy=dy20;
            this->Dy2=dy20<<1; // *2
            this->Error=0;
            this->ErrorStep=(dx20-this->XStep*dy20)<<1; // 2*r, dx=q*dy+r, r<dy
            double invDx20=1.0/dx20;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
              this->PValuesStep[i]=this->Dpv[i]*this->XStep;
              ++i;
              }
            this->DinvW=(invW2-invW0)*invDx20;
            this->Dz=(z2-z0)*invDx20;
            this->InvWStep=this->DinvW*this->XStep;
            this->ZStep=this->Dz*this->XStep;
            
#else
            if(!onRight)
              {
              this->Case=VTK_CASE_HORIZONTAL_BEGIN;
              this->First=1;
              this->Dy2=dy20<<1; // *2
              this->Dx2=dx20<<1; // *2
              this->Error=dx20;
              
              this->XStep=dx20/dy20; // integral division
              
              this->ErrorStep=this->XStep*this->Dy2;
              
              double invDx20=1.0/dx20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
                this->PValuesStep[i]=this->Dpv[i]*this->XStep;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDx20;
              this->Dz=(z2-z0)*invDx20;
              this->InvWStep=this->DinvW*this->XStep;
              this->ZStep=this->Dz*this->XStep;
              }
            else
              {
              this->Case=VTK_CASE_HORIZONTAL_END;
              
              this->InvW2=invW2;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues2[i]=pv2[i];
                ++i;
                }
             
              this->Zview2=z2;
                
              this->Dy2=dy20<<1; // *2
              this->Dx2=dx20<<1; // *2
              this->Error=dx20;
              this->XStep=dx20/dy20;
              this->ErrorStep=this->XStep*this->Dy2;
              
              double invDx20=1.0/dx20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
                this->PValuesStep[i]=this->Dpv[i]*this->XStep;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDx20;
              this->Dz=(z2-z0)*invDx20;
              this->InvWStep=this->DinvW*this->XStep;
              this->ZStep=this->Dz*this->XStep;
              
              while(this->Error<this->Dx2)
                {
                this->X+=this->IncX;
                this->InvW+= this->DinvW;
                i=0;
                while(i<VTK_VALUES_SIZE)
                  {
                  this->PValues[i]+=this->Dpv[i];
                  ++i;
                  }
                this->Zview+=this->Dz;
                
                this->Error+=this->Dy2;
                }
              this->Error-=this->Dx2;
              this->X-=this->IncX;
              this->InvW-= this->DinvW;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues[i]-=this->Dpv[i];
                ++i;
                }
              this->Zview-=this->Dz;
              }
#endif
#endif // EDGE_EQUATION
            }
          else
            {
            if(dx20==dy20)
              {
              this->Case=VTK_CASE_DIAGONAL;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
              }
            else
              {
#ifdef EDGE_EQUATION
              if(onRight)
                {
                this->Case=VTK_CASE_VERTICAL_IN_TO_OUT;
                }
              else
                {
                this->Case=VTK_CASE_VERTICAL_OUT_TO_IN;
                }
              this->Error=0;
              this->SDy=dy20;
              this->Dx=dx20;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
#else
              this->Case=VTK_CASE_MOSTLY_VERTICAL;
              this->Dx2=dx20<<1; // *2
              this->Dy2=dy20<<1; // *2
              this->Error=dy20;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
#endif
              }
            }
          }
        else
          {
          this->IncX=-1;
          if(-dx20>dy20)
            {
             // Mostly horizontal
#ifdef HORI_EDGE_EQUATION
            if(onRight)
              {
              this->Case=VTK_CASE_HORIZONTAL_OUT_TO_IN;
              }
            else
              {
              this->Case=VTK_CASE_HORIZONTAL_IN_TO_OUT;
              }
            this->Error=0;
            this->SDy=-dy20;
            this->XStep=dx20/dy20; // integral division
            this->Dx=dx20+this->XStep*this->SDy;
            
            double invDy20=1.0/dy20;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
              ++i;
              }
            this->DinvW=(invW2-invW0)*invDy20;
            this->Dz=(z2-z0)*invDy20;
#else
#ifdef MOST_SIGNIFICANT  
            this->Case=VTK_CASE_HORIZONTAL_MS;
            this->XStep=dx20/dy20; // integral division
            this->Dy=dy20;
            this->Dy2=dy20<<1; // *2
            this->Error=0;
            this->ErrorStep=(dx20+this->XStep*dy20)<<1; // 2*r, dx=q*dy+r, r<dy
            double invDx20=-1.0/dx20;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
              this->PValuesStep[i]=-this->Dpv[i]*this->XStep;
              ++i;
              }
            this->DinvW=(invW2-invW0)*invDx20;
            this->Dz=(z2-z0)*invDx20;
            this->InvWStep=-this->DinvW*this->XStep;
            this->ZStep=-this->Dz*this->XStep;
            
            
#else
            if(onRight)
              {
              this->Case=VTK_CASE_HORIZONTAL_BEGIN;
              this->First=1;
              this->Dy2=dy20<<1; // *2
              this->Dx2=(-dx20)<<1; // *2
              this->Error=-dx20;
              this->XStep=dx20/dy20;
              this->ErrorStep=-this->XStep*this->Dy2;
              
              double invDx20=-1.0/dx20;
              
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
                this->PValuesStep[i]=-this->Dpv[i]*this->XStep;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDx20;
              this->Dz=(z2-z0)*invDx20;
              this->InvWStep=-this->DinvW*this->XStep;
              this->ZStep=-this->Dz*this->XStep;
              }
            else
              {
              this->Case=VTK_CASE_HORIZONTAL_END;
              
              this->InvW2=invW2;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues2[i]=pv2[i];
                ++i;
                }
              this->Zview2=z2;
              
              this->Dy2=dy20<<1; // *2
              this->Dx2=(-dx20)<<1; // *2
              this->Error=-dx20;
              this->XStep=dx20/dy20;
              this->ErrorStep=-this->XStep*this->Dy2;
              
              double invDx20=-1.0/dx20;
              
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDx20;
                this->PValuesStep[i]=-this->Dpv[i]*this->XStep;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDx20;
              this->Dz=(z2-z0)*invDx20;
              this->InvWStep=-this->DinvW*this->XStep;
              this->ZStep=-this->Dz*this->XStep;
              
              while(this->Error<this->Dx2)
                {
                this->X+=this->IncX;
                this->InvW+= this->DinvW;
                i=0;
                while(i<VTK_VALUES_SIZE)
                  {
                  this->PValues[i]+=this->Dpv[i];
                  ++i;
                  }
                this->Zview+=this->Dz;
                
                this->Error+=this->Dy2;
                }
              this->Error-=this->Dx2;
              this->X-=this->IncX;
              
              this->InvW-= this->DinvW;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues[i]-=this->Dpv[i];
                ++i;
                }
              this->Zview-=this->Dz;
              }
#endif
#endif // EDGE_EQUATION
            }
          else
            {
            if(dx20==-dy20)
              {
              this->Case=VTK_CASE_DIAGONAL;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
              }
            else
              {
#ifdef EDGE_EQUATION
              if(onRight)
                {
                this->Case=VTK_CASE_VERTICAL_OUT_TO_IN; 
                }
              else
                {
                this->Case=VTK_CASE_VERTICAL_IN_TO_OUT;
                }
              this->Error=0;
              this->SDy=-dy20;
              this->Dx=dx20;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
#else
              this->Case=VTK_CASE_MOSTLY_VERTICAL;
              this->Dx2=(-dx20)<<1; // *2
              this->Dy2=dy20<<1; // *2
              this->Error=dy20;
              
              double invDy20=1.0/dy20;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->Dpv[i]=(pv2[i]-pv0[i])*invDy20;
                ++i;
                }
              this->DinvW=(invW2-invW0)*invDy20;
              this->Dz=(z2-z0)*invDy20;
#endif
              }
            }
          }
        }
    }
  
  // Check that the current abscissa is in the range given by the vertices.
  int ValidXRange()
    {
      if(this->X0<=this->X2)
        {
        return (this->X>=this->X0) && (this->X<=this->X2);
        }
      else
        {
        return (this->X>=this->X2) && (this->X<=this->X0);
        }
    }
  int GetX()
    {
      // assert("pre: valid_range" && ValidXRange() );
      return this->X;
    }
  double GetInvW() { return this->InvW; }
  double *GetPValues() { return this->PValues; }
  double GetZview() { return this->Zview; }
  
  void NextLine(int y)
    {
      int i;
      switch(this->Case)
        {
        case VTK_CASE_VERTICAL:
          // nothing to do with X
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
        case VTK_CASE_DIAGONAL:
          // X
          this->X+=this->IncX;
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
        case VTK_CASE_MOSTLY_VERTICAL:
          // X
          this->Error+=this->Dx2;
          if(this->Error>=this->Dy2)
            {
            this->Error-=this->Dy2;
            this->X+=this->IncX;
            }
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
        case VTK_CASE_VERTICAL_OUT_TO_IN:
          this->Error-=this->Dx;
          if(this->SDy>0)
            {
#ifdef STRICTLY_INSIDE
            if(this->Error<=0)
#else
            if(this->Error<0) // we are no more on the right side
#endif
              {
              this->Error+=this->SDy;
#ifdef STRICTLY_INSIDE
              assert("check: positive_equation" && this->Error>0);
#else           
              assert("check: positive_equation" && this->Error>=0);
#endif
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            if(this->Error>=0) // we are no more on the left side
#else
            if(this->Error>0) // we are no more on the left side
#endif
              {
              this->Error+=this->SDy;
#ifdef STRICTLY_INSIDE
//              assert("check: negative_equation" && this->Error>0);
#else
              assert("check: negative_equation" && this->Error<=0);
#endif
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
        case  VTK_CASE_VERTICAL_IN_TO_OUT:
          this->Error+=this->SDy-this->Dx;
          if(this->SDy<0)
            {
#ifdef STRICTLY_INSIDE
            if(this->Error<=0) // out: too far on left
#else
            if(this->Error<0) // out: too far on left
#endif
              {
              this->Error-=this->SDy;
#ifdef STRICTLY_INSIDE
              assert("check: positive_equation" && this->Error>0);
#else
              assert("check: positive_equation" && this->Error>=0);
#endif
              }
            else
              {
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            if(this->Error>=0) // out: too far on right
#else
            if(this->Error>0) // out: too far on right
#endif
              {
              this->Error-=this->SDy;
#ifdef STRICTLY_INSIDE
              assert("check: negative_equation" && this->Error<0);
#else
              assert("check: negative_equation" && this->Error<=0);
#endif
              }
            else
              {
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
          
        case VTK_CASE_HORIZONTAL_OUT_TO_IN:
          this->Error-=this->Dx;
          this->X+=this->XStep;
          if(this->SDy>0)
            {
#ifdef STRICTLY_INSIDE
            if(this->Error<=0) // we are no more on the right side
#else
            if(this->Error<0) // we are no more on the right side
#endif
              {
              this->Error+=this->SDy;
#ifdef STRICTLY_INSIDE
              assert("check: positive_equation" && this->Error>0);
#else
              assert("check: positive_equation" && this->Error>=0);
#endif
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            if(this->Error>=0) // we are no more on the left side
#else
            if(this->Error>0) // we are no more on the left side
#endif
              {
              this->Error+=this->SDy;
#ifdef STRICTLY_INSIDE
              assert("check: negative_equation" && this->Error<0);
#else
              assert("check: negative_equation" && this->Error<=0);
#endif
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
          
        case  VTK_CASE_HORIZONTAL_IN_TO_OUT:
          this->Error+=this->SDy-this->Dx;
          this->X+=this->XStep;
          if(this->SDy<0)
            {
#ifdef STRICTLY_INSIDE
            if(this->Error<=0) // out: too far on left
#else
            if(this->Error<0) // out: too far on left
#endif
              {
              this->Error-=this->SDy;
#ifdef STRICTLY_INSIDE
//              assert("check: positive_equation" && this->Error>0);
#else
              assert("check: positive_equation" && this->Error>=0);
#endif
              }
            else
              {
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            if(this->Error>=0) // out: too far on right
#else
            if(this->Error>0) // out: too far on right
#endif
              {
              this->Error-=this->SDy;
#ifdef STRICTLY_INSIDE
//            assert("check: negative_equation" && this->Error<0);  
#else
              assert("check: negative_equation" && this->Error<=0);
#endif
              }
            else
              {
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i];
            ++i;
            }
          this->Zview+=this->Dz;
          break;
          
        case VTK_CASE_HORIZONTAL_BEGIN:
          if(this->First)
            {
            this->First=0;
            }
          else
            {
            this->X+=this->XStep;
            
            this->InvW+=this->InvWStep;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->PValuesStep[i];
              ++i;
              }
            this->Zview+=this->ZStep;
            this->Error+=this->ErrorStep;
            }
          while(this->Error<this->Dx2)
            {
            this->X+=this->IncX;
            this->InvW+=this->DinvW;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->Dpv[i];
              ++i;
              }
            this->Zview+=this->Dz;
            
            this->Error+=this->Dy2;
            }
          this->Error-=this->Dx2;
          break;
        case VTK_CASE_HORIZONTAL_END:
          if(y==this->V2->GetScreenY())
            {
            this->X=this->V2->GetScreenX();
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]=this->PValues2[i];
              ++i;
              }
            this->Zview=this->Zview2;
            this->InvW=this->InvW2;
            }
          else
            {
            this->X+=this->XStep;
            
            this->InvW+=this->InvWStep;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->PValuesStep[i];
              ++i;
              }
            this->Zview+=this->ZStep;
            
            this->Error+=this->ErrorStep;
            
            while(this->Error<this->Dx2)
              {
              this->X+=this->IncX;
              this->InvW+=this->DinvW;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues[i]+=this->Dpv[i];
                ++i;
                }
              this->Zview+=this->Dz;
              
              this->Error+=this->Dy2;
              }
            this->Error-=this->Dx2;
            }
          break; 
        case VTK_CASE_HORIZONTAL_MS:
          this->Error+=this->ErrorStep;
          if(this->Error>=this->Dy)
            {
            this->Error-=this->Dy2;
            this->X+=this->XStep+this->IncX;
            this->InvW+=this->InvWStep+this->DinvW;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->PValuesStep[i]+this->Dpv[i];
              ++i;
              }
            this->Zview+=this->ZStep+this->Dz;
            }
          else
            {
            this->X+=this->XStep;
            this->InvW+=this->InvWStep;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->PValuesStep[i];
              ++i;
              }
            this->Zview+=this->ZStep;
            }
          break;
        }
    }
  
  void SkipLines(int deltaY,
                 int y)
    {
      if(deltaY==1)
        {
        this->NextLine(0);
        return;
        }
      
      int firstDeltaY;
      int i;
      switch(this->Case)
        {
        case VTK_CASE_VERTICAL:
          // nothing to do with X
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
        case VTK_CASE_DIAGONAL:
          // X
          this->X+=this->IncX*deltaY;
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
        case VTK_CASE_MOSTLY_VERTICAL:
          // X
          this->Error+=this->Dx2*deltaY;
          while(this->Error>=this->Dy2)
            {
            this->Error-=this->Dy2;
            this->X+=this->IncX;
            }
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
        case VTK_CASE_VERTICAL_OUT_TO_IN:
          this->Error-=this->Dx*deltaY;
          if(this->SDy>0)
            {
#ifdef STRICTLY_INSIDE
            while(this->Error<=0) // we are no more on the right side
#else
            while(this->Error<0) // we are no more on the right side
#endif
              {
              this->Error+=this->SDy;
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            while(this->Error>=0) // we are no more on the left side
#else            
            while(this->Error>0) // we are no more on the left side
#endif
              {
              this->Error+=this->SDy;
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
        case  VTK_CASE_VERTICAL_IN_TO_OUT:
          this->Error+=(this->SDy-this->Dx)*deltaY;
          this->X+=this->IncX*deltaY;
           if(this->SDy<0)
             {
#ifdef STRICTLY_INSIDE
             while(this->Error<=0) // out: too far on left
#else
             while(this->Error<0) // out: too far on left
#endif
               {
               this->Error-=this->SDy;
               this->X-=this->IncX;
               }
             }
           else
             {
#ifdef STRICTLY_INSIDE
             while(this->Error>=0) // out: too far on right
#else
             while(this->Error>0) // out: too far on right
#endif
               {
               this->Error-=this->SDy;
               this->X-=this->IncX;
               }
             }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
          
        case VTK_CASE_HORIZONTAL_OUT_TO_IN:
          this->Error-=this->Dx*deltaY;
          this->X+=this->XStep*deltaY;
          if(this->SDy>0)
            {
#ifdef STRICTLY_INSIDE
            while(this->Error<=0) // we are no more on the right side
#else
            while(this->Error<0) // we are no more on the right side
#endif
              {
              this->Error+=this->SDy;
              this->X+=this->IncX;
              }
            }
          else
            {
#ifdef STRICTLY_INSIDE
            while(this->Error>=0) // we are no more on the left side
#else
            while(this->Error>0) // we are no more on the left side
#endif
              {
              this->Error+=this->SDy;
              this->X+=this->IncX;
              }
            }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
        case  VTK_CASE_HORIZONTAL_IN_TO_OUT:
          this->Error+=(this->SDy-this->Dx)*deltaY;
          this->X+=(this->XStep+this->IncX)*deltaY;
//          this->X+=this->IncX*deltaY;
           if(this->SDy<0)
             {
#ifdef STRICTLY_INSIDE
             while(this->Error<=0) // out: too far on left
#else
             while(this->Error<0) // out: too far on left
#endif
               {
               this->Error-=this->SDy;
               this->X-=this->IncX;
               }
             }
           else
             {
#ifdef STRICTLY_INSIDE
             while(this->Error>=0) // out: too far on right
#else
             while(this->Error>0) // out: too far on right
#endif
               {
               this->Error-=this->SDy;
               this->X-=this->IncX;
               }
             }
          // Interpolate the values on inc y
          this->InvW+=this->DinvW*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->Dpv[i]*deltaY;
            ++i;
            }
          this->Zview+=this->Dz*deltaY;
          break;
          
        case VTK_CASE_HORIZONTAL_BEGIN:
          
          if(this->First)
            {
            this->First=0;
            firstDeltaY=deltaY-1;
            }
          else
            {
            firstDeltaY=deltaY;
            }
          
          this->X+=this->XStep*firstDeltaY;
          
          this->InvW+=this->InvWStep*firstDeltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->PValuesStep[i]*firstDeltaY;
            ++i;
            }
          this->Zview+=this->ZStep*firstDeltaY;
          this->Error+=this->ErrorStep*firstDeltaY;
          
          while(this->Error<this->Dx2)
            {
            this->X+=this->IncX;
            this->InvW+=this->DinvW;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->Dpv[i];
              ++i;
              }
            this->Zview+=this->Dz;
            
            this->Error+=this->Dy2;
            }
          this->Error-=this->Dx2;
          break;
        case VTK_CASE_HORIZONTAL_END:
          if(y==this->V2->GetScreenY())
            {
            this->X=this->V2->GetScreenX();
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]=this->PValues2[i];
              ++i;
              }
            this->Zview=this->Zview2;
            this->InvW=this->InvW2;
            }
          else
            {
            this->X+=this->XStep*deltaY;
            
            this->InvW+=this->InvWStep*deltaY;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->PValuesStep[i]*deltaY;
              ++i;
              }
            this->Zview+=this->ZStep*deltaY;
            
            this->Error+=this->ErrorStep*deltaY;
            
            while(this->Error<this->Dx2)
              {
              this->X+=this->IncX;
              this->InvW+=this->DinvW;
              i=0;
              while(i<VTK_VALUES_SIZE)
                {
                this->PValues[i]+=this->Dpv[i];
                ++i;
                }
              this->Zview+=this->Dz;
              
              this->Error+=this->Dy2;
              }
            this->Error-=this->Dx2;
            }
          break;
        case VTK_CASE_HORIZONTAL_MS:
          this->Error+=this->ErrorStep*deltaY;
          this->X+=this->XStep*deltaY;
          this->InvW+=this->InvWStep*deltaY;
          i=0;
          while(i<VTK_VALUES_SIZE)
            {
            this->PValues[i]+=this->PValuesStep[i]*deltaY;
            ++i;
            }
          this->Zview+=this->ZStep*deltaY;
          
          while(this->Error>=this->Dy)
            {
            this->Error-=this->Dy2;
            this->X+=this->IncX;
            this->InvW+=this->DinvW;
            i=0;
            while(i<VTK_VALUES_SIZE)
              {
              this->PValues[i]+=this->Dpv[i];
              ++i;
              }
            this->Zview+=this->Dz;
            }
          break;
        }
    }
  
protected:
  int Case;
  int Error; // error to the mid-point 
  int Dx2; // 2*dx
  int Dy2; // 2*dy
  int First; // use only with VTK_CASE_HORIZONTAL_BEGIN case
  int XStep; // dx/dy
  int ErrorStep; // XStep*Dy2

  vtkVertexEntry *V2;
  
  int IncX; // -1 or 1
  
  int X; // Current abscissa
  
  int X0; // for debugging
  int X2; // for debugging
  
  // Slope of 1/w
  double DinvW;
  // Current 1/W
  double InvW;
  // DinvW*XStep
  double InvWStep;
  // 1/W at the end vertex
  double InvW2;
  
  // Slope of the z coordinate in view space
  double Dz;
  // current z in view space
  double Zview;
  // Dz*XStep
  double ZStep;
  // z coordinate in view space at the end vertex
  double Zview2;
  
  // Slope of each projected values on the edge
  double Dpv[VTK_VALUES_SIZE];
  
  
  // Current projected values
  double PValues[VTK_VALUES_SIZE];
  // Dpv*XStep
  double PValuesStep[VTK_VALUES_SIZE];
  // Values at the end vertex.
  double PValues2[VTK_VALUES_SIZE];
  
  int Dy; // VTK_HORIZONTAL_MS
  int SDy; // VTK_VERTICAL_LEFT/RIGHT
  int Dx; // VTK_VERTICAL_LEFT/RIGHT
};

//-----------------------------------------------------------------------------
// During rasterization of a triangle, there is always one side with two
// edges and the other side with a single edge.
// This class manages the side with the two edges called top and bottom edges.
class vtkDoubleScreenEdge
  :public vtkScreenEdge
{
public:
  void Init(vtkVertexEntry *v0,
            vtkVertexEntry *v1,
            vtkVertexEntry *v2,
            int dx10,
            int dy10,
            int onRight)
    {
      this->Current=0;
      if(dy10!=0)
        {
        this->Top.Init(v0,v1,dx10,dy10,onRight);
        this->Current=&this->Top;
        }
      
      int dx21=v2->GetScreenX()-v1->GetScreenX();
      int dy21=v2->GetScreenY()-v1->GetScreenY();
      
      if(dy21!=0)
        {
        this->Bottom.Init(v1,v2,dx21,dy21,onRight);
        if(this->Current==0)
          {
          this->Current=&this->Bottom;
          }
        }
    }
  
  int GetX() { return this->Current->GetX(); }
  double GetInvW() { return this->Current->GetInvW(); }
  double GetZview() { return this->Current->GetZview(); }
  double *GetPValues() { return this->Current->GetPValues(); }
 
  void OnBottom(int skipped, int y)
    {
      this->Current=&this->Bottom;
      this->Current->OnBottom(skipped,y);
    }
  
  void NextLine(int y)
    {
      this->Current->NextLine(y);
    }
  void SkipLines(int deltaY,
                 int y)
    {
      this->Current->SkipLines(deltaY,y);
    }
  
protected:
  vtkSimpleScreenEdge Top;
  vtkSimpleScreenEdge Bottom;
  vtkScreenEdge *Current;
};

//-----------------------------------------------------------------------------
// Horizontal span between two points of two edges.
// Used during scan-conversion.
// It interpolates the values along the span.

class vtkSpan
{
public:
  // Initialize the span from the left abcissa x0 and the right absissa x1 and
  // from 1/W, the projected values and the z coordinate in view space at
  // thoses points. Set the current state to the left point.
  void Init(int x0,
            double invW0,
            double pValues0[VTK_VALUES_SIZE], // projected values
            double zView0,
            int x1,
            double invW1,
            double pValues1[VTK_VALUES_SIZE], // projected values
            double zView1)
    {
//      assert("pre: dx>=0" && x1-x0>=0);
      // x0=x1: the span is just a point
      
      int i;
      if(x0!=x1)
        {
        
        double invDx10=1.0/(x1-x0);
        i=0;
        while(i<VTK_VALUES_SIZE)
          {
          this->Dpv[i]=(pValues1[i]-pValues0[i])*invDx10;
          ++i;
          }
        this->DinvW=(invW1-invW0)*invDx10;
        this->Dz=(zView1-zView0)*invDx10;
        }
      else
        {
        i=0;
        while(i<VTK_VALUES_SIZE)
          {
          this->Dpv[i]=0;
          ++i;
          }
        this->DinvW=0;
        this->Dz=0;
        }
      
      this->Zview=zView0;
      this->InvW=invW0;
      i=0;
      double w=1/this->InvW;
      while(i<VTK_VALUES_SIZE)
        {
        this->PValues[i]=pValues0[i];
        this->Values[i]=this->PValues[i]*w;
        ++i;
        }
      this->X=x0;
      this->X1=x1;
    }
  
  // Is the current state after the right point?
  int IsAtEnd()
    {
      return this->X>this->X1;
    }
  
  // Current abscissa.
  int GetX() { return this->X; }
  // Current values.
  double *GetValues() { return this->Values; }
  // Current z coordinate in view space.
  double GetZview() { return this->Zview; }
  
  // Go the next abscissa from left to right.
  void NextPixel()
    {
      ++this->X;
      
       this->InvW+=this->DinvW;
       int i=0;
       double w=1/this->InvW;
       while(i<VTK_VALUES_SIZE)
         {
         this->PValues[i]+=this->Dpv[i];
         this->Values[i]=this->PValues[i]*w;
         ++i;
         }
       this->Zview+=this->Dz;
    }
  
protected:
  int X1; // abscissa at the right point.
  
  int X; // current abscissa
  
  // Slope of 1/w
  double DinvW;
  // current 1/W
  double InvW;
  
  // Slope of the z coordinate in view space
  double Dz;
  // current z coordinate in view space
  double Zview;
  
  // Slope of each projected values on the span
  double Dpv[VTK_VALUES_SIZE];
  // Current projected values
  double PValues[VTK_VALUES_SIZE];
  
  // Current values: Values=PValues/InvW
  double Values[VTK_VALUES_SIZE];
};


// Pimpl (i.e. private implementation) idiom

//typedef vtkstd::list<vtkPixelListEntry *> vtkPixelList;

class vtkPixelListEntryBlock
{
public:
  vtkPixelListEntryBlock(vtkIdType size)
    {
      assert("pre: positive_size" && size>0);
      this->Size=size;
      this->Next=0;
      this->Array=new vtkPixelListEntry[size];
      this->Last=this->Array+size-1;
      // link each entry to the next one
      vtkPixelListEntry *p;
      vtkPixelListEntry *q;
      p=this->Array;
      q=p+1;
      vtkIdType i=1;
      while(i<size)
        {
        p->SetNext(q);
        ++i;
        p=q;
        ++q;
        }
      p->SetNext(0);
    }
  ~vtkPixelListEntryBlock()
    {
      delete[] this->Array;
    }
  vtkIdType GetSize() { return this->Size; }
  vtkPixelListEntryBlock *GetNext() { return this->Next; }
  vtkPixelListEntry *GetFirst() { return this->Array; }
  vtkPixelListEntry *GetLast() { return this->Last; }
  void SetNext(vtkPixelListEntryBlock *other) { this->Next=other; }
  
protected:
  vtkIdType Size;
  vtkPixelListEntryBlock *Next;
  vtkPixelListEntry *Array;
  vtkPixelListEntry *Last;
};

const vtkIdType VTK_PIXEL_BLOCK_SIZE=64;

class vtkPixelListEntryMemory
{
public:
  vtkPixelListEntryMemory()
    {
      this->FirstBlock=new vtkPixelListEntryBlock(VTK_PIXEL_BLOCK_SIZE);
      this->FirstFreeElement=this->FirstBlock->GetFirst();
      this->Size=VTK_PIXEL_BLOCK_SIZE;
    }
  ~vtkPixelListEntryMemory()
    {
      vtkPixelListEntryBlock *p=this->FirstBlock;
      vtkPixelListEntryBlock *q;
      while(p!=0)
        {
        q=p->GetNext();
        delete p;
        p=q;
        }
    }
  vtkPixelListEntry *AllocateEntry()
    {
      if(this->FirstFreeElement==0)
        {
        this->AllocateBlock(this->Size<<1);
//        this->AllocateBlock(BLOCK_SIZE);
        }
      vtkPixelListEntry *result=this->FirstFreeElement;
      this->FirstFreeElement=result->GetNext();
      assert("post: result_exists" && result!=0);
      return result;
    }
  void FreeEntry(vtkPixelListEntry *e)
    {
      assert("pre: e_exists" && e!=0);
      
      // the following line works even if this->FirstFreeElement==0
      e->SetNext(this->FirstFreeElement);
      this->FirstFreeElement=e;
    }
  void FreeSubList(vtkPixelListEntry *first,
                   vtkPixelListEntry *last)
    {
      assert("pre: first_exists" && first!=0);
      assert("pre: last_exists" && last!=0);
      // pre: first==last can be true
      // the following line works even if this->FirstFreeElement==0
      last->SetNext(this->FirstFreeElement);
      this->FirstFreeElement=first;
    }
protected:
  
  void AllocateBlock(vtkIdType size)
    {
      assert("pre: positive_size" && size>0);
      vtkPixelListEntryBlock *b=new vtkPixelListEntryBlock(size);
      this->Size+=size;
      // Update the block linked list: starts with the new block
      b->SetNext(this->FirstBlock);
      this->FirstBlock=b;
      
      // Update the free element linked list.
      // It works even if this->FirstFreeElement==0
      b->GetLast()->SetNext(this->FirstFreeElement);
      this->FirstFreeElement=b->GetFirst();
    }
  
  vtkPixelListEntryBlock *FirstBlock;
  vtkPixelListEntry *FirstFreeElement;
  vtkIdType Size; // overall size, in number of elements, not in bytes
};


class vtkPixelList
{
public:
  vtkPixelList()
    {
      this->Size=0;
    }
  vtkPixelListEntry *GetFirst()
    {
      assert("pre: not_empty" && this->Size>0);
      return this->First;
    }
  vtkIdType GetSize() { return this->Size; }
  
  void AddAndSort(vtkPixelListEntry *p)
    {
      assert("pre: p_exists" && p!=0);
      if(this->Size==0)
        {
        p->SetPrevious(0);
        p->SetNext(0);
        this->First=p;
        this->Last=p;
        }
      else
        {
        vtkPixelListEntry *it=this->Last;
        int sorted=0;
        double z=p->GetZview();
        while(!sorted && it!=0)
          {
          // It is not uncommon for an external face and internal face to meet.
          // On the edge where this happens, an exit fragment and non-exit
          // fragment could be generated at the same point.  In this case, it is
          // very important that the exit fragment be last in the list.
          // Otherwise, the ray exit may be improperly marked as between the two
          // overlapping fragments. (Note that if you start to see "speckling"
          // in the image from filled spaces, we may need to add adjust the
          // tolerance to this calculation.)
          const double tolerance = 1.0e-8;
          if (p->GetExitFace())
            {
#ifdef BACK_TO_FRONT
            sorted=it->GetZview() >= z-tolerance;
#else
            sorted=it->GetZview() <= z+tolerance;
#endif
            }
          else
            {
#ifdef BACK_TO_FRONT
            sorted=it->GetZview() > z+tolerance;
#else
            sorted=it->GetZview() < z-tolerance;
#endif
            }
          if(!sorted)
            {
            it=it->GetPrevious();
            }
          }
        if(it==0) // first element
          {
          p->SetPrevious(0);
          p->SetNext(this->First);
          // this->First==0 is handled by case size==0
          this->First->SetPrevious(p);
          this->First=p;
          }
        else
          {
          if(it->GetNext()==0) // last element
            {
            it->SetNext(p);
            p->SetPrevious(it);
            p->SetNext(0);
            this->Last=p;
            }
          else // general case
            {
            vtkPixelListEntry *q=it->GetNext();
            q->SetPrevious(p);
            p->SetNext(q);
            p->SetPrevious(it);
            it->SetNext(p);
            }
          }
        }
      ++this->Size;
    }
  
  // the return pointer is used by the memory manager.
  void RemoveFirst(vtkPixelListEntryMemory *mm)
    {
      assert("pre: not_empty" && this->Size>0);
      assert("pre: mm_exists" && mm!=0);
      
      vtkPixelListEntry *p=this->First;
      if(this->Size>1)
        {
        this->First=p->GetNext();
        this->First->SetPrevious(0);
        }
      --this->Size;
      mm->FreeEntry(p);
    }
  
  // the return pointer on the first element is used by the memory manager.
  void Clear(vtkPixelListEntryMemory *mm)
    {
      assert("pre: mm_exists" && mm!=0);
      if(this->Size>0)
        {
        // it works even if first==last
        mm->FreeSubList(this->First,this->Last);
        this->Size=0;
        }
    }

protected:
  vtkIdType Size;
  vtkPixelListEntry *First;
  vtkPixelListEntry *Last;
};

//-----------------------------------------------------------------------------
// Store the pixel lists for all the frame.
class vtkPixelListFrame
{
public:
  typedef vtkstd::vector<vtkPixelList> VectorType;
 
  vtkPixelListFrame(int size)
    :Vector(size)
    {
    }
  
  // Return width*height
  vtkIdType GetSize() { return this->Vector.size(); }
  
  // Return the size of the list at pixel `i'.
  vtkIdType GetListSize(int i)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      return this->Vector[i].GetSize();
    }
  
  // Add a value the pixel list of pixel `i' and sort it in the list.
  void AddAndSort(int i,
                  vtkPixelListEntry *pixelEntry)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      assert("pre: pixelEntry_exists" &&  pixelEntry!=0);
      
      this->Vector[i].AddAndSort(pixelEntry);
    }
  
  // Return the first entry for pixel `i'.
  vtkPixelListEntry *GetFront(int i)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      assert("pre: not_empty" && this->GetListSize(i)>0);
      return this->Vector[i].GetFirst();
    }
  
  // Remove the first entry for pixel `i'.
  void PopFront(int i,
                vtkPixelListEntryMemory *mm)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      assert("pre: not_empty" && this->GetListSize(i)>0);
      assert("pre: mm_exists" && mm!=0);
      this->Vector[i].RemoveFirst(mm);
    }
  
  // Return the begin iterator for pixel `i'.
  vtkPixelListEntry *GetFirst(int i)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      return this->Vector[i].GetFirst();
    }
#if 0
  // Return the end iterator for pixel `i'.
  vtkstd::list<vtkPixelListEntry *>::iterator GetEndIterator(int i)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      return this->Vector[i].end();
    }
#endif
  // Clear the list of each pixel of the frame.
  void Clean(vtkPixelListEntryMemory *mm)
    {
      assert("pre: mm_exists" && mm!=0);
      vtkIdType i=0;
      vtkIdType c=this->Vector.size();
      while(i<c)
        {
        vtkPixelList *l=&(Vector[i]);
        l->Clear(mm);
        ++i;
        }
    }
  
  // Destructor.
  ~vtkPixelListFrame()
    {
#if 0
      vtkIdType i=0;
      vtkIdType c=this->Vector.size();
      while(i<c)
        {
        vtkPixelList *l=&(Vector[i]);
        while(!l->empty())
          {
          delete l->front();
          l->pop_front();
          }
        ++i;
        }
#endif
    }

  vtkPixelList *GetList(int i)
    {
      assert("pre: valid_i" && i>=0 && i<this->GetSize());
      return &(this->Vector[i]);
    }
  
protected:
  VectorType Vector;
  
  // the STL specification claims that
  // size() on a std: :list is permitted to be O(n)!!!!
//  vtkstd::vector<vtkIdType> Sizes;
  
//  vtkstd::list<vtkPixelListEntry *>::iterator It;
//  vtkstd::list<vtkPixelListEntry *>::iterator PreviousIt;
//  vtkstd::list<vtkPixelListEntry *>::iterator ItEnd;
};

//-----------------------------------------------------------------------------
// Store a triangle face. Ids are in increasing order. Orientation does not
// matter for the algorithm.
class vtkFace
{
public:
  enum {
    NOT_EXTERNAL,
    FRONT_FACE,
    BACK_FACE
  };

  // Initialization from face ids in increasing order.
  vtkFace(vtkIdType faceIds[3], int externalSide)
    {
      assert("pre: ordered ids" && faceIds[0]<faceIds[1]
             && faceIds[1]<faceIds[2]);
      this->FaceIds[0]=faceIds[0];
      this->FaceIds[1]=faceIds[1];
      this->FaceIds[2]=faceIds[2];
      this->Count=0;
      this->Rendered = 0;
      this->ExternalSide = externalSide;
    }
  
  // Return the 3 face ids.
  inline vtkIdType *GetFaceIds() { return this->FaceIds; }

  // Return whether this face is external.
  inline int GetExternalSide() { return this->ExternalSide; }

  // Are `this' and faceIds equal?
  int IsEqual(vtkIdType faceIds[3])
    {
      return (this->FaceIds[0]==faceIds[0])&&(this->FaceIds[1]==faceIds[1])
        &&(this->FaceIds[2]==faceIds[2]);
    }
  
  void Ref() { ++this->Count; }
  void Unref()
    {
      --this->Count;
      if(this->Count==0)
        {
        delete this;
        }
    }
  
  int GetRendered() { return this->Rendered; }
  void SetRendered(int value) { this->Rendered=value; }
  
  double GetScalar(int index)
    {
      assert("pre: valid_index" && index>=0 && index<=1);
      return this->Scalar[index];
    }
  
  void SetScalar(int index,
                 double value)
    {
      assert("pre: valid_index" && index>=0 && index<=1);
      this->Scalar[index]=value;
      assert("post: is_set" && this->GetScalar(index)==value);
    }
  
protected:
  vtkIdType FaceIds[3];
  int Count;
  int Rendered;
  int ExternalSide;

  double Scalar[2]; // 0: value for positive orientation,
  // 1: value for negative orientation.
 
private:
  vtkFace(); // not implemented
  vtkFace(const vtkFace &other); // not implemented
  vtkFace &operator=(const vtkFace &other); // not implemented
};

//-----------------------------------------------------------------------------
// For each vertex, store the list of faces incident on this vertex.
// It is view independent.
class vtkUseSet
{
public:
  typedef vtkstd::vector<vtkstd::list<vtkFace *> *> VectorType;
  VectorType Vector;

  vtkstd::list<vtkFace *> AllFaces; // to set up rendering to false.
  
  // Initialize with the number of vertices.
  vtkUseSet(int size)
    :Vector(size)
    {
      vtkIdType i=0;
      vtkIdType c=this->Vector.size();
      while(i<c)
        {
        this->Vector[i]=0;
        ++i;
        }
      this->CellScalars=0;
      this->NumberOfComponents=0;
    }
  
  // Destructor.
  ~vtkUseSet()
    {
      vtkIdType i=0;
      vtkIdType c=this->Vector.size();
      while(i<c)
        {
        if(this->Vector[i]!=0)
          {
          while(!this->Vector[i]->empty())
            {
            (*this->Vector[i]->begin())->Unref();
            this->Vector[i]->pop_front();
            }
          delete this->Vector[i];
          }
        ++i;
        }
      while(!this->AllFaces.empty())
        {
        (*this->AllFaces.begin())->Unref();
        this->AllFaces.pop_front();
        }
    }
  
  void SetCellScalars(int cellScalars)
    {
      this->CellScalars=cellScalars;
    }
  void SetNumberOfComponents(int numberOfComponents)
    {
      assert("pre: cell_mode" && this->CellScalars);
      this->NumberOfComponents=numberOfComponents;
    }
  
  // For each vertex, clear the list of faces incident to it.
  // also set number of cells per vertex to 0.
  void Clear()
    {
      vtkIdType i=0;
      vtkIdType c=this->Vector.size();
      while(i<c)
        {
        if(this->Vector[i]!=0)
          {
          while(!this->Vector[i]->empty())
            {
            (*this->Vector[i]->begin())->Unref();
            this->Vector[i]->pop_front();
            }
          delete this->Vector[i];
          this->Vector[i]=0;
          }
        ++i;
        }
      while(!this->AllFaces.empty())
        {
        (*this->AllFaces.begin())->Unref();
        this->AllFaces.pop_front();
        }
    }
  
  // Add face to each vertex only if the useset does not have the face yet.
  void AddFace(vtkIdType faceIds[3],
               vtkDataArray *scalars,
               vtkIdType cellIdx,
               int orientationChanged,
               bool external)
    {
      // Ignore degenerate faces.
      if ((faceIds[0] == faceIds[1]) || (faceIds[1] == faceIds[2])) return;

      assert("pre: ordered ids" && faceIds[0]<faceIds[1]
             && faceIds[1]<faceIds[2]);

      vtkFace *f=this->GetFace(faceIds);
      if(f==0)
        {
        int externalSide;
        if (external)
          {
          if (orientationChanged)
            {
            externalSide = vtkFace::BACK_FACE;
            }
          else
            {
            externalSide = vtkFace::FRONT_FACE;
            }
          }
        else
          {
          externalSide = vtkFace::NOT_EXTERNAL;
          }
        f=new vtkFace(faceIds, externalSide);
        this->AllFaces.push_back(f);
        f->Ref();
        // All the vertices of this face need to be fed
        int i=0;
        while(i<3)
          {
          vtkstd::list<vtkFace *> *p=this->Vector[faceIds[i]];
          if(p==0)
            {
            p=new vtkstd::list<vtkFace *>;
            this->Vector[faceIds[i]]=p;
            }
          p->push_back(f);
          f->Ref();
          ++i;
          }
        if(this->CellScalars)
          {
          int c=this->NumberOfComponents;
          int scalarNumber;
          if(orientationChanged)
            {
            scalarNumber=1;
            }
          else
            {
            scalarNumber=0;
            }
          if(c==1)
            {
            f->SetScalar(scalarNumber,scalars->GetComponent(cellIdx,0));
            }
          else
            {
            double tmp=0;
            double tmp2;
            i=0;
            while(i<c)
              {
              tmp2=scalars->GetComponent(cellIdx,i);
              tmp+=tmp2*tmp2;
              ++i;
              }
            f->SetScalar(scalarNumber,sqrt(tmp));
            }
          }
        }
      else
        {
        if(this->CellScalars)
          {
          int scalarNumber;
          if(orientationChanged)
            {
            scalarNumber=1;
            }
          else
            {
            scalarNumber=0;
            }
          int c=this->NumberOfComponents;
          if(c==1)
            {
            f->SetScalar(scalarNumber,scalars->GetComponent(cellIdx,0));
            }
          else
            {
            double tmp=0;
            double tmp2;
            int i=0;
            while(i<c)
              {
              tmp2=scalars->GetComponent(cellIdx,i);
              tmp+=tmp2*tmp2;
              ++i;
              }
            f->SetScalar(scalarNumber,sqrt(tmp));
            }
          }
        }
    }
  
  
  void SetNotRendered()
    {
      vtkstd::list<vtkFace *>::iterator it;
      vtkstd::list<vtkFace *>::iterator end;
      it=this->AllFaces.begin();
      end=this->AllFaces.end();
      while(it!=end)
        {
        (*it)->SetRendered(0);
        ++it;
        }
    }

protected:
  // Return pointer to face faceIds if the use set of vertex faceIds[0] have
  // this face, otherwise return null.
  vtkFace *GetFace(vtkIdType faceIds[3])
    {    
      vtkstd::list<vtkFace *> *useSet=this->Vector[faceIds[0]];
      vtkFace *result=0;
      
      if(useSet!=0)
        {
        this->It=(*useSet).begin();
        this->ItEnd=(*useSet).end();
        int found=0;
        while(!found && this->It!=this->ItEnd)
          {
          result=*this->It;
          found=result->IsEqual(faceIds);
          ++this->It;
          }
        if(!found)
          {
          result=0;
          }
        }
      return result;
    }
  
  int CellScalars;
  int NumberOfComponents;
  
  
  // Used in GetFace()
  vtkstd::list<vtkFace *>::iterator It;
  vtkstd::list<vtkFace *>::iterator ItEnd;
};

// For each vertex, store its projection. It is view-dependent.
class vtkVertices
{
public:
  typedef vtkstd::vector<vtkVertexEntry> VectorType;
  VectorType Vector;
  
  // Initialize with the number of vertices.
  vtkVertices(int size)
    :Vector(size)
    {
    }
};

};

using namespace vtkUnstructuredGridVolumeZSweepMapperNamespace;

//-----------------------------------------------------------------------------
// Implementation of the public class.

vtkStandardNewMacro(vtkUnstructuredGridVolumeZSweepMapper);

vtkCxxSetObjectMacro(vtkUnstructuredGridVolumeZSweepMapper, RayIntegrator,
                     vtkUnstructuredGridVolumeRayIntegrator);


//-----------------------------------------------------------------------------
// Description:
// Set MaxPixelListSize to 32.
vtkUnstructuredGridVolumeZSweepMapper::vtkUnstructuredGridVolumeZSweepMapper()
{
  this->MaxPixelListSize=64; // default value.
  
  this->ImageSampleDistance        =  1.0;
  this->MinimumImageSampleDistance =  1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->AutoAdjustSampleDistances  =  1;
  
  this->ImageMemorySize[0]     = 0;
  this->ImageMemorySize[1]     = 0;
  
  this->Image                  = NULL;
  this->RealRGBAImage=0;

  this->RenderTimeTable        = NULL;
  this->RenderVolumeTable      = NULL;
  this->RenderRendererTable    = NULL;
  this->RenderTableSize        = 0;  
  this->RenderTableEntries     = 0;

  this->ZBuffer                = NULL;
  this->ZBufferSize[0]         = 0;
  this->ZBufferSize[1]         = 0;
  this->ZBufferOrigin[0]       = 0;
  this->ZBufferOrigin[1]       = 0;
  
  this->IntermixIntersectingGeometry = 1;

  this->ImageDisplayHelper     = vtkRayCastImageDisplayHelper::New();
  
  this->PixelListFrame=0;
  
  this->Cell=vtkGenericCell::New();

  this->EventList=vtkPriorityQueue::New();
  
  this->UseSet=0;
  this->Vertices=0;
  
  this->PerspectiveTransform = vtkTransform::New();
  this->PerspectiveMatrix = vtkMatrix4x4::New();
  
  this->SimpleEdge=new vtkSimpleScreenEdge;
  this->DoubleEdge=new vtkDoubleScreenEdge;
  
  this->Span=new vtkSpan;
  
  this->RayIntegrator = NULL;
  this->RealRayIntegrator = NULL;
  
  this->IntersectionLengths=vtkDoubleArray::New();
  this->IntersectionLengths->SetNumberOfValues(1);
  this->NearIntersections=vtkDoubleArray::New();
  this->NearIntersections->SetNumberOfValues(1);
  this->FarIntersections=vtkDoubleArray::New();
  this->FarIntersections->SetNumberOfValues(1);
  
  this->MemoryManager=0;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGridVolumeZSweepMapper::~vtkUnstructuredGridVolumeZSweepMapper()
{
  if(this->MemoryManager!=0)
    {
    delete this->MemoryManager;
    }
  if(this->PixelListFrame!=0)
    {
    delete this->PixelListFrame;
    }
  this->Cell->Delete();
  this->EventList->Delete();
  
  this->ImageDisplayHelper->Delete();
  
  if(this->UseSet!=0)
    {
    delete this->UseSet;
    }
  
  if(this->Vertices!=0)
    {
    delete this->Vertices;
    }
  
  this->PerspectiveTransform->Delete();
  this->PerspectiveMatrix->Delete();

  delete this->SimpleEdge;
  delete this->DoubleEdge;
  delete this->Span;
  
  if ( this->Image )
    {
    delete [] this->Image;
    delete [] this->RealRGBAImage;
    }
  
  if ( this->RenderTableSize )
    {
    delete [] this->RenderTimeTable;
    delete [] this->RenderVolumeTable;
    delete [] this->RenderRendererTable;
    }
  
  this->SetRayIntegrator(NULL);
  if (this->RealRayIntegrator)
    {
    this->RealRayIntegrator->UnRegister(this);
    }
  
  this->IntersectionLengths->Delete();
  this->NearIntersections->Delete();
  this->FarIntersections->Delete();
}

//-----------------------------------------------------------------------------
float vtkUnstructuredGridVolumeZSweepMapper::RetrieveRenderTime(
  vtkRenderer *ren, 
  vtkVolume   *vol )
{
  int i;
  
  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      return this->RenderTimeTable[i];
      }
    }
  
  return 0.0;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::StoreRenderTime(
  vtkRenderer *ren, 
  vtkVolume   *vol, 
  float       time )
{
  int i;
  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      this->RenderTimeTable[i] = time;
      return;
      }
    }
  
  
  // Need to increase size
  if ( this->RenderTableEntries >= this->RenderTableSize )
    {
    if ( this->RenderTableSize == 0 )
      {
      this->RenderTableSize = 10;
      }
    else
      {
      this->RenderTableSize *= 2;
      }
    
    float       *oldTimePtr     = this->RenderTimeTable;
    vtkVolume   **oldVolumePtr   = this->RenderVolumeTable;
    vtkRenderer **oldRendererPtr = this->RenderRendererTable;
    
    this->RenderTimeTable     = new float [this->RenderTableSize];
    this->RenderVolumeTable   = new vtkVolume *[this->RenderTableSize];
    this->RenderRendererTable = new vtkRenderer *[this->RenderTableSize];
    
    for (i = 0; i < this->RenderTableEntries; i++ )
      {
      this->RenderTimeTable[i] = oldTimePtr[i];
      this->RenderVolumeTable[i] = oldVolumePtr[i];
      this->RenderRendererTable[i] = oldRendererPtr[i];
      }
    
    delete [] oldTimePtr;
    delete [] oldVolumePtr;
    delete [] oldRendererPtr;
    }
  
  this->RenderTimeTable[this->RenderTableEntries] = time;
  this->RenderVolumeTable[this->RenderTableEntries] = vol;
  this->RenderRendererTable[this->RenderTableEntries] = ren;
  
  this->RenderTableEntries++;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Max Pixel List Size: " << this->MaxPixelListSize << "\n";

  os << indent << "Image Sample Distance: " 
     << this->ImageSampleDistance << "\n";
  os << indent << "Minimum Image Sample Distance: " 
     << this->MinimumImageSampleDistance << "\n";
  os << indent << "Maximum Image Sample Distance: " 
     << this->MaximumImageSampleDistance << "\n";
  os << indent << "Auto Adjust Sample Distances: " 
     << this->AutoAdjustSampleDistances << "\n";
  os << indent << "Intermix Intersecting Geometry: "
    << (this->IntermixIntersectingGeometry ? "On\n" : "Off\n");

  // The PrintSelf test just search for words in the PrintSelf function
  // We add here the internal variable we don't want to display:
  // this->ImageViewportSize this->ImageOrigin this->ImageInUseSize
  
  if (this->RayIntegrator)
    {
    os << indent << "RayIntegrator: "
       << this->RayIntegrator->GetClassName() << endl;
    }
  else
    {
    os << indent << "RayIntegrator: (automatic)" << endl;
    }
}

//-----------------------------------------------------------------------------
// Description:
// Maximum size allowed for a pixel list. Default is 32.
// During the rendering, if a list of pixel is full, incremental compositing
// is performed. Even if it is a user setting, it is an advanced parameter.
// You have to understand how the algorithm works to change this value.
int vtkUnstructuredGridVolumeZSweepMapper::GetMaxPixelListSize()
{
  return this->MaxPixelListSize;
}

//-----------------------------------------------------------------------------
// Description:
// Change the maximum size allowed for a pixel list. It is an advanced
// parameter.
void vtkUnstructuredGridVolumeZSweepMapper::SetMaxPixelListSize(int size)
{
  assert("pre: positive_size" && size>1);
  this->MaxPixelListSize=size;
}

//-----------------------------------------------------------------------------
#define ESTABLISH_INTEGRATOR(classname)                                 \
  if (   !this->RealRayIntegrator                                       \
         || (!this->RealRayIntegrator->IsA(#classname)) )               \
    {                                                                   \
    if (this->RealRayIntegrator) this->RealRayIntegrator->UnRegister(this); \
    this->RealRayIntegrator = classname::New();                         \
    this->RealRayIntegrator->Register(this);                            \
    this->RealRayIntegrator->Delete();                                  \
    }                                                                   \
  
//-----------------------------------------------------------------------------
// Description:
// WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
// DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
// Render the volume
void vtkUnstructuredGridVolumeZSweepMapper::Render(vtkRenderer *ren,
                                                   vtkVolume *vol)
{
  vtkDebugMacro(<<"Render");
  
  // Check for input
  if(this->GetInput()==0)
    {
    vtkErrorMacro(<< "No Input!");
    return;
    }
  
  this->Scalars = this->GetScalars(this->GetInput(), this->ScalarMode,
                                   this->ArrayAccessMode,
                                   this->ArrayId, this->ArrayName,
                                   this->CellScalars);

  if(this->Scalars==0)
    {
    vtkErrorMacro("Can't use the ZSweep mapper without scalars!");
    return;
    }
  
  this->GetInput()->UpdateInformation();
  this->GetInput()->SetUpdateExtentToWholeExtent();
  this->GetInput()->Update();
  
   // Check to make sure we have an appropriate integrator.
  if (this->RayIntegrator)
    {
    if (this->RealRayIntegrator != this->RayIntegrator)
      {
      if (this->RealRayIntegrator)
        {
        this->RealRayIntegrator->UnRegister(this);
        }
      this->RealRayIntegrator = this->RayIntegrator;
      this->RealRayIntegrator->Register(this);
      }
    }
  else
    {
    if (this->CellScalars)
      {
      ESTABLISH_INTEGRATOR(vtkUnstructuredGridHomogeneousRayIntegrator);
      }
    else
      {
      if (vol->GetProperty()->GetIndependentComponents())
        {
        ESTABLISH_INTEGRATOR(vtkUnstructuredGridPreIntegration);
        }
      else
        {
        ESTABLISH_INTEGRATOR(vtkUnstructuredGridPartialPreIntegration);
        }
      }
    }
  // Start timing now. We didn't want to capture the update of the
  // input data in the times
  this->Timer->StartTimer();
  
  int oldImageMemorySize[2];
  oldImageMemorySize[0] = this->ImageMemorySize[0];
  oldImageMemorySize[1] = this->ImageMemorySize[1];

  // If we are automatically adjusting the size to achieve a desired frame
  // rate, then do that adjustment here. Base the new image sample distance 
  // on the previous one and the previous render time. Don't let
  // the adjusted image sample distance be less than the minimum image sample 
  // distance or more than the maximum image sample distance.
  float oldImageSampleDistance = this->ImageSampleDistance;
  if ( this->AutoAdjustSampleDistances )
    {
    float oldTime = this->RetrieveRenderTime( ren, vol );
    float newTime = vol->GetAllocatedRenderTime();
    this->ImageSampleDistance *= sqrt(oldTime / newTime);
    this->ImageSampleDistance = 
      (this->ImageSampleDistance>this->MaximumImageSampleDistance)?
      (this->MaximumImageSampleDistance):(this->ImageSampleDistance);
    this->ImageSampleDistance = 
      (this->ImageSampleDistance<this->MinimumImageSampleDistance)?
      (this->MinimumImageSampleDistance):(this->ImageSampleDistance);
    }
  
  // The full image fills the viewport. First, compute the actual viewport
  // size, then divide by the ImageSampleDistance to find the full image
  // size in pixels
  int width, height;
  ren->GetTiledSize(&width, &height);
  this->ImageViewportSize[0] = 
    static_cast<int>(width/this->ImageSampleDistance);
  this->ImageViewportSize[1] = 
    static_cast<int>(height/this->ImageSampleDistance);

  this->ImageInUseSize[0] = this->ImageViewportSize[0];
  this->ImageInUseSize[1] = this->ImageViewportSize[1];
  this->ImageOrigin[0] = 0;
  this->ImageOrigin[1] = 0;
  
  // What is a power of 2 size big enough to fit this image?
  this->ImageMemorySize[0] = 32;
  this->ImageMemorySize[1] = 32;
  while ( this->ImageMemorySize[0] < this->ImageInUseSize[0] )
    {
    this->ImageMemorySize[0] *= 2;
    }
  while ( this->ImageMemorySize[1] < this->ImageInUseSize[1] )
    {
    this->ImageMemorySize[1] *= 2;
    }
  
  // If the old image size is much too big (more than twice in
  // either direction) then set the old width to 0 which will
  // cause the image to be recreated
  if ( oldImageMemorySize[0] > 2*this->ImageMemorySize[0] ||
       oldImageMemorySize[1] > 2*this->ImageMemorySize[1] )
    {
    oldImageMemorySize[0] = 0;
    }
  
  // If the old image is big enough (but not too big - we handled
  // that above) then we'll bump up our required size to the
  // previous one. This will keep us from thrashing.
  if ( oldImageMemorySize[0] >= this->ImageMemorySize[0] &&
       oldImageMemorySize[1] >= this->ImageMemorySize[1] )
    {
    this->ImageMemorySize[0] = oldImageMemorySize[0];
    this->ImageMemorySize[1] = oldImageMemorySize[1];
    }
  
  int bufferSize=this->ImageMemorySize[0] * this->ImageMemorySize[1] * 4;
  
  // Do we already have a texture big enough? If not, create a new one and
  // clear it.
  if ( !this->Image ||
       this->ImageMemorySize[0] > oldImageMemorySize[0] ||
       this->ImageMemorySize[1] > oldImageMemorySize[1] )
    {
    // If there is an image there must be row bounds
    if ( this->Image )
      {
      delete [] this->Image;
      delete [] this->RealRGBAImage;
      }
    this->Image = new unsigned char[bufferSize];
    this->RealRGBAImage=new float[bufferSize];
    }

  // We have to clear the image, each time:
  memset(this->Image,0,bufferSize);
  
  vtkIdType j=0;
  while(j<bufferSize)
    {
    this->RealRGBAImage[j]=0;
    this->RealRGBAImage[j+1]=0;
    this->RealRGBAImage[j+2]=0;
    this->RealRGBAImage[j+3]=0;
    j+=4;
    }
  
  // Capture the zbuffer if necessary
  if ( this->IntermixIntersectingGeometry && 
       ren->GetNumberOfPropsRendered() )
    {
    int x1, x2, y1, y2;
    double *viewport   =  ren->GetViewport();
    int *renWinSize   =  ren->GetRenderWindow()->GetSize();
    
    // turn this->ImageOrigin into (x1,y1) in window (not viewport!)
    // coordinates. 
    x1 = static_cast<int> (
      viewport[0] * static_cast<float>(renWinSize[0]) +
      static_cast<float>(this->ImageOrigin[0]) * this->ImageSampleDistance );
    y1 = static_cast<int> (
      viewport[1] * static_cast<float>(renWinSize[1]) +
      static_cast<float>(this->ImageOrigin[1]) * this->ImageSampleDistance);
    
    // compute z buffer size
    this->ZBufferSize[0] = static_cast<int>(
      static_cast<float>(this->ImageInUseSize[0]) * this->ImageSampleDistance);
    this->ZBufferSize[1] = static_cast<int>(
      static_cast<float>(this->ImageInUseSize[1]) * this->ImageSampleDistance);
    
    // Use the size to compute (x2,y2) in window coordinates
    x2 = x1 + this->ZBufferSize[0] - 1;
    y2 = y1 + this->ZBufferSize[1] - 1;
    
    // This is the z buffer origin (in viewport coordinates)
    this->ZBufferOrigin[0] = static_cast<int>(
      static_cast<float>(this->ImageOrigin[0]) * this->ImageSampleDistance);
    this->ZBufferOrigin[1] = static_cast<int>(
      static_cast<float>(this->ImageOrigin[1]) * this->ImageSampleDistance);
    
    // Capture the z buffer
    this->ZBuffer = ren->GetRenderWindow()->GetZbufferData(x1,y1,x2,y2);
    }
      
  this->RealRayIntegrator->Initialize(vol, this->Scalars);
  
  // Here is the Zsweep algorithm:
  
  // 1. For each vertex, find the list of incident faces (the "use set") (3.1)
  // In the original paper, it deals with incident cells but the chapter about
  // the parallel version in the dissertation deals with faces, which makes
  // more sense. Hence, there is no need for the sparsification step (3.5.1)
  // It is view-independent, so it can be reused for the next call to Render()
  // if the dataset did not change.
  vtkDebugMacro(<<"BuildUseSets: start");
  this->BuildUseSets();
  vtkDebugMacro(<<"BuildUseSets: done");
  
  // 2. Sort the vertices by z-coordinates (view-dependent) in view space.
  // For each vertex, compute its camera coordinates and sort it
  // by z in an heap. The heap is called the "event list".
  // The heap stores the Id of the vertices.
  // It is view-dependent. 
  vtkDebugMacro(<<"ProjectAndSortVertices: start");
  this->ProjectAndSortVertices(ren,vol);
  vtkDebugMacro(<<"ProjectAndSortVertices: done");
  
  // 3. Create an empty "pixel list" (two way linked list) for each pixel of
  //    the screen.
  vtkDebugMacro(<<"CreateAndCleanPixelList: start");
  this->CreateAndCleanPixelList();
  vtkDebugMacro(<<"CreateAndCleanPixelList: done");
  
  // 4. Main loop
  // (section 2 paragraph 11)
  vtkDebugMacro(<<"MainLoop: start");
  this->MainLoop(ren->GetRenderWindow());
  vtkDebugMacro(<<"MainLoop: done");
  
  // The algorithm is done: send to result to the final image.
  if ( !ren->GetRenderWindow()->GetAbortRender() )
    {
    float depth;
    if ( this->IntermixIntersectingGeometry )
      {
      depth = this->GetMinimumBoundsDepth( ren, vol );
      }
    else
      {
      depth = -1;
      }

    // copy the double image into the unsigned char image:
    
    j=0;
    while(j<bufferSize)
      {
      float alpha=this->RealRGBAImage[j+3];
      if(alpha!=0)
        {
        this->Image[j+0]=this->ColorComponentRealToByte(this->RealRGBAImage[j+0]);
        this->Image[j+1]=this->ColorComponentRealToByte(this->RealRGBAImage[j+1]);
        this->Image[j+2]=this->ColorComponentRealToByte(this->RealRGBAImage[j+2]);
        this->Image[j+3]=this->ColorComponentRealToByte(alpha);
        }
      else
        {
        this->Image[j+0]=0;
        this->Image[j+1]=0;
        this->Image[j+2]=0;
        this->Image[j+3]=0;
        }
      j+=4;
      }
    this->ImageDisplayHelper->
      RenderTexture( vol, ren,
                     this->ImageMemorySize,
                     this->ImageViewportSize,
                     this->ImageInUseSize,
                     this->ImageOrigin,
                     depth,
                     this->Image );
    
    this->Timer->StopTimer();
    this->TimeToDraw = this->Timer->GetElapsedTime();
    this->StoreRenderTime( ren, vol, this->TimeToDraw );
    }
  else
    {
    this->ImageSampleDistance = oldImageSampleDistance;
    }
  
  if ( this->ZBuffer )
    {
    delete [] this->ZBuffer;
    this->ZBuffer = NULL;
    }
  
  this->UpdateProgress(1.0);
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::AllocateUseSet(vtkIdType size)
{
  if(this->UseSet!=0)
    {
    if(size>static_cast<vtkIdType>(this->UseSet->Vector.size()))
      {
      delete this->UseSet;
      this->UseSet=new vtkUseSet(size);
      }
    else
      {
      this->UseSet->Clear();
      }
    }
  else
    {
    this->UseSet=new vtkUseSet(size);
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::AllocateVertices(vtkIdType size)
{
  if(this->Vertices!=0)
    {
    if(size>static_cast<vtkIdType>(this->Vertices->Vector.size()))
      {
      delete this->Vertices;
      this->Vertices=new vtkVertices(size);
      }
    }
  else
    {
    this->Vertices=new vtkVertices(size);
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::BuildUseSets()
{
  int needsUpdate = 0;
  
  // If we have never created the list, we need updating
  if (this->UseSet==0 )
    {
    needsUpdate = 1;
    }
  
  // If the data has changed in some way then we need to update
  vtkUnstructuredGrid *input = this->GetInput();
  if ( input->GetMTime() > this->SavedTriangleListMTime.GetMTime() )
    {
    needsUpdate = 1;
    }
  
  if(this->CellScalars &&
     this->GetMTime() > this->SavedTriangleListMTime.GetMTime())
    {
    needsUpdate=1;
    }
  
  // If we don't need updating, return
  if ( !needsUpdate )
    {
    return;
    }
  
  vtkIdType numberOfCells=input->GetNumberOfCells();
  vtkIdType numberOfPoints=input->GetNumberOfPoints();

  vtkIdList *cellNeighbors = vtkIdList::New();
  
  // init the use set of each vertex
  this->AllocateUseSet(numberOfPoints);
  
  this->UseSet->SetCellScalars(this->CellScalars);
  if(this->CellScalars)
    {
    this->UseSet->SetNumberOfComponents(
      this->Scalars->GetNumberOfComponents());
    }
  // for each cell
  vtkIdType cellIdx=0;
  while(cellIdx<numberOfCells)
    {
    input->GetCell(cellIdx,this->Cell);
    
    vtkIdType faces=this->Cell->GetNumberOfFaces();
    vtkIdType faceidx=0;
    vtkCell *face;
    vtkIdType faceIds[3];
    vtkIdType orderedFaceIds[3];
    // for each face
    while(faceidx<faces)
      {
      face=this->Cell->GetFace(faceidx);
      faceIds[0]=face->GetPointId(0);
      faceIds[1]=face->GetPointId(1);
      faceIds[2]=face->GetPointId(2);
      int orientationChanged=this->ReorderTriangle(faceIds,orderedFaceIds);
      input->GetCellNeighbors(cellIdx, face->GetPointIds(), cellNeighbors);
      bool external = (cellNeighbors->GetNumberOfIds() == 0);
      
      // Add face only if it is not already in the useset.
      this->UseSet->AddFace(orderedFaceIds, this->Scalars,
                            cellIdx, orientationChanged, external);
      
      ++faceidx;
      }
    ++cellIdx;
    }
  cellNeighbors->Delete();
  this->SavedTriangleListMTime.Modified();
}

//-----------------------------------------------------------------------------
// Description:
// Reorder vertices `v' in increasing order in `w'. Return if the orientation
// has changed.
int vtkUnstructuredGridVolumeZSweepMapper::ReorderTriangle(vtkIdType v[3],
                                                           vtkIdType w[3])
{
  if(v[0]>v[1])
    {
    if(v[1]>v[2])
      {
      // v[2] is the min
      w[0]=v[2];
      w[1]=v[0];
      w[2]=v[1];
      }
    else
      {
      // v[1] is the min
      w[0]=v[1];
      w[1]=v[2];
      w[2]=v[0];
      }
    }
  else
    {
    if(v[0]>v[2])
      {
      // v[2] is the min
      w[0]=v[2];
      w[1]=v[0];
      w[2]=v[1];
      }
    else
      {
      // v[0] is the min
      w[0]=v[0];
      w[1]=v[1];
      w[2]=v[2];
      }
    }
  // At this point the triangle start with the min id and the
  // order did not change
  // Now, ensure that the two last id are in increasing order
  int result=w[1]>w[2];
  if(result)
    {
    vtkIdType tmp=w[1];
    w[1]=w[2];
    w[2]=tmp;
    }
  return result;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::ProjectAndSortVertices(
  vtkRenderer *ren,
  vtkVolume *vol)
{
  assert("pre: empty list" && this->EventList->GetNumberOfItems()==0);
  
  vtkUnstructuredGrid *input = this->GetInput();
  vtkIdType numberOfPoints=input->GetNumberOfPoints();
  
  vtkIdType pointId=0;
  vtkVertexEntry *vertex=0;
  // Pre-computation for the projection.
  
  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  // Get the view matrix in two steps - there is a one step method in camera
  // but it turns off stereo so we do not want to use that one
  vtkCamera *cam = ren->GetActiveCamera();
  this->PerspectiveTransform->Identity();
  this->PerspectiveTransform->Concatenate(
    cam->GetProjectionTransformMatrix(aspect[0]/aspect[1], 0.0, 1.0 ));
  this->PerspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  this->PerspectiveTransform->Concatenate(vol->GetMatrix());
  this->PerspectiveMatrix->DeepCopy(this->PerspectiveTransform->GetMatrix());
  
  this->AllocateVertices(numberOfPoints);
  
  while(pointId<numberOfPoints)
    {
    vertex=&(this->Vertices->Vector[pointId]);
    
    // Projection
    //
    double inPoint[4];
    input->GetPoint(pointId,inPoint);
    inPoint[3] = 1.0;
    
    double outPoint[4];
    this->PerspectiveMatrix->MultiplyPoint( inPoint, outPoint );
    assert("outPoint[3]" && outPoint[3]!=0.0);
    
    double invW=1/outPoint[3];
    double zView = outPoint[2]*invW;
    
    int xScreen=static_cast<int>((outPoint[0]*invW+1)*0.5*this->ImageViewportSize[0]-this->ImageOrigin[0]);
    int yScreen=static_cast<int>((outPoint[1]*invW+1)*0.5*this->ImageViewportSize[1]-this->ImageOrigin[1]);
    
    double outWorldPoint[4];

    vol->GetMatrix()->MultiplyPoint( inPoint, outWorldPoint );

    assert("check: vol no projection" && outWorldPoint[3]==1);
    
    double scalar;
    if(this->CellScalars) // cell attribute
      {
      scalar=0; // ignored
      }
    else // point attribute
      {
      int numComp=this->Scalars->GetNumberOfComponents();
      if(numComp==1)
        {
        scalar=this->Scalars->GetComponent(pointId,0);
        }
      else
        {
        int comp=0;
        scalar=0;
        while(comp<numComp)
          {
          double value=this->Scalars->GetComponent(pointId,comp);
          scalar+=value*value;
          ++comp;
          }
        scalar=sqrt(scalar);
        }
      }
   
    vertex->Set(xScreen,yScreen,outWorldPoint[0]/outWorldPoint[3],
                outWorldPoint[1]/outWorldPoint[3],
                outWorldPoint[2]/outWorldPoint[3],zView,scalar,invW);
    
    // Sorting
    //
    // we store -z because the top of the priority list is the
    // smallest value
#ifdef BACK_TO_FRONT
    this->EventList->Insert(-zView,pointId);
#else
    this->EventList->Insert(zView,pointId);
#endif
    ++pointId;
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::CreateAndCleanPixelList()
{
  // paper: a "pixel list" is a double linked list. We put that in a queue.
  vtkIdType size=this->ImageInUseSize[0]*this->ImageInUseSize[1];
  if(this->PixelListFrame!=0)
    {
    if(this->PixelListFrame->GetSize()<size)
      {
      delete this->PixelListFrame;
      this->PixelListFrame=0;
      }
    }
  
  if(this->PixelListFrame==0)
    {
    this->PixelListFrame=new vtkPixelListFrame(size);
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::MainLoop(vtkRenderWindow *renWin)
{
  double previousZTarget=0.0;
  double zTarget;
  vtkIdType vertex;
  
// used to know if the next vertex is on the same plane
  double currentZ; // than the previous one. If so, the z-target has to be
  // updated (without calling the compositing function)
  if(this->EventList->GetNumberOfItems()==0)
    {
    return; // we are done.
    }
  
  // initialize the "previous z-target" to the z-coordinate of the first
  // vertex.
  vertex=this->EventList->Peek(0,previousZTarget);
  
#ifdef BACK_TO_FRONT
  previousZTarget=-previousZTarget; // because the EventList store -z
#endif
  
  // (section 2 paragraph 11)
  // initialize the "z-target" with the maximum z-coordinate of the adjacent
  // vertices to the first vertex. The adjacent vertices can be found
  // indirectly by using the "use set" of the first vertex (cells), and
  // by taking the vertices of all those cells.
  //
  zTarget=previousZTarget;
  vtkstd::list<vtkFace *>::iterator it;
  vtkstd::list<vtkFace *>::iterator itEnd;
  
//  this->MaxRecordedPixelListSize=0;
  this->MaxPixelListSizeReached=0;
  this->XBounds[0]=this->ImageInUseSize[0];
  this->XBounds[1]=0;
  this->YBounds[0]=this->ImageInUseSize[1];
  this->YBounds[1]=0;
  
  vtkIdType progressCount=0;
  vtkIdType sum=this->EventList->GetNumberOfItems();
  
  if(this->MemoryManager==0)
    {
    this->MemoryManager=new vtkPixelListEntryMemory;
    }
  
  this->UseSet->SetNotRendered();
  
  int aborded=0;
  // for each vertex of the "event list"
  while(this->EventList->GetNumberOfItems()>0)
    {
    this->UpdateProgress(static_cast<double>(progressCount)/sum);
    
    aborded=renWin->CheckAbortStatus();
    if(aborded)
      {
      break;
      }
    ++progressCount;
    //  the z coordinate of the current vertex defines the "sweep plane".
    vertex=this->EventList->Pop(0,currentZ);

    if(this->UseSet->Vector[vertex]!=0)
      { // otherwise the vertex is not useful, basically this is the
      // end we reached the last ztarget
      
#ifdef BACK_TO_FRONT
    currentZ=-currentZ; // because the EventList store -z
#endif
    
    if(previousZTarget==currentZ)
      {
      // the new vertex is on the same sweep plane than the previous vertex
      // that defined a z target
      // => the z target has to be updated accordingly
      // This is also the case for the first vertex.
      it=this->UseSet->Vector[vertex]->begin();
      itEnd=this->UseSet->Vector[vertex]->end();
      
      // for each face incident with the vertex
      while(it!=itEnd)
        {
        vtkFace *face=(*it);
        // for each point of the face, get the closest z
        vtkIdType *vids=face->GetFaceIds();
        vtkIdType i=0;
        while(i<3)
          {
          double z=this->Vertices->Vector[vids[i]].GetZview();
#ifdef BACK_TO_FRONT
          if(z<zTarget)
#else
            if(z>zTarget)
#endif
            {
            zTarget=z;
            }
          ++i;
          }
        ++it;
        }
      }
  
    // Time to call the composite function?
#ifdef BACK_TO_FRONT
    if(currentZ<zTarget)
#else
      if(currentZ>zTarget)
#endif
      {
      this->CompositeFunction(zTarget);
      
      // Update the zTarget
      previousZTarget=zTarget;
      
      it=this->UseSet->Vector[vertex]->begin();
      itEnd=this->UseSet->Vector[vertex]->end();
      // for each cell incident with the vertex
      while(it!=itEnd)
        {
        vtkFace *face=(*it);
        // for each point of the face, get the closest z
        vtkIdType *vids=face->GetFaceIds();
        vtkIdType i=0;
        while(i<3)
          {
          double z=this->Vertices->Vector[vids[i]].GetZview();
#ifdef BACK_TO_FRONT
          if(z<zTarget)
#else
            if(z>zTarget)
#endif
            {
            zTarget=z;
            }
          ++i;
          }
        ++it;
        }
      }
    else
      {
      if(this->MaxPixelListSizeReached)
        {
        this->CompositeFunction(currentZ);
        // We do not update the zTarget in this case.
        }
      }
    
    //  use the "use set" (cells) of the vertex to get the cells that are
    //  incident on the vertex, and that have this vertex as
    //  minimal z-coordinate,
    
    it=this->UseSet->Vector[vertex]->begin();
    itEnd=this->UseSet->Vector[vertex]->end();
    
    while(it!=itEnd)
      {
      vtkFace *face=(*it);
      if(!face->GetRendered())
        {
        vtkIdType *vids=face->GetFaceIds();
        if(this->CellScalars)
          {
          this->FaceScalars[0]=face->GetScalar(0);
          this->FaceScalars[1]=face->GetScalar(1);
          }
        this->RasterizeFace(vids, face->GetExternalSide());
        face->SetRendered(1);
        }
#if 0 // face search
      // for each point of the face, get the closest z
      vtkIdType *vids=face->GetFaceIds();
      vtkIdType minVertex=vids[0];
      double farestZ=this->Vertices->Vector[vids[0]].GetZview();
      
      vtkIdType i=1;
      while(i<3)
        {
        double z=this->Vertices->Vector[vids[i]].GetZview();
#ifdef BACK_TO_FRONT
        if(z>farestZ)
#else
          if(z<farestZ)
#endif
          {
          farestZ=z;
          minVertex=vids[i];
          }
        ++i;
        }
      if(minVertex==vertex)
        {
//        if(face->GetRendered())
//          {
//          cout<<"FACE ALREADY RENDERED!!!!"<<endl;
//          }
        this->RasterizeFace(vids);
//        face->SetRendered(1);
        }
#endif // face search      
      ++it;
      }
      } // if useset of vertex is not null
    } // while(eventList->GetNumberOfItems()>0)

  if(!aborded)
    {
    // Here a final compositing
    vtkDebugMacro(<<"Flush Compositing");
//   this->SavePixelListFrame();
#ifdef BACK_TO_FRONT
    this->CompositeFunction(-2);
#else
    this->CompositeFunction(2);
#endif
    }
  else
    {
    this->EventList->Reset();
    }
  this->PixelListFrame->Clean(this->MemoryManager);
//  vtkDebugMacro(<<"MaxRecordedPixelListSize="<<this->MaxRecordedPixelListSize);
  
  assert("post: empty_list" && this->EventList->GetNumberOfItems()==0);
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::SavePixelListFrame()
{
  vtkPolyData *dataset=vtkPolyData::New();
  
  vtkIdType height=this->ImageInUseSize[1];
  vtkIdType width=this->ImageInUseSize[0];
  vtkPixelListEntry *current;
  vtkIdType i;
  
  vtkPoints *pts=vtkPoints::New();
  pts->SetDataTypeToDouble();
  
  vtkDoubleArray *dataArray=vtkDoubleArray::New();
  vtkCellArray *vertices=vtkCellArray::New();
  vtkIdType pointId=0;
  
//  height=151;
//  width=151;
  
  vtkIdType y=0; //150;
  while(y<height)
    {
    vtkIdType x=0; //150;
    while(x<width)
      {     
      i=y*this->ImageInUseSize[0]+x;
      current=this->PixelListFrame->GetFirst(i);
      while(current!=0)
        {
        double *values=current->GetValues();
        
        double point[3];
        point[0]=x;
        point[1]=y;
        point[2]=values[2]; // zWorld
        
        pts->InsertNextPoint(point);
        dataArray->InsertNextValue(values[3]);
        vertices->InsertNextCell(1,&pointId);
        current=current->GetNext();
        ++pointId;
        }
      ++x;
      }
    ++y;
    }
  dataset->SetPoints(pts);
  pts->Delete();
  dataset->SetVerts(vertices);
  vertices->Delete();
  dataset->GetPointData()->SetScalars(dataArray);
  dataArray->Delete();
  
  vtkXMLPolyDataWriter *writer=vtkXMLPolyDataWriter::New();
  writer->SetFileName("pixellistframe.vtp");
  writer->SetInput(dataset);
  writer->SetIdTypeToInt32();
  dataset->Delete();
  writer->Write();
  writer->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Perform a scan conversion of a triangle, interpolating z and the scalar.
void vtkUnstructuredGridVolumeZSweepMapper::RasterizeFace(vtkIdType faceIds[3],
                                                          int externalSide)
{
  // The triangle is splitted by an horizontal line passing through the
  // second vertex v1 (y-order)
  // Hence, on one side there one edge (v0v2), on the other side there are two
  // edges (v0v1 and v1v2).
  
  vtkVertexEntry *v0=&(this->Vertices->Vector[faceIds[0]]);
  vtkVertexEntry *v1=&(this->Vertices->Vector[faceIds[1]]);
  vtkVertexEntry *v2=&(this->Vertices->Vector[faceIds[2]]);

  bool exitFace = false;
  
  // Find the orientation of the triangle on the screen to get the right
  // scalar
  if((externalSide != vtkFace::NOT_EXTERNAL) || this->CellScalars)
    {
    // To find the "winding" of the triangle as projected in screen space, we
    // perform the cross section.  The result trivially points along the Z axis.
    // It's magnitude is proportional to the triangle area and its direction
    // points away from the "front" face (what we are really interested in).
    // Since we know the cross product points in the Z direction, we only need
    // the Z component.
    int vec0[2], vec1[2];
    vec0[0] = v1->GetScreenX() - v0->GetScreenX();
    vec0[1] = v1->GetScreenY() - v0->GetScreenY();
    vec1[0] = v2->GetScreenX() - v0->GetScreenX();
    vec1[1] = v2->GetScreenY() - v0->GetScreenY();
    int zcross= vec0[0]*vec1[1] - vec0[1]*vec1[0];
    if(zcross<0)
      {
      this->FaceSide=1;
      }
    else
      {
      this->FaceSide=0;
      }

    // When determining the exit face, be conservative.  If the triangle is too
    // small to determine the orientation, it is better to assume that it is
    // exit than not exit.  This is because if it is misclassified as exit, then
    // we simply will not fill a rather small tet.  If it is misclassified as
    // not exit when it is, it could potential cause the filling of a large
    // space.
    if (externalSide == vtkFace::FRONT_FACE)
      {
#ifdef BACK_TO_FRONT
      exitFace = (zcross >= 0);
#else
      exitFace = (zcross <= 0);
#endif
      }
    else if (externalSide == vtkFace::BACK_FACE)
      {
#ifdef BACK_TO_FRONT
      exitFace = (zcross <= 0);
#else
      exitFace = (zcross >= 0);
#endif
      }
    }
  
  this->RasterizeTriangle(v0,v1,v2,exitFace);
}

//-----------------------------------------------------------------------------
// Description:
// Perform a scan conversion of a triangle, interpolating z and the scalar.
void  vtkUnstructuredGridVolumeZSweepMapper::RasterizeTriangle(
                                                            vtkVertexEntry *ve0,
                                                            vtkVertexEntry *ve1,
                                                            vtkVertexEntry *ve2,
                                                            bool externalFace)
{
  assert("pre: ve0_exists" && ve0!=0);
  assert("pre: ve1_exists" && ve1!=0);
  assert("pre: ve2_exists" && ve2!=0);
  
  vtkVertexEntry *v0=ve0;
  vtkVertexEntry *v1=ve1;
  vtkVertexEntry *v2=ve2;
  
  // The triangle is splitted by an horizontal line passing through the
  // second vertex v1 (y-order)
  // Hence, on one side there one edge (v0v2), on the other side there are two
  // edges (v0v1 and v1v2).
  
  // Order vertices by y screen.
  
  vtkVertexEntry *tmp;
  
  if(v0->GetScreenY()>v1->GetScreenY())
    {
    tmp=v0;
    v0=v1;
    v1=tmp;
    }
  if(v0->GetScreenY()>v2->GetScreenY())
    {
    tmp=v1;
    v1=v0;
    v0=v2;
    v2=tmp;
    }
  else
    {
    if(v1->GetScreenY()>v2->GetScreenY())
      {
      tmp=v1;
      v1=v2;
      v2=tmp;
      }
    }
  
  if(v0->GetScreenY()<this->YBounds[0])
    {
    if(v0->GetScreenY()>=0)
      {
      this->YBounds[0]=v0->GetScreenY();
      }
    else
      {
      this->YBounds[0]=0;
      }
    }
  if(v2->GetScreenY()>this->YBounds[1])
    {
    if(v2->GetScreenY()<this->ImageInUseSize[1])
      {
      this->YBounds[1]=v2->GetScreenY();
      }
    else
      {
      this->YBounds[1]=this->ImageInUseSize[1]-1;
      }
    }
  
  int x=v0->GetScreenX();
  
  if(x<this->XBounds[0])
    {
    if(x>=0)
      {
      this->XBounds[0]=x;
      }
    else
      {
      this->XBounds[0]=0;
      }
    }
  else
    {
    if(x>this->XBounds[1])
      {
      if(x<this->ImageInUseSize[0])
        {
        this->XBounds[1]=x;
        }
      else
        {
        this->XBounds[1]=this->ImageInUseSize[0]-1;
        }
      }
    }
  x=v1->GetScreenX();
  
  if(x<this->XBounds[0])
    {
    if(x>=0)
      {
      this->XBounds[0]=x;
      }
    else
      {
      this->XBounds[0]=0;
      }
    }
  else
    {
    if(x>this->XBounds[1])
      {
       if(x<this->ImageInUseSize[0])
        {
        this->XBounds[1]=x;
        }
      else
        {
        this->XBounds[1]=this->ImageInUseSize[0]-1;
        }
      }
    }
  
  x=v2->GetScreenX();
  
  if(x<this->XBounds[0])
    {
    if(x>=0)
      {
      this->XBounds[0]=x;
      }
    else
      {
      this->XBounds[0]=0;
      }
    }
  else
    {
    if(x>this->XBounds[1])
      {
      if(x<this->ImageInUseSize[0])
        {
        this->XBounds[1]=x;
        }
      else
        {
        this->XBounds[1]=this->ImageInUseSize[0]-1;
        }
      }
    }
  
  int dy20=v2->GetScreenY()-v0->GetScreenY();
  int dx10=v1->GetScreenX()-v0->GetScreenX();
  int dx20=v2->GetScreenX()-v0->GetScreenX();
  int dy10=v1->GetScreenY()-v0->GetScreenY();
  
  int det=dy20*dx10-dx20*dy10;
  
  vtkScreenEdge *leftEdge=0;
  vtkScreenEdge *rightEdge=0;
  
  if(det==0) //v0v1v2 aligned or v0=v1=v2
    {
    // easy case: v0=v1=v2 render the 3 points
    if(v0->GetScreenX()==v1->GetScreenX() && v0->GetScreenX()==v2->GetScreenX()
       && v0->GetScreenY()==v1->GetScreenY()
       && v0->GetScreenY()==v2->GetScreenY())
      {
      x=v0->GetScreenX();
      int y=v0->GetScreenY();
      if(x>=0 && x<this->ImageInUseSize[0] && y>=0 &&
         y<this->ImageInUseSize[1])
        {
        vtkIdType i=y*this->ImageInUseSize[0]+x;
        // Write the pixel
        vtkPixelListEntry *p0=this->MemoryManager->AllocateEntry();
        p0->Init(v0->GetValues(),v0->GetZview(), externalFace);
        if(this->CellScalars)
          {
          p0->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
          }
        this->PixelListFrame->AddAndSort(i,p0);
        
        vtkPixelListEntry *p1=this->MemoryManager->AllocateEntry();
        p1->Init(v1->GetValues(),v1->GetZview(), externalFace);
        if(this->CellScalars)
          {
          p1->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
          }
        this->PixelListFrame->AddAndSort(i,p1);
        
        vtkPixelListEntry *p2=this->MemoryManager->AllocateEntry();
        p2->Init(v2->GetValues(),v2->GetZview(), externalFace);
        if(this->CellScalars)
          {
          p2->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
          }
        this->PixelListFrame->AddAndSort(i,p2);
        
        
//        if(this->PixelListFrame->GetListSize(i)>this->MaxRecordedPixelListSize)
//          {
//          this->MaxRecordedPixelListSize=this->PixelListFrame->GetListSize(i);
//          }
        
        if(!this->MaxPixelListSizeReached)
          {
          this->MaxPixelListSizeReached=this->PixelListFrame->GetListSize(i)>
            this->MaxPixelListSize;
          } 
        }
      }
    else // line
      {
      this->RasterizeLine(v0,v1,externalFace);
      this->RasterizeLine(v1,v2,externalFace);
      this->RasterizeLine(v0,v2,externalFace);
      }
    return;
    }
  else
    {
    if(det>0) //v0v1 on right
      {
       this->DoubleEdge->Init(v0,v1,v2,dx10,dy10,1); // true=on right
       rightEdge=this->DoubleEdge;
       this->SimpleEdge->Init(v0,v2,dx20,dy20,0);
       leftEdge=this->SimpleEdge;
       }
     else
       {
       // v0v1 on left
       this->DoubleEdge->Init(v0,v1,v2,dx10,dy10,0); // true=on right
       leftEdge=this->DoubleEdge;
       this->SimpleEdge->Init(v0,v2,dx20,dy20,1);
       rightEdge=this->SimpleEdge;
       }
    }
  
  int y=v0->GetScreenY();
  int y1=v1->GetScreenY();
  int y2=v2->GetScreenY();
  
  int skipped=0;
  
  if(y1>=0) // clipping
    {
    
    if(y1>=this->ImageInUseSize[1]) // clipping
      {
      y1=this->ImageInUseSize[1]-1;
      }
    
    while(y<=y1)
      {
      if(y>=0 && y<this->ImageInUseSize[1]) // clipping
        {
        this->RasterizeSpan(y,leftEdge,rightEdge,externalFace);
        }
      ++y;
      if(y<=y1)
        {
        leftEdge->NextLine(y);
        rightEdge->NextLine(y);
        }
      }
    }
  else
    {
    leftEdge->SkipLines(y1-y,y1);
    rightEdge->SkipLines(y1-y,y1);
    y=y1;
    skipped=1;
    }
  
  if(y<this->ImageInUseSize[1]) // clipping
    {
    leftEdge->OnBottom(skipped,y);
    rightEdge->OnBottom(skipped,y);
    
    if(y2>=this->ImageInUseSize[1]) // clipping
      {
      y2=this->ImageInUseSize[1]-1;
      }
    
    while(y<=y2)
      {
      if(y>=0) // clipping, needed in case of no top
        {
        this->RasterizeSpan(y,leftEdge,rightEdge,externalFace);
        }
      ++y;
      leftEdge->NextLine(y);
      rightEdge->NextLine(y);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::RasterizeSpan(int y,
                                                          vtkScreenEdge *left,
                                                          vtkScreenEdge *right,
                                                          bool exitFace)
{
  assert("pre: left_exists" && left!=0);
  assert("pre: right_exists" && right!=0);
  
  vtkIdType i=y*this->ImageInUseSize[0];
  
  this->Span->Init(left->GetX(),
                   left->GetInvW(),
                   left->GetPValues(),
                   left->GetZview(),
                   right->GetX(),
                   right->GetInvW(),
                   right->GetPValues(),
                   right->GetZview());
  
  while(!this->Span->IsAtEnd())
    {
    int x=this->Span->GetX();
    if(x>=0 && x<this->ImageInUseSize[0]) // clipping
      {
      vtkIdType j=i+x;
      // Write the pixel
      vtkPixelListEntry *p=this->MemoryManager->AllocateEntry();
      p->Init(this->Span->GetValues(),this->Span->GetZview(), exitFace);
      
      if(this->CellScalars)
        {
        p->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
        }
      this->PixelListFrame->AddAndSort(j,p);
      
      
//      if(this->PixelListFrame->GetListSize(j)>this->MaxRecordedPixelListSize)
//        {
//        this->MaxRecordedPixelListSize=this->PixelListFrame->GetListSize(j);
//        }
      
      if(!this->MaxPixelListSizeReached)
        {
        this->MaxPixelListSizeReached=this->PixelListFrame->GetListSize(j)>
          this->MaxPixelListSize;
        }
      }
    this->Span->NextPixel();
    }
}

enum
{
  VTK_LINE_CONSTANT=0,
  VTK_LINE_BRESENHAM,
  VTK_LINE_DIAGONAL
};

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::RasterizeLine(vtkVertexEntry *v0,
                                                          vtkVertexEntry *v1,
                                                          bool exitFace)
{
  assert("pre: v0_exists" && v0!=0);
  assert("pre: v1_exists" && v1!=0);
  assert("pre: y_ordered" && v0->GetScreenY()<=v1->GetScreenY());
  
  int lineCase;
  int xIncrement; // if true increment x, if false increment y
  int dx;
  int dy;
  int xSign;
  
  // initialization is not useful, it is just to remove compiler warnings
  int dx2=0;
  int dy2=0;
  int e=0;
  
  double values[VTK_VALUES_SIZE];
  double pValues[VTK_VALUES_SIZE];
  
  double dPv[VTK_VALUES_SIZE];
  double dInvW;
  double dZ;
  
  double zView;
  double invW;
  
  int i;
  
  int x=v0->GetScreenX();
  int y=v0->GetScreenY();
  
  // 1. Find the case
  dx=v1->GetScreenX()-v0->GetScreenX();
  if(dx<0)
    {
    dx=-dx;
    xSign=-1;
    }
  else
    {
    xSign=1;
    }
  dy=v1->GetScreenY()-v0->GetScreenY();
  xIncrement=dx>dy;
  if(xIncrement)
    {
    if(dy==0)
      {
      lineCase=VTK_LINE_CONSTANT;
      }
    else
      {
      lineCase=VTK_LINE_BRESENHAM;
      dx2=dx<<1;
      dy2=dy<<1;
      e=dx;
      }
    
    double invDx=1.0/dx;
    i=0;
    invW=v0->GetInvW();
    double invW1=v1->GetInvW();
    double *val0=v0->GetValues();
    double *val1=v1->GetValues();
    while(i<VTK_VALUES_SIZE)
      {
      values[i]=val0[i];
      pValues[i]=values[i]*invW;
      dPv[i]=(val1[i]*invW1-pValues[i])*invDx;
      ++i;
      }
    dInvW=(invW1-invW)*invDx;
    zView=v0->GetZview();
    dZ=(v1->GetZview()-zView)*invDx;
    }
  else
    {
    if(dx==0)
      {
      if(dy==0)
        {
        // render both points and return
        // write pixel
        if(x>=0 && x<this->ImageInUseSize[0] && y>=0 &&
           y<this->ImageInUseSize[1]) // clipping
          {
          vtkIdType j=y*this->ImageInUseSize[0]+x; // mult==bad!!
          // Write the pixel
          vtkPixelListEntry *p0=this->MemoryManager->AllocateEntry();
          p0->Init(v0->GetValues(),v0->GetZview(), exitFace);
          
          if(this->CellScalars)
            {
            p0->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
            }
          this->PixelListFrame->AddAndSort(j,p0);
          
          // Write the pixel
          vtkPixelListEntry *p1=this->MemoryManager->AllocateEntry();
          p1->Init(v1->GetValues(),v1->GetZview(), exitFace);
          
          if(this->CellScalars)
            {
            p1->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
            }
          this->PixelListFrame->AddAndSort(j,p1);
          
          if(!this->MaxPixelListSizeReached)
            {
            this->MaxPixelListSizeReached=this->PixelListFrame->GetListSize(j)>
              this->MaxPixelListSize;
            }
          }
        return;
        }
      else
        {
        lineCase=VTK_LINE_CONSTANT;
        }
      }
    else
      {
      if(dy==dx)
        {
        lineCase=VTK_LINE_DIAGONAL;
        }
      else
        {
        lineCase=VTK_LINE_BRESENHAM;
        dx2=dx<<1;
        dy2=dy<<1;
        e=dy;
        }
      }
    double invDy=1.0/dy;
    i=0;
    invW=v0->GetInvW();
    double invW1=v1->GetInvW();
    double *val0=v0->GetValues();
    double *val1=v1->GetValues();
    while(i<VTK_VALUES_SIZE)
      {
      values[i]=val0[i];
      pValues[i]=values[i]*invW;
      dPv[i]=(val1[i]*invW1-pValues[i])*invDy;
      ++i;
      }
    dInvW=(invW1-invW)*invDy;
    zView=v0->GetZview();
    dZ=(v1->GetZview()-zView)*invDy;
    }
      
  // 2. Iterate over each pixel of the straight line.
  int done=0;
  while(!done)
    {
    // write pixel
    if(x>=0 && x<this->ImageInUseSize[0] && y>=0 &&
       y<this->ImageInUseSize[1]) // clipping
      {
      vtkIdType j=y*this->ImageInUseSize[0]+x; // mult==bad!!
      // Write the pixel
      vtkPixelListEntry *p0=this->MemoryManager->AllocateEntry();
      p0->Init(values,zView,exitFace);
      
      if(this->CellScalars)
        {
        p0->GetValues()[VTK_VALUES_SCALAR_INDEX]=this->FaceScalars[this->FaceSide];
        }
      this->PixelListFrame->AddAndSort(j,p0);
   
      if(!this->MaxPixelListSizeReached)
        {
        this->MaxPixelListSizeReached=this->PixelListFrame->GetListSize(j)>
          this->MaxPixelListSize;
        }
      }
    
    // next pixel
    switch(lineCase)
      {
      case VTK_LINE_CONSTANT:
        if(xIncrement)
          {
          x+=xSign;
          if(xSign>0)
            {
            done=x>v1->GetScreenX();
            }
          else
            {
            done=x<v1->GetScreenX();
            }
          }
        else
          {
          ++y;
          done=y>v1->GetScreenY();
          }
        // values, invw, zview
        break;
      case VTK_LINE_DIAGONAL:
        ++y;
        x+=xSign;
        done=y>v1->GetScreenY();
        // values, invw, zview
        break;
      case VTK_LINE_BRESENHAM:
        if(xIncrement)
          {
          x+=xSign;
          e+=dy2;
          if(e>=dx2)
            {
            e-=dx2;
            ++y;
            }
          if(xSign>0)
            {
            done=x>v1->GetScreenX();
            }
          else
            {
            done=x<v1->GetScreenX();
            }
          }
        else
          {
          ++y;
          e+=dx2;
          if(e>=dy2)
            {
            e-=dy2;
            x+=xSign;
            }
          done=y>v1->GetScreenY();
          }
        // values, invw, zview
        break;
      }
    if(!done)
      {
      invW+=dInvW;
      i=0;
      double w=1.0/invW;
      while(i<VTK_VALUES_SIZE)
        {
        pValues[i]+=dPv[i];
        values[i]=pValues[i]*w;
        ++i;
        }
      zView+=dZ;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeZSweepMapper::CompositeFunction(double zTarget)
{
  int y=this->YBounds[0];
  vtkIdType i=y*this->ImageInUseSize[0]+this->XBounds[0];
  
  vtkIdType index=(y*this->ImageMemorySize[0]+this->XBounds[0])<< 2; // *4
  vtkIdType indexStep=this->ImageMemorySize[0]<<2; // *4
  
  vtkPixelListEntry *current;
  vtkPixelListEntry *next;
  double zBuffer=0;
  
  int newXBounds[2];
  int newYBounds[2];
  
  newXBounds[0]=this->ImageInUseSize[0];
  newXBounds[1]=0;
  newYBounds[0]=this->ImageInUseSize[1];
  newYBounds[1]=0;

  int xMin=this->XBounds[0];
  int xMax=this->XBounds[1];
  int yMax=this->YBounds[1];
  
  vtkPixelList *pixel;
  int x;
  vtkIdType j;
  vtkIdType index2;
  int done;
  int doIntegration;
  double length;
  float *color;
  
  // for each pixel in the bounding box
  while(y<=yMax)
    {
    x=xMin;
    j=i;
    index2=index;
    while(x<=xMax)
      {
      pixel=this->PixelListFrame->GetList(j);
      // we need at least two entries per pixel to perform compositing
      if(pixel->GetSize()>=2)
        {
        current=pixel->GetFirst();
        next=current->GetNext();
#ifdef BACK_TO_FRONT
        done=current->GetZview()<=zTarget || next->GetZview()<=zTarget;
#else
        done=current->GetZview()>=zTarget || next->GetZview()>=zTarget;
#endif
        
        if(!done && this->ZBuffer!=0)
          {
          // value of the z buffer at the current pixel.
          zBuffer=this->GetZBufferValue(x,y);
          }
        
        while(!done)
          {
          if (current->GetExitFace())
            {
            // Do not do the integration if the current face is an exit face.
            // The space between current and next should be empty.
            doIntegration = 0;
            }
          else
            {
            if(this->ZBuffer!=0)
              {
              // check that current and next are in front of the z-buffer value
              doIntegration=current->GetZview()<zBuffer
                && next->GetZview()<zBuffer;
              }
            else
              {
              doIntegration=1;
              }
            }
          
          if(doIntegration)
            {
            if(current->GetZview()!=next->GetZview())
              {
              // length in world coordinates
              length=sqrt(vtkMath::Distance2BetweenPoints(
                            current->GetValues(),next->GetValues()));
              if(length!=0)
//              if(length>=0.4)
                {
                color=this->RealRGBAImage+index2;
                this->IntersectionLengths->SetValue(0,length);
                
                if(this->CellScalars)
                  {
                  // same value for near and far intersection
                  this->NearIntersections->SetValue(0,current->GetValues()[VTK_VALUES_SCALAR_INDEX]);
                  this->FarIntersections->SetValue(0,current->GetValues()[VTK_VALUES_SCALAR_INDEX]);
                  }
                else
                  {
                  this->NearIntersections->SetValue(0,current->GetValues()[VTK_VALUES_SCALAR_INDEX]);
                  this->FarIntersections->SetValue(0,next->GetValues()[VTK_VALUES_SCALAR_INDEX]);
                  }
#ifdef BACK_TO_FRONT
                this->RealRayIntegrator->Integrate(this->IntersectionLengths,
                                                   this->FarIntersections,
                                                   this->NearIntersections,
                                                   color);
#else
                this->RealRayIntegrator->Integrate(this->IntersectionLengths,
                                                   this->NearIntersections,
                                                   this->FarIntersections,
                                                   color);
#endif
                } // length!=0
              } // current->GetZview()!=next->GetZview()
            } // doIntegration
          
          // Next entry
          pixel->RemoveFirst(this->MemoryManager); // remove current
          done=pixel->GetSize()<2; // empty queue?
          if(!done)
            {
            current=next;
            next=current->GetNext();
#ifdef BACK_TO_FRONT
            done=next->GetZview()<=zTarget;
#else
            done=next->GetZview()>=zTarget;
#endif
            }
          } // while(!done)
        }
      if(pixel->GetSize()>=2)
        {
        if(x<newXBounds[0])
          {
          newXBounds[0]=x;
          }
        else
          {
          if(x>newXBounds[1])
            {
            newXBounds[1]=x;
            }
          }
        if(y<newYBounds[0])
          {
          newYBounds[0]=y;
          }
        else
          {
          if(y>newYBounds[1])
            {
            newYBounds[1]=y;
            }
          }
        }
      
      // next abscissa
      ++j;
      index2+=4;
      ++x;
      }
    // next ordinate
    i=i+this->ImageInUseSize[0];
    index+=indexStep;
    ++y;
    }
  
  // Update the bounding box. Useful for the delayed compositing

  this->XBounds[0]=newXBounds[0];
  this->XBounds[1]=newXBounds[1];
  this->YBounds[0]=newYBounds[0];
  this->YBounds[1]=newYBounds[1];

  this->MaxPixelListSizeReached=0;
}
 
//-----------------------------------------------------------------------------
// Description:
// Convert and clamp a float color component into a unsigned char.
unsigned char vtkUnstructuredGridVolumeZSweepMapper::ColorComponentRealToByte(
  float color)
{
  int val=static_cast<int>(color*255.0);
  if(val>255)
    {
    val=255;
    }
  else
    {
    if(val<0)
      {
      val=0;
      }
    }
  return static_cast<unsigned char>(val);
}

//-----------------------------------------------------------------------------
double vtkUnstructuredGridVolumeZSweepMapper::GetZBufferValue(int x,
                                                              int y)
{
  int xPos, yPos;
  
  xPos = static_cast<int>(static_cast<float>(x) * this->ImageSampleDistance);
  yPos = static_cast<int>(static_cast<float>(y) * this->ImageSampleDistance);
  
  xPos = (xPos >= this->ZBufferSize[0])?(this->ZBufferSize[0]-1):(xPos);
  yPos = (yPos >= this->ZBufferSize[1])?(this->ZBufferSize[1]-1):(yPos);
  
  return *(this->ZBuffer + yPos*this->ZBufferSize[0] + xPos);
}

//-----------------------------------------------------------------------------
double vtkUnstructuredGridVolumeZSweepMapper::GetMinimumBoundsDepth(
  vtkRenderer *ren,
  vtkVolume   *vol )
{
  double bounds[6];
  vol->GetBounds( bounds );
  
  ren->ComputeAspect();
  double *aspect = ren->GetAspect();

  // Get the view matrix in two steps - there is a one step method in camera
  // but it turns off stereo so we do not want to use that one
  vtkCamera *cam = ren->GetActiveCamera();
  this->PerspectiveTransform->Identity();
  this->PerspectiveTransform->Concatenate(
    cam->GetProjectionTransformMatrix(aspect[0]/aspect[1], 0.0, 1.0 ));
  this->PerspectiveTransform->Concatenate(cam->GetViewTransformMatrix());
  this->PerspectiveMatrix->DeepCopy(this->PerspectiveTransform->GetMatrix());
  
  double minZ = 1.0;
  
  for ( int k = 0; k < 2; k++ )
    {
    for ( int j = 0; j < 2; j++ )
      {
      for ( int i = 0; i < 2; i++ )
        {
        double inPoint[4];
        inPoint[0] = bounds[  i];
        inPoint[1] = bounds[2+j];
        inPoint[2] = bounds[4+k];
        inPoint[3] = 1.0;
        
        double outPoint[4];
        this->PerspectiveMatrix->MultiplyPoint( inPoint, outPoint );
        double testZ = outPoint[2] / outPoint[3];
        minZ = ( testZ < minZ ) ? (testZ) : (minZ);
        }
      }
    }
  
  return minZ;
}
