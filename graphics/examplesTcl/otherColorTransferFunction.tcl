# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "grep -v vtkColorTransferFunction | grep -v 0x | grep -v Modified"
set rtComparator "diff"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "ctf test started"

    vtkColorTransferFunction ctf1
    puts $fileid "ctf1: "
    puts $fileid "ctf1 [ctf1 Print]"
    
    puts $fileid "ctf1 GetTotalSize:           [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"
    ctf1 AddRedPoint 1.0 .5
    puts $fileid "ctf1(AddRedPoint):           Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddGreenPoint 1.0 .4
    puts $fileid "ctf1(AddGreenPoint):         Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddBluePoint 1.0 .3
    puts $fileid "ctf1(AddBluePoint):          Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBPoint 2.0 .1 .2 .3
    puts $fileid "ctf1(AddRGBPoint):           Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveRedPoint 1.0
    puts $fileid "ctf1(RemoveRedPoint):        Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveGreenPoint 1.0
    puts $fileid "ctf1(RemoveGreenPoint):      Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveBluePoint 1.0
    puts $fileid "ctf1(RemoveBluePoint):       Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveBluePoint 1.0
    puts $fileid "ctf1(RemoveBluePoint again): Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveRGBPoint 2.0
    puts $fileid "ctf1(RemoveRGBPoint):        Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBPoint 1.0 .1 .2 .3
    puts $fileid "ctf1(AddRGBPoint):           Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveAllPoints
    puts $fileid "ctf1(RemoveAllPoints):       Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRedSegment 2.0 0.0 10.0 .5
    puts $fileid "ctf1(AddRedSegment):         Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRedSegment 20.0 0.0 10.0 .5
    puts $fileid "ctf1(AddRedSegment):         Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBSegment 15 0 0 0 30 1 1 1
    puts $fileid "ctf1(AddRGBSegment):         Total Size is: [ctf1 GetTotalSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    puts $fileid "ctf1 GetValue 0.0:           [ctf1 GetValue 0.0]"
    puts $fileid "ctf1 GetValue 2.0:           [ctf1 GetValue 2.0]"
    puts $fileid "ctf1 GetValue 20.0:          [ctf1 GetValue 20.0]"
    puts $fileid "ctf1 GetValue 25.0:          [ctf1 GetValue 25.0]"
    puts $fileid "ctf1 GetValue 30.0:          [ctf1 GetValue 30.0]"
    puts $fileid "ctf1 GetRedValue 20.0:       [ctf1 GetRedValue 20.0]"
    puts $fileid "ctf1 GetGreenValue 25.0:     [ctf1 GetGreenValue 25.0]"
    puts $fileid "ctf1 GetBlueValue 30.0:      [ctf1 GetBlueValue 30.0]"
    puts $fileid "ctf1 GetBlueValue 50.0:      [ctf1 GetBlueValue 50.0]"

    vtkColorTransferFunction ctf2
    ctf2 DeepCopy ctf1
    ctf2 SetClamping 0
    puts $fileid "ctf2 SetClamping 0"
    puts $fileid "ctf2 GetValue 0.0:           [ctf2 GetValue 0.0]"
    puts $fileid "ctf2 GetValue 2.0:           [ctf2 GetValue 2.0]"
    puts $fileid "ctf2 GetValue 20.0:          [ctf2 GetValue 20.0]"
    puts $fileid "ctf2 GetValue 25.0:          [ctf2 GetValue 25.0]"
    puts $fileid "ctf2 GetValue 30.0:          [ctf2 GetValue 30.0]"
    puts $fileid "ctf2 GetRedValue 20.0:       [ctf2 GetRedValue 20.0]"
    puts $fileid "ctf2 GetGreenValue 25.0:     [ctf2 GetGreenValue 25.0]"
    puts $fileid "ctf2 GetBlueValue 30.0:      [ctf2 GetBlueValue 30.0]"
    puts $fileid "ctf2 GetBlueValue 50.0:      [ctf2 GetBlueValue 50.0]"

    ctf1 Delete
    ctf2 Delete
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
