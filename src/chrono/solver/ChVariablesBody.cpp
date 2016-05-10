//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010 Alessandro Tasora
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#include "chrono/solver/ChVariablesBody.h"

namespace chrono {

    ChVariablesBody& ChVariablesBody::operator=(const ChVariablesBody& other) {
    if (&other == this)
        return *this;

    // copy parent class data
    ChVariables::operator=(other);

    // copy class data
    user_data = other.user_data;

    return *this;
}

// Register into the object factory, to enable run-time
// dynamic creation and persistence
ChClassRegisterABSTRACT<ChVariablesBody> a_registration_ChVariablesBody;

}  // end namespace chrono
