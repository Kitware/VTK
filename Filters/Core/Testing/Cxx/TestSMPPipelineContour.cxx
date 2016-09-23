/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNew.h"
#include "vtkRTAnalyticSource.h"
#include "vtkPolyData.h"
#include "vtkTimerLog.h"
#include "vtkSMPTools.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkExtentTranslator.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkImageData.h"
#include "vtkThreadedCompositeDataPipeline.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkSmartPointer.h"

const int EXTENT = 100;
static int WholeExtent[] = {-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT};
const int NUMBER_OF_PIECES = 50;
static vtkImageData* Pieces[NUMBER_OF_PIECES];

class vtkCreateImageData
{
  vtkSMPThreadLocalObject<vtkRTAnalyticSource> ImageSources;
  vtkNew<vtkExtentTranslator> Translator;

public:
  void Initialize()
  {
    vtkRTAnalyticSource*& source = this->ImageSources.Local();
    source->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkRTAnalyticSource*& source = this->ImageSources.Local();

    for (vtkIdType i=begin; i<end; i++)
    {
      int extent[6];
      this->Translator->PieceToExtentThreadSafe(i,
                                                NUMBER_OF_PIECES,
                                                0,
                                                WholeExtent,
                                                extent,
                                                vtkExtentTranslator::BLOCK_MODE,
                                                0);
      source->UpdateExtent(extent);
      vtkImageData* piece = vtkImageData::New();
      piece->ShallowCopy(source->GetOutput());
      Pieces[i] = piece;
    }
  }

  void Reduce()
  {
  }
};

int TestSMPPipelineContour(int, char *[])
{
  vtkSMPTools::Initialize(2);

  vtkCreateImageData cid;
  vtkNew<vtkTimerLog> tl;

  tl->StartTimer();
  vtkSMPTools::For(0, NUMBER_OF_PIECES, cid);
  tl->StopTimer();

  cout << "Creation time: " << tl->GetElapsedTime() << endl;

  vtkNew<vtkMultiBlockDataSet> mbds;
  for (int i=0; i<NUMBER_OF_PIECES; i++)
  {
    mbds->SetBlock(i, Pieces[i]);
    Pieces[i]->Delete();
  }

  vtkNew<vtkThreadedCompositeDataPipeline> executive;

  vtkNew<vtkSynchronizedTemplates3D> cf;
  cf->SetExecutive(executive.GetPointer());
  cf->SetInputData(mbds.GetPointer());
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, 200);
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  cout << "Execution time: " << tl->GetElapsedTime() << endl;

  vtkIdType numCells = 0;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(static_cast<vtkCompositeDataSet*>(cf->GetOutputDataObject(0))->NewIterator());
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
  {
    vtkPolyData* piece = static_cast<vtkPolyData*>(iter->GetCurrentDataObject());
    numCells += piece->GetNumberOfCells();
    iter->GoToNextItem();
  }

  cout << "Total num. cells: " << numCells << endl;

  vtkNew<vtkRTAnalyticSource> rt;
  rt->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
  rt->Update();

  vtkNew<vtkSynchronizedTemplates3D> st;
  st->SetInputData(rt->GetOutput());
  st->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  st->SetValue(0, 200);

  tl->StartTimer();
  st->Update();
  tl->StopTimer();

  cout << "Serial execution time: " << tl->GetElapsedTime() << endl;

  cout << "Serial num. cells: " << st->GetOutput()->GetNumberOfCells() << endl;

  if (st->GetOutput()->GetNumberOfCells() != numCells)
  {
    cout << "Number of cells did not match." << endl;
    return EXIT_FAILURE;
  }


#if 0
  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetInputData(cf->GetOutputDataObject(0));
  writer->SetFileName("contour.vtm");
  writer->SetDataModeToAscii();
  writer->Write();
#endif

  return EXIT_SUCCESS;
}
