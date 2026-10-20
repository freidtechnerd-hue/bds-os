# ClassiCube

Source: https://github.com/ClassiCube/ClassiCube

Pinned commit: 33891a4e7e586941240261bc911312f228be57d3

License: BSD-3-Clause plus notices in license.txt.
BDS platform and build configuration live in ../../classicube and ../../tools/build_classicube.py.

BDS modification: Graphics_SoftGPU.c calls Platform_BDS_Poll between batches
and every 64 world quads to keep the polled PS/2 FIFO serviced during long
software frames. The adapter queues button transitions for the next event pass.
`Graphics_SoftGPU.c` also rounds depth to the stored float precision before comparison on x87, preventing water depth/color pass striping, and rejects zero-area triangles.
