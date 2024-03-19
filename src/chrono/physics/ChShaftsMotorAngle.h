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

#ifndef CHSHAFTSMOTORANGLE_H
#define CHSHAFTSMOTORANGLE_H

#include "chrono/physics/ChShaftsMotor.h"
#include "chrono/solver/ChConstraintTwoGeneric.h"
#include "chrono/functions/ChFunction.h"

namespace chrono {

/// Motor to enforce the rotation angle r(t) between two shafts, using a rheonomic constraint.
/// The angle of shaft A with respect to shaft B is set trhough a function of time f(t) and an optional angle offset:
///    r(t) = f(t) + offset
/// Note: no compliance is allowed, so if the actuator hits an undeformable obstacle it hits a pathological situation
/// and the solver result can be unstable/unpredictable. Think at it as a servo drive with "infinitely stiff" control.
/// This type of motor is very easy to use, stable and efficient and should be used if the 'infinitely stiff' control
/// assumption  is a good approximation of what you simulate (e.g., very good and reactive controllers). By default it
/// is initialized with linear ramp: df/dt = 1. Use SetAngleFunction() to change to other motion functions.
class ChApi ChShaftsMotorAngle : public ChShaftsMotor {
  public:
    ChShaftsMotorAngle();
    ChShaftsMotorAngle(const ChShaftsMotorAngle& other);
    ~ChShaftsMotorAngle() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChShaftsMotorAngle* Clone() const override { return new ChShaftsMotorAngle(*this); }

    /// Set the rotation angle function f(t), in rad.
    /// Note that is must be at least C0 continuous. Ideally, f is C1, otherwise it produces spikes in accelerations.
    void SetAngleFunction(const std::shared_ptr<ChFunction> function) { f_rot = function; }

    /// Gets the rotation angle function f(t).
    std::shared_ptr<ChFunction> GetAngleFunction() const { return f_rot; }

    /// Get the initial angle offset for f(t)=0, in rad (default: 0).
    /// Rotation of the two axes will be r(t) = f(t) + offset.
    void SetAngleOffset(double mo) { rot_offset = mo; }

    /// Get initial offset for f(t)=0, in rad.
    double GetAngleOffset() const { return rot_offset; }

    /// Initialize the motor, given two shafts to join.
    /// The first shaft is the 'output' shaft of the motor, the second is the 'truss', often fixed and not rotating.
    /// The torque is applied to the output shaft, while the truss shafts gets the same torque but with opposite sign.
    /// Both shafts must belong to the same ChSystem.
    bool Initialize(std::shared_ptr<ChShaft> shaft_1,  ///< first shaft to join (motor output shaft)
                    std::shared_ptr<ChShaft> shaft_2   ///< second shaft to join (motor truss)
                    ) override;

    /// Get the current motor torque between shaft2 and shaft1, expressed as applied to shaft1.
    virtual double GetMotorTorque() const override { return motor_torque; }

    /// Return current constraint violation.
    double GetConstraintViolation() const { return violation; }

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;

  private:
    std::shared_ptr<ChFunction> f_rot;
    double rot_offset;

    double violation;                   ///< constraint violation
    double motor_torque;                ///< motor torque
    ChConstraintTwoGeneric constraint;  ///< used as an interface to the solver

    virtual unsigned int GetNumConstraintsBilateral() override { return 1; }

    virtual void Update(double mytime, bool update_assets = true) override;

    virtual void IntStateGatherReactions(const unsigned int off_L, ChVectorDynamic<>& L) override;
    virtual void IntStateScatterReactions(const unsigned int off_L, const ChVectorDynamic<>& L) override;
    virtual void IntLoadResidual_CqL(const unsigned int off_L,
                                     ChVectorDynamic<>& R,
                                     const ChVectorDynamic<>& L,
                                     const double c) override;
    virtual void IntLoadConstraint_C(const unsigned int off,
                                     ChVectorDynamic<>& Qc,
                                     const double c,
                                     bool do_clamp,
                                     double recovery_clamp) override;
    virtual void IntLoadConstraint_Ct(const unsigned int off, ChVectorDynamic<>& Qc, const double c) override;
    virtual void IntToDescriptor(const unsigned int off_v,
                                 const ChStateDelta& v,
                                 const ChVectorDynamic<>& R,
                                 const unsigned int off_L,
                                 const ChVectorDynamic<>& L,
                                 const ChVectorDynamic<>& Qc) override;
    virtual void IntFromDescriptor(const unsigned int off_v,
                                   ChStateDelta& v,
                                   const unsigned int off_L,
                                   ChVectorDynamic<>& L) override;

    virtual void InjectConstraints(ChSystemDescriptor& mdescriptor) override;
    virtual void ConstraintsBiReset() override;
    virtual void ConstraintsBiLoad_C(double factor = 1, double recovery_clamp = 0.1, bool do_clamp = false) override;
    virtual void ConstraintsBiLoad_Ct(double factor = 1) override;
    virtual void ConstraintsLoadJacobians() override;
    virtual void ConstraintsFetch_react(double factor = 1) override;
};

CH_CLASS_VERSION(ChShaftsMotorAngle, 0)

}  // end namespace chrono

#endif
