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

#include "vtkActor.h"
#include "vtkCubeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"


@interface VTKViewController ()

// Views
@property (strong, nonatomic) IBOutlet VTKView *vtkView;
@property (strong, nonatomic) IBOutlet UIVisualEffectView *headerContainer;

// VTK
@property (nonatomic) vtkSmartPointer<vtkRenderer> renderer;

// VTK handlers
@property (nonatomic) VTKGestureHandler *vtkGestureHandler;

@end

@implementation VTKViewController

// MARK: UIViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // UI Gestures
    [self setupGestures];

    // Rendering + data
    // TODO: move to new VTK handlers
    [self setupPipeline];

    // VTK Gestures
    self.vtkGestureHandler = [[VTKGestureHandler alloc] initWithVtkView:self.vtkView];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

// MARK: Data

- (void)setupPipeline {
    self.renderer = vtkSmartPointer<vtkRenderer>::New();

    self.vtkView.renderWindow->AddRenderer(self.renderer);

    auto cubeSource = vtkSmartPointer<vtkCubeSource>::New();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(cubeSource->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    self.renderer->SetBackground(0.4, 0.4, 0.4);
    self.renderer->SetBackground2(0.2, 0.2, 0.2);
    self.renderer->GradientBackgroundOn();
    self.renderer->AddActor(actor);
    self.renderer->ResetCamera();
}

// MARK: Gestures

- (void)setupGestures {
    // Add the double tap gesture recognizer
    UITapGestureRecognizer *doubleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                                                          action:@selector(onDoubleTap:)];
    [doubleTapRecognizer setNumberOfTapsRequired:2];
    [self.view addGestureRecognizer:doubleTapRecognizer];
    doubleTapRecognizer.delegate = self;
}

- (void)onDoubleTap:(UITapGestureRecognizer *)sender {
    [UIView animateWithDuration:0.2 animations:^{
        BOOL show = self.headerContainer.alpha < 1.0;
        self.headerContainer.alpha = show ? 1.0 : 0.0;
    }];
}

@end
