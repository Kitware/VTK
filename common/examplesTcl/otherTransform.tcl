# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "grep -v 0x | sed -e s/-0\ /0\ /g | grep -v Modified"
set rtComparator "diff"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "transform test started"

    vtkTransform trans
    puts $fileid "vtkTransform: "
    puts $fileid "trans [trans Print]"
    
    trans RotateWXYZ 20 1 1 2
    puts $fileid "RotateWXYZ 20 1 1 2"
    puts $fileid "trans [trans Print]"
    
    trans Translate 1 10 2
    puts $fileid "Translate 1 10 2"
    puts $fileid "trans [trans Print]"
    
    set invtrans [trans GetInverse]
    $invtrans Update
    puts $fileid "GetInverse"
    puts $fileid "trans GetInverse [$invtrans Print]"

    trans Inverse
    puts $fileid "Inverse"
    puts $fileid "trans [trans Print]"

    trans Identity
    puts $fileid "Identity"
    puts $fileid "trans [trans Print]"

    trans RotateWXYZ 20 1 1 2
    trans Translate 1 10 2
    puts $fileid "RotateWXYZ 20 1 1 2"
    puts $fileid "Translate 1 10 2"
    puts $fileid ""

    puts $fileid "trans TransformPoint  1 2 3  [trans TransformPoint 1 2 3]"
    puts $fileid ""

    puts $fileid "trans TransformNormal 1 2 3  [trans TransformNormal 1 2 3]"
    puts $fileid ""

    puts $fileid "trans TransformVector 1 2 3  [trans TransformVector 1 2 3]"
    puts $fileid ""

    vtkProjectionTransform ptrans
    puts $fileid "vtkProjectionTransform: "
    puts $fileid "ptrans [ptrans Print]"

    vtkMatrix4x4 matrix
    matrix SetElement 3 0 0.1
    matrix SetElement 3 1 0.05
    matrix SetElement 3 2 0.3
    ptrans SetMatrix matrix
    puts $fileid "SetElement 3 0 0.1"
    puts $fileid "SetElement 3 1 0.05"
    puts $fileid "SetElement 3 2 0.3"
    puts $fileid "ptrans [ptrans Print]"

    set invtrans [ptrans GetInverse]
    $invtrans Update
    puts $fileid "GetInverse"
    puts $fileid "ptrans GetInverse [$invtrans Print]"

    ptrans Inverse
    puts $fileid "Inverse"
    puts $fileid "ptrans [ptrans Print]"

    ptrans Identity
    puts $fileid "Identity"
    puts $fileid "ptrans [ptrans Print]"

    ptrans SetMatrix matrix
    puts $fileid "SetElement 3 0 0.1"
    puts $fileid "SetElement 3 1 0.05"
    puts $fileid "SetElement 3 2 0.3"

    puts $fileid "ptrans TransformPoint  1 2 3  [ptrans TransformPoint 1 2 3]"
    puts $fileid ""

    vtkIdentityTransform itrans
    puts $fileid "vtkIdentityTransform: "
    puts $fileid "itrans [itrans Print]"

    puts $fileid "itrans TransformPoint  1 2 3  [itrans TransformPoint 1 2 3]"
    puts $fileid ""

    puts $fileid "itrans TransformNormal 1 2 3  [itrans TransformNormal 1 2 3]"
    puts $fileid ""

    puts $fileid "itrans TransformVector 1 2 3  [itrans TransformVector 1 2 3]"
    puts $fileid ""

    vtkPerspectiveTransformConcatenation pctrans
    puts $fileid "vtkPerspectiveTransformConcatenation: "
    puts $fileid "pctrans [pctrans Print]"

    pctrans Concatenate itrans
    pctrans Concatenate ptrans
    pctrans PostMultiply
    pctrans Concatenate trans
    pctrans Update
    puts $fileid "Concatenate itrans"
    puts $fileid "Concatenate ptrans"
    puts $fileid "PostMultiply"
    puts $fileid "Concatenate trans"
    puts $fileid "Update"
    puts $fileid "pctrans [pctrans Print]"

    vtkGeneralTransformConcatenation gctrans
    puts $fileid "vtkGeneralTransformConcatenation: "
    puts $fileid "gctrans [gctrans Print]"

    gctrans Concatenate itrans
    gctrans Concatenate ptrans
    gctrans PostMultiply
    gctrans Concatenate trans
    gctrans Update
    puts $fileid "Concatenate itrans"
    puts $fileid "Concatenate ptrans"
    puts $fileid "PostMultiply"
    puts $fileid "Concatenate trans"
    puts $fileid "Update"
    puts $fileid "gctrans [gctrans Print]"

    puts $fileid "Should be the same to within roundoff error"
    puts $fileid "gctrans TransformPoint 1 2 3  [gctrans TransformPoint 1 2 3]"
    puts $fileid "pctrans TransformPoint 1 2 3  [pctrans TransformPoint 1 2 3]"
    puts $fileid ""

    puts $fileid "GetInverse"
    puts $fileid "gctrans TransformPoint -0.998804092407 10.4192266464 4.93264579773  [[gctrans GetInverse] TransformPoint -0.998804092407 10.4192266464 4.93264579773]"
    puts $fileid "pctrans TransformPoint -0.998804152012 10.4192276001 4.93264627457  [[pctrans GetInverse] TransformPoint -0.998804152012 10.4192276001 4.93264627457]"
    puts $fileid ""

    puts $fileid "Inverse"
    gctrans Inverse
    pctrans Inverse
    puts $fileid "gctrans TransformPoint -0.998804092407 10.4192266464 4.93264579773  [gctrans TransformPoint -0.998804092407 10.4192266464 4.93264579773]"
    puts $fileid "pctrans TransformPoint -0.998804152012 10.4192276001 4.93264627457  [pctrans TransformPoint -0.998804152012 10.4192276001 4.93264627457]"
    puts $fileid ""

    puts $fileid "Identity"
    gctrans Identity
    pctrans Identity
    puts $fileid "gctrans TransformPoint 1 2 3  [gctrans TransformPoint 1 2 3]"
    puts $fileid "pctrans TransformPoint 1 2 3  [pctrans TransformPoint 1 2 3]"
    puts $fileid ""

    puts $fileid "transform test completed"

    trans Delete
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
