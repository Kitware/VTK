## Window / Level Control
proc WindowLevelOn {lut} {
    global activeLut
    global .windowLevel.f1.level
    set activeLut $lut
    set inverseVideo [$lut GetInverseVideo]
    .windowLevel.f1.level set [$lut GetLevel];
    .windowLevel.f1.window set [$lut GetWindow];
    .windowLevel configure
    SetInverseVideo
    wm deiconify .windowLevel;
}

## Create windowLevel popup
toplevel .windowLevel;
wm title .windowLevel {Window Level}
wm withdraw .windowLevel;

frame .windowLevel.f1;
scale .windowLevel.f1.window -label Window -from 1 -to 4095 -orient horizontal -command SetWindow
scale .windowLevel.f1.level -label Level -from 1 -to 4095 -orient horizontal -command SetLevel
checkbutton .windowLevel.f1.video -text "Inverse Video" -variable inverseVideo -command SetInverseVideo
pack  .windowLevel.f1.window .windowLevel.f1.level .windowLevel.f1.video -side  top

button .windowLevel.dismiss -text "Dismiss" -command "wm withdraw .windowLevel; unset activeLut";
pack .windowLevel.f1 .windowLevel.dismiss -side top -pady .1i -padx .1i -fill x -expand 1;


proc SetWindow window {
	global activeLut
	$activeLut SetWindow $window;
	$activeLut Build;
        renWin Render;
}
proc SetLevel level {
	global activeLut
	$activeLut SetLevel $level;
	$activeLut Build;
        renWin Render;
}
proc SetInverseVideo {} {
	global activeLut inverseVideo
	if { $inverseVideo == 0 } {
		$activeLut InverseVideoOff;
	} else {
		$activeLut InverseVideoOn;
	}		
	$activeLut Build;
        renWin Render;
}

