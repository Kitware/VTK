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
    
    puts $fileid "ctf1 GetSize:                [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"
    ctf1 AddRGBPoint 1.0 .5 0.2 0.4
    puts $fileid "ctf1(AddRGBPoint):           Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddHSVPoint 1.0 .4 0.5 0.6
    puts $fileid "ctf1(AddHSVPoint):           Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBPoint 2.0 .1 .2 .3
    puts $fileid "ctf1(AddRGBPoint):           Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemovePoint 1.0
    puts $fileid "ctf1(RemovePoint):           Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBPoint 1.0 .1 .2 .3
    puts $fileid "ctf1(AddRGBPoint):           Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 RemoveAllPoints
    puts $fileid "ctf1(RemoveAllPoints):       Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBSegment 2.0 0.0 0.0 0.0 10.0 .5 0.0 0.0
    puts $fileid "ctf1(AddRGBSegment):         Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBSegment 20.0 0.0 0.0 0.0 10.0 .5 0.0 0.0
    puts $fileid "ctf1(AddRGBSegment):         Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddRGBSegment 15 0 0 0 30 1 1 1
    puts $fileid "ctf1(AddRGBSegment):         Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    ctf1 AddHSVSegment 35 0 1 1 50 0.9 0.5 0.5
    puts $fileid "ctf1(AddHSVSegment):         Size is: [ctf1 GetSize]"
    puts $fileid "ctf1 GetRange:               [ctf1 GetRange]"

    puts $fileid "ctf1 GetValue 0.0:           [ctf1 GetValue 0.0]"
    puts $fileid "ctf1 GetValue 2.0:           [ctf1 GetValue 2.0]"
    puts $fileid "ctf1 GetValue 20.0:          [ctf1 GetValue 20.0]"
    puts $fileid "ctf1 GetValue 25.0:          [ctf1 GetValue 25.0]"
    puts $fileid "ctf1 GetValue 30.0:          [ctf1 GetValue 30.0]"
    puts $fileid "ctf1 GetValue 40.0:          [ctf1 GetValue 40.0]"
    puts $fileid "ctf1 GetValue 90.0:          [ctf1 GetValue 90.0]"


    ctf1 SetColorSpaceToHSV
    puts $fileid "ctf1 SetColorSpaceToHSV"

    puts $fileid "ctf1 GetValue 0.0:           [ctf1 GetValue 0.0]"
    puts $fileid "ctf1 GetValue 2.0:           [ctf1 GetValue 2.0]"
    puts $fileid "ctf1 GetValue 20.0:          [ctf1 GetValue 20.0]"
    puts $fileid "ctf1 GetValue 25.0:          [ctf1 GetValue 25.0]"
    puts $fileid "ctf1 GetValue 30.0:          [ctf1 GetValue 30.0]"
    puts $fileid "ctf1 GetValue 40.0:          [ctf1 GetValue 40.0]"
    puts $fileid "ctf1 GetValue 90.0:          [ctf1 GetValue 90.0]"

    ctf1 SetColorSpaceToRGB
    puts $fileid "ctf1 SetColorSpaceToRGB"

    vtkColorTransferFunction ctf2
    ctf2 DeepCopy ctf1
    ctf2 SetClamping 1
    puts $fileid "ctf2 SetClamping 1"
    puts $fileid "ctf2 GetValue 0.0:           [ctf2 GetValue 0.0]"
    puts $fileid "ctf2 GetValue 2.0:           [ctf2 GetValue 2.0]"
    puts $fileid "ctf2 GetValue 20.0:          [ctf2 GetValue 20.0]"
    puts $fileid "ctf2 GetValue 25.0:          [ctf2 GetValue 25.0]"
    puts $fileid "ctf2 GetValue 30.0:          [ctf2 GetValue 30.0]"
    puts $fileid "ctf2 GetValue 40.0:          [ctf2 GetValue 40.0]"
    puts $fileid "ctf2 GetValue 90.0:          [ctf2 GetValue 90.0]"

    ctf1 Delete
    ctf2 Delete
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
