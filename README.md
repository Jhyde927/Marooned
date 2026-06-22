# Marooned
![Game Screenshot](assets/screenshots/dungeon1.png)

**Marooned** is a 3D first person adventure game set in the 1700s pirate era on a island full of dinosaurs. Below the islands are dungeons crawling with skeletons, spiders, and worse. Armed with only a rusty sword and your trusty blunderbuss, make your way through the dungeons to fight the boss at the end of the demo. 

### Table of Contents
- [Features](#features)
- [Installation](#installation)
- - [Cloning](#cloning)
- - [CMake](#cmake)
- - [Make](#make)
- - [Distribution](#for-distribution)
- [Contributing](#contributing)
- [License](#license)

## Features
- Fully 3D environments.
- 2D "billboard" enemies with unique AI.
- Over world island maps generated from 4k grayscale images. 
- Dungeons generated from PNG image pixel by pixel.
- Combat with a blunderbuss, sword and a magic staff.
- Collectible weapons, potions and gold.
- Multiple boss fights. 

![Gameplay demo](assets/screenshots/demo.gif)
## Installation
### Prerequisites
- [Git](https://git-scm.com/downloads)
- [CMake](https://cmake.org/download/) or [Make](https://gnuwin32.sourceforge.net/packages/make.htm)
- A C++17 compiler, like [MinGW](https://www.mingw-w64.org/) or [GCC](https://gcc.gnu.org/)
- [Raylib 5.5](https://www.raylib.com/) (if using Make)

### Cloning
```bash
git clone https://github.com/Jhyde927/Marooned.git
cd Marooned
```
### CMake
Raylib is fetched automatically. This way of building is cross-platform.
```bash
mkdir build
cmake -B build
cmake --build build
```
And then run with:
```bash
./build/Marooned
```
### Make
For building with Make you must install Raylib yourself. Build with:
```bash
make
```
And then run with:
```bash
./Marooned
```
### For Distribution
For Windows all DLLs are found in dlls/win64/. Package everything with:
```bash
./windows_build
```
For Linux run:
```bash
./linux_build.sh
```
You might have to run this command beforehand if you get permission errors or an error that the file is not executable. This is a single use command:
```bash
chmod +x linux_build.sh
```

## Contributing
Feel free to create PRs or issues. To create a PR:

1. Fork the repository.
2. Create a new branch:
```bash
git checkout -b feature-name
```
3. Make your changes.
4. Push your branch:
```bash
git push origin feature-name
```
5. Create a pull request and describe made changes.

## License
This project is licensed under the [MIT License](LICENSE.txt). Feel free to use, copy, modify, distribute and sell this project.
