# Marooned
![Game Screenshot](assets/screenshots/dungeon1.png)
**Marooned** is a 3D FPS adventure dungeon crawler set in the 1700s pirate era on a chain of Caribbean islands. The player is marooned on an island with only a sword, a blunderbuss and a health potion. They have to fight off pirates, go through multiple dungeons and fight a boss to escape.

### Table of Contents
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Features
- Combat with a blunderbuss, sword and a magic staff.
- Multiple enemies with unique AI.
- Dungeons.
- Collectible weapons, potions and gold.
- A boss at the end of the game.

![Gameplay demo](assets/screenshots/demo.gif)
## Installation
### 🪟 Windows
#### Prerequisites
Install:
- [Git](https://git-scm.com/downloads)
- [MinGW-w64](https://www.mingw-w64.org/) or another C++ compiler
- [Raylib 5.5](https://www.raylib.com/)

#### Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/Jhyde927/Marooned.git

2. Navigate to the repository:
```bash
cd Marooned
```

3. Build the project with Make:
```bash
make
```
4. Run Marooned.exe

## Usage
The `assets` folder and all `.dll`/`.so`/`.a` files must be in the same folder in which the executable is run.
  

### Linux
#### Prerequisites
You must install git, make (or cmake), a c++ compiler and raylib 5.5.
#### Install Steps
1. Clone the repository:
```bash
git clone https://github.com/Jhyde927/Marooned.git
```

2. Navigate to the repository:
```bash
cd Marooned
```

3. Build the project with Make:
```bash
make
```
The executable will be found in `Marooned.exe` or `Marooned`, depending on your OS. Or, alternatively, build with CMake for Linux:
```bash
cd build
cmake ..
make
```
And install library files (`.so`, `.a`) if necessary:
```bash
make install
```
If building with CMake, the executable will be found in `build/marooned`. The CMake file is strictly for Linux builds. If something didn't work, please open an issue and describe what went wrong.

## Usage
Simply run the executable file after building. The `assets` folder and all `.dll`/`.so`/`.a` files must be in the same folder in which the executable is run.

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
