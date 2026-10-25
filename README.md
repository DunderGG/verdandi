<div align="center">

<img src="resources/social_preview_wordmark.png" width="512" alt="Verdandi icon">

[![CI][ci-badge]][ci]
[![Coverage][coverage-badge]][coverage]
[![Release][release-badge]][release]
[![License: GPL-3.0][license-badge]](LICENSE)
[![Platform: Windows 10 | 11][platform-badge]](#-prerequisites)
[![C++20][cpp-badge]](docs/architecture.md)
[![Qt 6][qt-badge]](docs/architecture.md)

</div>

[ci-badge]: https://github.com/DunderGG/verdandi/actions/workflows/ci.yml/badge.svg?branch=main
[ci]: https://github.com/DunderGG/verdandi/actions/workflows/ci.yml?query=branch%3Amain
[coverage-badge]: https://img.shields.io/endpoint?url=https%3A%2F%2Fdundergg.github.io%2Fverdandi%2Fcoverage_badge.json
[coverage]: https://dundergg.github.io/verdandi/
[release-badge]: https://img.shields.io/github/v/release/DunderGG/verdandi
[release]: https://github.com/DunderGG/verdandi/releases/latest
[license-badge]: https://img.shields.io/github/license/DunderGG/verdandi
[platform-badge]: https://img.shields.io/badge/platform-Windows%2010%20%7C%2011-0078D6
[cpp-badge]: https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white
[qt-badge]: https://img.shields.io/badge/Qt-6-41CD52?logo=qt&logoColor=white

Verdandi provides a clear view of system health, resource usage, network connectivity, and running processes while keeping its own footprint minimal.

Built with C++20, Qt 6, and direct Windows APIs for accurate, low-overhead monitoring.

## ✨ Features

- **Dashboard** — At-a-glance cards for CPU, memory, disk, network, and uptime, with an overall system status. Choose which cards to show.
- **Performance charts** — Live charts of CPU (total and per logical processor), memory (used and committed), disk space used per volume, and network throughput per adapter, each with detailed readouts. Keep up to 1,800 samples of history: 30 minutes at the default refresh interval.
- **Processes** — Sortable process list with CPU, memory, I/O rate, threads, and handles. Filter by name or PID, or show each process under its parent as a tree. New processes are highlighted, and ended ones are marked before they disappear.
- **Network view** — Each adapter's type, status, link speed, IPv4 and IPv6 addresses, DNS servers, network name and category, with live send and receive charts. Connectivity level, metered and roaming status update as soon as Windows reports a change. "Show all adapters" also lists the adapters Windows hides.
- **Process management** — End a process after confirming. Only your own processes that are not elevated can be ended.
- **Light and dark** — Follows Windows' light or dark mode, also when it changes, or stays light or dark whatever Windows uses. Windows' contrast themes keep their own colours.
- **Settings** — Set the refresh interval (0.5 to 10 seconds), the colour scheme, byte units (KiB or KB), network throughput in bytes or bits per second, the dashboard's cards, and the log level from File > Settings. Changes apply at once and are saved.
- **Sleep and resume** — After the PC wakes up, rates start again from fresh readings instead of averaging over the time it slept.
- **One window** — Starting Verdandi while it runs brings its window to the front, restored if it was minimized, instead of opening a second one.

*Screenshots coming soon — the application is under active development.*

## ⬇️ Download

Download `Verdandi-<version>-x64.zip` from the [latest release](https://github.com/DunderGG/verdandi/releases/latest), unzip it into a folder you can write to, such as your Desktop or Documents, and run `verdandi.exe`. It needs Windows 10 or 11, 64-bit, and nothing else: no installer, no administrator rights and no runtime to install.

- **Portable:** Verdandi keeps its settings and its log in its own folder, beside `verdandi.exe`, so deleting the folder removes it. To update, unzip the new version into a folder of its own and copy your `settings.json` across.
- **Windows SmartScreen** may say "Windows protected your PC", because the releases are not code-signed yet. Click **More info**, then **Run anyway**.
- **Checksums:** each release's `SHA256SUMS.txt` holds the SHA-256 of its zips. Compare them with the output of `Get-FileHash .\Verdandi-<version>-x64.zip` in PowerShell.
- The release's `-symbols.zip` holds the program database, which is needed only to analyze crash reports.

## ⚙️ Settings, log and command line

- **Settings** are saved to `settings.json` whenever you change them in File > Settings. Without the file, Verdandi starts with the defaults.
- **The log** is written to `verdandi.log`, and rotated at 5 MB.
- **Where they are kept:** while a `settings.json` lies beside `verdandi.exe`, as in the release's zip, Verdandi is portable: it keeps both files in that folder and writes nothing to your user folders. Otherwise, as in a build from source, they are in `%APPDATA%\Verdandi` and `%LOCALAPPDATA%\Verdandi`. Delete the `settings.json` beside `verdandi.exe` to use your user folders, or copy yours from `%APPDATA%\Verdandi` beside `verdandi.exe` to take your settings along.
- **`--log-level <level>`** logs at `trace`, `debug`, `info`, `warn`, `error`, `critical` or `off` for one run, in place of the level in the settings, without saving it. While Verdandi runs, a new start brings its window to the front and does not use the option.
- **`--version`** shows the version and the commit it was built from, as Help > About Verdandi does.

## 🔨 Building from source

### 🧱 Prerequisites

- **Windows 10** (version 2004+) or **Windows 11**
- **Visual Studio 2022 or 2026** with the "Desktop development with C++" workload, **or** the standalone [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/) (for VS Code users)
- **C++ Clang tools for Windows**, a component in the Visual Studio Installer. The coverage build needs it, and it brings the clang-format that `.\scripts\build.ps1 -Format` runs before each commit (CI fails on unformatted code).
- **CMake 3.25+**
- **Ninja**
- **vcpkg** (bundled with Visual Studio or standalone) with `VCPKG_ROOT` set
- **Git**

All dependencies (Qt 6, spdlog, GoogleTest, nlohmann/json) are managed by vcpkg and downloaded automatically during project configuration.

### 🚀 Quick start (Automated)

The fastest way to get started is using the repository scripts. From PowerShell:

```powershell
# Check or install missing tools, locate/bootstrap vcpkg, and persist environment variables:
powershell -ExecutionPolicy Bypass -File .\scripts\bootstrap.ps1 -InstallMissing -PersistEnvironment

# Build from any terminal (automatically initializes the x64 MSVC environment):
.\scripts\build.ps1

# Build and run tests:
.\scripts\build.ps1 -Test
```

The tests' results and coverage on `main`, and their trend, are on the [test and coverage dashboard](https://dundergg.github.io/verdandi/), which CI publishes after each push to `main`.

### Manual build

If you prefer building manually without scripts:

1. Open your Visual Studio version's **Developer PowerShell** or **x64 Native Tools Command Prompt** (so `cl.exe` is active on `PATH`).
2. Ensure `VCPKG_ROOT` is set in your session (e.g. `$env:VCPKG_ROOT = "C:\vcpkg"` or to Visual Studio's `VC\vcpkg`).
3. Run:

```powershell
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The first build will compile dependencies (including Qt 6) from source. Set up [vcpkg binary caching](https://learn.microsoft.com/en-us/vcpkg/users/binarycaching) to speed up subsequent clean builds.

See [CONTRIBUTING.md](CONTRIBUTING.md) for full manual setup details, IDE configuration, and troubleshooting.

## How it works

Verdandi reads every metric directly from Windows' native APIs, with no PDH counters, WMI polling, or Tool Help snapshots in between. Background threads collect the metrics and hand the UI immutable snapshots, so the window stays responsive.

See [architecture.md](docs/architecture.md) for the full technical design, including [the Windows API behind each metric](docs/architecture.md#windows-api-choices).

## Roadmap

See [roadmap.md](roadmap.md) for the detailed implementation plan. Upcoming features include:

- System tray integration
- Alert rules and notifications
- GPU monitoring
- ETW-powered diagnostics
- Data export (CSV/JSON)

## The name

Verdandi (Old Norse *Verðandi*) is one of the three Norns of Norse mythology. Her name comes from the verb *verða*, "to become", and is often read as "what is happening now": the present, beside Urd, what has been, and Skuld, what shall be. A monitor shows what a system is doing now, so it carries her name, and its icon draws her V as a chart line.

## 🤝 Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for setup instructions, code style guidelines, and how to submit changes.

## 👤 Author

**David Bennehag** - [@DunderGG](https://github.com/DunderGG) - [dunder.gg](https://dunder.gg)

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE). See the [LICENSE](LICENSE) file for the full text.

