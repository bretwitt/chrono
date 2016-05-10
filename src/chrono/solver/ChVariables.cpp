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

#include "chrono/solver/ChVariables.h"

namespace chrono {

    ChVariables& ChVariables::operator=(const ChVariables& other) {
    if (&other == this)
        return *this;

    this->disabled = other.disabled;

    if (other.qb) {
        if (qb == NULL)
            qb = new ChMatrixDynamic<>;
        qb->CopyFromMatrix(*other.qb);
    } else {
        delete qb;
        qb = NULL;
    }

    if (other.fb) {
        if (fb == NULL)
            fb = new ChMatrixDynamic<>;
        fb->CopyFromMatrix(*other.fb);
    } else {
        delete fb;
        fb = NULL;
    }

    this->ndof = other.ndof;
    this->offset = other.offset;

    return *this;
}

// Register into the object factory, to enable run-time
// dynamic creation and persistence
// ChClassRegister<ChVariables> a_registration_ChVariables;

}  // end namespace chrono
