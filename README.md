# Tower Defender VR - *Collaborative Multiplayer Virtual Reality Tower Defense Game*

This is a collaborative multiplayer virtual reality tower defense game built
for the [Oculus Rift](https://www.oculus.com/rift/) on Windows using modern
OpenGL. Bring your friends and join the fun-filled adventure of crushing
castle crashers and defending your valuable treasure as heroic archers!

Here's a quick video demonstration of the game.

[![Demonstration on YouTube](/doc/images/Preview.jpg)](https://www.youtube.com/watch?v=0salJ3tKCK0)

*Link: [https://www.youtube.com/watch?v=0salJ3tKCK0](https://www.youtube.com/watch?v=0salJ3tKCK0)*

## Library Dependencies

In order to make this game, we used a couple of third party libraries in assisting with
audio and networking. You can run the game as is on Visual Studio using the pre-compiled
libraries located in `shared/` or you can re-compile the libraries yourself following
these *[instructions](https://github.com/peternmai/TowerDefender/shared/)*.

The libraries we used are listed below

* [Oculus Rift SDK](https://developer.oculus.com/documentation/pcsdk/latest/concepts/dg-sdk-setup/) *- Interacting with the Oculus Rift*
* [OpenAL-Soft](https://github.com/kcat/openal-soft) *- Audio*
* [rpclib](https://github.com/rpclib/rpclib) *- RPC library for C++*

## Running the Game from Source Code (Windows x64)

1. Install [Visual Studio](https://visualstudio.microsoft.com/)
    1. Use `Desktop development with C++`
1. Install the [Oculus Rift Software](https://www.oculus.com/rift/setup/)
1. Start game server
    1. Open `server/TowerDefender_Server.sln` on Visual Studio
    1. Change Solution Configurations from `Debug` to `Release`
    1. Make sure Solution Platforms is `x64`
    1. Press `F5` to run code
        1. You will be prompted for a port number to use. (Eg. 8080)
1. Have player 1 join the game session
    1. Open `client/TowerDefender_Client.sln` on Visual Studio
    1. Change Solution Configurations from `Debug` to `Release`
    1. Make sure Solution Platforms is `x64`
    1. Link Dynamic-Link Libraries (dll)
        1. Press `Alt+Enter` to go to Project Property Page
        1. Go to `Configuration Properties` -> `Debugging`
        1. Set "Environment" to `PATH=%PATH%;$(SolutionDir)..\shared\lib`
        1. Click `Apply` and `OK`
    1. Press `F5` to run code
        1. You will be prompted for the IP address of the server
        1. You will be prompted for the port number of the server
1. Have player 2 join the game session
    1. On a different computer, repeat step 4

## Special Thanks

* [View Audio Credits](https://github.com/peternmai/TowerDefender/client/audio)
* [View Model Credits](https://github.com/peternmai/TowerDefender/client/objects)

## Known Issues

* Audio - Sound will only play from default speaker
  * Temporary Solution: Change default speaker to the Oculus Rift
