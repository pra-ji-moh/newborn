# The newborn

**Open it on your phone: https://pra-ji-moh.github.io/newborn/**

The newborn's own brain, running in your browser. It moves its own eyes over
what your camera sees, makes its own sounds, and jerks (your phone buzzes,
where it can). Nothing on the page tells it what to do: it is drawn to what it
is getting better at predicting, and gets bored of what it knows.

| Its sense | From |
|---|---|
| Its eyes, 8 by 8 | your camera (tap *Use live camera and mic*), or a photo or video you show it |
| Its hearing, 8 bands | your microphone, which also hears its own voice from the speaker |
| Its skin, 8 places | your fingers on the picture: how firmly, how many, slow strokes |
| Its inner ear | your phone's motion sensors, and its own jerks |

It keeps the last 30,000 moments of its life exactly as they came, and you can
light any one of them to see what it saw then. That changes nothing in it.

**Everything stays on your device.** The camera, the sound, your touches and
its memories go only into its brain, inside the page. Nothing is uploaded. It
keeps its life in this browser's own storage, on this device only.

`index.html` is the whole of it: the page, and the brain (C, compiled to
WebAssembly, about 12 KB) inside it. It is built from the Ciall project's
`brain/ob_life.c` and `phone/crib.html`.
