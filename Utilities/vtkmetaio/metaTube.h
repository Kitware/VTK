/*=========================================================================

  Program:   MetaIO
  Module:    metaTube.h
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

#ifndef ITKMetaIO_METATUBE_H
#define ITKMetaIO_METATUBE_H

#include "metaUtils.h"
#include "metaObject.h"

#include <list>


/*!    MetaTube (.h and .cpp)
 *
 * Description:
 *    Reads and Writes MetaTubeFiles.
 *
 * \author Julien Jomier
 * 
 * \date May 22, 2002
 */

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

class TubePnt
{
public:

  TubePnt(int dim)
  { 
    m_Dim = dim;
    m_X = new float[m_Dim];
    m_T = new float[m_Dim];
    m_V1= new float[m_Dim];
    m_V2= new float[m_Dim];
    for(unsigned int i=0;i<m_Dim;i++)
    {
      m_X[i] = 0;
      m_V1[i]= 0;
      m_V2[i]= 0;
      m_T[i]= 0;
    }
    m_R=0;
    //Color is red by default
    m_Color[0]=1.0;
    m_Color[1]=0.0;
    m_Color[2]=0.0;
    m_Color[3]=1.0;
    m_ID = -1;
  }

  ~TubePnt()
  {
    delete []m_X;
    delete []m_V1;
    delete []m_V2;
    delete []m_T;
  };
  
  unsigned int m_Dim;
  float* m_V1;
  float* m_V2;
  float* m_X;
  float* m_T;
  float m_R;
  float m_Color[4];
  int   m_ID;
};




class METAIO_EXPORT MetaTube : public MetaObject
  {

  /////
  //
  // PUBLIC
  //
  ////
  public:

   typedef METAIO_STL::list<TubePnt*> PointListType;
    ////
    //
    // Constructors & Destructor
    //
    ////
    MetaTube(void);

    MetaTube(const char *_headerName);   

    MetaTube(const MetaTube *_tube); 
    
    MetaTube(unsigned int dim);

    ~MetaTube(void);

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

    //    Root(...)
    //       Optional Field
    //       Set if this tube is a root
    void  Root(bool root);
    bool   Root(void) const;


    //    ParentPoint(...)
    //       Optional Field
    //       Set the point number of the parent tube where the branch occurs
    void  ParentPoint(int parentpoint);
    int   ParentPoint(void) const;

    void  Clear(void);

    PointListType &  GetPoints(void) {return m_PointList;}
    const PointListType &  GetPoints(void) const {return m_PointList;}
    
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

    int m_ParentPoint;  // "ParentPoint = "     -1

    bool m_Root;         // "Root = "            0

    int m_NPoints;      // "NPoints = "         0

    char m_PointDim[255]; // "PointDim = "       "x y z r"

    PointListType m_PointList;
    MET_ValueEnumType m_ElementType;
  };

#if (METAIO_USE_NAMESPACE)
};
#endif

#endif
