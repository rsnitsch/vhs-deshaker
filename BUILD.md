# Building vhs-deshaker

## Prerequisites

Download and install CMake.

Download latest OpenCV 4.x release.

## Windows / Visual Studio 2019

Open CMake GUI. Select vhs-deshaker directory as source. Create a subfolder _build and choose it as build folder.

Click Configure. It will likely fail to find OpenCV. If so, set ``OpenCV_DIR`` to the build folder within the
extracted OpenCV folder.

Click Configure again and then click Generate.

Now open ``_build\vhs-deshaker.sln`` with Visual Studio.

Switch to the Release config and then do Build\Build vhs-deshaker.

The executable should thus be created in the folder _build\src\Release.

## Running vhs-deshaker from within Visual Studio

Right click the vhs-deshaker project and choose Properties. In Debugging add to Environment this line:

    PATH=$(OpenCV_DIR)\bin;$(OpenCV_DIR)\x64\vc15\bin;$(PATH)

Right click vhs-deshaker project and choose "Set as start project".

You should also add the commandline arguments and set the working directory.
