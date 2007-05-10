/*=========================================================================

  Program:   MetaIO
  Module:    metaSurface.h
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

#ifndef ITKMetaIO_METASURFACE_H
#define ITKMetaIO_METASURFACE_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaSurface (.h and .cxx)
 *
 * Description:
 *    Reads and Writes MetaSurfaceFiles.
 *
 * \author Julien Jomier
 * 
 * \date July 02, 2002
 * 
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class SurfacePnt
{
public:

  SurfacePnt(int dim)
  { 
    m_Dim = dim;
    m_X = new float[m_Dim];
    m_V = new float[m_Dim];
    for(unsigned int i=0;i<m_Dim;i++)
    {
      m_X[i] = 0;
      m_V[i] = 0;
    } 
    //Color is red by default
    m_Color[0]=1.0;
    m_Color[1]=0.0;
    m_Color[2]=0.0;
    m_Color[3]=1.0;
  }
  ~SurfacePnt()
  {
    delete []m_X;
    delete []m_V;
  };
  
  unsigned int m_Dim;
  float* m_X;
  float* m_V;
  float  m_Color[4];
};




class METAIO_EXPORT MetaSurface : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<SurfacePnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaSurface(void);

    MetaSurface(const char *_headerName);   

    MetaSurface(const MetaSurface *_surface); 
    
    MetaSurface(unsigned int dim);

    ~MetaSurface(void);

    void PrintInfo(void) const;

    void CopyInfo(const MetaObject * _object);

    //    NPoints(...)
    //       Required Field
    //       Number of points wich compose the tube
    void  NPoints(int npnt);
    int   NPoints(void) const;

    //    PointDim(...)
    //       Required Field
    //       Definition of points
    void        PointDim(const char* pointDim);
    const char* PointDim(void) const;


    void  Clear(void);

    PointListType & GetPoints(void) {return m_PointList;}
    const PointListType & GetPoints(void) const {return m_PointList;}
    
    MET_ValueEnumType ElementType(void) const;
    void  ElementType(MET_ValueEnumType _elementType);

  ////
  //
  // PROTECTED
  //
  ////
  protected:

    bool  m_ElementByteOrderMSB;

    void  M_Destroy(void);

    void  M_SetupReadFields(void);

    void  M_SetupWriteFields(void);

    bool  M_Read(void);

    bool  M_Write(void);

    int m_NPoints;      // "NPoints = "         0

    char m_PointDim[255]; // "PointDim = "       "x y z r"

    PointListType m_PointList;

    MET_ValueEnumType m_ElementType;

  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
