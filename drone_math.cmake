# CMake configuration for drone mathematical operations
# Supporting G++ compiler with C++17 standard

# Force G++ compiler on all platforms
find_program(GXX_COMPILER NAMES g++-13 g++-12 g++-11 g++ PATHS
    /usr/bin
    /usr/local/bin
    /opt/homebrew/bin
    /opt/local/bin
)

find_program(GCC_COMPILER NAMES gcc-13 gcc-12 gcc-11 gcc PATHS
    /usr/bin
    /usr/local/bin
    /opt/homebrew/bin
    /opt/local/bin
)

if(GXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${GXX_COMPILER})
    message(STATUS "Forcing G++ compiler: ${GXX_COMPILER}")
else()
    message(FATAL_ERROR "G++ compiler not found. Please install G++")
endif()

if(GCC_COMPILER)
    set(CMAKE_C_COMPILER ${GCC_COMPILER})
    message(STATUS "Forcing GCC compiler: ${GCC_COMPILER}")
else()
    message(FATAL_ERROR "GCC compiler not found. Please install GCC")
endif()

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable color diagnostics to avoid compatibility issues
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Remove any problematic color diagnostic flags
string(REPLACE "-fcolor-diagnostics" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-fno-color-diagnostics" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-fdiagnostics-color=always" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-fdiagnostics-color" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Compiler-specific options for G++
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # G++ optimization flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

    # Enable vectorization for mathematical operations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsimd-cost-model=unlimited")

    # Thread safety for drone swarm operations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    message(STATUS "Configured for G++ ${CMAKE_CXX_COMPILER_VERSION} with C++17")
endif()

# Link standard math library
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")

# Try to find Eigen3 for linear algebra operations
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIR}")
    add_compile_definitions(HAVE_EIGEN3)
    include_directories(${EIGEN3_INCLUDE_DIR})
endif()

# Additional libraries for advanced mathematical operations
find_library(BLAS_LIB blas)
if(BLAS_LIB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lblas")
    message(STATUS "Found BLAS library: ${BLAS_LIB}")
endif()

find_library(LAPACK_LIB lapack)
if(LAPACK_LIB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -llapack")
    message(STATUS "Found LAPACK library: ${LAPACK_LIB}")
endif()

# Platform-specific optimizations
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(PLATFORM_LINUX)
    # Linux-specific real-time capabilities
    find_library(RT_LIB rt)
    if(RT_LIB)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lrt")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(PLATFORM_MACOS)
    # macOS-specific frameworks
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Accelerate")
        message(STATUS "Using macOS Accelerate framework")
    endif()
endif()

# Compiler definitions for drone operations
add_compile_definitions(
    DRONE_MATH_PRECISION=double
    DRONE_COORDINATION_ENABLED=1
)

# Debug and Release configurations
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG_DRONE_OPERATIONS")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -DOPTIMIZED_DRONE_MATH")

# Final cleanup to ensure no color diagnostic flags remain
foreach(flag_var CMAKE_CXX_FLAGS CMAKE_C_FLAGS
                 CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                 CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
    if(DEFINED ${flag_var})
        string(REPLACE "-fcolor-diagnostics" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "-fno-color-diagnostics" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "-fdiagnostics-color=always" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "-fdiagnostics-color" "" ${flag_var} "${${flag_var}}")
    endif()
endforeach()

message(STATUS "Drone mathematics configuration loaded for G++ with C++17")
