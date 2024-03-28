// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2023 projectchrono.org
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
// Template for a multibody deformable tire.
//
// =============================================================================

//// TODO:
//// - implement CalculateInertiaProperties
//// - add false coloring support

#include <algorithm>

#include "chrono/assets/ChVisualShapeSphere.h"
#include "chrono_vehicle/wheeled_vehicle/tire/ChMBTire.h"

namespace chrono {
namespace vehicle {

ChMBTire::ChMBTire(const std::string& name) : ChDeformableTire(name) {
    m_model = chrono_types::make_shared<MBTireModel>();
    m_model->m_tire = this;
    m_model->m_stiff = false;
    m_model->m_full_jac = false;
}

void ChMBTire::SetTireGeometry(const std::vector<double>& ring_radii,
                               const std::vector<double>& ring_offsets,
                               int num_divs,
                               double rim_radius) {
    assert(ring_radii.size() > 1);
    assert(ring_radii.size() == ring_offsets.size());

    m_model->m_radii = ring_radii;
    m_model->m_offsets = ring_offsets;

    m_model->m_num_rings = (int)ring_radii.size();
    m_model->m_num_divs = num_divs;

    m_model->m_rim_radius = rim_radius;
}

void ChMBTire::SetTireMass(double mass) {
    m_mass = mass;
}

void ChMBTire::SetMeshSpringCoefficients(double kC, double cC, double kT, double cT) {
    m_model->m_kC = kC;
    m_model->m_cC = cC;
    m_model->m_kT = kT;
    m_model->m_cT = cT;
}

void ChMBTire::SetBendingSpringCoefficients(double kB, double cB) {
    m_model->m_kB = kB;
    m_model->m_cB = cB;
}

void ChMBTire::SetRadialSpringCoefficients(double kR, double cR) {
    m_model->m_kR = kR;
    m_model->m_cR = cR;
}

void ChMBTire::IsStiff(bool val) {
    m_model->m_stiff = val;
}

void ChMBTire::UseFullJacobian(bool val) {
    m_model->m_full_jac = val;
}

void ChMBTire::SetTireContactMaterial(const ChContactMaterialData& mat_data) {
    m_contact_mat_data = mat_data;
}

double ChMBTire::GetRadius() const {
    return *std::max_element(m_model->m_radii.begin(), m_model->m_radii.end());
}

double ChMBTire::GetRimRadius() const {
    return m_model->m_rim_radius;
}

double ChMBTire::GetWidth() const {
    return m_model->m_offsets.back() - m_model->m_offsets.front();
}

void ChMBTire::CreateContactMaterial() {
    m_contact_mat =
        std::static_pointer_cast<ChContactMaterialSMC>(m_contact_mat_data.CreateMaterial(ChContactMethod::SMC));
}

void ChMBTire::Initialize(std::shared_ptr<ChWheel> wheel) {
    ChTire::Initialize(wheel);

    ChSystem* system = wheel->GetSpindle()->GetSystem();
    assert(system);

    // Add the underlying MB tire model (as a PhysicsItem) to the system *before* its construction.
    // This way, all its components will have an associated system during construction.
    system->Add(m_model);

    // Set internal tire pressure (if enabled)
    if (m_pressure_enabled && m_pressure <= 0)
        m_pressure = GetDefaultPressure();

    // Set contact material properties (if enabled)
    if (m_contact_enabled)
        CreateContactMaterial();

    // Construct the underlying tire model, attached to the wheel spindle body
    m_model->m_wheel = wheel->GetSpindle();
    m_model->Construct();
}

void ChMBTire::Synchronize(double time, const ChTerrain& terrain) {
    //// TODO
}

void ChMBTire::Advance(double step) {
    //// TODO
}

TerrainForce ChMBTire::ReportTireForce(ChTerrain* terrain) const {
    TerrainForce terrain_force;
    //// TODO
    return terrain_force;
}

TerrainForce ChMBTire::ReportTireForceLocal(ChTerrain* terrain, ChCoordsys<>& tire_frame) const {
    std::cerr << "ChMBTire::ReportTireForceLocal not implemented." << std::endl;
    throw std::runtime_error("ChMBTire::ReportTireForceLocal not implemented.");
}

void ChMBTire::AddVisualizationAssets(VisualizationType vis) {
    m_model->AddVisualizationAssets(vis);
}

void ChMBTire::RemoveVisualizationAssets() {
    ChPart::RemoveVisualizationAssets(m_model);
}

void ChMBTire::InitializeInertiaProperties() {
    if (!IsInitialized())
        return;

    ChVector3d com;
    m_model->CalculateInertiaProperties(com, m_inertia);
    m_com = ChFrame<>(com, QUNIT);
}

void ChMBTire::UpdateInertiaProperties() {
    if (!IsInitialized())
        return;

    ChVector3d com;
    m_model->CalculateInertiaProperties(com, m_inertia);
    m_com = ChFrame<>(com, QUNIT);
}

// =============================================================================

int MBTireModel::NodeIndex(int ir, int id) {
    // If ring index out-of-bounds, return -1
    if (ir < 0 || ir >= m_num_rings)
        return -1;

    // Make sure to use a positive value of the division index
    while (id < 0)
        id += m_num_divs;

    // Wrap around circumference if needed
    return ir * m_num_divs + (id % m_num_divs);
}

int MBTireModel::RimNodeIndex(int ir, int id) {
    // If ring index out-of-bounds, return -1
    if (ir != 0 && ir != m_num_rings - 1)
        return -1;

    // Make sure to use a positive value of the division index
    while (id < 0)
        id += m_num_divs;

    // Wrap around circumference if needed
    int ir_local = (ir == 0) ? 0 : 1;
    return ir_local * m_num_divs + (id % m_num_divs);
}

void MBTireModel::CalcNormal(int ir, int id, ChVector3d& normal, double& area) {
    // Get locations of previous and next nodes in the two directions
    const auto& posS = m_nodes[NodeIndex(ir, id - 1)]->GetPos();
    const auto& posN = m_nodes[NodeIndex(ir, id + 1)]->GetPos();
    const auto& posE = (ir == 0) ? m_rim_nodes[RimNodeIndex(0, id)]->GetPos()  //
                                 : m_nodes[NodeIndex(ir - 1, id)]->GetPos();
    const auto& posW = (ir == m_num_rings - 1) ? m_rim_nodes[RimNodeIndex(m_num_rings - 1, id)]->GetPos()  //
                                               : m_nodes[NodeIndex(ir + 1, id)]->GetPos();

    // Notes:
    // 1. normal could be approximated better, by averaging the normals of the 4 incident triangular faces
    // 2. if using the current approximation, might as well return the scaled direction (if only used for pressure
    // forces)
    ChVector3d dir = Vcross(posN - posS, posE - posW);
    area = dir.Length();
    normal = dir / area;
}

// -----------------------------------------------------------------------------
// Spring forces and Jacobians
//
// Jacobian matrices are linear combination of the form:
//    Kfactor * [K] + Rfactor * [R]
// where
// - [K] is the partial derivative wrt position-level states ("stiffness")
// - [R] is the partial derivative wrt velocity-level states ("damping")

// Constant threshold for checking zero length vectors
static const double zero_length = 1e-6;

// Constant threshold for checking zero angles
static const double zero_angle = 1e-3;

// Perturbation for FD Jacobian approximation
static const double FD_step = 1e-3;

void MBTireModel::Spring2::Initialize() {
    const auto& pos1 = node1->GetPos();
    const auto& pos2 = node2->GetPos();

    l0 = (pos2 - pos1).Length();
}

void MBTireModel::GridSpring2::Initialize(bool stiff) {
    Spring2::Initialize();
    if (stiff) {
        std::vector<ChVariables*> vars;
        vars.push_back(&node1->Variables());
        vars.push_back(&node2->Variables());
        KRM.SetVariables(vars);
    }
}

void MBTireModel::EdgeSpring2::Initialize(bool stiff, bool full_jac) {
    Spring2::Initialize();
    if (stiff) {
        std::vector<ChVariables*> vars;
        if (full_jac)
            vars.push_back(&wheel->Variables());
        vars.push_back(&node2->Variables());
        KRM.SetVariables(vars);
    }
}

void MBTireModel::Spring2::CalculateForce() {
    const auto& pos1 = node1->GetPos();
    const auto& pos2 = node2->GetPos();
    const auto& vel1 = node1->GetPosDt();
    const auto& vel2 = node2->GetPosDt();

    auto d = pos2 - pos1;
    double l = d.Length();
    assert(l > zero_length);
    d /= l;
    double ld = Vdot(vel2 - vel1, d);

    double f = k * (l - l0) + c * ld;

    ChVector3d vforce = f * d;
    force1 = +vforce;
    force2 = -vforce;
}

// Calculate a 3x3 block used in assembling Jacobians for Spring2
ChMatrix33<> MBTireModel::Spring2::CalculateJacobianBlock(double Kfactor, double Rfactor) {
    const auto& pos1 = node1->GetPos();
    const auto& pos2 = node2->GetPos();
    const auto& vel1 = node1->GetPosDt();
    const auto& vel2 = node2->GetPosDt();

    auto d = pos2 - pos1;
    double l = d.Length();
    assert(l > zero_length);
    d /= l;
    double ld = Vdot(vel2 - vel1, d);
    auto dd = (vel2 - vel1 - ld * d) / l;

    ChVectorN<double, 3> d_vec = d.eigen();
    ChVectorN<double, 3> dd_vec = dd.eigen();

    ChMatrix33<> D = d_vec * d_vec.transpose();
    ChMatrix33<> DD = d_vec * dd_vec.transpose();

    double f = k * (l - l0) + c * ld;

    ChMatrix33<> A = (Kfactor * k + Rfactor * c - Kfactor * f / l) * D +  //
                     (Kfactor * f / l) * ChMatrix33<>::Identity() +       //
                     Kfactor * c * DD;

    return A;
}

// For a linear spring connecting two grid nodes, the Jacobian is a 6x6 matrix:
//   d[f1;f2]/d[n1;n2]
// where f1, f2 are the nodal forces and n1, n2 are the states of the 2 nodes.
void MBTireModel::GridSpring2::CalculateJacobian(double Kfactor, double Rfactor) {
    ChMatrixRef K = KRM.GetMatrix();

    ////std::cout << "Grid spring2 " << inode1 << "-" << inode2;
    ////std::cout << "  K size: " << K.rows() << "x" << K.cols() << std::endl;

    auto A = CalculateJacobianBlock(Kfactor, Rfactor);

    K.block(0, 0, 3, 3) = -A;  // block for F1 and node1
    K.block(0, 3, 3, 3) = A;   // block for F1 and node2
    K.block(3, 0, 3, 3) = A;   // block for F2 and node1
    K.block(3, 3, 3, 3) = -A;  // block for F2 and node2

    ////auto J = CalculateJacobianFD(Kfactor, Rfactor);
    ////std::cout << "Grid spring2 " << inode1 << "-" << inode2 << std::endl << "-------" << std::endl;
    ////std::cout << K << std::endl;
    ////std::cout << "-------\n";
    ////std::cout << J << std::endl;
    ////std::cout << "-------\n";
    ////std::cout << "err_2   = " << (J - K).norm() / K.norm() << "\n";
    ////std::cout << "err_1   = " << (J - K).lpNorm<1>() / K.lpNorm<1>() << "\n";
    ////std::cout << "err_inf = " << (J - K).lpNorm<Eigen::Infinity>() / K.lpNorm<Eigen::Infinity>() << "\n";
    ////std::cout << "=======" << std::endl;
}

ChMatrixNM<double, 6, 6> MBTireModel::GridSpring2::CalculateJacobianFD(double Kfactor, double Rfactor) {
    ChMatrixNM<double, 6, 6> K;
    ChMatrixNM<double, 6, 6> R;

    K.setZero();
    R.setZero();

    ChVector3d pos1 = node1->GetPos();
    ChVector3d pos2 = node2->GetPos();
    ChVector3d vel1 = node1->GetPosDt();
    ChVector3d vel2 = node2->GetPosDt();

    CalculateForce();
    auto force1_0 = force1;
    auto force2_0 = force2;

    // node1 states (columns 0,1,2)
    for (int i = 0; i < 3; i++) {
        pos1[i] += FD_step;
        node1->SetPos(pos1);
        CalculateForce();
        K.col(i).segment(0, 3) = (force1.eigen() - force1_0.eigen()) / FD_step;
        K.col(i).segment(3, 3) = (force2.eigen() - force2_0.eigen()) / FD_step;
        pos1[i] -= FD_step;
        node1->SetPos(pos1);

        vel1[i] += FD_step;
        node1->SetPosDt(vel1);
        CalculateForce();
        R.col(i).segment(0, 3) = (force1.eigen() - force1_0.eigen()) / FD_step;
        R.col(i).segment(3, 3) = (force2.eigen() - force2_0.eigen()) / FD_step;
        vel1[i] -= FD_step;
        node1->SetPosDt(vel1);
    }

    // node2 states (columms 3,4,5)
    for (int i = 0; i < 3; i++) {
        pos2[i] += FD_step;
        node2->SetPos(pos2);
        CalculateForce();
        K.col(3 + i).segment(0, 3) = (force1.eigen() - force1_0.eigen()) / FD_step;
        K.col(3 + i).segment(3, 3) = (force2.eigen() - force2_0.eigen()) / FD_step;
        pos2[i] -= FD_step;
        node2->SetPos(pos2);

        vel2[i] += FD_step;
        node2->SetPosDt(vel2);
        CalculateForce();
        R.col(3 + i).segment(0, 3) = (force1.eigen() - force1_0.eigen()) / FD_step;
        R.col(3 + i).segment(3, 3) = (force2.eigen() - force2_0.eigen()) / FD_step;
        vel2[i] -= FD_step;
        node2->SetPosDt(vel2);
    }

    return Kfactor * K + Rfactor * R;
}

// For a linear spring connecting a rim node and a grid node, the Jacobian is a:
//    9x9 matrix (if full_jac=true) or
//    3x3 matrix (if full_jac=false)
// Note the order of generalized forces and states is:
//    wheel state (6)
//    node2 state (3)
void MBTireModel::EdgeSpring2::CalculateJacobian(bool full_jac, double Kfactor, double Rfactor) {
    ChMatrixRef K = KRM.GetMatrix();

    ////std::cout << "Edge spring2 " << inode1 << "-" << inode2;
    ////std::cout << "  K size: " << K.rows() << "x" << K.cols() << std::endl;

    auto A = CalculateJacobianBlock(Kfactor, Rfactor);

    if (!full_jac) {
        K.block(0, 0, 3, 3) = A;  // block for F2 and node2
        return;
    } else {
        ChVector3d p = wheel->GetPos();          // wheel body position
        ChQuaterniond q = wheel->GetRot();       // wheel body orientation quaternion
        ChMatrix33 R = wheel->GetRotMat();       // wheel body rotation matrix
        ChVector3d v = wheel->GetPosDt();       // wheel linear velocity
        ChVector3d w = wheel->GetAngVelLocal();  // wheel angular velocity (in local frame)

        ChVectorN<double, 4> e;   // quaternion
        ChVectorN<double, 3> rp;  // local positon

        // Jac_Af_m = 2 *
        //       [2*e0*rp1+e2*rp3-e3*rp2, 2*e1*rp1+e2*rp2+e3*rp3, e0*rp3+e1*rp2,          e1*rp3-e0*rp2;
        //        2*e0*rp2-e1*rp3+e3*rp1, e2*rp1-e0 *rp3,         e1*rp1+2*e2*rp2+e3*rp3, e0*rp1+e2*rp3;
        //        2*e0*rp3+e1*rp2-e2*rp1, e0*rp2 + e3*rp1,        e3*rp2-e0*rp1,          e1*rp1+e2*rp2+2*e3*rp3];
        ChMatrixNM<double, 3, 4> Jac_Af_m;

        Jac_Af_m(0, 0) = 2 * e[0] * rp[0] + e[2] * rp[2] - e[3] * rp[1];  // 2*e0*rp1 + e2*rp3 - e3*rp2,
        Jac_Af_m(0, 1) = 2 * e[1] * rp[0] + e[2] * rp[1] + e[3] * rp[2];  // 2*e1*rp1 + e2*rp2 + e3*rp3
        Jac_Af_m(0, 2) = e[0] * rp[2] + e[1] * rp[1];                     // e0*rp3 + e1*rp2
        Jac_Af_m(0, 3) = e[1] * rp[2] - e[0] * rp[1];                     // e1*rp3 - e0*rp2

        Jac_Af_m(1, 0) = 2 * e[0] * rp[1] - e[1] * rp[2] + e[3] * rp[0];  // 2*e0*rp2 - e1*rp3 + e3*rp1
        Jac_Af_m(1, 1) = e[2] * rp[0] - e[0] * rp[2];                     // e2*rp1 - e0 *rp3
        Jac_Af_m(1, 2) = e[1] * rp[0] + 2 * e[2] * rp[1] + e[3] * rp[2];  // e1*rp1 + 2*e2*rp2 + e3*rp3
        Jac_Af_m(1, 3) = e[0] * rp[0] + e[2] * rp[2];                     // e0 *rp1 + e2 *rp3

        Jac_Af_m(2, 0) = 2 * e[0] * rp[2] + e[1] * rp[1] - e[2] * rp[0];  // 2*e0*rp3 + e1*rp2 - e2*rp1
        Jac_Af_m(2, 1) = e[0] * rp[1] + e[3] * rp[0];                     // e0*rp2 + e3*rp1
        Jac_Af_m(2, 2) = e[3] * rp[1] - e[0] * rp[0];                     // e3*rp2 - e0*rp1
        Jac_Af_m(2, 3) = e[1] * rp[0] + e[2] * rp[1] + 2 * e[3] * rp[2];  // e1*rp1 + e2*rp2 + 2*e3*rp3

        Jac_Af_m *= 2;

        K.block(0, 0, 3, 3) = -A;
        K.block(0, 3, 3, 4) = -A * Jac_Af_m;
        K.block(0, 7, 3, 3) = A;

        K.block(3, 0, 3, 3) = A;
        K.block(3, 3, 3, 3) = A * Jac_Af_m;
        K.block(3, 6, 3, 4) = -A;
    }

    //// TODO - full_jac = true
}

void MBTireModel::Spring3::Initialize() {
    const auto& pos_p = node_p->GetPos();
    const auto& pos_c = node_c->GetPos();
    const auto& pos_n = node_n->GetPos();

    auto dir_p = (pos_c - pos_p).GetNormalized();
    auto dir_n = (pos_n - pos_c).GetNormalized();

    double cosA = Vdot(dir_p, dir_n);
    a0 = std::acos(cosA);
}

void MBTireModel::GridSpring3::Initialize(bool stiff) {
    Spring3::Initialize();
    if (stiff) {
        std::vector<ChVariables*> vars;
        vars.push_back(&node_p->Variables());
        vars.push_back(&node_c->Variables());
        vars.push_back(&node_n->Variables());
        KRM.SetVariables(vars);
    }
}

void MBTireModel::EdgeSpring3::Initialize(bool stiff, bool full_jac) {
    Spring3::Initialize();
    if (stiff) {
        std::vector<ChVariables*> vars;
        if (full_jac)
            vars.push_back(&wheel->Variables());
        vars.push_back(&node_c->Variables());
        vars.push_back(&node_n->Variables());
        KRM.SetVariables(vars);
    }
}

void MBTireModel::Spring3::CalculateForce() {
    const auto& pos_p = node_p->GetPos();
    const auto& pos_c = node_c->GetPos();
    const auto& pos_n = node_n->GetPos();
    ////const auto& vel_p = node_p->GetPos_dt();
    ////const auto& vel_c = node_c->GetPos_dt();
    ////const auto& vel_n = node_n->GetPos_dt();

    auto d_p = pos_c - pos_p;
    auto d_n = pos_n - pos_c;
    double l_p = d_p.Length();
    double l_n = d_n.Length();
    assert(l_p > zero_length);
    assert(l_n > zero_length);
    d_p /= l_p;
    d_n /= l_n;

    double cosA = Vdot(d_p, d_n);
    double a = std::acos(cosA);

    if (std::abs(a - a0) < zero_angle) {
        force_p = VNULL;
        force_c = VNULL;
        force_n = VNULL;
        return;
    }

    auto cross = Vcross(d_p, d_n);
    double length_cross = cross.Length();
    if (length_cross > zero_length) {
        cross /= length_cross;
    } else {  // colinear points
        cross = wheel->TransformDirectionLocalToParent(t0);
    }

    auto F_p = k * ((a - a0) / l_p) * Vcross(cross, d_p);
    auto F_n = k * ((a - a0) / l_n) * Vcross(cross, d_n);

    force_p = -F_p;
    force_c = F_p + F_n;
    force_n = -F_n;
}

ChMatrixNM<double, 6, 9> MBTireModel::Spring3::CalculateJacobianBlockJ1(double Kfactor, double Rfactor) {
    const auto& pos_p = node_p->GetPos();
    const auto& pos_c = node_c->GetPos();
    const auto& pos_n = node_n->GetPos();

    auto dp = pos_c - pos_p;
    auto dn = pos_n - pos_c;

    ChStarMatrix33<> skew_dp(dp);
    ChStarMatrix33<> skew_dn(dn);

    double lp = dp.Length();
    double ln = dn.Length();

    auto np = dp / lp;
    auto nn = dn / ln;

    auto t = Vcross(np, nn);
    double lt = t.Length();
    if (lt < zero_length) {
        t = wheel->TransformDirectionLocalToParent(t0);
        lt = 1;
    }

    double cosA = Vdot(np, nn);
    double a = std::acos(cosA);
    double sinA = std::sin(a);

    // Calculate D_pp, D_nn, D_pn
    ChMatrix33<> D_pp = np.eigen() * np.eigen().transpose();
    ChMatrix33<> D_nn = nn.eigen() * nn.eigen().transpose();

    // multiplier
    double scale = k / (lt * lp * ln);
    scale *= Kfactor;
    double scale2 = (sinA * cosA - (a - a0)) / (lt * lt);

    auto Bp = 1 / lp * Vcross(t, np);
    auto Bn = 1 / ln * Vcross(t, nn);

    // common vector
    ChVectorN<double, 6> B;
    B.block(0, 0, 3, 1) = scale * Bp.eigen();
    B.block(3, 0, 3, 1) = scale * Bn.eigen();

    ChMatrix33<> dA1 = skew_dn * (ChMatrix33<>::Identity() - D_pp);
    ChMatrix33<> dA3 = skew_dp * (ChMatrix33<>::Identity() - D_nn);

    ChVectorN<double, 3> dA_dalp_13 = scale2 * dA1.transpose() * t.eigen() + sinA * (ChMatrix33<>::Identity() - D_pp).transpose() * dn.eigen();
    ChVectorN<double, 3> dA_dalp_79 = scale2 * dA3.transpose() * t.eigen() - sinA * (ChMatrix33<>::Identity() - D_nn).transpose() * dp.eigen();
    ChVectorN<double, 3> dA_dalp_46 = -dA_dalp_13 - dA_dalp_79;

    ChMatrixNM<double, 6, 9> J1;
    J1.block(0, 0, 6, 1) = dA_dalp_13[0] * B;
    J1.block(0, 1, 6, 1) = dA_dalp_13[1] * B;
    J1.block(0, 2, 6, 1) = dA_dalp_13[2] * B;

    J1.block(0, 3, 6, 1) = dA_dalp_46[0] * B;
    J1.block(0, 4, 6, 1) = dA_dalp_46[1] * B;
    J1.block(0, 5, 6, 1) = dA_dalp_46[2] * B;

    J1.block(0, 6, 6, 1) = dA_dalp_79[0] * B;
    J1.block(0, 7, 6, 1) = dA_dalp_79[1] * B;
    J1.block(0, 8, 6, 1) = dA_dalp_79[2] * B;

    return J1;
}

ChMatrixNM<double, 6, 9> MBTireModel::Spring3::CalculateJacobianBlockJ2(double Kfactor, double Rfactor) {
    const auto& pos_p = node_p->GetPos();
    const auto& pos_c = node_c->GetPos();
    const auto& pos_n = node_n->GetPos();

    auto dp = pos_c - pos_p;
    auto dn = pos_n - pos_c;

    double lp = dp.Length();
    double ln = dn.Length();
    assert(lp > zero_length);
    assert(ln > zero_length);
    auto np = dp / lp;
    auto nn = dn / ln;

    auto t = Vcross(np, nn);
    double lt = t.Length();
    if (lt < zero_length) {
        t = wheel->TransformDirectionLocalToParent(t0);
        lt = 1;
    }

    double cosA = Vdot(np, nn);
    double a = std::acos(cosA);

    // Calculate D_pp, D_nn, D_pn
    ChMatrix33<> D_pp = np.eigen() * np.eigen().transpose();
    ChMatrix33<> D_nn = nn.eigen() * nn.eigen().transpose();
    ChMatrix33<> D_pn = np.eigen() * nn.eigen().transpose();

    // multiplier
    double scale = k * (a - a0) / (lt * lp * ln);
    scale *= Kfactor;

    ChMatrix33<> dBn_dalp_1 = (ChMatrix33<>::Identity() - D_nn) * (ChMatrix33<>::Identity() - D_pp);
    ChMatrix33<> dBn_dalp_3 = (lp / ln) * (D_pn + D_pn.transpose() + cosA * (ChMatrix33<>::Identity() - 3 * D_nn));
    ChMatrix33<> dBn_dalp_2 = -dBn_dalp_1 - dBn_dalp_3;

    ChMatrix33<> dBp_dalp_1 = (ln / lp) * (D_pn + D_pn.transpose() + cosA * (ChMatrix33<>::Identity() - 3 * D_pp));
    ChMatrix33<> dBp_dalp_3 = dBn_dalp_1.transpose();
    ChMatrix33<> dBp_dalp_2 = -dBp_dalp_1 - dBp_dalp_3;

    ChMatrixNM<double, 6, 9> J2;
    J2.block(0, 0, 3, 3) = scale * dBp_dalp_1;
    J2.block(0, 3, 3, 3) = scale * dBp_dalp_2;
    J2.block(0, 6, 3, 3) = scale * dBp_dalp_3;

    J2.block(3, 0, 3, 3) = scale * dBn_dalp_1;
    J2.block(3, 3, 3, 3) = scale * dBn_dalp_2;
    J2.block(3, 6, 3, 3) = scale * dBn_dalp_3;

    return J2;
}

// For a rotational spring connecting three grid nodes, the Jacobian is a 9x9 matrix:
//   d[f_p;f_c;f_n]/d[n_p;n_c;n_n]
// where f_p, f_c, and f_n are the nodal forces and n_p, n_c, and n_n are the states of the 3 nodes.
void MBTireModel::GridSpring3::CalculateJacobian(double Kfactor, double Rfactor) {
    ChMatrixRef K = KRM.GetMatrix();

    ////std::cout << "Grid spring3 " << inode_p << "-" << inode_c << "-" << inode_n;
    ////std::cout << "  K size: " << K.rows() << "x" << K.cols() << std::endl;

    auto J1 = CalculateJacobianBlockJ1(Kfactor, Rfactor);
    auto J2 = CalculateJacobianBlockJ2(Kfactor, Rfactor);

    ChMatrixNM<double, 3, 9> Jp = J1.topRows(3)    + J2.topRows(3);
    ChMatrixNM<double, 3, 9> Jn = J1.bottomRows(3) + J2.bottomRows(3);

    // assemble Jacobian matrix
    //    force_p = -F_p;
    //    force_c = +F_p + F_n;
    //    force_n = -F_n;
    K.block(0, 0, 3, 9) = -Jp;
    K.block(3, 0, 3, 9) =  Jp + Jn;
    K.block(6, 0, 3, 9) = -Jn;

    ////auto J = CalculateJacobianFD(Kfactor, Rfactor);
    ////std::cout << "Grid spring3 " << inode_p << "-" << inode_c << "-" << inode_n << std::endl << "-------" << std::endl;
    ////std::cout << K << std::endl;
    ////std::cout << "-------\n";
    ////std::cout << J << std::endl;
    ////std::cout << "-------\n";
    ////std::cout << "err_2   = " << (J - K).norm() << "  " << (J - K).norm() / K.norm() << "\n";
    ////std::cout << "err_1   = " << (J - K).lpNorm<1>() << "  " << (J - K).lpNorm<1>() / K.lpNorm<1>() << "\n";
    ////std::cout << "err_inf = " << (J - K).lpNorm<Eigen::Infinity>()
    ////          << "  " << (J - K).lpNorm<Eigen::Infinity>() / K.lpNorm<Eigen::Infinity>()
    ////          << "\n";
    ////std::cout << "=======" << std::endl;
}

ChMatrixNM<double, 9, 9> MBTireModel::GridSpring3::CalculateJacobianFD(double Kfactor, double Rfactor) {
    ChMatrixNM<double, 9, 9> K;
    ChMatrixNM<double, 9, 9> R;

    K.setZero();
    R.setZero();

    ChVector3d pos_p = node_p->GetPos();
    ChVector3d pos_c = node_c->GetPos();
    ChVector3d pos_n = node_n->GetPos();
    ChVector3d vel_p = node_p->GetPosDt();
    ChVector3d vel_c = node_c->GetPosDt();
    ChVector3d vel_n = node_n->GetPosDt();

    CalculateForce();
    auto force_p_0 = force_p;
    auto force_c_0 = force_c;
    auto force_n_0 = force_n;

    // node_p states (columns 0,1,2)
    for (int i = 0; i < 3; i++) {
        pos_p[i] += FD_step;
        node_p->SetPos(pos_p);
        CalculateForce();
        K.col(i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        K.col(i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        K.col(i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        pos_p[i] -= FD_step;
        node_p->SetPos(pos_p);

        vel_p[i] += FD_step;
        node_p->SetPosDt(vel_p);
        CalculateForce();
        R.col(i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        R.col(i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        R.col(i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        vel_p[i] -= FD_step;
        node_p->SetPosDt(vel_p);
    }

    // node_c states (columns 3,4,5)
    for (int i = 0; i < 3; i++) {
        pos_c[i] += FD_step;
        node_c->SetPos(pos_c);
        CalculateForce();
        K.col(3 + i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        K.col(3 + i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        K.col(3 + i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        pos_c[i] -= FD_step;
        node_c->SetPos(pos_c);

        vel_c[i] += FD_step;
        node_c->SetPosDt(vel_c);
        CalculateForce();
        R.col(3 + i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        R.col(3 + i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        R.col(3 + i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        vel_c[i] -= FD_step;
        node_c->SetPosDt(vel_c);
    }

    // node_n states (columns 6,7,8)
    for (int i = 0; i < 3; i++) {
        pos_n[i] += FD_step;
        node_n->SetPos(pos_n);
        CalculateForce();
        K.col(6 + i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        K.col(6 + i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        K.col(6 + i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        pos_n[i] -= FD_step;
        node_n->SetPos(pos_n);

        vel_n[i] += FD_step;
        node_n->SetPosDt(vel_n);
        CalculateForce();
        R.col(6 + i).segment(0, 3) = (force_p.eigen() - force_p_0.eigen()) / FD_step;
        R.col(6 + i).segment(3, 3) = (force_c.eigen() - force_c_0.eigen()) / FD_step;
        R.col(6 + i).segment(6, 3) = (force_n.eigen() - force_n_0.eigen()) / FD_step;
        vel_n[i] -= FD_step;
        node_n->SetPosDt(vel_n);
    }

    return Kfactor * K + Rfactor * R;
}

// For a rotational spring connecting a rim node and 2 grid nodes, the Jacobian is a:
//    12x12 matrix (if full_jac=true) or
//    6x6 matrix (if full_jac=false)
// Note the order of generalized forces and states is:
//    wheel state (6)
//    node_c state (3)
//    node_n state (3)
void MBTireModel::EdgeSpring3::CalculateJacobian(bool full_jac, double Kfactor, double Rfactor) {
    ChMatrixRef K = KRM.GetMatrix();

    ////std::cout << "Edge spring3 " << inode_p << "-" << inode_c << "-" << inode_n;
    ////std::cout << "  K size: " << K.rows() << "x" << K.cols() << std::endl;

    auto J1 = CalculateJacobianBlockJ1(Kfactor, Rfactor);
    auto J2 = CalculateJacobianBlockJ2(Kfactor, Rfactor);

    ChMatrixNM<double, 3, 9> Jp = J1.topRows(3) + J2.topRows(3);
    ChMatrixNM<double, 3, 9> Jn = J1.bottomRows(3) + J2.bottomRows(3);

    // substitution of all into matrix
    if (!full_jac) {
        // substitution of all into matrix
        K.block(0, 0, 3, 6) = -Jn.rightCols(6);
        K.block(3, 0, 3, 6) = Jn.rightCols(6);
        return;
    } else {
        ChVector3d p = wheel->GetPos();          // wheel body position
        ChQuaterniond q = wheel->GetRot();       // wheel body orientation quaternion
        ChMatrix33 R = wheel->GetRotMat();       // wheel body rotation matrix
        ChVector3d v = wheel->GetPosDt();        // wheel linear velocity
        ChVector3d w = wheel->GetAngVelLocal();  // wheel angular velocity (in local frame)

        ChVectorN<double, 4> e;   // quaternion
        ChVectorN<double, 3> rp;  // local positon

        // Jac_Af_m = 2 *
        //       [2*e0*rp1+e2*rp3-e3*rp2, 2*e1*rp1+e2*rp2+e3*rp3, e0*rp3+e1*rp2,          e1*rp3-e0*rp2;
        //        2*e0*rp2-e1*rp3+e3*rp1, e2*rp1-e0 *rp3,         e1*rp1+2*e2*rp2+e3*rp3, e0*rp1+e2*rp3;
        //        2*e0*rp3+e1*rp2-e2*rp1, e0*rp2 + e3*rp1,        e3*rp2-e0*rp1,          e1*rp1+e2*rp2+2*e3*rp3];
        ChMatrixNM<double, 3, 4> Jac_Af_m;

        Jac_Af_m(0, 0) = 2 * e[0] * rp[0] + e[2] * rp[2] - e[3] * rp[1];  // 2*e0*rp1 + e2*rp3 - e3*rp2,
        Jac_Af_m(0, 1) = 2 * e[1] * rp[0] + e[2] * rp[1] + e[3] * rp[2];  // 2*e1*rp1 + e2*rp2 + e3*rp3
        Jac_Af_m(0, 2) = e[0] * rp[2] + e[1] * rp[1];                     // e0*rp3 + e1*rp2
        Jac_Af_m(0, 3) = e[1] * rp[2] - e[0] * rp[1];                     // e1*rp3 - e0*rp2

        Jac_Af_m(1, 0) = 2 * e[0] * rp[1] - e[1] * rp[2] + e[3] * rp[0];  // 2*e0*rp2 - e1*rp3 + e3*rp1
        Jac_Af_m(1, 1) = e[2] * rp[0] - e[0] * rp[2];                     // e2*rp1 - e0 *rp3
        Jac_Af_m(1, 2) = e[1] * rp[0] + 2 * e[2] * rp[1] + e[3] * rp[2];  // e1*rp1 + 2*e2*rp2 + e3*rp3
        Jac_Af_m(1, 3) = e[0] * rp[0] + e[2] * rp[2];                     // e0 *rp1 + e2 *rp3

        Jac_Af_m(2, 0) = 2 * e[0] * rp[2] + e[1] * rp[1] - e[2] * rp[0];  // 2*e0*rp3 + e1*rp2 - e2*rp1
        Jac_Af_m(2, 1) = e[0] * rp[1] + e[3] * rp[0];                     // e0*rp2 + e3*rp1
        Jac_Af_m(2, 2) = e[3] * rp[1] - e[0] * rp[0];                     // e3*rp2 - e0*rp1
        Jac_Af_m(2, 3) = e[1] * rp[0] + e[2] * rp[1] + 2 * e[3] * rp[2];  // e1*rp1 + e2*rp2 + 2*e3*rp3

        Jac_Af_m *= 2;

        ChMatrixNM<double, 6, 4> J2_quat = J2.leftCols(3) * Jac_Af_m;
        ChMatrixNM<double, 6, 4> J1_quat;

        ChMatrixNM<double, 4, 3> Jac_Af_mT = Jac_Af_m.transpose();
        J1_quat.block(0, 0, 6, 1) =
            Jac_Af_mT(0, 1) * J1.col(0) + Jac_Af_mT(0, 2) * J1.col(2) + Jac_Af_mT(0, 3) * J1.col(3);
        J1_quat.block(0, 1, 6, 1) =
            Jac_Af_mT(1, 1) * J1.col(0) + Jac_Af_mT(1, 2) * J1.col(2) + Jac_Af_mT(1, 3) * J1.col(3);
        J1_quat.block(0, 2, 6, 1) =
            Jac_Af_mT(2, 1) * J1.col(0) + Jac_Af_mT(2, 2) * J1.col(2) + Jac_Af_mT(2, 3) * J1.col(3);
        J1_quat.block(0, 3, 6, 1) =
            Jac_Af_mT(3, 1) * J1.col(0) + Jac_Af_mT(3, 2) * J1.col(2) + Jac_Af_mT(3, 3) * J1.col(3);

        // insert new columns
        ChMatrixNM<double, 3, 13> Jp_new;
        Jp_new.block(0, 0, 3, 3) = J1.topRows(3).leftCols(3) + J2.topRows(3).leftCols(3);
        Jp_new.block(0, 3, 3, 4) = J1_quat.topRows(3) + J2_quat.topRows(3);
        Jp_new.block(0, 7, 3, 6) = J1.topRows(3).rightCols(6) + J2.topRows(3).rightCols(6);

        ChMatrixNM<double, 3, 13> Jn_new;
        Jn_new.block(0, 0, 3, 3) = J1.bottomRows(3).leftCols(3) + J2.bottomRows(3).leftCols(3);
        Jn_new.block(0, 3, 3, 4) = J1_quat.bottomRows(3) + J2_quat.bottomRows(3);
        Jn_new.block(0, 7, 3, 6) = J1.bottomRows(3).rightCols(6) + J2.bottomRows(3).rightCols(6);

        K.block(0, 0, 3, 13) = -Jp;
        K.block(3, 0, 3, 13) = Jp + Jn;
        K.block(6, 0, 3, 13) = -Jn;
    }
}

// -----------------------------------------------------------------------------

void MBTireModel::Construct() {
    m_num_rim_nodes = 2 * m_num_divs;
    m_num_nodes = m_num_rings * m_num_divs;
    m_num_faces = 2 * (m_num_rings - 1) * m_num_divs;

    m_node_mass = m_tire->GetMass() / m_num_nodes;

    // Create the visualization shape and get accessors to the underling trimesh
    m_trimesh_shape = chrono_types::make_shared<ChVisualShapeTriangleMesh>();
    auto trimesh = m_trimesh_shape->GetMesh();
    auto& vertices = trimesh->GetCoordsVertices();
    auto& normals = trimesh->GetCoordsNormals();
    auto& idx_vertices = trimesh->GetIndicesVertexes();
    auto& idx_normals = trimesh->GetIndicesNormals();
    ////auto& uv_coords = trimesh->GetCoordsUV();
    auto& colors = trimesh->GetCoordsColors();

    // ------------ Nodes

    // Create the FEA nodes (with positions expressed in the global frame) and load the mesh vertices.
    m_nodes.resize(m_num_nodes);
    vertices.resize(m_num_nodes);
    double dphi = CH_2PI / m_num_divs;
    int k = 0;
    for (int ir = 0; ir < m_num_rings; ir++) {
        double y = m_offsets[ir];
        double r = m_radii[ir];
        for (int id = 0; id < m_num_divs; id++) {
            double phi = id * dphi;
            double x = r * std::cos(phi);
            double z = r * std::sin(phi);
            vertices[k] = m_wheel->TransformPointLocalToParent(ChVector3d(x, y, z));
            m_nodes[k] = chrono_types::make_shared<fea::ChNodeFEAxyz>(vertices[k]);
            m_nodes[k]->SetMass(m_node_mass);
            m_nodes[k]->m_TotalMass = m_node_mass;
            k++;
        }
    }

    // Create the FEA nodes attached to the rim
    m_rim_nodes.resize(m_num_rim_nodes);
    k = 0;
    {
        double y = m_offsets[0];
        for (int id = 0; id < m_num_divs; id++) {
            double phi = id * dphi;
            double x = m_rim_radius * std::cos(phi);
            double z = m_rim_radius * std::sin(phi);
            auto loc = m_wheel->TransformPointLocalToParent(ChVector3d(x, y, z));
            m_rim_nodes[k] = chrono_types::make_shared<fea::ChNodeFEAxyz>(loc);
            m_rim_nodes[k]->SetMass(m_node_mass);
            m_rim_nodes[k]->m_TotalMass = m_node_mass;
            k++;
        }
    }
    {
        double y = m_offsets[m_num_rings - 1];
        for (int id = 0; id < m_num_divs; id++) {
            double phi = id * dphi;
            double x = m_rim_radius * std::cos(phi);
            double z = m_rim_radius * std::sin(phi);
            auto loc = m_wheel->TransformPointLocalToParent(ChVector3d(x, y, z));
            m_rim_nodes[k] = chrono_types::make_shared<fea::ChNodeFEAxyz>(loc);
            m_rim_nodes[k]->SetMass(m_node_mass);
            m_rim_nodes[k]->m_TotalMass = m_node_mass;
            k++;
        }
    }

    // ------------ Springs

    // Create circumferential linear springs (node-node)
    for (int ir = 0; ir < m_num_rings; ir++) {
        for (int id = 0; id < m_num_divs; id++) {
            int inode1 = NodeIndex(ir, id);
            int inode2 = NodeIndex(ir, id + 1);

            GridSpring2 spring;
            spring.inode1 = inode1;
            spring.inode2 = inode2;
            spring.node1 = m_nodes[inode1].get();
            spring.node2 = m_nodes[inode2].get();
            spring.wheel = m_wheel.get();
            spring.k = m_kC;
            spring.c = m_cC;

            spring.Initialize(m_stiff);
            m_grid_lin_springs.push_back(spring);
        }
    }

    // Create transversal linear springs (node-node and node-rim)
    for (int id = 0; id < m_num_divs; id++) {
        // radial springs connected to the rim at first ring
        {
            int inode1 = RimNodeIndex(0, id);
            int inode2 = NodeIndex(0, id);

            EdgeSpring2 spring;
            spring.inode1 = inode1;
            spring.inode2 = inode2;
            spring.node1 = m_rim_nodes[inode1].get();
            spring.node2 = m_nodes[inode2].get();
            spring.wheel = m_wheel.get();
            spring.k = m_kR;
            spring.c = m_cR;

            spring.Initialize(m_stiff, m_full_jac);
            m_edge_lin_springs.push_back(spring);
        }

        // node-node springs
        for (int ir = 0; ir < m_num_rings - 1; ir++) {
            int inode1 = NodeIndex(ir, id);
            int inode2 = NodeIndex(ir + 1, id);

            GridSpring2 spring;
            spring.inode1 = inode1;
            spring.inode2 = inode2;
            spring.node1 = m_nodes[inode1].get();
            spring.node2 = m_nodes[inode2].get();
            spring.wheel = m_wheel.get();
            spring.k = m_kT;
            spring.c = m_cT;

            spring.Initialize(m_stiff);
            m_grid_lin_springs.push_back(spring);
        }

        // radial springs connected to the rim at last ring
        {
            int inode1 = RimNodeIndex(m_num_rings - 1, id);
            int inode2 = NodeIndex(m_num_rings - 1, id);

            EdgeSpring2 spring;
            spring.inode1 = inode1;
            spring.inode2 = inode2;
            spring.node1 = m_rim_nodes[inode1].get();
            spring.node2 = m_nodes[inode2].get();
            spring.wheel = m_wheel.get();
            spring.k = m_kR;
            spring.c = m_cR;

            spring.Initialize(m_stiff, m_full_jac);
            m_edge_lin_springs.push_back(spring);
        }
    }

    // Create circumferential rotational springs (node-node)
    for (int ir = 0; ir < m_num_rings; ir++) {
        for (int id = 0; id < m_num_divs; id++) {
            int inode_p = NodeIndex(ir, id - 1);
            int inode_c = NodeIndex(ir, id);
            int inode_n = NodeIndex(ir, id + 1);

            GridSpring3 spring;
            spring.inode_p = inode_p;
            spring.inode_c = inode_c;
            spring.inode_n = inode_n;
            spring.node_p = m_nodes[inode_p].get();
            spring.node_c = m_nodes[inode_c].get();
            spring.node_n = m_nodes[inode_n].get();
            spring.wheel = m_wheel.get();
            spring.t0 = ChVector3d(0, 1, 0);
            spring.k = m_kB;
            spring.c = m_cB;

            spring.Initialize(m_stiff);
            m_grid_rot_springs.push_back(spring);
        }
    }

    // Create transversal rotational springs (node-node and node-rim)
    for (int id = 0; id < m_num_divs; id++) {
        double phi = id * dphi;
        ChVector3d t0(-std::sin(phi), 0, std::cos(phi));

        // torsional springs connected to the rim at first ring
        {
            int inode_p = RimNodeIndex(0, id);
            int inode_c = NodeIndex(0, id);
            int inode_n = NodeIndex(1, id);

            EdgeSpring3 spring;
            spring.inode_p = inode_p;
            spring.inode_c = inode_c;
            spring.inode_n = inode_n;
            spring.node_p = m_rim_nodes[inode_p].get();
            spring.node_c = m_nodes[inode_c].get();
            spring.node_n = m_nodes[inode_n].get();
            spring.wheel = m_wheel.get();
            spring.t0 = t0;
            spring.k = m_kB;
            spring.c = m_cB;

            spring.Initialize(m_stiff, m_full_jac);
            m_edge_rot_springs.push_back(spring);
        }

        // node-node torsional springs
        for (int ir = 1; ir < m_num_rings - 1; ir++) {
            int inode_p = NodeIndex(ir - 1, id);
            int inode_c = NodeIndex(ir, id);
            int inode_n = NodeIndex(ir + 1, id);

            GridSpring3 spring;
            spring.inode_p = inode_p;
            spring.inode_c = inode_c;
            spring.inode_n = inode_n;
            spring.node_p = m_nodes[inode_p].get();
            spring.node_c = m_nodes[inode_c].get();
            spring.node_n = m_nodes[inode_n].get();
            spring.wheel = m_wheel.get();
            spring.t0 = t0;
            spring.k = m_kB;
            spring.c = m_cB;

            spring.Initialize(m_stiff);
            m_grid_rot_springs.push_back(spring);
        }

        // torsional springs connected to the rim at last ring
        {
            int inode_p = RimNodeIndex(m_num_rings - 1, id);
            int inode_c = NodeIndex(m_num_rings - 1, id);
            int inode_n = NodeIndex(m_num_rings - 2, id);

            EdgeSpring3 spring;
            spring.inode_p = inode_p;
            spring.inode_c = inode_c;
            spring.inode_n = inode_n;
            spring.node_p = m_rim_nodes[inode_p].get();
            spring.node_c = m_nodes[inode_c].get();
            spring.node_n = m_nodes[inode_n].get();
            spring.wheel = m_wheel.get();
            spring.t0 = -t0;
            spring.k = m_kB;
            spring.c = m_cB;

            spring.Initialize(m_stiff, m_full_jac);
            m_edge_rot_springs.push_back(spring);
        }
    }

    // ------------ Collision and visualization meshes

    // Auxiliary face information (for possible collision mesh)
    struct FaceAuxData {
        ChVector3i nbr_node;   // neighbor (opposite) vertex/node for each face vertex
        ChVector3b owns_node;  // vertex/node owned by the face?
        ChVector3b owns_edge;  // edge owned by the face?
    };
    std::vector<FaceAuxData> auxdata(m_num_faces);

    // Create the mesh faces and define auxiliary data (for possible collision mesh)
    idx_vertices.resize(m_num_faces);
    idx_normals.resize(m_num_faces);
    k = 0;
    for (int ir = 0; ir < m_num_rings - 1; ir++) {
        bool last = ir == m_num_rings - 2;
        for (int id = 0; id < m_num_divs; id++) {
            int v1 = NodeIndex(ir, id);
            int v2 = NodeIndex(ir + 1, id);
            int v3 = NodeIndex(ir + 1, id + 1);
            int v4 = NodeIndex(ir, id + 1);
            idx_vertices[k] = {v1, v2, v3};
            idx_normals[k] = {v1, v2, v3};
            auxdata[k].nbr_node = {NodeIndex(ir + 2, id + 1), v4, NodeIndex(ir, id - 1)};
            auxdata[k].owns_node = {true, last, false};
            auxdata[k].owns_edge = {true, last, true};
            k++;
            idx_vertices[k] = {v1, v3, v4};
            idx_normals[k] = {v1, v3, v4};
            auxdata[k].nbr_node = {NodeIndex(ir + 1, id + 2), NodeIndex(ir - 1, id), v2};
            auxdata[k].owns_node = {false, false, false};
            auxdata[k].owns_edge = {false, false, true};
            k++;
        }
    }

    // Create the contact surface of the specified type and initialize it using the underlying model
    if (m_tire->IsContactEnabled()) {
        auto contact_mat = m_tire->GetContactMaterial();

        switch (m_tire->GetContactSurfaceType()) {
            case ChDeformableTire::ContactSurfaceType::NODE_CLOUD: {
                auto contact_surf = chrono_types::make_shared<fea::ChContactSurfaceNodeCloud>(contact_mat);
                contact_surf->SetPhysicsItem(this);
                for (const auto& node : m_nodes)
                    contact_surf->AddNode(node, m_tire->GetContactNodeRadius());
                m_contact_surf = contact_surf;
                break;
            }
            case ChDeformableTire::ContactSurfaceType::TRIANGLE_MESH: {
                auto contact_surf = chrono_types::make_shared<fea::ChContactSurfaceMesh>(contact_mat);
                contact_surf->SetPhysicsItem(this);
                for (k = 0; k < m_num_faces; k++) {
                    const auto& node1 = m_nodes[idx_vertices[k][0]];
                    const auto& node2 = m_nodes[idx_vertices[k][1]];
                    const auto& node3 = m_nodes[idx_vertices[k][2]];
                    const auto& edge_node1 = (auxdata[k].nbr_node[0] != -1 ? m_nodes[auxdata[k].nbr_node[0]] : nullptr);
                    const auto& edge_node2 = (auxdata[k].nbr_node[1] != -1 ? m_nodes[auxdata[k].nbr_node[1]] : nullptr);
                    const auto& edge_node3 = (auxdata[k].nbr_node[2] != -1 ? m_nodes[auxdata[k].nbr_node[2]] : nullptr);
                    contact_surf->AddFace(node1, node2, node3,                                                        //
                                          edge_node1, edge_node2, edge_node3,                                         //
                                          auxdata[k].owns_node[0], auxdata[k].owns_node[1], auxdata[k].owns_node[2],  //
                                          auxdata[k].owns_edge[0], auxdata[k].owns_edge[1], auxdata[k].owns_edge[2],  //
                                          m_tire->GetContactFaceThickness());
                }

                m_contact_surf = contact_surf;
                break;
            }
        }
    }

    // Initialize mesh colors and vertex normals
    colors.resize(m_num_nodes, ChColor(1, 0, 0));
    normals.resize(m_num_nodes, ChVector3d(0, 0, 0));

    // Calculate face normals, accumulate vertex normals, and count number of faces adjacent to each vertex
    std::vector<int> accumulators(m_num_nodes, 0);
    for (int it = 0; it < m_num_faces; it++) {
        // Calculate the triangle normal as a normalized cross product.
        ChVector3d nrm = Vcross(vertices[idx_vertices[it][1]] - vertices[idx_vertices[it][0]],
                                vertices[idx_vertices[it][2]] - vertices[idx_vertices[it][0]]);
        nrm.Normalize();
        // Increment the normals of all incident vertices by the face normal
        normals[idx_normals[it][0]] += nrm;
        normals[idx_normals[it][1]] += nrm;
        normals[idx_normals[it][2]] += nrm;
        // Increment the count of all incident vertices by 1
        accumulators[idx_normals[it][0]] += 1;
        accumulators[idx_normals[it][1]] += 1;
        accumulators[idx_normals[it][2]] += 1;
    }

    // Set vertex normals to average values over all adjacent faces
    for (int in = 0; in < m_num_nodes; in++) {
        normals[in] /= (double)accumulators[in];
    }
}

void MBTireModel::CalculateInertiaProperties(ChVector3d& com, ChMatrix33<>& inertia) {
    //// TODO
}

// Set position and velocity of rim nodes from wheel/spindle state
void MBTireModel::SetRimNodeStates() {
    double dphi = CH_2PI / m_num_divs;
    int k = 0;
    {
        double y = m_offsets[0];
        for (int id = 0; id < m_num_divs; id++) {
            double phi = id * dphi;

            double x = m_rim_radius * std::cos(phi);
            double z = m_rim_radius * std::sin(phi);
            auto pos_loc = ChVector3d(x, y, z);
            m_rim_nodes[k]->SetPos(m_wheel->TransformPointLocalToParent(pos_loc));
            m_rim_nodes[k]->SetPosDt(m_wheel->PointSpeedLocalToParent(pos_loc));
            k++;
        }
    }
    {
        double y = m_offsets[m_num_rings - 1];
        for (int id = 0; id < m_num_divs; id++) {
            double phi = id * dphi;

            double x = m_rim_radius * std::cos(phi);
            double z = m_rim_radius * std::sin(phi);
            auto pos_loc = ChVector3d(x, y, z);
            m_rim_nodes[k]->SetPos(m_wheel->TransformPointLocalToParent(pos_loc));
            m_rim_nodes[k]->SetPosDt(m_wheel->PointSpeedLocalToParent(pos_loc));
            k++;
        }
    }
}

// Calculate and set forces at each node and accumulate wheel loads.
// Note: the positions and velocities of nodes attached to the wheel are assumed to be updated.
void MBTireModel::CalculateForces() {
    // Initialize nodal force accumulators
    std::vector<ChVector3d> nodal_forces(m_num_nodes, VNULL);

    // Initialize wheel force and moment accumulators
    //// TODO: update wheel torque
    m_wheel_force = VNULL;   // body force, expressed in global frame
    m_wheel_torque = VNULL;  // body torque, expressed in local frame

    // ------------ Gravitational and pressure forces

    ChVector3d gforce = m_node_mass * GetSystem()->GetGravitationalAcceleration();
    bool pressure_enabled = m_tire->IsPressureEnabled();
    double pressure = m_tire->GetPressure();
    ChVector3d normal;
    double area;
    for (int ir = 0; ir < m_num_rings; ir++) {
        for (int id = 0; id < m_num_divs; id++) {
            int k = NodeIndex(ir, id);
            nodal_forces[k] = gforce;

            if (pressure_enabled) {
                //// TODO: revisit pressure forces when springs are in place (to hold the mesh together)

                // Option 1
                CalcNormal(ir, id, normal, area);
                nodal_forces[k] += (0.5 * pressure * area) * normal;

                // Option 2
                ////normal = (m_nodes[k]->GetPos() - w_pos).GetNormalized();
                /// nodal_forces[k] += (pressure * area) * normal;
            }
        }
    }

    // ------------ Spring forces

    // Forces in mesh linear springs (node-node)
    for (auto& spring : m_grid_lin_springs) {
        spring.CalculateForce();
        nodal_forces[spring.inode1] += spring.force1;
        nodal_forces[spring.inode2] += spring.force2;
    }

    // Forces in edge linear springs (rim node: node1)
    for (auto& spring : m_edge_lin_springs) {
        spring.CalculateForce();
        m_wheel_force += spring.force1;
        nodal_forces[spring.inode2] += spring.force2;
    }

    // Forces in mesh rotational springs (node-node)
    for (auto& spring : m_grid_rot_springs) {
        spring.CalculateForce();
        nodal_forces[spring.inode_p] += spring.force_p;
        nodal_forces[spring.inode_c] += spring.force_c;
        nodal_forces[spring.inode_n] += spring.force_n;
    }

    // Forces in edge rotational springs (rim node: node_p)
    for (auto& spring : m_edge_rot_springs) {
        spring.CalculateForce();
        m_wheel_force += spring.force_p;
        nodal_forces[spring.inode_c] += spring.force_c;
        nodal_forces[spring.inode_n] += spring.force_n;
    }

    // ------------ Apply loads on FEA nodes

    for (int k = 0; k < m_num_nodes; k++) {
        m_nodes[k]->SetForce(nodal_forces[k]);
    }
}

// -----------------------------------------------------------------------------

void MBTireModel::SyncCollisionModels() {
    if (m_contact_surf)
        m_contact_surf->SyncCollisionModels();
}

void MBTireModel::AddCollisionModelsToSystem(ChCollisionSystem* coll_sys) const {
    assert(GetSystem());
    if (m_contact_surf) {
        m_contact_surf->SyncCollisionModels();
        m_contact_surf->AddCollisionModelsToSystem(coll_sys);
    }
}

void MBTireModel::RemoveCollisionModelsFromSystem(ChCollisionSystem* coll_sys) const {
    assert(GetSystem());
    if (m_contact_surf)
        m_contact_surf->RemoveCollisionModelsFromSystem(coll_sys);
}

// -----------------------------------------------------------------------------

// Notes:
// The implementation of these functions is similar to those in ChMesh.
// It is assumed that none of the FEA nodes is fixed.

void MBTireModel::SetupInitial() {
    // Calculate DOFs and state offsets
    m_dofs = 0;
    m_dofs_w = 0;
    for (auto& node : m_nodes) {
        node->SetupInitial(GetSystem());
        m_dofs += node->GetNumCoordsPosLevelActive();
        m_dofs_w += node->GetNumCoordsVelLevelActive();
    }
}

//// TODO: Cache (normalized) locations around the rim to save sin/cos evaluations
void MBTireModel::Setup() {
    // Recompute DOFs and state offsets
    m_dofs = 0;
    m_dofs_w = 0;
    for (auto& node : m_nodes) {
        // Set node offsets in state vectors (based on the offsets of the container)
        node->NodeSetOffsetPosLevel(GetOffset_x() + m_dofs);
        node->NodeSetOffsetVelLevel(GetOffset_w() + m_dofs_w);

        // Count the actual degrees of freedom (consider only nodes that are not fixed)
        m_dofs += node->GetNumCoordsPosLevelActive();
        m_dofs_w += node->GetNumCoordsVelLevelActive();
    }

    // Update visualization mesh
    auto trimesh = m_trimesh_shape->GetMesh();
    auto& vertices = trimesh->GetCoordsVertices();
    auto& normals = trimesh->GetCoordsNormals();
    auto& colors = trimesh->GetCoordsColors();

    for (int k = 0; k < m_num_nodes; k++) {
        vertices[k] = m_nodes[k]->GetPos();
    }

    //// TODO: update vertex normals and colors
}

void MBTireModel::Update(double t, bool update_assets) {
    // Parent class update
    ChPhysicsItem::Update(t, update_assets);
}

// -----------------------------------------------------------------------------

void MBTireModel::AddVisualizationAssets(VisualizationType vis) {
    if (vis == VisualizationType::NONE)
        return;

    m_trimesh_shape->SetWireframe(true);
    AddVisualShape(m_trimesh_shape);

    for (const auto& node : m_rim_nodes) {
        auto sph = chrono_types::make_shared<ChVisualShapeSphere>(0.01);
        sph->SetColor(ChColor(1.0f, 0.0f, 0.0f));
        auto loc = m_wheel->TransformPointParentToLocal(node->GetPos());
        m_wheel->AddVisualShape(sph, ChFrame<>(loc));
    }

    //// TODO
    //// properly remove visualization assets added to the wheel body (requires caching the visual shapes)
}

// =============================================================================

void MBTireModel::InjectVariables(ChSystemDescriptor& descriptor) {
    for (auto& node : m_nodes)
        node->InjectVariables(descriptor);
}

void MBTireModel::InjectKRMMatrices(ChSystemDescriptor& descriptor) {
    if (!m_stiff)
        return;

    for (auto& spring : m_grid_lin_springs)
        descriptor.InsertKRMBlock(&spring.KRM);
    for (auto& spring : m_edge_lin_springs)
        descriptor.InsertKRMBlock(&spring.KRM);
    for (auto& spring : m_grid_rot_springs)
        descriptor.InsertKRMBlock(&spring.KRM);
    for (auto& spring : m_edge_rot_springs)
        descriptor.InsertKRMBlock(&spring.KRM);
}

void MBTireModel::LoadKRMMatrices(double Kfactor, double Rfactor, double Mfactor) {
    if (!m_stiff)
        return;

    for (auto& spring : m_grid_lin_springs)
        spring.CalculateJacobian(Kfactor, Rfactor);
    for (auto& spring : m_edge_lin_springs)
        spring.CalculateJacobian(m_full_jac, Kfactor, Rfactor);
    for (auto& spring : m_grid_rot_springs)
        spring.CalculateJacobian(Kfactor, Rfactor);
    for (auto& spring : m_edge_rot_springs)
        spring.CalculateJacobian(m_full_jac, Kfactor, Rfactor);
}

void MBTireModel::IntStateGather(const unsigned int off_x,
                                 ChState& x,
                                 const unsigned int off_v,
                                 ChStateDelta& v,
                                 double& T) {
    unsigned int local_off_x = 0;
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateGather(off_x + local_off_x, x, off_v + local_off_v, v, T);
        local_off_x += node->GetNumCoordsPosLevelActive();
        local_off_v += node->GetNumCoordsVelLevelActive();
    }

    T = GetChTime();
}

void MBTireModel::IntStateScatter(const unsigned int off_x,
                                  const ChState& x,
                                  const unsigned int off_v,
                                  const ChStateDelta& v,
                                  const double T,
                                  bool full_update) {
    unsigned int local_off_x = 0;
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateScatter(off_x + local_off_x, x, off_v + local_off_v, v, T);
        local_off_x += node->GetNumCoordsPosLevelActive();
        local_off_v += node->GetNumCoordsVelLevelActive();
    }

    Update(T, full_update);
}

void MBTireModel::IntStateGatherAcceleration(const unsigned int off_a, ChStateDelta& a) {
    unsigned int local_off_a = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateGatherAcceleration(off_a + local_off_a, a);
        local_off_a += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntStateScatterAcceleration(const unsigned int off_a, const ChStateDelta& a) {
    unsigned int local_off_a = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateScatterAcceleration(off_a + local_off_a, a);
        local_off_a += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntStateIncrement(const unsigned int off_x,
                                    ChState& x_new,
                                    const ChState& x,
                                    const unsigned int off_v,
                                    const ChStateDelta& Dv) {
    unsigned int local_off_x = 0;
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateIncrement(off_x + local_off_x, x_new, x, off_v + local_off_v, Dv);
        local_off_x += node->GetNumCoordsPosLevelActive();
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntStateGetIncrement(const unsigned int off_x,
                                       const ChState& x_new,
                                       const ChState& x,
                                       const unsigned int off_v,
                                       ChStateDelta& Dv) {
    unsigned int local_off_x = 0;
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntStateGetIncrement(off_x + local_off_x, x_new, x, off_v + local_off_v, Dv);
        local_off_x += node->GetNumCoordsPosLevelActive();
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntLoadResidual_F(const unsigned int off, ChVectorDynamic<>& R, const double c) {
    // Synchronize position and velocity of rim nodes with wheel/spindle state
    SetRimNodeStates();

    // Calculate spring forces:
    // - set them as applied forces on the FEA nodes
    // - accumulate force and torque on wheel/spindle body
    CalculateForces();

    // Load nodal forces into R
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntLoadResidual_F(off + local_off_v, R, c);
        local_off_v += node->GetNumCoordsVelLevelActive();
    }

    ////std::cout << "   " << m_wheel_force << "             " << m_wheel_torque << std::endl;

    // Load wheel body forces into R
    if (m_wheel->Variables().IsActive()) {
        R.segment(m_wheel->Variables().GetOffset() + 0, 3) += c * m_wheel_force.eigen();
        R.segment(m_wheel->Variables().GetOffset() + 3, 3) += c * m_wheel_torque.eigen();
    }
}

void MBTireModel::IntLoadResidual_Mv(const unsigned int off,
                                     ChVectorDynamic<>& R,
                                     const ChVectorDynamic<>& w,
                                     const double c) {
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntLoadResidual_Mv(off + local_off_v, R, w, c);
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntLoadLumpedMass_Md(const unsigned int off, ChVectorDynamic<>& Md, double& err, const double c) {
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntLoadLumpedMass_Md(off + local_off_v, Md, err, c);
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntToDescriptor(const unsigned int off_v,
                                  const ChStateDelta& v,
                                  const ChVectorDynamic<>& R,
                                  const unsigned int off_L,
                                  const ChVectorDynamic<>& L,
                                  const ChVectorDynamic<>& Qc) {
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntToDescriptor(off_v + local_off_v, v, R);
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

void MBTireModel::IntFromDescriptor(const unsigned int off_v,
                                    ChStateDelta& v,
                                    const unsigned int off_L,
                                    ChVectorDynamic<>& L) {
    unsigned int local_off_v = 0;
    for (auto& node : m_nodes) {
        node->NodeIntFromDescriptor(off_v + local_off_v, v);
        local_off_v += node->GetNumCoordsVelLevelActive();
    }
}

}  // end namespace vehicle
}  // end namespace chrono