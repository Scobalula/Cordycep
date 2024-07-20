![Example](src/Parasyte.CLI/Banner.png)

[![Releases](https://img.shields.io/github/downloads/Scobalula/Cordycep/total.svg)](https://github.com/Scobalula/Cordycep/releases) [![License](https://img.shields.io/github/license/Scobalula/Cordycep.svg)](https://github.com/Scobalula/Cordycep/blob/main/LICENSE.md) [![Discord](https://img.shields.io/badge/chat-Discord-blue.svg)](https://discord.gg/eY2Y5p2PEp)

# Cordycep

Cordycep is a tool that utilizes game data to handle loading game files. Cordycep was made to work around more strict anti-cheats used in modern titles without comprimising them.

# Project Situation

Cordycep was originally developed by [Scobalula](https://github.com/Scobalula). Now it's mainly maintained by [dest1yo](https://github.com/dest1yo).

**Join my Discord server at the banner to get the latest version info, since project in GitHub may not be updated synchronously.**

# How Cordycep Works

Cordycep works by taking game executables and essentially patching them and then utilizing internal code to handle the main brunt of loading the data. Cordycep then handles the more higher level functions of asset and string tables that tools can read from.

Cordycep should in theory be 90% identical to reading from a game directly, and most of the time the same or similar code paths can be used in existing tools to read from Cordycep.

# Building Cordycep

- Pull the Cordycep repository `git clone https://github.com/Scobalula/Cordycep.git`
- Set up [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-cmd#1---set-up-vcpkg) with MSBuild for extra libraries we need.
- Compile the project, vcpkg will help us integrate the libraries automatically.

# Understanding the Source Code

Ultimately a lot of the juicy stuff happens under Commands and Handlers, the rest are helpers for those and should be easy to understand what they do.

Code quality is mixed as a lot of Cordycep is code from when Celerium was a thing, it was constantly evolving and the focus was on getting it out the door given how fast things were changing.

For a lot of the handlers there is a toml file that dictates game patterns, files, and other info.
