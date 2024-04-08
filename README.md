# Raymarching V1.1

Render 3D objects using raymarching algorithm.  

![Rendered on GPU](https://github.com/RevelcoS/Raymarching/raw/master/out_gpu.png)

## Features

* Windows support
* GPU rendering
* CPU faster rendering

## Setup

Note: MacOS support is deprecated.  
The project setup is only available on Windows under [MSYS2](https://www.msys2.org/) UCRT64 environment.  

Download dependencies:

```sh
./libs.sh
```

Download dependencies from GitHub:

```sh
git submodule init && git submodule update
```

Setup libraries (LiteMath, stb):

```sh
make libs
```

Build the project:

```sh
make all
```

## Render

To run the project execute:

```sh
make run
```

Rendering initial scene might take ~1 hour.  
For faster rendering change  
MengerSponge iterations in scene file to `2` and SSAA::kernel in constants.h to `1`.  

## Scene

To render your own scene modify `scene/objects.txt`.  
Objects description:  

```txt
Bounds <float>size
Light <float3>position
Color <float3>color
Box <float3>position <float3>dimensions
Cross <float3>position <float3>dimensions
Sphere <float3>position <float>radius
DeathStar <float3>position <float>radius
MengerSponge <float3>position <float>size <int>iterations
```

## Runtime

CPU: Intel Core i7-7700HQ 2.80GHz  
Memory: SODIMM 16GB 2.40GHz  
GPU: NVIDIA GeForce GTX 1060

```sh
...Loading scene
...Setting up context
...Compiling shaders
...Generating buffers
...Rendering
Render with CPU (1 thread):     3226.6s
Render with OpenMP (4 threads): 819.268s
Render with GPU:                280.258s
Copy to GPU:                    0.0053246s
Render + Copy on GPU:           280.263s
```
