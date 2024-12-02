#pragma once
#include <string>

#include "CImg.h"
#include "glm/vec3.hpp"
//#include "glm/detail/func_geometric.inl"
using namespace cimg_library;
class NormalMap
{
private:
 const double intensity(const float pPixel[3])
 {
  const double r = static_cast<double>(pPixel[0]);
  const double g = static_cast<double>(pPixel[1]);
  const double b = static_cast<double>(pPixel[2]);

  const double average = (r + g + b) / 3.0;

  return average / 255.0;
 }

 const int clamp(int pX, int pMax)
 {
  if (pX > pMax)
  {
   return pMax;
  }
  else if (pX < 0)
  {
   return 0;
  }
  else
  {
   return pX;
  }
 }

 // transform -1 - 1 to 0 - 255
 const uint8_t map_component(double pX)
 {
  return (pX + 1.0) * (255.0 / 2.0);
 }
public:
 void create_normal_from_height(std::string heightmapPath, double pStrength = 2.0)
 {
  CImg<float> heightmap;
  heightmap.load(heightmapPath.c_str());
  CImg<float> normalmap;
  normalmap.assign((float)heightmap.width(),(float)heightmap.height(), 1, 4); //RGBA - Normal map on RGB, height on A
  for (size_t row = 0; row < normalmap.width(); ++row)
  {
   for (size_t column = 0; column < normalmap.height(); ++column)
   {
    // surrounding pixels heights
    float topLeft     = heightmap.atXY(clamp(row - 1, normalmap.width()), clamp(column - 1, normalmap.height()),0,0);
    float top         = heightmap.atXY(clamp(row - 1, normalmap.width()), clamp(column,     normalmap.height()),0,0);
    float topRight    = heightmap.atXY(clamp(row - 1, normalmap.width()), clamp(column + 1, normalmap.height()),0,0);
    float right       = heightmap.atXY(clamp(row,     normalmap.width()), clamp(column + 1, normalmap.height()),0,0);
    float bottomRight = heightmap.atXY(clamp(row + 1, normalmap.width()), clamp(column + 1, normalmap.height()),0,0);
    float bottom      = heightmap.atXY(clamp(row + 1, normalmap.width()), clamp(column,     normalmap.height()),0,0);
    float bottomLeft  = heightmap.atXY(clamp(row + 1, normalmap.width()), clamp(column - 1, normalmap.height()),0,0);
    float left        = heightmap.atXY(clamp(row,     normalmap.width()), clamp(column - 1, normalmap.height()),0,0);

    // their intensities
    const double tl = topLeft    /255.0;
    const double t  = top        /255.0;
    const double tr = topRight   /255.0;
    const double r  = right      /255.0;
    const double br = bottomRight/255.0;
    const double b  = bottom     /255.0;
    const double bl = bottomLeft /255.0;
    const double l  = left       /255.0;

    // sobel filter
    const double dX = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
    const double dY = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
    const double dZ = 1.0 / pStrength;

    glm::vec3 vec(dX,dY,dZ);
    //math::vector3d v(dX, dY, dZ);
    //v.normalize();
    vec = glm::normalize(vec);

    // convert to rgb
    const float color[4](map_component(vec.x), map_component(vec.y), map_component(vec.z),heightmap.atXY(column,row, 0,0));
    //result(row, column) = pixel(map_component(vec.x), map_component(vec.y), map_component(vec.z));
    normalmap.draw_point(row,column,0,color);
   }
  }
  normalmap.save_png("heightmaps/normalmap.png",4);
 }   
};
