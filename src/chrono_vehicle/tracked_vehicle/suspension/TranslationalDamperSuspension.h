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
// Authors: Radu Serban
// =============================================================================
//
// Torsion-bar suspension system using linear dampers constructed with data from
// file (JSON format)
//
// =============================================================================

#ifndef TRANSLATIONAL_DAMPER_SUSPENSION_H
#define TRANSLATIONAL_DAMPER_SUSPENSION_H

#include "chrono_vehicle/ChApiVehicle.h"
#include "chrono_vehicle/tracked_vehicle/suspension/ChTranslationalDamperSuspension.h"

#include "chrono_thirdparty/rapidjson/document.h"

namespace chrono {
namespace vehicle {

/// @addtogroup vehicle_tracked_suspension
/// @{

/// Torsion-bar suspension system using linear dampers constructed with data from
/// file (JSON format)
class CH_VEHICLE_API TranslationalDamperSuspension : public ChTranslationalDamperSuspension {
  public:
    TranslationalDamperSuspension(const std::string& filename, bool has_shock, bool lock_arm);
    TranslationalDamperSuspension(const rapidjson::Document& d, bool has_shock, bool lock_arm);
    ~TranslationalDamperSuspension();

    /// Return the mass of the arm body.
    virtual double GetArmMass() const override { return m_arm_mass; }
    /// Return the moments of inertia of the arm body.
    virtual const ChVector3d& GetArmInertia() const override { return m_arm_inertia; }
    /// Return a visualization radius for the arm body.
    virtual double GetArmVisRadius() const override { return m_arm_radius; }

    /// Return the free (rest) angle of the spring element.
    virtual double GetSpringRestAngle() const override { return m_spring_rest_angle; }
    /// Return the functor object for the torsional spring torque.
    virtual std::shared_ptr<ChLinkRSDA::TorqueFunctor> GetSpringTorqueFunctor() const override {
        return m_spring_torqueCB;
    }
    /// Return the functor object for the (optional) linear rotational damper.
    virtual std::shared_ptr<ChLinkRSDA::TorqueFunctor> GetDamperTorqueFunctor() const override {
        return m_damper_torqueCB;
    }
    /// Return the functor object for the translational shock force.
    virtual std::shared_ptr<ChLinkTSDA::ForceFunctor> GetShockForceFunctor() const override { return m_shock_forceCB; }

  private:
    virtual const ChVector3d GetLocation(PointId which) override { return m_points[which]; }

    virtual void Create(const rapidjson::Document& d) override;

    double m_spring_rest_angle;
    std::shared_ptr<ChLinkRSDA::TorqueFunctor> m_spring_torqueCB;
    std::shared_ptr<ChLinkRSDA::TorqueFunctor> m_damper_torqueCB;
    std::shared_ptr<ChLinkTSDA::ForceFunctor> m_shock_forceCB;

    ChVector3d m_points[NUM_POINTS];

    double m_arm_mass;
    ChVector3d m_arm_inertia;
    double m_arm_radius;
};

/// @} vehicle_tracked_suspension

}  // end namespace vehicle
}  // end namespace chrono

#endif
