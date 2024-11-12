// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkGlyph3DMapper.h"
#include "vtkHardwarePicker.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollectionSource.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include "vtkTextSource.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkXMLImageDataWriter.h"

#include <cstdlib>

// This unit test exercises using a hardware selector
// to pick geometry rendered by `vtkPolyDataMapper`,
// `vtkCompositePolyDataMapper`, and `vtkGlyph3DMapper`.
//
// With the `--save-attachments` argument, you can dump
// all the attachments of the render window to .vti files.
// Look for `color.vti`, `colorrgba.vti`, `depth.vti` and `ids.vti`
// images in the directory from which the executable is run.
// It is useful to open them in ParaView and inspect the values
// per pixel.

int TestHardwareSelector(int argc, char* argv[])
{
  bool saveAttachments = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i++], "--save-attachments"))
    {
      saveAttachments = true;
      break;
    }
  }

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  // Adds a cone using a vtkActor
  vtkNew<vtkConeSource> cone;
  cone->SetCenter(-2, 2, -2);
  vtkNew<vtkActor> coneActor;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  coneActor->SetMapper(coneMapper);
  coneActor->RotateZ(90);
  renderer->AddActor(coneActor);

  // Adds a sphere using a vtkActor
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(25, 2, 2);
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  renderer->AddActor(sphereActor);

  // Adds a composite dataset using a vtkActor
  vtkNew<vtkPartitionedDataSetCollectionSource> shapes;
  shapes->SetNumberOfShapes(12);
  vtkNew<vtkCompositePolyDataMapper> shapesMapper;
  shapesMapper->SetInputConnection(shapes->GetOutputPort());
  vtkNew<vtkActor> shapesActor;
  shapesActor->SetMapper(shapesMapper);
  renderer->AddActor(shapesActor);

  // Adds a glyph using a vtkActor
  vtkNew<vtkPlaneSource> grid;
  grid->SetCenter(15, 4, -2);
  grid->SetXResolution(2);
  grid->SetYResolution(2);
  vtkNew<vtkCylinderSource> cylinder;
  vtkNew<vtkGlyph3DMapper> glyphMapper;
  glyphMapper->SetScaleFactor(0.3);
  glyphMapper->SetSourceConnection(cylinder->GetOutputPort());
  glyphMapper->SetInputConnection(grid->GetOutputPort());
  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glyphMapper);
  renderer->AddActor(glyphActor);

  // Adds text using a vtkActor2D
  vtkNew<vtkTextSource> text;
  text->SetText("WebGPU Hardware Selector");
  vtkNew<vtkPolyDataMapper2D> textMapper;
  // vtkTextSource generates a vtkPolyData whose points are in display coordinate system i.e, (0...w
  // x 0...h) where w is the width of the render window and h is the height of the render window.
  // So use a vtkCoordinate instance that maps the input from display to viewport.
  vtkNew<vtkCoordinate> displayToViewport;
  displayToViewport->SetCoordinateSystemToDisplay();
  textMapper->SetTransformCoordinate(displayToViewport);
  textMapper->SetInputConnection(text->GetOutputPort());
  vtkNew<vtkActor2D> textActor;
  textActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  textActor->GetPositionCoordinate()->SetValue(0.4, 0.1);
  textActor->SetMapper(textMapper);
  renderer->AddViewProp(textActor);

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->SetSize(1280, 720);
  renWin->Render();

  {
    vtkNew<vtkHardwareSelector> selector;
    selector->SetCaptureZValues(true);
    selector->SetRenderer(renderer);
    {
      selector->SetArea(336.0, 265.0, 342.0, 272.0);
      auto selection = vtk::TakeSmartPointer(selector->Select());
      if (selection->GetNumberOfNodes() != 1)
      {
        std::cerr << "Expected 1 nodes, got " << selection->GetNumberOfNodes() << "nodes\n";
        return EXIT_FAILURE;
      }
      auto* node = selection->GetNode(0);
      if (node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) != 0)
      {
        std::cerr << "Expected propId = 0, got "
                  << node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) << "\n";
        return EXIT_FAILURE;
      }
      else
      {
        std::ostringstream oss;
        node->GetSelectionList()->PrintValues(oss);
        if (oss.str() != "5 6 ")
        {
          std::cerr << "Expected selected Ids = " << oss.str() << " got 5 6 \n";
        }
      }
    }
    {
      selector->SetArea(872.0, 346.0, 880.0, 352.0);
      auto selection = vtk::TakeSmartPointer(selector->Select());
      if (selection->GetNumberOfNodes() != 1)
      {
        std::cerr << "Expected 1 nodes, got " << selection->GetNumberOfNodes() << "nodes\n";
        return EXIT_FAILURE;
      }
      auto* node = selection->GetNode(0);
      if (node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) != 1)
      {
        std::cerr << "Expected propId = 1, got "
                  << node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) << "\n";
        return EXIT_FAILURE;
      }
      else
      {
        std::ostringstream oss;
        node->GetSelectionList()->PrintValues(oss);
        if (oss.str() != "4 5 6 7 67 76 77 86 87 88 89 ")
        {
          std::cerr << "Expected selected Ids = " << oss.str()
                    << " got 4 5 6 7 67 76 77 86 87 88 89 \n";
          return EXIT_FAILURE;
        }
      }
    }
    {
      selector->SetArea(366.0, 245.0, 1016.0, 316.0);
      auto selection = vtk::TakeSmartPointer(selector->Select());
      if (selection->GetNumberOfNodes() != 12)
      {
        std::cerr << "Expected 12 nodes, got " << selection->GetNumberOfNodes() << "nodes\n";
        return EXIT_FAILURE;
      }
      auto* node = selection->GetNode(0);
      if (node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) != 2)
      {
        std::cerr << "Expected propId = 2, got "
                  << node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) << "\n";
        return EXIT_FAILURE;
      }
      else
      {
        std::ostringstream oss;
        node->GetSelectionList()->PrintValues(oss);
        if (oss.str() !=
          "1160 1163 1170 1258 1260 1261 1263 1355 1360 1364 1452 1457 2262 2355 2356 2358 2363 "
          "2364 2366 2368 2370 2373 2375 2378 2380 2381 2382 2383 2386 2388 2389 2392 2393 2396 "
          "2400 2404 2405 2439 2447 2448 2459 2461 2462 2463 2464 2465 2466 2467 2469 2471 2472 "
          "2473 2474 2476 2479 2480 2481 2482 2485 2486 2487 2488 2491 2492 2495 2496 2499 2500 "
          "2503 2504 2505 2528 2535 2537 2542 2544 2545 2552 2555 2557 2558 2559 2561 2563 2564 "
          "2565 2566 2568 2569 2571 2573 2576 2648 2649 2654 2656 2659 2661 2750 2754 2755 2851 "
          "2947 2949 3044 3048 3147 3239 3243 3244 3247 3250 3341 3346 3348 3350 3351 3353 3356 "
          "3358 3438 3441 3443 3444 3445 3448 3449 3451 3453 3454 3455 3457 3458 3459 3460 3461 "
          "3463 3464 3466 3468 3469 3470 3472 3473 3475 3477 3480 3481 3484 3510 3519 3522 3526 "
          "3536 3539 3542 3545 3548 3549 3551 3552 3554 3555 3558 3559 3560 3561 3564 3565 3568 "
          "3569 3572 3573 3576 3577 3580 3581 3582 3611 3612 3614 3621 3622 3630 3634 3637 3641 "
          "3643 3644 3647 3650 3653 3654 3656 3657 3659 3660 3663 3664 3667 3668 3669 3671 3672 "
          "3675 3676 3677 3679 3681 3705 3711 3712 3714 3716 3718 3719 3720 3722 3723 3729 3734 "
          "3737 3738 3741 3743 3744 3745 3747 3748 3749 3751 3752 3753 3755 3756 3757 3759 3760 "
          "3761 3762 3763 3764 3765 3767 3768 3769 3770 3772 3773 3774 3775 3777 3779 3805 3806 "
          "3812 3816 3818 3819 3829 3830 3833 3835 3836 3837 3839 3840 3841 3843 3844 3845 3846 "
          "3847 3848 3849 3850 3851 3852 3853 3854 3855 3857 3859 3860 3861 3862 3863 3864 3865 "
          "3866 3867 3868 3870 3871 3872 3873 3874 3875 3876 3907 3909 3910 3915 3916 3919 3923 "
          "3926 3928 3931 3933 3935 3936 3937 3939 3941 3943 3944 3945 3947 3949 3951 3952 3953 "
          "3954 3955 3956 3957 3958 3961 3963 3964 3965 3966 3967 3969 3970 3972 3973 3974 3975 "
          "4002 4006 4009 4011 4013 4014 4016 4017 4029 4030 4032 4035 4037 4038 4039 4041 4042 "
          "4043 4044 4045 4046 4047 4048 4049 4050 4051 4052 4053 4055 4056 4058 4059 4060 4061 "
          "4062 4063 4064 4066 4068 4070 4071 4073 4102 4106 4109 4111 4112 4114 4115 4118 4121 "
          "4123 4125 4126 4127 4129 4131 4133 4134 4135 4136 4137 4138 4139 4140 4141 4143 4144 "
          "4145 4146 4147 4148 4149 4150 4152 4153 4155 4156 4157 4158 4159 4160 4161 4162 4163 "
          "4164 4165 4166 4168 4169 4172 4201 4203 4204 4205 4206 4211 4212 4215 4217 4221 4222 "
          "4223 4225 4227 4229 4230 4232 4234 4235 4236 4237 4239 4242 4245 4247 4248 4251 4253 "
          "4254 4257 4258 4261 4262 4265 4266 4267 4296 4298 4300 4302 4304 4306 4308 4309 4310 "
          "4311 4317 4319 4322 4323 4325 4326 4328 4329 4331 4332 4333 4334 4335 4336 4337 4339 "
          "4340 4341 4342 4343 4345 4346 4347 4349 4350 4351 4352 4353 4354 4355 4356 4358 4359 "
          "4360 4361 4363 4364 4366 4367 4394 4398 4399 4403 4406 4409 4412 4413 4416 4418 4421 "
          "4423 4425 4426 4428 4429 4430 4431 4432 4433 4434 4435 4436 4437 4438 4440 4441 4442 "
          "4443 4444 4445 4446 4447 4448 4449 4450 4451 4452 4453 4454 4455 4457 4458 4459 4460 "
          "4461 4463 4464 4465 4490 4496 4498 4500 4503 4504 4506 4507 4512 4515 4517 4518 4521 "
          "4522 4523 4525 4526 4527 4528 4529 4530 4531 4532 4534 4535 4536 4537 4538 4540 4541 "
          "4542 4544 4545 4546 4548 4549 4551 4553 4554 4555 4556 4559 4560 4563 4564 4588 4592 "
          "4595 4597 4599 4600 4602 4603 4604 4607 4608 4612 4615 4616 4617 4619 4620 4622 4623 "
          "4624 4625 4627 4628 4630 4631 4632 4633 4634 4635 4636 4637 4638 4639 4640 4642 4643 "
          "4645 4646 4648 4649 4651 4652 4653 4654 4655 4656 4657 4658 4661 4690 4692 4694 4696 "
          "4697 4699 4701 4702 4703 4708 4711 4713 4715 4716 4717 4718 4719 4720 4721 4722 4723 "
          "4725 4726 4727 4728 4729 4730 4731 4733 4734 4735 4736 4737 4738 4739 4740 4742 4743 "
          "4744 4745 4746 4747 4748 4749 4751 4752 4753 4755 4756 4757 4758 4759 4761 4783 4787 "
          "4789 4790 4795 4796 4797 4798 ")
        {
          std::cerr
            << "Expected selected Ids = " << oss.str()
            << " got 1160 1163 1170 1258 1260 1261 1263 1355 1360 1364 1452 1457 2262 2355 2356 "
               "2358 2363 2364 2366 2368 2370 2373 2375 2378 2380 2381 2382 2383 2386 2388 2389 "
               "2392 2393 2396 2400 2404 2405 2439 2447 2448 2459 2461 2462 2463 2464 2465 2466 "
               "2467 2469 2471 2472 2473 2474 2476 2479 2480 2481 2482 2485 2486 2487 2488 2491 "
               "2492 2495 2496 2499 2500 2503 2504 2505 2528 2535 2537 2542 2544 2545 2552 2555 "
               "2557 2558 2559 2561 2563 2564 2565 2566 2568 2569 2571 2573 2576 2648 2649 2654 "
               "2656 2659 2661 2750 2754 2755 2851 2947 2949 3044 3048 3147 3239 3243 3244 3247 "
               "3250 3341 3346 3348 3350 3351 3353 3356 3358 3438 3441 3443 3444 3445 3448 3449 "
               "3451 3453 3454 3455 3457 3458 3459 3460 3461 3463 3464 3466 3468 3469 3470 3472 "
               "3473 3475 3477 3480 3481 3484 3510 3519 3522 3526 3536 3539 3542 3545 3548 3549 "
               "3551 3552 3554 3555 3558 3559 3560 3561 3564 3565 3568 3569 3572 3573 3576 3577 "
               "3580 3581 3582 3611 3612 3614 3621 3622 3630 3634 3637 3641 3643 3644 3647 3650 "
               "3653 3654 3656 3657 3659 3660 3663 3664 3667 3668 3669 3671 3672 3675 3676 3677 "
               "3679 3681 3705 3711 3712 3714 3716 3718 3719 3720 3722 3723 3729 3734 3737 3738 "
               "3741 3743 3744 3745 3747 3748 3749 3751 3752 3753 3755 3756 3757 3759 3760 3761 "
               "3762 3763 3764 3765 3767 3768 3769 3770 3772 3773 3774 3775 3777 3779 3805 3806 "
               "3812 3816 3818 3819 3829 3830 3833 3835 3836 3837 3839 3840 3841 3843 3844 3845 "
               "3846 3847 3848 3849 3850 3851 3852 3853 3854 3855 3857 3859 3860 3861 3862 3863 "
               "3864 3865 3866 3867 3868 3870 3871 3872 3873 3874 3875 3876 3907 3909 3910 3915 "
               "3916 3919 3923 3926 3928 3931 3933 3935 3936 3937 3939 3941 3943 3944 3945 3947 "
               "3949 3951 3952 3953 3954 3955 3956 3957 3958 3961 3963 3964 3965 3966 3967 3969 "
               "3970 3972 3973 3974 3975 4002 4006 4009 4011 4013 4014 4016 4017 4029 4030 4032 "
               "4035 4037 4038 4039 4041 4042 4043 4044 4045 4046 4047 4048 4049 4050 4051 4052 "
               "4053 4055 4056 4058 4059 4060 4061 4062 4063 4064 4066 4068 4070 4071 4073 4102 "
               "4106 4109 4111 4112 4114 4115 4118 4121 4123 4125 4126 4127 4129 4131 4133 4134 "
               "4135 4136 4137 4138 4139 4140 4141 4143 4144 4145 4146 4147 4148 4149 4150 4152 "
               "4153 4155 4156 4157 4158 4159 4160 4161 4162 4163 4164 4165 4166 4168 4169 4172 "
               "4201 4203 4204 4205 4206 4211 4212 4215 4217 4221 4222 4223 4225 4227 4229 4230 "
               "4232 4234 4235 4236 4237 4239 4242 4245 4247 4248 4251 4253 4254 4257 4258 4261 "
               "4262 4265 4266 4267 4296 4298 4300 4302 4304 4306 4308 4309 4310 4311 4317 4319 "
               "4322 4323 4325 4326 4328 4329 4331 4332 4333 4334 4335 4336 4337 4339 4340 4341 "
               "4342 4343 4345 4346 4347 4349 4350 4351 4352 4353 4354 4355 4356 4358 4359 4360 "
               "4361 4363 4364 4366 4367 4394 4398 4399 4403 4406 4409 4412 4413 4416 4418 4421 "
               "4423 4425 4426 4428 4429 4430 4431 4432 4433 4434 4435 4436 4437 4438 4440 4441 "
               "4442 4443 4444 4445 4446 4447 4448 4449 4450 4451 4452 4453 4454 4455 4457 4458 "
               "4459 4460 4461 4463 4464 4465 4490 4496 4498 4500 4503 4504 4506 4507 4512 4515 "
               "4517 4518 4521 4522 4523 4525 4526 4527 4528 4529 4530 4531 4532 4534 4535 4536 "
               "4537 4538 4540 4541 4542 4544 4545 4546 4548 4549 4551 4553 4554 4555 4556 4559 "
               "4560 4563 4564 4588 4592 4595 4597 4599 4600 4602 4603 4604 4607 4608 4612 4615 "
               "4616 4617 4619 4620 4622 4623 4624 4625 4627 4628 4630 4631 4632 4633 4634 4635 "
               "4636 4637 4638 4639 4640 4642 4643 4645 4646 4648 4649 4651 4652 4653 4654 4655 "
               "4656 4657 4658 4661 4690 4692 4694 4696 4697 4699 4701 4702 4703 4708 4711 4713 "
               "4715 4716 4717 4718 4719 4720 4721 4722 4723 4725 4726 4727 4728 4729 4730 4731 "
               "4733 4734 4735 4736 4737 4738 4739 4740 4742 4743 4744 4745 4746 4747 4748 4749 "
               "4751 4752 4753 4755 4756 4757 4758 4759 4761 4783 4787 4789 4790 4795 4796 4797 "
               "4798 \n";
          return EXIT_FAILURE;
        }
      }
    }
    {
      selector->SetArea(659.0, 379.0, 681.0, 395.0);
      auto selection = vtk::TakeSmartPointer(selector->Select());
      if (selection->GetNumberOfNodes() != 1)
      {
        std::cerr << "Expected 1 nodes, got " << selection->GetNumberOfNodes() << "nodes\n";
        return EXIT_FAILURE;
      }
      auto* node = selection->GetNode(0);
      if (node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) != 3)
      {
        std::cerr << "Expected propId = 3, got "
                  << node->GetProperties()->Get(vtkSelectionNode::PROP_ID()) << "\n";
        return EXIT_FAILURE;
      }
      else
      {
        std::ostringstream oss;
        node->GetSelectionList()->PrintValues(oss);
        if (oss.str() != "3 4 5 ")
        {
          std::cerr << "Expected selected Ids = " << oss.str() << " got 3 4 5 \n";
          return EXIT_FAILURE;
        }
      }
    }
  }

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    vtkNew<vtkRenderedAreaPicker> picker;
    iren->SetPicker(picker);
    iren->SetRenderWindow(renWin);
    // use a rubber band pick style for area selections.
    vtkNew<vtkInteractorStyleRubberBandPick> style;
    iren->SetInteractorStyle(style);
    style->SetDefaultRenderer(renderer);

    vtkNew<vtkCallbackCommand> onEndPick;
    onEndPick->SetCallback(
      [](vtkObject* caller, unsigned long, void*, void*)
      {
        auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
        auto* areaPicker = vtkRenderedAreaPicker::SafeDownCast(interactor->GetPicker());
        vtkNew<vtkHardwareSelector> _selector;
        _selector->SetCaptureZValues(true);
        _selector->SetRenderer(areaPicker->GetRenderer());

        double x0 = areaPicker->GetRenderer()->GetPickX1();
        double y0 = areaPicker->GetRenderer()->GetPickY1();
        double x1 = areaPicker->GetRenderer()->GetPickX2();
        double y1 = areaPicker->GetRenderer()->GetPickY2();

        _selector->SetArea(
          static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
        auto selection = vtk::TakeSmartPointer(_selector->Select());
        selection->Print(cout);
      });
    iren->AddObserver(vtkCommand::EndPickEvent, onEndPick);
    iren->Start();
  }

  vtkSmartPointer<vtkImageData> depthImage =
    vtkWebGPURenderWindow::SafeDownCast(renWin)->SaveAttachmentToVTI(
      vtkWebGPURenderWindow::AttachmentTypeForVTISnapshot::Depth);
  vtkSmartPointer<vtkImageData> rgbImage =
    vtkWebGPURenderWindow::SafeDownCast(renWin)->SaveAttachmentToVTI(
      vtkWebGPURenderWindow::AttachmentTypeForVTISnapshot::ColorRGB);
  vtkSmartPointer<vtkImageData> rgbaImage =
    vtkWebGPURenderWindow::SafeDownCast(renWin)->SaveAttachmentToVTI(
      vtkWebGPURenderWindow::AttachmentTypeForVTISnapshot::ColorRGBA);
  vtkSmartPointer<vtkImageData> idsImage =
    vtkWebGPURenderWindow::SafeDownCast(renWin)->SaveAttachmentToVTI(
      vtkWebGPURenderWindow::AttachmentTypeForVTISnapshot::Ids);

  if (saveAttachments)
  {
    vtkNew<vtkXMLImageDataWriter> writer;
    writer->SetInputData(depthImage);
    writer->SetFileName("depth.vti");
    writer->Write();
    writer->SetInputData(rgbImage);
    writer->SetFileName("color.vti");
    writer->Write();
    writer->SetInputData(rgbaImage);
    writer->SetFileName("colorrgba.vti");
    writer->Write();
    writer->SetInputData(idsImage);
    writer->SetFileName("ids.vti");
    writer->Write();
  }
  return retVal != vtkTesting::FAILED ? EXIT_SUCCESS : EXIT_SUCCESS;
}
