catch {load vtktcl}
catch {load vtktcl}
# Developed By Majeid Alyassin

wm withdraw .
toplevel .load_param  
wm title .load_param {Load Parameters}
wm resizable .load_param 0 0
wm withdraw .load_param

# Interface to Load parameters ...
frame .load_param.t1 -relief ridge -borderwidth 3
frame .load_param.t1.process1
button .load_param.t1.process1.b1 -text "Load Filename..." -command LFN
frame .load_param.t1.group1
frame .load_param.t1.group2
label .load_param.t1.group1.l1 -text "Start Slice --- Num. of Slices"
label .load_param.t1.group2.l2 -text "Aspect Ratio X:Y:Z"
entry .load_param.t1.group1.first -width 4 
entry .load_param.t1.group1.number -width 4
entry .load_param.t1.group2.arx -width 4 
entry .load_param.t1.group2.ary -width 4
entry .load_param.t1.group2.arz -width 4
button .load_param.b2 -text "OK" -command OKB
button .load_param.b3 -text "Cancel" -command CancelB

# pack frames togther ...
pack .load_param.t1 -side top
pack .load_param.t1.process1 .load_param.t1.group1 .load_param.t1.group2  -side top
pack  .load_param.t1.process1.b1 \
      -side top -expand 1 -in .load_param.t1.process1 -fill both -padx 2 -pady 2
pack  .load_param.t1.process1 \
       -expand 1 -in .load_param.t1 -side top -fill both -padx 2 -pady 2
pack  .load_param.b2 .load_param.b3\
      -side left -expand 1 -in .load_param -fill both -padx 2 -pady 2

pack .load_param.t1.group1.l1 -in .load_param.t1.group1 -side top -fill both -expand 1
pack .load_param.t1.group2.l2 -in .load_param.t1.group2 -side top -fill both -expand 1
pack .load_param.t1.group1.first .load_param.t1.group1.number -padx 3 -pady 2 \
	-in .load_param.t1.group1 -side left -fill both -expand 1
pack  .load_param.t1.group1 \
      -side top -expand 1 -in .load_param.t1 -fill both -padx 2 -pady 2
pack .load_param.t1.group2.arx .load_param.t1.group2.ary  .load_param.t1.group2.arz -padx 3 -pady 2 \
	-in .load_param.t1.group2 -side left -fill both -expand 1
pack  .load_param.t1.group2 \
      -side top -expand 1 -in .load_param.t1 -fill both -padx 2 -pady 2

global firstslice numslices subx suby subz 

.load_param.t1.group1.first insert 0 $firstslice
.load_param.t1.group1.number insert 0 $numslices

bind .load_param.t1.group1.first       <Return> { PreSlice }
bind .load_param.t1.group1.number      <Return> { PreSlice }

.load_param.t1.group2.arx insert 0 $subx
.load_param.t1.group2.ary insert 0 $suby
.load_param.t1.group2.arz insert 0 $subz

bind .load_param.t1.group2.arx      <Return> { PreAR }
bind .load_param.t1.group2.ary      <Return> { PreAR }
bind .load_param.t1.group2.arz      <Return> { PreAR }


proc LFN {} {
       wm deiconify .file
}

proc PreSlice {} {
   global  firstslice numslices
   set firstslice  [.load_param.t1.group1.first get]
   set numslices   [.load_param.t1.group1.number get]
   puts $firstslice
   puts $numslices


}
proc PreAR {} {
   global  subx suby subz
   set subx   [.load_param.t1.group2.arx get]
   set suby   [.load_param.t1.group2.ary get]
   set subz   [.load_param.t1.group2.arz get]
   puts $subx
   puts $suby
   puts $subz

}
proc  OKB {} {
    global viewer viewer1 ss composite  mp
    global prefix numslices firstslice subx suby subz
    reader  SetFilePrefix $prefix
    puts    $prefix
    reader  SetDataDimensions 512 512 $numslices 1
    reader  SetFirst $firstslice
    set firstslice  [.load_param.t1.group1.first get]
    set numslices   [.load_param.t1.group1.number get]
    puts " first slice = $firstslice & lastslice [expr $firstslice + $numslices]" 
    set subx   [.load_param.t1.group2.arx get]
    set suby   [.load_param.t1.group2.ary get]
    set subz   [.load_param.t1.group2.arz get]
    puts "$subx:$suby:$subz"
    ss  SetSamplingFactors $subx $suby $subz
    composite SetMagnificationFactors $subx $suby $subz
    mip SetProjectionRange $subz [expr $numslices-1-$subz]
    .ctan.cta5.group.lowerprj delete 0 10
    .ctan.cta5.group.lowerprj insert 0 $subz
    .ctan.cta5.group.upperprj delete 0 10
    .ctan.cta5.group.upperprj insert 0 [expr $numslices-1-$subz]
    wm withdraw .load_param
}
proc  CancelB {} {
    wm withdraw .load_param
}
