// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "ArrayGroupModel.h"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCellGrid.h>
#include <vtkCellGridCellCenters.h>
#include <vtkCellGridCellSource.h>
#include <vtkCellGridComputeSides.h>
#include <vtkCellGridMapper.h>
#include <vtkCellGridToUnstructuredGrid.h>
#include <vtkCellMetadata.h>
#include <vtkDoubleArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QTableView>
#include <QVBoxLayout>

#include <cmath>
#include <cstdlib>
#include <random>

using namespace vtk::literals;

namespace
{

void updateGlyphSources(vtkCellGridCellSource* cellSource, QComboBox* attributeSelector)
{
  cellSource->Update();
  int idx = -1;
  auto currentSource = attributeSelector->currentText();
  attributeSelector->clear();
  attributeSelector->addItem("–none–");
  auto* cellGrid = cellSource->GetOutput();
  for (const auto& attribute : cellGrid->GetCellAttributeList())
  {
    if (attribute == cellGrid->GetShapeAttribute())
    {
      continue;
    }

    if (attribute->GetName().IsValid() && attribute->GetName().HasData())
    {
      auto label = QString::fromStdString(attribute->GetName().Data());
      if (label == currentSource)
      {
        idx = attributeSelector->count();
      }
      attributeSelector->addItem(label);
    }
  }
  if (idx >= 0)
  {
    attributeSelector->setCurrentText(currentSource);
  }
  else
  {
    attributeSelector->setCurrentIndex(0);
  }
}

void updateArrayGroups(ArrayGroupModel& model, vtkCellGridCellSource* cellSource,
  QComboBox* groupSelector, bool signalChange = true)
{
  cellSource->Update();
  int idx = -1;
  groupSelector->clear();
  auto* cellGrid = cellSource->GetOutput();
  auto curGroupName = model.groupName();
  for (const auto& entry : cellGrid->GetArrayGroups())
  {
    vtkStringToken groupName(static_cast<vtkStringToken::Hash>(entry.first));
    if (groupName.IsValid() && groupName.HasData())
    {
      if (groupName == curGroupName)
      {
        idx = groupSelector->count();
      }
      groupSelector->addItem(QString::fromStdString(groupName.Data()));
    }
  }
  if (idx < 0 && groupSelector->count() > 0)
  {
    idx = 0;
    curGroupName = groupSelector->currentText().toStdString();
  }
  model.setGroupName(curGroupName, signalChange);
}

} // anonymous namespace

