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
// Authors: Alessandro Tasora
// =============================================================================

#include "chrono/physics/ChShaftsMotorAngle.h"

namespace chrono {


// Register into the object factory, to enable run-time dynamic creation and persistence
CH_FACTORY_REGISTER(ChShaftsMotorAngle)

ChShaftsMotorAngle::ChShaftsMotorAngle() : rot_offset(0), violation(0), motor_torque(0) {
    // default motion function : a ramp
    this->f_rot = chrono_types::make_shared<ChFunctionRamp>(
        0.0,   // default y(0)
        1.0    // default dy/dx , i.e.   1 [rad/s]
        );
    
}

ChShaftsMotorAngle::ChShaftsMotorAngle(const ChShaftsMotorAngle& other)
    : ChShaftsMotorBase(other), violation(0), motor_torque(0) {
    this->f_rot = other.f_rot;
    this->rot_offset = other.rot_offset;
}

bool ChShaftsMotorAngle::Initialize(std::shared_ptr<ChShaft> mshaft1, std::shared_ptr<ChShaft> mshaft2) {
    // Parent class initialize
    if (!ChShaftsMotorBase::Initialize(mshaft1, mshaft2))
        return false;

    ChShaft* mm1 = mshaft1.get();
    ChShaft* mm2 = mshaft2.get();

    constraint.SetVariables(&mm1->Variables(), &mm2->Variables());

    SetSystem(shaft1->GetSystem());

    return true;
}

void ChShaftsMotorAngle::Update(double mytime, bool update_assets) {
    // Inherit time changes of parent class
    ChShaftsMotorBase::Update(mytime, update_assets);

    // Update class data
    this->f_rot->Update(mytime); // call callbacks if any
    violation = GetMotorAngle() - f_rot->GetVal(mytime) - rot_offset;
}

//// STATE BOOKKEEPING FUNCTIONS

void ChShaftsMotorAngle::IntStateGatherReactions(const unsigned int off_L, ChVectorDynamic<>& L) {
        L(off_L) =  motor_torque;
}

void ChShaftsMotorAngle::IntStateScatterReactions(const unsigned int off_L, const ChVectorDynamic<>& L) {
        motor_torque =  L(off_L);
}

void ChShaftsMotorAngle::IntLoadResidual_CqL(const unsigned int off_L,    // offset in L multipliers
                                        ChVectorDynamic<>& R,        // result: the R residual, R += c*Cq'*L
                                        const ChVectorDynamic<>& L,  // the L vector
                                        const double c               // a scaling factor
                                        ) {
    constraint.MultiplyTandAdd(R, L(off_L) * c);
}

void ChShaftsMotorAngle::IntLoadConstraint_C(const unsigned int off_L,  // offset in Qc residual
                                        ChVectorDynamic<>& Qc,     // result: the Qc residual, Qc += c*C
                                        const double c,            // a scaling factor
                                        bool do_clamp,             // apply clamping to c*C?
                                        double recovery_clamp      // value for min/max clamping of c*C
                                        ) {
    double res = c * (GetMotorAngle()  - this->f_rot->GetVal(this->GetChTime()) - this->rot_offset);

    if (do_clamp) {
        res = std::min(std::max(res, -recovery_clamp), recovery_clamp);
    }
    Qc(off_L) += res;
}

void ChShaftsMotorAngle::IntLoadConstraint_Ct(const unsigned int off_L,  // offset in Qc residual
                                         ChVectorDynamic<>& Qc,     // result: the Qc residual, Qc += c*Ct
                                         const double c             // a scaling factor
                                         ) {
    double ct = - this->f_rot->GetDer(this->GetChTime());
    Qc(off_L) += c * ct;
}

void ChShaftsMotorAngle::IntToDescriptor(const unsigned int off_v,  // offset in v, R
                                    const ChStateDelta& v,
                                    const ChVectorDynamic<>& R,
                                    const unsigned int off_L,  // offset in L, Qc
                                    const ChVectorDynamic<>& L,
                                    const ChVectorDynamic<>& Qc) {
     constraint.Set_l_i(L(off_L));
     constraint.Set_b_i(Qc(off_L));
}

void ChShaftsMotorAngle::IntFromDescriptor(const unsigned int off_v,  // offset in v
                                      ChStateDelta& v,
                                      const unsigned int off_L,  // offset in L
                                      ChVectorDynamic<>& L) {
    L(off_L) = constraint.Get_l_i();
}

// SOLVER INTERFACES

void ChShaftsMotorAngle::InjectConstraints(ChSystemDescriptor& mdescriptor) {

    mdescriptor.InsertConstraint(&constraint);
}

void ChShaftsMotorAngle::ConstraintsBiReset() {

     constraint.Set_b_i(0.);
}

void ChShaftsMotorAngle::ConstraintsBiLoad_C(double factor, double recovery_clamp, bool do_clamp) {

    double res = factor * (GetMotorAngle()  - this->f_rot->GetVal(this->GetChTime()) - this->rot_offset);

    constraint.Set_b_i(constraint.Get_b_i() + factor * res);
}

void ChShaftsMotorAngle::ConstraintsBiLoad_Ct(double factor) {

    double ct = - this->f_rot->GetDer(this->GetChTime());
    constraint.Set_b_i(constraint.Get_b_i() + factor * ct);
}

void ChShaftsMotorAngle::ConstraintsLoadJacobians() {
    constraint.Get_Cq_a()(0) = 1;
    constraint.Get_Cq_b()(0) = -1;
}

void ChShaftsMotorAngle::ConstraintsFetch_react(double factor) {
    motor_torque = - constraint.Get_l_i() * factor;
}

//////// FILE I/O

void ChShaftsMotorAngle::ArchiveOut(ChArchiveOut& archive_out) {
    // version number
    archive_out.VersionWrite<ChShaftsMotorAngle>();

    // serialize parent class
    ChShaftsMotorBase::ArchiveOut(archive_out);

    // serialize all member data:
    archive_out << CHNVP(motor_torque);
    archive_out << CHNVP(this->rot_offset);
    archive_out << CHNVP(this->f_rot);

}

/// Method to allow de serialization of transient data from archives.
void ChShaftsMotorAngle::ArchiveIn(ChArchiveIn& archive_in) {
    // version number
    /*int version =*/ archive_in.VersionRead<ChShaftsMotorAngle>();

    // deserialize parent class:
    ChShaftsMotorBase::ArchiveIn(archive_in);

    // deserialize all member data:
    archive_in >> CHNVP(motor_torque);
    archive_in >> CHNVP(this->rot_offset);
    archive_in >> CHNVP(this->f_rot);
    constraint.SetVariables(&shaft1->Variables(), &shaft2->Variables());
}




}  // end namespace chrono
