#
# sample script to generate a sequence of azimuthal changes
# to the current camera
#

set camera [ren1 GetActiveCamera]
KeyNew Azimuth $camera SetPosition

#
# define the Render proc for key framing
#
proc KeyRender {} {
    [ren1 GetLights] InitTraversal
    set light [[ren1 GetLights] GetNextItem]
    set camera [ren1 GetActiveCamera]
    $camera SetClippingRange 1 100
    eval $light SetPosition [$camera GetPosition]
    eval $light SetFocalPoint [$camera GetFocalPoint]
    renWin Render
}

#
# define the key frames
#
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 2
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 36} {incr i} {
  $camera Azimuth 10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth 2
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth 1
KeyAdd Azimuth [$camera GetPosition]

$camera Azimuth 0
KeyAdd Azimuth [$camera GetPosition]

$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -2
KeyAdd Azimuth [$camera GetPosition]
for {set i 0} {$i <= 36} {incr i} {
  $camera Azimuth -10
  KeyAdd Azimuth [$camera GetPosition]
}
$camera Azimuth -2
KeyAdd Azimuth [$camera GetPosition]
$camera Azimuth -1
KeyAdd Azimuth [$camera GetPosition]

KeyRun Azimuth 120
