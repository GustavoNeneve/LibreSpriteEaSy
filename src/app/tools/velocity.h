// LibreSprite
// Copyright (C) 2024  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "gfx/point.h"

namespace app {
namespace tools {

// Tracks the velocity (pixel delta per movement event) of the pointer.
// The raw delta is computed each time update() is called, and an
// exponential moving-average (EMA) smoothed estimate is also maintained.
class VelocityVector {
public:
  // alpha: EMA smoothing factor in (0, 1].
  //   1.0 = no smoothing (raw delta only)
  //   0.1 = heavy smoothing
  explicit VelocityVector(double alpha = 0.5)
    : m_alpha(alpha)
    , m_velocity(0.0, 0.0)
  {
  }

  // Reset to zero velocity (call when starting a new stroke).
  void reset()
  {
    m_velocity = gfx::PointF(0.0, 0.0);
  }

  // Feed a new raw delta (currentPoint - previousPoint).
  void update(const gfx::Point& delta)
  {
    gfx::PointF raw(static_cast<double>(delta.x),
                    static_cast<double>(delta.y));
    m_velocity = gfx::PointF(
      m_alpha * raw.x + (1.0 - m_alpha) * m_velocity.x,
      m_alpha * raw.y + (1.0 - m_alpha) * m_velocity.y);
  }

  // Returns the smoothed velocity vector.
  const gfx::PointF& velocity() const { return m_velocity; }

  // Returns the magnitude (speed) of the smoothed velocity.
  double speed() const
  {
    return std::sqrt(m_velocity.x * m_velocity.x +
                     m_velocity.y * m_velocity.y);
  }

private:
  double      m_alpha;
  gfx::PointF m_velocity;
};

} // namespace tools
} // namespace app
