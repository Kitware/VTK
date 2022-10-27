## Fix bugs in CAVE

Fix bug requiring users to dolly the camera to the origin to get better
head tracking in the CAVE by decoupling ParaView's camera from the head
tracking position.

Fix volume rendering bug in the CAVE by using the eye position rather
than the camera position as the starting point for raycasting.
