#
# save a vtk camera to a file in tcl format
#
proc saveCamera { fileName } {
    set filePtr [open $fileName w]
    set camera "\[ren1 GetActiveCamera\]"
    set activeCamera [ren1 GetActiveCamera]
    puts $filePtr "$camera SetPosition [$activeCamera GetPosition]"
    puts $filePtr "$camera SetFocalPoint [$activeCamera GetFocalPoint]"
    puts $filePtr "$camera SetViewAngle [$activeCamera GetViewAngle]"
    puts $filePtr "$camera SetViewUp [$activeCamera GetViewUp]"
    puts $filePtr "$camera SetClippingRange [$activeCamera GetClippingRange]"
    close $filePtr
}
