# Installation Guide for UAV Swarm Coordination System

This guide provides step-by-step instructions to install all required tools and dependencies to build and run the Lingua Franca-based drone swarm coordination system.

## System Requirements

- **Operating System**: Linux (Ubuntu 20.04+, Fedora 35+), macOS (Big Sur+), or Windows with WSL2
- **RAM**: Minimum 4GB, recommended 8GB+
- **Disk Space**: At least 2GB for tools and dependencies
- **Network**: Internet connection for downloading packages

## Installation Steps

### 1. Install Lingua Franca Compiler

The Lingua Franca compiler (lfc) is required to compile .lf files to C++ code.

#### Option A: Install from Source (Recommended)

```bash
# Install prerequisites
# On Ubuntu/Debian:
sudo apt update
sudo apt install git openjdk-17-jdk

# On macOS:
brew install git openjdk@17

# On Fedora/RHEL:
sudo dnf install git java-17-openjdk-devel

# Clone and build Lingua Franca
git clone https://github.com/lf-lang/lingua-franca.git
cd lingua-franca

# Build the compiler (this may take 5-10 minutes)
./gradlew buildLfc

# Add to PATH
export PATH=$PATH:$(pwd)/bin

# Make permanent (choose your shell)
echo 'export PATH=$PATH:'$(pwd)'/bin' >> ~/.bashrc  # For bash
echo 'export PATH=$PATH:'$(pwd)'/bin' >> ~/.zshrc   # For zsh
```

#### Option B: Install via Package Manager

```bash
# macOS with Homebrew
brew install lf-lang/tap/lingua-franca

# Arch Linux
yay -S lingua-franca-git

# For other systems, check: https://github.com/lf-lang/lingua-franca/releases
```

#### Verify Installation
```bash
lfc --version
# Should output: lfc 0.x.x
```

#### Install reactor-cpp Runtime Library

The C++ target requires the reactor-cpp runtime library. Install it after building Lingua Franca:

```bash
# Navigate to the Lingua Franca directory
cd lingua-franca

# Install reactor-cpp dependencies
sudo apt install libboost-all-dev  # Ubuntu/Debian
# OR for other systems:
# sudo dnf install boost-devel      # Fedora/RHEL
# brew install boost               # macOS

# Build and install reactor-cpp
git submodule update --init --recursive
cd cpp
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install

# Update library cache (Linux only)
sudo ldconfig

# Verify installation
find /usr/local -name "*reactor*" -type f 2>/dev/null | head -5
```

### 2. Install G++ C++ Compiler

The system uses G++ (GNU Compiler Collection) for C++17 support and mathematical optimization features. G++ version 11 or later is required.

#### Ubuntu/Debian (20.04+)
```bash
# Install G++ and development tools
sudo apt update
sudo apt install g++ build-essential

# Install specific G++ version (optional)
sudo apt install g++-13 g++-12 g++-11

# Verify installation
g++ --version
```

#### Fedora/RHEL/CentOS
```bash
# Install development tools and G++
sudo dnf groupinstall "Development Tools"
sudo dnf install gcc-c++

# Install specific G++ version (optional)
sudo dnf install gcc-toolset-13-gcc-c++

# Verify installation
g++ --version
```

#### macOS
```bash
# Install G++ via Homebrew
brew install gcc

# This installs g++ alongside gcc
# The actual command may be g++-13, g++-12, etc.

# Verify installation
g++ --version
# or if installed via Homebrew:
g++-13 --version
```

#### Arch Linux
```bash
# G++ is available in the official repositories
sudo pacman -S gcc

# Verify installation
g++ --version
```

#### Windows (WSL2)
```bash
# Using WSL2 with Ubuntu
sudo apt update
sudo apt install g++ build-essential

# Verify installation
g++ --version
```

### 3. Install CMake

CMake 3.16+ is required for building the generated C++ code.

#### Ubuntu/Debian
```bash
sudo apt install cmake

# For newer versions:
sudo apt install software-properties-common
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

#### Fedora/RHEL
```bash
sudo dnf install cmake
```

#### macOS
```bash
brew install cmake
```

#### Arch Linux
```bash
sudo pacman -S cmake
```

### 4. Install Mathematical Libraries (Optional but Recommended)

These libraries provide optimized mathematical operations for drone coordination algorithms.

#### Eigen3 (Linear Algebra Library)
```bash
# Ubuntu/Debian
sudo apt install libeigen3-dev

# Fedora/RHEL
sudo dnf install eigen3-devel

# macOS
brew install eigen

# Arch Linux
sudo pacman -S eigen
```

#### BLAS and LAPACK (High-Performance Math)
```bash
# Ubuntu/Debian
sudo apt install libblas-dev liblapack-dev libatlas-base-dev

# Fedora/RHEL
sudo dnf install blas-devel lapack-devel atlas-devel

# macOS (uses Accelerate framework - already included)
# No additional installation needed

