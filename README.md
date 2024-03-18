# Raymarching V1.0
Render 3D objects using raymarching algorithm.

## Setup
The project setup is only available on OSX.
Also, make sure [brew](https://formulae.brew.sh/) is installed, the project uses [libomp](https://formulae.brew.sh/formula/libomp) library.
Download dependencies from GitHub:
```sh
$ git submodule init && git submodule update
```
Setup libraries (LiteMath, stb, libomp):
```sh
$ make libs
```
Build the project:
```sh
$ make all
```

## Render
To run the project execute:
```sh
$ make run
```
Rendering initial scene might take ~1 min.  
For faster rendering change
MengerSponge iterations in scene file to `2`.

## Scene
To render your own scene modify `scene/objects.txt`.  
Objects description:
```
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
CPU: 2,5GHz quad-core Intel Core i7  
Memory: 16GB 1600MHz DDR3  
Graphics: Intel Iris Pro 1536MB  
```sh
Rendering with CPU (1 thread):		68.1477s
Rendering with OpenMP (4 threads):	21.9033s
Rendering with GPU (sphere):		0.031496s
```

## Work in progress...
The GPU implentation is not done yet
