![Example](src/Parasyte.CLI/Banner.png)

Cordycep is a tool that utilizes game data to handle loading game files. Cordycep was made to work around more strict anti-cheats used in modern titles without comprimising them.

# How Cordycep Works

Cordycep works by taking game executables and essentially patching them and then utilizing internal code to handle the main brunt of loading the data. Cordycep then handles the more higher level functions of asset and string tables that tools can read from. 

Cordycep should in theory be 90% identical to reading from a game directly, and most of the time the same or similar code paths can be used in existing tools to read from Cordycep.

# Building Cordycep

Building Cordycep is simple.

First you'll need the following vcpkg libraries:

```
vcpkg install lz4:x64-windows-static
vcpkg install detours:x64-windows-static
vcpkg install zlib:x64-windows-static
vcpkg install nlohmann-json:x64-windows-static
```

Then you can clone the repo and compile directly.

# Understanding the Source Code

Ultimately a lot of the juicy stuff happens under Commands and Handlers, the rest are helpers for those and should be easy to understand what they do.

Code quality is mixed as a lot of Cordycep is code from when Celerium was a thing, it was constantly evolving and the focus was on getting it out the door given how fast things were changing.

For a lot of the handlers there is a json file that dictates game patterns, files, and other info.

# Future of Cordycep

Cordycep is now abandoned but I have opened sourced it for the community to work on. 