int main(int argc, char* argv[])
{
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QApplication app(argc, argv);

  // main window
  QMainWindow mainWindow;
  mainWindow.resize(1200, 900);

  // control area
  QDockWidget controlDock;
  mainWindow.addDockWidget(Qt::BottomDockWidgetArea, &controlDock);

  QLabel controlDockTitle("Editor");
  controlDockTitle.setMargin(20);
  controlDock.setTitleBarWidget(&controlDockTitle);

  QPointer<QVBoxLayout> dockLayout = new QVBoxLayout();
  QWidget layoutContainer;
  layoutContainer.setLayout(dockLayout);
  controlDock.setWidget(&layoutContainer);

  QLabel cellTypeLabel("Cell Type");
  QComboBox cellType;
  QLabel arrayGroupLabel("Array Group");
  QComboBox arrayGroupSelector;
  QLabel bdyLabel("Boundaries");
  QCheckBox bdyBtn;
  QLabel glyLabel("Glyphs");
  QComboBox glySelector;
  QHBoxLayout hbct;
  QHBoxLayout hbag;
  QHBoxLayout hbbd;
  hbct.addWidget(&cellTypeLabel);
  hbct.addWidget(&cellType);
  hbag.addWidget(&arrayGroupLabel);
  hbag.addWidget(&arrayGroupSelector);
  hbbd.addWidget(&bdyLabel);
  hbbd.addWidget(&bdyBtn);
  hbbd.addWidget(&glyLabel);
  hbbd.addWidget(&glySelector);
  dockLayout->addLayout(&hbct);
  dockLayout->addLayout(&hbag);
  dockLayout->addLayout(&hbbd);
  // dockLayout->addWidget(&cellType);
  // dockLayout->addWidget(&arrayGroupSelector);

  QTableView tableView;
  dockLayout->addWidget(&tableView);

  // render area
  QPointer<QVTKOpenGLNativeWidget> vtkRenderWidget = new QVTKOpenGLNativeWidget();
  mainWindow.setCentralWidget(vtkRenderWidget);

  // VTK part
  vtkNew<vtkGenericOpenGLRenderWindow> window;
  vtkRenderWidget->setRenderWindow(window.Get());

  vtkNew<vtkCellGridCellSource> cellSource;
  vtkStringToken initialCellType = "vtkDGHex";
  cellSource->SetCellType(initialCellType.Data().c_str());
  cellSource->Update();
  int idx = 0;
  for (const auto& registeredCellType : vtkCellMetadata::CellTypes())
  {
    if (registeredCellType == initialCellType)
    {
      idx = cellType.count();
    }
    cellType.addItem(QString::fromStdString(registeredCellType.Data()));
  }
  cellType.setCurrentIndex(idx);

  ArrayGroupModel model(cellSource->GetOutput(), "points", nullptr);
  tableView.setModel(&model);

  vtkNew<vtkCellGridComputeSides> cellSides;
  cellSides->SetInputDataObject(0, cellSource->GetOutput());
  cellSides->PreserveRenderableInputsOn();
  cellSides->OmitSidesForRenderableInputsOff();

  vtkNew<vtkCellGridMapper> mapper;
  vtkNew<vtkActor> actor;
  mapper->SetInputConnection(cellSides->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetEdgeVisibility(true);
  actor->GetProperty()->SetRepresentationToSurface();

  vtkNew<vtkCellGridComputeSides> cellEdges;
  cellEdges->SetInputDataObject(0, cellSource->GetOutput());
  cellEdges->SetOutputDimensionControl(vtkCellGridSidesQuery::SideFlags::EdgesOfInputs);
  cellEdges->PreserveRenderableInputsOff();
  cellEdges->OmitSidesForRenderableInputsOff();

  vtkNew<vtkCellGridMapper> bdyMapper;
  vtkNew<vtkActor> bdyActor;
  bdyMapper->SetInputConnection(cellEdges->GetOutputPort());
  bdyActor->SetMapper(bdyMapper);
  bdyActor->GetProperty()->SetEdgeVisibility(true);
  bdyActor->GetProperty()->SetRepresentationToSurface();
  bdyActor->SetVisibility(false); // Turn off initially.

  vtkNew<vtkCellGridComputeSides> centerSides;
  vtkNew<vtkCellGridCellCenters> sideCenters;
  vtkNew<vtkCellGridToUnstructuredGrid> ugridCvt;
  vtkNew<vtkGlyph3DMapper> glyMapperCC;
  vtkNew<vtkGlyph3DMapper> glyMapperSC;
  vtkNew<vtkArrowSource> arrow;
  vtkNew<vtkActor> glyActorCC;
  vtkNew<vtkActor> glyActorSC;
  centerSides->SetInputDataObject(0, cellSource->GetOutput());
  centerSides->SetOutputDimensionControl(vtkCellGridSidesQuery::SideFlags::AllSides);
  centerSides->OmitSidesForRenderableInputsOff();
  sideCenters->SetInputConnection(centerSides->GetOutputPort());
  ugridCvt->SetInputConnection(sideCenters->GetOutputPort());
  // Put arrows at side centers
  glyMapperSC->SetInputConnection(ugridCvt->GetOutputPort());
  glyMapperSC->OrientOn();
  glyMapperSC->SetOrientationArray("curl");
  glyMapperSC->SetSourceConnection(arrow->GetOutputPort());
  glyMapperSC->ScalingOn();
  glyMapperSC->SetScaleMode(vtkGlyph3DMapper::SCALE_BY_MAGNITUDE);
  glyMapperSC->SetScaleArray("curl");
  glyMapperSC->SetScaleFactor(1.);
  glyActorSC->SetMapper(glyMapperSC);
  glyActorSC->SetVisibility(false);

  vtkNew<vtkCellGridCellCenters> cellCenters;
  vtkNew<vtkCellGridToUnstructuredGrid> ugridCvtCC;
  cellCenters->SetInputConnection(cellSource->GetOutputPort());
  ugridCvtCC->SetInputConnection(cellCenters->GetOutputPort());
  // Put arrows at cell center
  glyMapperCC->SetInputConnection(ugridCvtCC->GetOutputPort());
  glyMapperCC->OrientOn();
  glyMapperCC->SetOrientationArray("curl");
  glyMapperCC->SetSourceConnection(arrow->GetOutputPort());
  glyMapperCC->ScalingOn();
  glyMapperCC->SetScaleMode(vtkGlyph3DMapper::SCALE_BY_MAGNITUDE);
  glyMapperCC->SetScaleArray("curl");
  glyMapperCC->SetScaleFactor(1.);
  glyActorCC->SetMapper(glyMapperCC);
  glyActorCC->SetVisibility(false);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(bdyActor);
  renderer->AddActor(glyActorCC);
  renderer->AddActor(glyActorSC);

  renderer->ResetCamera();
  window->AddRenderer(renderer);

  // Re-render upon each user edit of a cell-grid data-array:
  QObject::connect(&model, &ArrayGroupModel::dataChanged,
    [&vtkRenderWidget, &cellEdges, &centerSides, &cellCenters]()
    {
      cellCenters->Modified();
      cellEdges->Modified();
      centerSides->Modified();
      vtkRenderWidget->renderWindow()->Render();
    });

  QObject::connect(&bdyBtn, &QCheckBox::toggled,
    [&](bool enabled)
    {
      bdyActor->SetVisibility(enabled);
      actor->SetVisibility(!enabled);
      vtkRenderWidget->renderWindow()->Render();
    });

  QObject::connect(&glySelector, &QComboBox::currentTextChanged,
    [&](const QString& text)
    {
      if (text == QString("–none–"))
      {
        glyActorCC->SetVisibility(false);
        glyActorSC->SetVisibility(false);
      }
      else
      {
        glyActorCC->SetVisibility(true);
        glyActorSC->SetVisibility(true);
        glyMapperCC->SetOrientationArray(text.toStdString().c_str());
        glyMapperCC->SetScaleArray(text.toStdString().c_str());
        glyMapperSC->SetOrientationArray(text.toStdString().c_str());
        glyMapperSC->SetScaleArray(text.toStdString().c_str());
      }
      vtkRenderWidget->renderWindow()->Render();
    });

  // connect the buttons
  QObject::connect(&cellType, &QComboBox::currentTextChanged,
    [&](const QString& text)
    {
      cellSource->SetCellType(text.toStdString().c_str());
      updateGlyphSources(cellSource, &glySelector);
      updateArrayGroups(model, cellSource, &arrayGroupSelector, true);
      vtkRenderWidget->renderWindow()->Render();
    });
  QObject::connect(&arrayGroupSelector, &QComboBox::currentTextChanged,
    [&](const QString& text) { model.setGroupName(text.toStdString(), true); });
  updateGlyphSources(cellSource, &glySelector);
  updateArrayGroups(model, cellSource, &arrayGroupSelector, false);

  QShortcut exitKey(QKeySequence("Ctrl+Q"), &mainWindow);
  QObject::connect(&exitKey, &QShortcut::activated, [&]() { app.exit(); });
  mainWindow.show();

  return app.exec();
}
