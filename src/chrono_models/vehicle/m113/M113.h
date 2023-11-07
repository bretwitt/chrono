// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban
// =============================================================================
//
// Wrapper classes for modeling an entire M113 vehicle assembly
// (including the vehicle itself and the powertrain).
//
// =============================================================================

#ifndef M113_H
#define M113_H

#include <array>
#include <string>

#include "chrono_models/ChApiModels.h"
#include "chrono_models/vehicle/m113/M113_Vehicle.h"

namespace chrono {
namespace vehicle {
namespace m113 {

/// @addtogroup vehicle_models_m113
/// @{

/// Definition of the m113 assembly.
/// This class encapsulates a concrete tracked vehicle model with parameters corresponding to
/// a typical m113 and the powertrain model.
class CH_MODELS_API M113 {
  public:
    M113();
    M113(ChSystem* system);

    ~M113();

    void SetContactMethod(ChContactMethod val) { m_contactMethod = val; }

    void SetChassisFixed(bool val) { m_fixed = val; }
    void SetChassisCollisionType(CollisionType val) { m_chassisCollisionType = val; }
    void SetWheelCollisionType(bool roadwheel_as_cylinder, bool idler_as_cylinder) {
        m_wheel_cyl = roadwheel_as_cylinder;
        m_idler_cyl = idler_as_cylinder;
    }

    void SetBrakeType(BrakeType brake_type) { m_brake_type = brake_type; }
    void SetTrackShoeType(TrackShoeType shoe_type) { m_shoe_type = shoe_type; }
    void SetDoublePinTrackShoeType(DoublePinTrackShoeType topology) { m_shoe_topology = topology; }
    void SetANCFTrackShoeElementType(ChTrackShoeBandANCF::ElementType type) { m_ancf_element_type = type; }
    void SetANCFTrackShoeNumElements(int num_elements_length, int num_elements_width) {
        m_ancf_num_elements_length = num_elements_length;
        m_ancf_num_elements_width = num_elements_width;
    }
    void SetANCFTrackShoeCurvatureConstraints(bool constrain_curvature) {
        m_ancf_constrain_curvature = constrain_curvature;
    }
    void SetDrivelineType(DrivelineTypeTV driveline_type) { m_driveline_type = driveline_type; }
    void SetEngineType(EngineModelType val) { m_engineType = val; }
    void SetTransmissionType(TransmissionModelType val) { m_transmissionType = val; }

    void SetTrackBushings(bool val) { m_use_track_bushings = val; }
    void SetSuspensionBushings(bool val) { m_use_suspension_bushings = val; }
    void SetTrackStiffness(bool val) { m_use_track_RSDA = val; }

    void SetInitPosition(const ChCoordsys<>& pos) { m_initPos = pos; }
    void SetInitFwdVel(double fwdVel) { m_initFwdVel = fwdVel; }

    void SetCollisionSystemType(ChCollisionSystemType collsys_type) { m_collsysType = collsys_type; }

    void SetGyrationMode(bool val) { m_gyration_mode = val; }

    void SetAerodynamicDrag(double Cd, double area, double air_density);

    void CreateTrack(bool val) { m_create_track = val; }

    ChSystem* GetSystem() const { return m_vehicle->GetSystem(); }
    ChTrackedVehicle& GetVehicle() const { return *m_vehicle; }
    std::shared_ptr<ChChassis> GetChassis() const { return m_vehicle->GetChassis(); }
    std::shared_ptr<ChBodyAuxRef> GetChassisBody() const { return m_vehicle->GetChassisBody(); }
    std::shared_ptr<ChDrivelineTV> GetDriveline() const { return m_vehicle->GetDriveline(); }

    void Initialize();

    void SetChassisVisualizationType(VisualizationType vis) { m_vehicle->SetChassisVisualizationType(vis); }
    void SetSprocketVisualizationType(VisualizationType vis) { m_vehicle->SetSprocketVisualizationType(vis); }
    void SetIdlerVisualizationType(VisualizationType vis) { m_vehicle->SetIdlerVisualizationType(vis); }
    void SetSuspensionVisualizationType(VisualizationType vis) { m_vehicle->SetSuspensionVisualizationType(vis); }
    void SetIdlerWheelVisualizationType(VisualizationType vis) { m_vehicle->SetIdlerWheelVisualizationType(vis); }
    void SetRoadWheelVisualizationType(VisualizationType vis) { m_vehicle->SetRoadWheelVisualizationType(vis); }
    void SetTrackShoeVisualizationType(VisualizationType vis) { m_vehicle->SetTrackShoeVisualizationType(vis); }

    void Synchronize(double time,
                     const DriverInputs& driver_inputs);
    void Synchronize(double time,
                     const DriverInputs& driver_inputs,
                     const TerrainForces& shoe_forces_left,
                     const TerrainForces& shoe_forces_right);
    void Advance(double step);

    void LogConstraintViolations() { m_vehicle->LogConstraintViolations(); }

  protected:
    ChContactMethod m_contactMethod;
    ChCollisionSystemType m_collsysType;
    CollisionType m_chassisCollisionType;
    bool m_fixed;
    bool m_create_track;
    bool m_wheel_cyl;
    bool m_idler_cyl;

    BrakeType m_brake_type;
    TrackShoeType m_shoe_type;
    DoublePinTrackShoeType m_shoe_topology;
    ChTrackShoeBandANCF::ElementType m_ancf_element_type;
    bool m_ancf_constrain_curvature;
    int m_ancf_num_elements_length;
    int m_ancf_num_elements_width;
    DrivelineTypeTV m_driveline_type;
    EngineModelType m_engineType;
    TransmissionModelType m_transmissionType;

    bool m_use_track_bushings;
    bool m_use_suspension_bushings;
    bool m_use_track_RSDA;

    ChCoordsys<> m_initPos;
    double m_initFwdVel;

    bool m_gyration_mode;

    bool m_apply_drag;
    double m_Cd;
    double m_area;
    double m_air_density;

    ChSystem* m_system;
    M113_Vehicle* m_vehicle;
};

/// @} vehicle_models_sedan

}  // namespace m113
}  // end namespace vehicle
}  // end namespace chrono

#endif
