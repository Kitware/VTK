// vtkDLGDlg.h : header file
//

#pragma once

#include "vtkMFCWindow.h"

// CvtkDLGDlg dialog
class CvtkDLGDlg : public CDialog
{
public:
  CvtkDLGDlg(CWnd* pParent = NULL);  // standard constructor

  enum { IDD = IDD_VTKDLG_DIALOG };

  afx_msg void OnDestroy();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnBtnLoadFile();
  afx_msg void OnBtnResetScene();

private:
  void ExecutePipeline();

  vtkMFCWindow          *pvtkMFCWindow;

  vtkDataSetReader        *pvtkDataSetReader;
  vtkRenderer            *pvtkRenderer;
  vtkDataSetMapper        *pvtkDataSetMapper;
  vtkActor            *pvtkActor;
  vtkActor2D            *pvtkActor2D;
  vtkTextMapper          *pvtkTextMapper;

  POINT  ptBorder;
  HICON  m_hIcon;

protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support
  virtual BOOL OnInitDialog();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();

  DECLARE_MESSAGE_MAP()
};
