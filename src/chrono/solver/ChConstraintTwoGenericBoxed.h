//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010 Alessandro Tasora
// Copyright (c) 2013 Project Chrono
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHCONSTRAINTTWOGENERICBOXED_H
#define CHCONSTRAINTTWOGENERICBOXED_H

#include "chrono/solver/ChConstraintTwoGeneric.h"

namespace chrono {

/// This class is inherited from the base ChConstraintTwoGeneric,
/// which can be used for most pairwise constraints, and adds the
/// feature that the multiplier must be
///        l_min < l < l_max
/// that is, the multiplier is 'boxed'.
/// Note that, if l_min = 0 and l_max = infinite, this can work
/// also as an unilateral constraint..
///  Before starting the solver one must provide the proper
/// values in constraints (and update them if necessary), i.e.
/// must set at least the c_i and b_i values, and jacobians.

class ChApi ChConstraintTwoGenericBoxed : public ChConstraintTwoGeneric {
    CH_RTTI(ChConstraintTwoGenericBoxed, ChConstraintTwoGeneric)

    //
    // DATA
    //

  protected:
    double l_min;
    double l_max;

  public:
    //
    // CONSTRUCTORS
    //
    /// Default constructor
    ChConstraintTwoGenericBoxed() {
        l_min = -1.;
        l_max = 1.;
    }

    /// Construct and immediately set references to variables
    ChConstraintTwoGenericBoxed(ChVariables* mvariables_a, ChVariables* mvariables_b)
        : ChConstraintTwoGeneric(mvariables_a, mvariables_b) {
        l_min = -1.;
        l_max = 1.;
    }

    /// Copy constructor
    ChConstraintTwoGenericBoxed(const ChConstraintTwoGenericBoxed& other) : ChConstraintTwoGeneric(other) {
        l_min = other.l_min;
        l_max = other.l_max;
    }

    virtual ~ChConstraintTwoGenericBoxed(){};

    virtual ChConstraintTwoGenericBoxed* new_Duplicate() { return new ChConstraintTwoGenericBoxed(*this); }

    /// Assignment operator: copy from other object
    ChConstraintTwoGenericBoxed& operator=(const ChConstraintTwoGenericBoxed& other) {
        if (&other == this)
            return *this;

        // copy parent class data
        ChConstraintTwoGeneric::operator=(other);

        l_min = other.l_min;
        l_max = other.l_max;

        return *this;
    }

    //
    // FUNCTIONS
    //
    /// Set lower/upper limit for the multiplier.
    void SetBoxedMinMax(double mmin, double mmax) {
        assert(mmin <= mmax);
        l_min = mmin;
        l_max = mmax;
    }

    /// Get the lower limit for the multiplier
    double GetBoxedMin() { return l_min; }
    /// Get the upper limit for the multiplier
    double GetBoxedMax() { return l_max; }

    /// For iterative solvers: project the value of a possible
    /// 'l_i' value of constraint reaction onto admissible orthant/set.
    /// This 'boxed implementation overrides the default do-nothing case.
    virtual void Project() {
        if (l_i < l_min)
            l_i = l_min;
        if (l_i > l_max)
            l_i = l_max;
    }

    /// Given the residual of the constraint computed as the
    /// linear map  mc_i =  [Cq]*q + b_i + cfm*l_i , returns the
    /// violation of the constraint, considering inequalities, etc.
    virtual double Violation(double mc_i);

    //
    // STREAMING
    //

    /// Method to allow deserializing a persistent binary archive (ex: a file)
    /// into transient data.
    virtual void StreamIN(ChStreamInBinary& mstream);

    /// Method to allow serializing transient data into a persistent
    /// binary archive (ex: a file).
    virtual void StreamOUT(ChStreamOutBinary& mstream);
};

}  // end namespace chrono

#endif