# Arch Linux
sudo pacman -S blas lapack
```

### 5. Install Additional Development Tools

#### Make and Build Tools
```bash
# Ubuntu/Debian
sudo apt install build-essential

# Fedora/RHEL
sudo dnf groupinstall "Development Tools"

# macOS
xcode-select --install

# Arch Linux
sudo pacman -S base-devel
```

#### Git (for version control)
```bash
# Ubuntu/Debian
sudo apt install git

# Fedora/RHEL
sudo dnf install git

# macOS
brew install git  # or use Xcode Command Line Tools

# Arch Linux
sudo pacman -S git
```

## Windows Installation (WSL2)

For Windows users, we recommend using Windows Subsystem for Linux (WSL2):

1. **Install WSL2:**
   ```powershell
   # Run in PowerShell as Administrator
   wsl --install -d Ubuntu-22.04
   ```

2. **Restart** your computer when prompted

3. **Open Ubuntu** from Start Menu

4. **Follow the Ubuntu installation steps** above within WSL2

## Verification Script

Create and run this verification script to ensure all tools are properly installed:

```bash
#!/bin/bash
# save as verify_installation.sh

echo "=== Verifying Installation ==="

# Check Lingua Franca
if command -v lfc &> /dev/null; then
    echo "✓ Lingua Franca: $(lfc --version)"
else
    echo "✗ Lingua Franca: NOT FOUND"
fi

# Check G++
if command -v g++ &> /dev/null; then
    VERSION=$(g++ --version | head -n1)
    echo "✓ G++: $VERSION"
else
    echo "✗ G++: NOT FOUND"
fi

# Check CMake
if command -v cmake &> /dev/null; then
    echo "✓ CMake: $(cmake --version | head -n1)"
else
    echo "✗ CMake: NOT FOUND"
fi

# Check reactor-cpp runtime
if find /usr/local -name "*reactor*" -type f 2>/dev/null | grep -q reactor; then
    echo "✓ reactor-cpp: Found"
else
    echo "✗ reactor-cpp: NOT FOUND (required for C++ target)"
fi

# Check Boost (required by reactor-cpp)
if ldconfig -p 2>/dev/null | grep -q boost || brew list boost &>/dev/null || pkg-config --exists boost; then
    echo "✓ Boost: Found"
else
    echo "✗ Boost: NOT FOUND (required for reactor-cpp)"
fi

# Check mathematical libraries
if pkg-config --exists eigen3 2>/dev/null; then
    echo "✓ Eigen3: Found"
else
    echo "○ Eigen3: Not found (optional)"
fi

if ldconfig -p 2>/dev/null | grep -q blas; then
    echo "✓ BLAS: Found"
else
    echo "○ BLAS: Not found (optional)"
fi

echo "=== Installation Check Complete ==="
```

Run the verification:
```bash
chmod +x verify_installation.sh
./verify_installation.sh
```

## Troubleshooting

### Common Issues

#### 1. "lfc: command not found"
- Ensure Lingua Franca bin directory is in your PATH
- Reload your shell: `source ~/.bashrc` or `source ~/.zshrc`

#### 2. "g++: No such file or directory"
- Install G++: `sudo apt install g++` (Ubuntu) or `brew install gcc` (macOS)
- Verify G++ installation: `which g++`

#### 3. CMake version too old
- Install CMake from official repository (see CMake installation steps above)
- Use snap: `sudo snap install cmake --classic`

#### 4. Java not found during Lingua Franca build
- Install OpenJDK 17: `sudo apt install openjdk-17-jdk`
- Set JAVA_HOME: `export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64`

#### 5. "Could NOT find reactor-cpp" error
- Install Boost library: `sudo apt install libboost-all-dev` (Ubuntu) or `brew install boost` (macOS)
- Build and install reactor-cpp runtime (see reactor-cpp installation steps above)
- Verify installation: `find /usr/local -name "*reactor*" -type f`
- If still not found, set CMAKE_PREFIX_PATH: `export CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH`

#### 6. G++ compilation errors
- Ensure you have C++17 support: `g++ -std=c++17 --version`
- On older systems, install a newer G++ version (11 or later)
- Check that all required headers are available

#### 7. Permission denied errors
- Use `sudo` for system package installations
- Ensure user has write permissions for build directories

### Getting Help

If you encounter issues not covered here:

1. **Check official documentation:**
   - [Lingua Franca Documentation](https://www.lf-lang.org/docs/)
   - [G++ Documentation](https://gcc.gnu.org/releases.html)

2. **Community support:**
   - [Lingua Franca GitHub Issues](https://github.com/lf-lang/lingua-franca/issues)
   - [Stack Overflow](https://stackoverflow.com/questions/tagged/lingua-franca)

3. **System-specific help:**
   - Ubuntu: `man apt` for package management
   - Fedora: `man dnf` for package management
   - macOS: `man brew` for Homebrew usage

## Next Steps

After successful installation, proceed to the main [README.md](./README.md) for build instructions and usage examples.