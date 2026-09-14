<p align="center">
  <img src="docs/icon.png" width="128" height="128" alt="Felsic Notes Icon">
</p>

<h1 align="center">Felsic Notes</h1>

> **Frictionless notes. Pure productivity.**

Felsic Notes is a local-first, lightweight Markdown editor engineered for speed and portability. Originally built in Python, it is now entirely powered by **modern C++ and Qt6**, delivering a truly native experience that keeps pace with your thoughts—not the other way around.

![Felsic Notes Interface](docs/window.png)

This project does not aim to be a fully featured alternative to Obsidian; rather, it is a streamlined sidekick for your daily note-taking, designed to be fast, portable, and distraction-free.

## Key Features

- **Truly Portable**: Run Felsic from anywhere—USB drives, cloud folders (OneDrive, Google Drive), or your desktop. Zero installation required on Linux (AppImage) and a clean per-user install on Windows.
- **Native Performance**: Built with C++ and Qt6 for millisecond startup times and optimized memory usage (typically under 60MB RAM).
- **Obsidian Companion**: Seamlessly edit your Obsidian vaults without interference. Use both editors simultaneously without conflicts.
- **Pure Markdown**: Your data is yours. Notes are saved as standard `.md` files with no proprietary formats or vendor lock-in.
- **Lightning-Fast Search**: Find any note instantly with a local search engine optimized for extreme speed.
- **Multiplatform**: A consistent, native experience across Windows, macOS, and Linux.

## Getting Started

1. **Download**: Grab the latest version for your OS from the [Releases](https://github.com/deomkds/felsic-notes/releases) page. The builds are generated automatically by our CI/CD pipeline!
   - **Windows**: Download the `.msi` file. It installs safely without requiring administrator privileges.
   - **Linux**: Download the `.AppImage` file, make it executable, and double-click to run.
2. **Launch**: Open the application.
3. **Select Folder**: Point Felsic to your notes folder or Obsidian vault and start writing.

## Roadmap

- [ ] Multi-language support (Translations)
- [ ] Global search (Search inside note contents)
- [ ] Improved keyboard navigation

## License

Felsic Notes is free software licensed under the GPL-3.0. See the [LICENSE](https://github.com/deomkds/felsic-notes/blob/main/LICENSE) file for details.