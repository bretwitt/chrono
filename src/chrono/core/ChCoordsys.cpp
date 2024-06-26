// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================

#include "chrono/core/ChCoordsys.h"

namespace chrono {

const ChCoordsysd CSYSNULL(ChVector3d(0, 0, 0), ChQuaterniond(0, 0, 0, 0));
const ChCoordsysd CSYSNORM(ChVector3d(0, 0, 0), ChQuaterniond(1, 0, 0, 0));

}  // end namespace chrono
