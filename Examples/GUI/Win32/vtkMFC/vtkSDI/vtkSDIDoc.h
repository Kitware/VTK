// vtkSDIDoc.h : interface of the CvtkSDIDoc class
//


#pragma once

class CvtkSDIDoc : public CDocument
{
protected: // create from serialization only
  CvtkSDIDoc();
  DECLARE_DYNCREATE(CvtkSDIDoc)

// Attributes
private:
  vtkDataSetReader *pvtkDataSetReader;

// Operations
public:

// Overrides
  public:
  virtual BOOL OnNewDocument();
  virtual void Serialize(CArchive& ar);

// Implementation
public:
  virtual ~CvtkSDIDoc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

private:
  void ExecutePipeline();
  void RemoveActors();

  vtkDataSetMapper  *pvtkDataSetMapper;
  vtkActor      *pvtkActor;

  vtkActor2D      *pvtkActor2D;
  vtkTextMapper    *pvtkTextMapper;

// Generated message map functions
protected:
  DECLARE_MESSAGE_MAP()
public:
  virtual void OnCloseDocument();
  virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
};


