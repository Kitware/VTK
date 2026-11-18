# WebGPU: faster background clear

The WebGPU renderer now clears the color, selection ids and depth attachments with the render
pass load operations when it covers the whole window with a flat background. It used to load
all three attachments and then draw a quad over the viewport to fill in the background, which
cost GPU time on every pixel of every frame. Gradient and textured backgrounds, renderers that
cover only part of the window, a camera scissor and custom render passes still draw the quad. The
rendered images are unchanged; the saving grows with the window size and matters most for light
scenes.
