# UAV Swarm Coordination using Lingua Franca

This project implements a collaborative drone swarm system using the Lingua Franca coordination language with C++ as the target runtime. The system enables multiple UAVs to execute missions cooperatively, maintain formation flight, detect obstacles, and adapt through avoidance or engagement strategies.

## Overview

The system consists of several Lingua Franca reactors that work together to coordinate drone swarm behavior:

- **src/DroneSwarm.lf** - Main coordination reactor managing the entire swarm
- **src/FormationController.lf** - Handles formation flight patterns (diamond, line, V-formation, circle)
- **src/ObstacleDetector.lf** - Detects obstacles and calculates avoidance vectors
- **src/MissionExecutor.lf** - Manages mission execution and waypoint navigation
- **src/DroneComms.lf** - Handles inter-drone communication and mesh networking

## Key Features

### Formation Flight
- Multiple formation types: Diamond, Line, V-Formation, Circle
- Dynamic formation switching during flight
- Collision avoidance between swarm members
- Adaptive spacing based on mission requirements

### Obstacle Detection & Avoidance
- Real-time obstacle detection using sensor data
- Threat classification (Safe, Caution, Danger, Critical)
- Automatic avoidance vector calculation
- Option for threat engagement (defensive operations)

### Mission Management
- Support for various mission types:
  - PATROL - Area surveillance
  - RECONNAISSANCE - Intelligence gathering
  - SEARCH_RESCUE - Emergency response
  - ESCORT - Protection missions
  - CARGO_DELIVERY - Transport operations
  - INTERCEPT - Threat response
  - RTB (Return to Base) - Safe return

### Communication System
- Mesh network topology for resilient communication
- Position sharing between drones
- Emergency broadcast capabilities
- Network health monitoring
- Encryption support for secure operations

## Project Structure

```
uav_lf/
├── src/                          # Lingua Franca source files
│   ├── DroneSwarm.lf            # Main coordination reactor
│   ├── FormationController.lf    # Formation flight control
│   ├── ObstacleDetector.lf      # Obstacle detection and avoidance
│   ├── MissionExecutor.lf       # Mission management
│   └── DroneComms.lf           # Inter-drone communication
├── drone_math.cmake             # CMake configuration for G++ 14
├── README.md                    # This file
├── INSTALL.md                   # Installation instructions
└── src-gen/                     # Generated C++ code (after compilation)
```

## System Architecture

### Reactor Network
```
DroneSwarm (Main)
├── DroneNode instances (individual drones)
│   ├── FormationController
│   ├── ObstacleDetector
│   ├── MissionExecutor
│   └── DroneComms
└── Swarm-level coordination
```

### Data Flow
1. **Position Updates** - Drones broadcast position every 100ms
2. **Formation Commands** - Formation controller calculates target positions
3. **Obstacle Alerts** - Obstacle detector triggers avoidance behaviors
4. **Mission Updates** - Mission executor manages waypoint progression
5. **Communication** - Mesh network maintains swarm connectivity

## Technical Requirements

### Lingua Franca Target
- **Target Language**: C++ (Cpp target)
- **Compiler**: G++ (GNU Compiler Collection)
- **C++ Standard**: C++17

### Dependencies
- Lingua Franca compiler
- G++ compiler (version 11 or later)
- CMake 3.16+
- Mathematical libraries (Eigen3, BLAS/LAPACK optional)

## Build Configuration

The C++ target is specified directly in the Lingua Franca files using:
```lf
target Cpp {
    cmake-include: "../drone_math.cmake"
}
```

The system includes a CMake configuration (`drone_math.cmake`) optimized for:
- G++ compiler features and optimization
- Mathematical optimization flags (-O3 -march=native)
- Vectorization for performance (-ftree-vectorize)
- Platform-specific optimizations (Linux/macOS)

### Compilation Flags
```cmake
-O3 -march=native -ffast-math -funroll-loops
-ftree-vectorize -fsimd-cost-model=unlimited -pthread
```

## Usage

### Compiling with Lingua Franca

1. **Compile the Lingua Franca code to C++:**
```bash
lfc src/DroneSwarm.lf
```

2. **Build the generated C++ code:**
```bash
make -C src-gen/DroneSwarm
```

### Running the Swarm
```bash
./src-gen/DroneSwarm/DroneSwarm
```

**Note:** The target language (C++) is specified within each `.lf` file using the `target Cpp` directive, not as a command-line option to the `lfc` compiler.

## Configuration Parameters

### Drone Configuration
- `drone_id`: Unique identifier for each drone
- `initial_x/y/z`: Starting positions
- `communication_range`: Radio range in meters
- `detection_range`: Sensor range for obstacles

### Formation Parameters
- `formation_type`: 0=diamond, 1=line, 2=V, 3=circle
- `formation_spacing`: Distance between drones
- `collision_threshold`: Minimum separation distance

### Mission Parameters
- `mission_timeout`: Maximum mission duration
- `waypoint_tolerance`: Acceptable distance to waypoint
- `mission_priority`: 0=low to 3=critical

## Safety Features

### Autonomous Safety
- Automatic return-to-base on low battery/fuel
- Emergency abort on system failures
- Collision avoidance with other drones
- Obstacle detection and avoidance

### Communication Security
- Optional message encryption
- Network authentication
- Secure mesh topology
- Emergency broadcast protocols

## Simulation vs Real-World

This implementation includes simulation components for:
- Sensor data generation
- Network communication delays
- Physics modeling
- Obstacle scenarios

For real-world deployment, replace simulation components with:
- Actual sensor interfaces (LIDAR, cameras, radar)
- Radio communication hardware
- Flight control system integration
- GPS/IMU data sources

## Extension Points

The modular reactor design allows easy extension:
- Additional formation patterns
- New mission types
- Enhanced obstacle detection algorithms
- Advanced communication protocols
- Integration with ground control systems

## Legal and Ethical Considerations

This system is designed for:
- ✅ Defensive security operations
- ✅ Search and rescue missions
- ✅ Surveillance and reconnaissance
- ✅ Scientific research applications

**Important**: This code is for defensive and civilian applications only. Users must comply with local regulations regarding UAV operations and ensure proper authorization for any deployment.