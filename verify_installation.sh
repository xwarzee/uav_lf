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
