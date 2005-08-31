/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCDocument.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if !defined(AFX_VTKMFCDOCUMENT_H__5D36A982_8081_11D2_985E_00A0CC243C06__INCLUDED_)
#define AFX_VTKMFCDOCUMENT_H__5D36A982_8081_11D2_985E_00A0CC243C06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// vtkMFCDocument.h : header file
//
#include "vtkActorCollection.h"
#include "vtkVolumeCollection.h"
#include "vtkActor2DCollection.h"

/////////////////////////////////////////////////////////////////////////////
// vtkMFCDocument document

class vtkMFCDocument : public CDocument
{
protected:
        vtkMFCDocument();           // protected constructor used by dynamic creation
        DECLARE_DYNCREATE(vtkMFCDocument)

        vtkPropCollection *Props;

// Attributes
public:

// Operations
public:
  vtkPropCollection *GetViewProps() {return this->Props;};

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(vtkMFCDocument)
        public:
        virtual void Serialize(CArchive& ar);   // overridden for document i/o
        protected:
        virtual BOOL OnNewDocument();
        //}}AFX_VIRTUAL

// Implementation
public:
        virtual ~vtkMFCDocument();
#ifdef _DEBUG
        virtual void AssertValid() const;
        virtual void Dump(CDumpContext& dc) const;
#endif

        // Generated message map functions
protected:
        //{{AFX_MSG(vtkMFCDocument)
                // NOTE - the ClassWizard will add and remove member functions here.
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMFCDOCUMENT_H__5D36A982_8081_11D2_985E_00A0CC243C06__INCLUDED_)
