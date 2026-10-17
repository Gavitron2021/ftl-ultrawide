# Build from source

The beta ZIP supplies our compiled, unsigned 32-bit launcher. A source checkout requires Visual Studio 2022 Build Tools with **Desktop development with C++**, including x86 tools. From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\release\build-native.ps1
```

The script finds MSVC with `vswhere`; optional `-VcVarsPath` selects a specific installed `vcvars32.bat`. Output is `release/bin/FTL-Ultrawide.exe`, using the static C++ runtime. It does not launch/install the game.

For the Python tools and tests, use Python 3.11+:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r .\release\requirements.txt
.\.venv\Scripts\python.exe -m unittest discover -s .\release\tests -v
```

Follow [INSTALL.md](INSTALL.md) to build the private local installation from owned files and separate author downloads. Reconstructed textures are verified by approved pixel hashes, independent of PNG compression differences. Validation also checks exact panorama/center-crop matching, production profile inheritance and unchanged base sprite resources.

`release/package.py` makes the public ZIP/checksum from the Git file list plus our compiled launcher, refusing forbidden files. Local payloads, game executables, full game archives, saves, backups, logs and upstream dependencies are excluded. GitHub's green Code download contains source rather than the compiled launcher; use a beta release ZIP or build it yourself.
