// vtkMDIDoc.h : interface of the CvtkMDIDoc class
//


#pragma once

class CvtkMDIDoc : public CDocument
{
protected: // create from serialization only
  CvtkMDIDoc();
  DECLARE_DYNCREATE(CvtkMDIDoc)

// Attributes
public:
  vtkDataSetReader *pvtkDataSetReader;

// Operations
public:

// Overrides
  public:
  virtual BOOL OnNewDocument();
  virtual void Serialize(CArchive& ar);

// Implementation
public:
  virtual ~CvtkMDIDoc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
  DECLARE_MESSAGE_MAP()
public:
  virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
  virtual void OnCloseDocument();
};


