# Tower Defender VR - Shared Libraries

This folder (`shared/lib`) includes pre-compiled libraries used in the game. Specifically...
* Machine Type: Win64
* Configuration: Release
* Runtime Library: Multi-threaded (/MT)

## Re-compile LibOVR from Oculus Rift SDK

More information can be [here](https://developer.oculus.com/documentation/pcsdk/latest/concepts/dg-sdk-setup/).

## Re-compile OpenAL-Soft

```
git clone https://github.com/kcat/openal-soft
cd openal-soft/build
cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Re-compile rpclib

```
git clone https://github.com/rpclib/rpclib
mkdir rpclib/build
cd rpclib/build
cmake .. -G "Visual Studio 15 2017 Win64" -DCMAKE_CXX_FLAGS_RELEASE="-MT"
cmake --build . --config Release
```
