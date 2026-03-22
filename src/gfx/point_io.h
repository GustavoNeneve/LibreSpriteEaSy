// Aseprite Gfx Library
// Copyright (C) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#pragma once

#include "gfx/point.h"
#include <iosfwd>

namespace gfx {

  template<typename T>
  inline std::ostream& operator<<(std::ostream& os, const PointT<T>& point)
  {
    return os << "(" << point.x << ", " << point.y << ")";
  }

}
