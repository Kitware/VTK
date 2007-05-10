/*=========================================================================

  Program:   MetaIO
  Module:    metaContour.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaTypes.h"

#ifndef ITKMetaIO_METACONTOUR_H
#define ITKMetaIO_METACONTOUR_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

/**
 * Description:
 *    Reads and Writes MetaContour Files.
 *
 * \author Julien Jomier
 * 
 * \date March 2006
 * 
 */
class METAIO_EXPORT ContourControlPnt
{
public:

  ContourControlPnt(int dim)
    {
    m_Id = 0;
    m_Dim = dim;
    m_X = new float[m_Dim];
    m_XPicked = new float[m_Dim];
    m_V = new float[m_Dim];
    for(unsigned int i=0;i<m_Dim;i++)
      {
      m_X[i] = 0;
      m_XPicked[i] = 0;
      m_V[i] = 0;
      } 
    //Color is red by default
    m_Color[0]=1.0;
    m_Color[1]=0.0;
    m_Color[2]=0.0;
    m_Color[3]=1.0;
    }
  ~ContourControlPnt()
    {
    delete [] m_X;
    delete [] m_XPicked;
    delete [] m_V;
    };
  
  unsigned int m_Dim;
  unsigned int m_Id;
  float* m_X;
  float* m_XPicked;
  float* m_V;
  float  m_Color[4];
};


class METAIO_EXPORT ContourInterpolatedPnt
{
public:

  ContourInterpolatedPnt(int dim)
    {
    m_Dim = dim;
    m_Id = 0;
    m_X = new float[m_Dim];
    //Color is red by default
    m_Color[0]=1.0;
    m_Color[1]=0.0;
    m_Color[2]=0.0;
    m_Color[3]=1.0;
    }

  ~ContourInterpolatedPnt()
    {
    delete []m_X;
    };
  
  unsigned int m_Dim;
  float* m_X;
  unsigned int  m_Id;
  float  m_Color[4];
};


class METAIO_EXPORT MetaContour : public MetaObject
{

public:

 typedef METAIO_STL::list<ContourControlPnt*> ControlPointListType;
 typedef METAIO_STL::list<ContourInterpolatedPnt*> InterpolatedPointListType;

 MetaContour(void);
 MetaContour(const char *_headerName);   
 MetaContour(const MetaContour *_Contour);    
 MetaContour(unsigned int dim);

 ~MetaContour(void);

  void PrintInfo(void) const;
  void CopyInfo(const MetaObject * _object);

  //    NPoints(...)
  //       Required Field
  //       Number of points wich compose the tube
  int   NControlPoints(void) const;

  //    ControlPointDim(...)
  //       Required Field
  //       Definition of points
  void        ControlPointDim(const char* pointDim);
  const char* ControlPointDim(void) const;

  MET_InterpolationEnumType  Interpolation(void) const;
  void Interpolation(MET_InterpolationEnumType _interpolation);

  int   NInterpolatedPoints(void) const;

  void        InterpolatedPointDim(const char* pointDim);
  const char* InterpolatedPointDim(void) const;

  void Closed(bool close);
  bool Closed();

  void     AttachedToSlice(long int slice);
  long int AttachedToSlice();

  void DisplayOrientation(int display);
  int  DisplayOrientation();

  void  Clear(void);

  ControlPointListType & GetControlPoints(void) 
    {return m_ControlPointsList;}
  const ControlPointListType & GetControlPoints(void) const 
    {return m_ControlPointsList;}
   
  InterpolatedPointListType & GetInterpolatedPoints(void) 
    {return m_InterpolatedPointsList;}
  const InterpolatedPointListType & GetInterpolatedPoints(void) const 
    {return m_InterpolatedPointsList;}

protected:

  bool  m_ElementByteOrderMSB;
  void  M_Destroy(void);
  void  M_SetupReadFields(void);
  void  M_SetupWriteFields(void);
  bool  M_Read(void);
  bool  M_Write(void);

  int m_NControlPoints;
  int m_NInterpolatedPoints;
  char m_ControlPointDim[255];
  char m_InterpolatedPointDim[255];
  bool m_Closed;
  MET_InterpolationEnumType m_InterpolationType;
  ControlPointListType      m_ControlPointsList;
  InterpolatedPointListType m_InterpolatedPointsList;

  int       m_DisplayOrientation;
  long      m_AttachedToSlice;

};

#if (METAIO_USE_NAMESPACE)
};
#endif


#endif
