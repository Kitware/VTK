# All tests need to:
# Define 2 variables and 1 proc
#     var 1: rtSelector - a shell command line to select the output to be compared
#     var 2: rtComparator - a shell command to compare the selected output with the baseline
#     proc 1: rtOtherTest (fileid)
# A few statements at the end to run outside the regression testing framework
#

catch {load vtktcl}

set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v StartTime: | grep -v 0x | grep -v Modified | grep -v 'Compute Time:'"
set rtSelector "sed -e s/0x0/0/ | sed -e s/-0/0/ | grep -v -i thread | grep -v Time: | grep -v 0x | grep -v Modified"
set rtComparator "diff -b"

proc rtOtherTest { fileid } {
#actual test
    puts $fileid "Collection test started"

    vtkActor2D a1
      a1 SetLayerNumber 5
    vtkActor2D a2
      a2 SetLayerNumber 4
    vtkActor2D a3
      a3 SetLayerNumber 3
    vtkActor2D a4
      a4 SetLayerNumber 3
    vtkActor2D a5
      a5 SetLayerNumber 2
    vtkActor2D a6
      a6 SetLayerNumber 1
    vtkActor2DCollection a2dc
      a2dc AddItem a1
      a2dc AddItem a2
      a2dc AddItem a3
      a2dc AddItem a4
      a2dc AddItem a5
      a2dc ReplaceItem 3 a6

    puts $fileid "Before Sort"
    a2dc InitTraversal
    for {set i 0} {$i < [a2dc GetNumberOfItems]} {incr i} {
	puts $fileid "$i: [a2dc GetNextItem]"
    }

    puts $fileid "After Sort"
    a2dc Sort
    a2dc InitTraversal
    for {set i 0} {$i < [a2dc GetNumberOfItems]} {incr i} {
	puts $fileid "$i: [a2dc GetNextItem]"
    }

    puts $fileid "After RemoveItem a5, a2, a4"
    a2dc RemoveItem a5
    a2dc RemoveItem a2
    a2dc RemoveItem a4
    a2dc InitTraversal
    for {set i 0} {$i < [a2dc GetNumberOfItems]} {incr i} {
	puts $fileid "$i: [a2dc GetNextItem]"
    }

    puts $fileid "Reference counts"
    puts "a1 Ref count is [a1 GetReferenceCount]"
    puts "a2 Ref count is [a2 GetReferenceCount]"
    puts "a3 Ref count is [a3 GetReferenceCount]"
    puts "a4 Ref count is [a4 GetReferenceCount]"
    puts "a5 Ref count is [a5 GetReferenceCount]"
    puts "a6 Ref count is [a6 GetReferenceCount]"

    a2dc Delete
    
    puts $fileid "Reference counts after delete"
    puts "a1 Ref count is [a1 GetReferenceCount]"
    puts "a2 Ref count is [a2 GetReferenceCount]"
    puts "a3 Ref count is [a3 GetReferenceCount]"
    puts "a4 Ref count is [a4 GetReferenceCount]"
    puts "a5 Ref count is [a5 GetReferenceCount]"
    puts "a6 Ref count is [a6 GetReferenceCount]"
}

# All tests should end with the following...

if {![info exists rtOutId]} {
    rtOtherTest stdout
    exit
}

wm withdraw .
