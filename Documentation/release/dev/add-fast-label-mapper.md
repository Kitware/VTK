## Add vtkFastLabeledDataMapper

The new `vtkFastLabeledDataMapper` uses GPU texture acceleration to draw labels at much
higher frame rates. When hundreds or thousands of labels are on screen at one time,
existing mappers can drop below 1 fps. This acceleration is designed to render the
same number of labels at over 60 fps so a user can label many points while retaining
interactive rotations and animations.
