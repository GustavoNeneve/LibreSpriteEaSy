// LibreSprite
// Copyright (C) 2024  LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

namespace app {
namespace tools {

// Options for the Dynamics / Stabilizer panel.
//
// stabilizerFactor controls the smoothing intensity of the Stabilizer:
//   1  = no smoothing  (stabilizer center jumps directly to the cursor)
//   16 = default (recommended) value
//   >16 = progressively more inertia / lag
//
// When stabilizer is false (or stabilizerFactor <= 1) the Stabilizer is
// effectively disabled: raw pointer positions are passed straight through.
struct DynamicsOptions {
  // Whether the stroke-smoothing stabilizer is active.
  bool stabilizer = false;

  // Smoothing strength. Must be >= 1.
  // Each movement event, the stabilizer center advances
  //   distance / stabilizerFactor  pixels toward the real cursor.
  int stabilizerFactor = 16;
};

} // namespace tools
} // namespace app
