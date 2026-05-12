#ifndef CORE_TYPES_HPP
#define CORE_TYPES_HPP

// Core module: Basic data types used throughout the project
// This file contains fundamental geometric and mathematical types

namespace core {

// 2D vector
struct Vec2 { 
    float x, y; 
};

// 3D vector
struct Vec3 { 
    float x, y, z; 
};

// Angle representation (pitch, yaw, roll)
struct QAngle { 
    float x, y, z; 
};

// 2D bounding box
struct Box2D { 
    float left, top, right, bottom; 
};

} // namespace core

#endif // CORE_TYPES_HPP
