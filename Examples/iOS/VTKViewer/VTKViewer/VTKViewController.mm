//
//  VTKViewController.m
//  VTKViewer
//
//  Created by Max Smolens on 6/19/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "VTKViewController.h"

#import "VTKGestureHandler.h"
#import "VTKView.h"

#include "vtkBoundingBox.h"
#include "vtkIOSRenderWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkPolyDataMapper.h"

@interface VTKViewController ()

// Views
@property (strong, nonatomic) IBOutlet VTKView *vtkView;

// VTK
@property (nonatomic) vtkSmartPointer<vtkRenderer> renderer;
@property (assign) vtkBoundingBox boundingBox;

@property (nonatomic) VTKGestureHandler *vtkGestureHandler;

@end

@implementation VTKViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    [self setupPipeline];

    self.vtkGestureHandler = [[VTKGestureHandler alloc] initWithView:self.view vtkView:self.vtkView];
}

- (void)setupPipeline {
    self.renderer = vtkSmartPointer<vtkRenderer>::New();

    self.vtkView.renderWindow->AddRenderer(self.renderer);

    auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cubeSource->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    self.renderer->SetBackground(0.2, 0.2, 0.2);
    self.renderer->SetBackground2(0.0, 0.0, 0.0);
    self.renderer->GradientBackgroundOn();
    self.renderer->AddActor(actor);
    self.renderer->ResetCamera();

    self.boundingBox.AddBounds(actor->GetBounds());
}
- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
