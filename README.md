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

Rendering initial scene might take ~1 min.  
For faster rendering change
MengerSponge iterations in scene file to `2`.

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

CPU: Intel Core i5-13500H 2.60GHz  
Memory: 16GB 4800MHz LPDDR5  
GPU: Intel Iris Xe Graphics  

```sh
Render with CPU (1 thread):     31.2233s
Render with OpenMP (4 threads): 4.60926s
Render with GPU:                2.43829s
Copy to GPU:                    0.0049152s
Render + Copy on GPU:           2.44321s
```
