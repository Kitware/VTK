## Fix HyperTreeGridAxisClip when insideout is true

Using inside out (true) did not give the expected result.
Indeed, thin cells cut by the plane must always be kept.
