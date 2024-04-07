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
// Authors: Alessandro Tasora, Radu Serban 
// =============================================================================

#include <cstdlib>
#include <algorithm>

#include "chrono/collision/bullet/ChCollisionModelBullet.h"
#include "chrono/physics/ChSystem.h"
#include "chrono/physics/ChMatterPeridynamics.h"
#include "chrono/physics/ChProximityContainerPeridynamics.h"

namespace chrono {
namespace fea {

using namespace geometry;

// Register into the object factory, to enable run-time dynamic creation and persistence
CH_FACTORY_REGISTER(ChMatterPeridynamics)

ChNodePeridynamics::ChNodePeridynamics()
    : container(nullptr), pos_ref(VNULL), UserForce(VNULL), volume(0.01), h_rad(0.1), coll_rad(0.001), hardening(0) {

    SetMass(0.01);
    density = GetMass() / volume;
}

ChNodePeridynamics::ChNodePeridynamics(const ChNodePeridynamics& other) : ChNodeXYZ(other) {

    //auto cshape = chrono_types::make_shared<ChCollisionShapePoint>(other.container->GetMaterialSurface(), VNULL, other.coll_rad);
    auto cshape = chrono_types::make_shared<ChCollisionShapeSphere>(other.container->GetMaterialSurface(),other.coll_rad);

    collision_model->AddShape(cshape);

    pos_ref = other.pos_ref;
    UserForce = other.UserForce;
    h_rad = other.h_rad;
    coll_rad = other.coll_rad;
    SetMass(other.GetMass());
    volume = other.volume;
    density = other.density;
    hardening = other.hardening;

    t_strain = other.t_strain;
    p_strain = other.p_strain;
    e_strain = other.e_strain;
    e_stress = other.e_stress;

    container = other.container;

    variables = other.variables;
}

ChNodePeridynamics::~ChNodePeridynamics() {
}

ChNodePeridynamics& ChNodePeridynamics::operator=(const ChNodePeridynamics& other) {
    if (&other == this)
        return *this;

    ChNodeXYZ::operator=(other);

    //auto cshape = chrono_types::make_shared<ChCollisionShapePoint>(other.container->GetMaterialSurface(), VNULL, other.coll_rad);
    auto cshape = chrono_types::make_shared<ChCollisionShapeSphere>(other.container->GetMaterialSurface(), other.coll_rad);

    collision_model->AddShape(cshape);

    collision_model->SetContactable(this);

    pos_ref = other.pos_ref;
    UserForce = other.UserForce;
    h_rad = other.h_rad;
    SetCollisionRadius(other.coll_rad);
    SetMass(other.GetMass());
    volume = other.volume;
    density = other.density;
    hardening = other.hardening;

    t_strain = other.t_strain;
    p_strain = other.p_strain;
    e_strain = other.e_strain;
    e_stress = other.e_stress;

    container = other.container;

    variables = other.variables;

    return *this;
}


void ChNodePeridynamics::SetHorizonRadius(double mr) {
    h_rad = mr;
    double aabb_rad = h_rad / 2;  // to avoid too many pairs: bounding boxes hemisizes will sum..  __.__--*--
    if (auto mshape = std::dynamic_pointer_cast<ChCollisionModelBullet>(GetCollisionModel()->GetShape(0).first))
        mshape->SetSphereRadius(coll_rad, ChMax(0.0, aabb_rad - coll_rad));
}

void ChNodePeridynamics::SetCollisionRadius(double mr) {
    coll_rad = mr;
    double aabb_rad = h_rad / 2;  // to avoid too many pairs: bounding boxes hemisizes will sum..  __.__--*--
    if (auto mshape = std::dynamic_pointer_cast<ChCollisionModelBullet>(GetCollisionModel()->GetShape(0).first))
        mshape->SetSphereRadius(coll_rad, ChMax(0.0, aabb_rad - coll_rad));
}


void ChNodePeridynamics::ContactForceLoadResidual_F(const ChVector<>& F,
                                                    const ChVector<>& T,
                                                const ChVector<>& abs_point,
                                                ChVectorDynamic<>& R) {
    R.segment(NodeGetOffsetW(),3) += F.eigen();
}

void ChNodePeridynamics::ComputeJacobianForContactPart(const ChVector<>& abs_point,
                                                   ChMatrix33<>& contact_plane,
                                                   type_constraint_tuple& jacobian_tuple_N,
                                                   type_constraint_tuple& jacobian_tuple_U,
                                                   type_constraint_tuple& jacobian_tuple_V,
                                                   bool second) {
    ChMatrix33<> Jx1 = contact_plane.transpose();
    if (!second)
        Jx1 *= -1;

    jacobian_tuple_N.Get_Cq().segment(0,3) = Jx1.row(0);
    jacobian_tuple_U.Get_Cq().segment(0,3) = Jx1.row(1);
    jacobian_tuple_V.Get_Cq().segment(0,3) = Jx1.row(2);
}

ChPhysicsItem* ChNodePeridynamics::GetPhysicsItem() {
    return container;
}

// -----------------------------------------------------------------------------

/// CLASS FOR Peridynamics NODE CLUSTER

ChMatterPeridynamics::ChMatterPeridynamics() : viscosity(0), do_collide(false) {
    // Default: VonMises material
    material = chrono_types::make_shared<ChContinuumPlasticVonMises>();

    // Default: NSC material
    matsurface = chrono_types::make_shared<ChMaterialSurfaceNSC>();
}

ChMatterPeridynamics::ChMatterPeridynamics(const ChMatterPeridynamics& other) : ChIndexedNodes(other) {
    do_collide = other.do_collide;

    matsurface = other.matsurface;

    ResizeNnodes(other.GetNnodes(),0.001);
}

ChMatterPeridynamics::~ChMatterPeridynamics() {
    // delete nodes
    ResizeNnodes(0,0);
}

void ChMatterPeridynamics::ReplaceMaterial(std::shared_ptr<ChContinuumElastoplastic> newmaterial) {
    material = newmaterial;
}

void ChMatterPeridynamics::ResizeNnodes(int newsize, double mhorizon) {
    bool oldcoll = GetCollide();
    SetCollide(false);  // this will remove old particle coll.models from coll.engine, if previously added

    nodes.resize(newsize);

    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j] = chrono_types::make_shared<ChNodePeridynamics>();

        nodes[j]->variables.SetUserData((void*)this);
        
        nodes[j]->h_rad = mhorizon;

        //auto cshape = chrono_types::make_shared<ChCollisionShapePoint>(matsurface, VNULL, 0.5); //// TEST
        auto cshape = chrono_types::make_shared<ChCollisionShapeSphere>(matsurface, mhorizon*0.5); //// TEST
        nodes[j]->GetCollisionModel()->AddShape(cshape);
        //nodes[j]->GetCollisionModel()->Build(); OBSOLETE
    }

    SetCollide(oldcoll);  // this will also add particle coll.models to coll.engine, if already in a ChSystem
}

std::shared_ptr<ChNodePeridynamics> ChMatterPeridynamics::AddNode(ChVector<double> initial_state, double mhorizon) {
    auto newp = chrono_types::make_shared<ChNodePeridynamics>();

    newp->SetPos(initial_state);
    newp->SetPosReference(initial_state);

    nodes.push_back(newp);

    newp->SetMatterContainer(this);

    newp->variables.SetUserData((void*)this);

    newp->h_rad = mhorizon;

    auto cshape = chrono_types::make_shared<ChCollisionShapePoint>(matsurface, VNULL, mhorizon*0.5);  //// TEST
    newp->AddCollisionShape(cshape);

    return (newp);
}

void ChMatterPeridynamics::FillBox(const ChVector<> size,
                               const double spacing,
                               const double initial_density,
                               const ChCoordsys<> boxcoords,
                               const bool do_centeredcube,
                               const double horizon_sfactor,
                               const double randomness) {
    int samples_x = (int)(size.x() / spacing);
    int samples_y = (int)(size.y() / spacing);
    int samples_z = (int)(size.z() / spacing);
    int totsamples = 0;

    double mrandomness = randomness;
    if (do_centeredcube)
        mrandomness = randomness * 0.5;


    double horizon = horizon_sfactor * spacing;

    for (int ix = 0; ix < samples_x; ix++)
        for (int iy = 0; iy < samples_y; iy++)
            for (int iz = 0; iz < samples_z; iz++) {
                ChVector<> pos(ix * spacing - 0.5 * size.x(), iy * spacing - 0.5 * size.y(), iz * spacing - 0.5 * size.z());
                pos += ChVector<>(mrandomness * ChRandom() * spacing, mrandomness * ChRandom() * spacing,
                                  mrandomness * ChRandom() * spacing);
                auto newp = AddNode(boxcoords.TransformLocalToParent(pos), horizon);
                newp->is_elastic = true;
                if ((ix == 0) || (ix == samples_x - 1) || (iy == 0) || (iy == samples_y - 1) || (iz == 0) || (iz == samples_z - 1)) {
                    newp->is_boundary = true;
                }

                totsamples++;

                if (do_centeredcube && !((ix == samples_x - 1) || (iy == samples_y - 1) || (iz == samples_z - 1))) {
                    ChVector<> pos2 = pos + 0.5 * ChVector<>(spacing, spacing, spacing);
                    pos2 += ChVector<>(mrandomness * ChRandom() * spacing, mrandomness * ChRandom() * spacing,
                                       mrandomness * ChRandom() * spacing);
                    newp = AddNode(boxcoords.TransformLocalToParent(pos2), horizon);
                    newp->is_elastic = true;
                    //newp->is_boundary = true;
                    totsamples++;
                }
            }

    double mtotvol = size.x() * size.y() * size.z();
    double mtotmass = mtotvol * initial_density;
    double nodemass = mtotmass / (double)totsamples;

    for (unsigned int ip = 0; ip < GetNnodes(); ip++) {
        // downcasting
        std::shared_ptr<ChNodePeridynamics> mnode(nodes[ip]);
        assert(mnode);

 //       mnode->SetHorizonRadius(horizon);
 //       mnode->SetCollisionRadius(spacing * 0.1);
        mnode->SetMass(nodemass);
    }

    GetMaterial()->Set_density(initial_density);
}



void ChMatterPeridynamics::SetupInitialBonds() {

    this->GetSystem()->Setup();
    auto coll_sys = this->GetSystem()->GetCollisionSystem();
    if (!coll_sys)
         return;
    coll_sys->Initialize();
    this->GetSystem()->ComputeCollisions();
   
    // First, find if any ChProximityContainerPeridynamics object is present
    // in the system,

    std::shared_ptr<ChProximityContainerPeridynamics> edges;
    for (auto& otherphysics : GetSystem()->Get_otherphysicslist()) {
        if ((edges = std::dynamic_pointer_cast<ChProximityContainerPeridynamics>(otherphysics)))
            break;
    }
    assert(edges);  // If using a ChMatterPeridynamics, you must add also a ChProximityContainerPeridynamics.

    edges->UpdateProximalToElastic();

    for (auto& anode : this->nodes) {
        if (anode->is_elastic && !anode->is_boundary) {
            
            // If disabling collision, remove from collision system if already processed
            if (anode->GetCollisionModel()->HasImplementation()) {
                coll_sys->Remove(anode->GetCollisionModel());
            }
        }
    }
}


// STATE BOOKKEEPING FUNCTIONS

void ChMatterPeridynamics::IntStateGather(const unsigned int off_x,  // offset in x state vector
                                      ChState& x,                // state vector, position part
                                      const unsigned int off_v,  // offset in v state vector
                                      ChStateDelta& v,           // state vector, speed part
                                      double& T                  // time
                                      ) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        x.segment(off_x + 3 * j, 3) = nodes[j]->pos.eigen();
        v.segment(off_v + 3 * j, 3) = nodes[j]->pos_dt.eigen();
    }
    T = GetChTime();
}

void ChMatterPeridynamics::IntStateScatter(const unsigned int off_x,  // offset in x state vector
                                       const ChState& x,          // state vector, position part
                                       const unsigned int off_v,  // offset in v state vector
                                       const ChStateDelta& v,     // state vector, speed part
                                       const double T,            // time
                                       bool full_update           // perform complete update
) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->pos = x.segment(off_x + 3 * j, 3);
        nodes[j]->pos_dt = v.segment(off_v + 3 * j, 3);
    }
    SetChTime(T);
    Update(T, full_update);
}

void ChMatterPeridynamics::IntStateGatherAcceleration(const unsigned int off_a, ChStateDelta& a) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        a.segment(off_a + 3 * j, 3) = nodes[j]->pos_dtdt.eigen();
    }
}

void ChMatterPeridynamics::IntStateScatterAcceleration(const unsigned int off_a, const ChStateDelta& a) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->SetPos_dtdt(a.segment(off_a + 3 * j, 3));
    }
}

void ChMatterPeridynamics::IntLoadResidual_F(
    const unsigned int off,  // offset in R residual (not used here! use particle's offsets)
    ChVectorDynamic<>& R,    // result: the R residual, R += c*F
    const double c           // a scaling factor
    ) {
    // COMPUTE THE FORCES HERE

    // First, find if any ChProximityContainerPeridynamics object is present
    // in the system,

    std::shared_ptr<ChProximityContainerPeridynamics> edges;
    for (auto otherphysics : GetSystem()->Get_otherphysicslist()) {
        if ((edges = std::dynamic_pointer_cast<ChProximityContainerPeridynamics>(otherphysics)))
            break;
    }
    assert(edges);  // If using a ChMatterPeridynamics, you must add also a ChProximityContainerPeridynamics.

    // 1- Per-node initialization

    #pragma omp parallel for
    for (int j = 0; j < nodes.size(); j++) {
        nodes[j]->J.setZero();
        nodes[j]->Amoment.setZero();
        nodes[j]->t_strain.setZero();
        nodes[j]->e_stress.setZero();
        nodes[j]->UserForce = VNULL;
        nodes[j]->density = 0;
    }

    // 2- Per-edge initialization and accumulation of values in particles's J, Amoment, m_v, density

    edges->AccumulateStep1();

    // 3- Per-node inversion of A and computation of strain stress

    #pragma omp parallel for
    for (int j = 0; j < nodes.size(); j++) {
        std::shared_ptr<ChNodePeridynamics> mnode(nodes[j]);
        assert(mnode);

        // node volume is v=mass/density
        if (mnode->density > 0)
            mnode->volume = mnode->GetMass() / mnode->density;
        else
            mnode->volume = 0;

        // Compute A inverse
        ChMatrix33<> M_tmp = mnode->Amoment;

        if (std::abs(M_tmp.determinant()) < 0.00003) {
            mnode->Amoment.setZero();   // deactivate if not possible to invert
            mnode->e_strain.setZero();  // detach
        } else {
            mnode->Amoment = M_tmp.inverse();

            // Compute J = ( A^-1 * [dwg | dwg | dwg] )' + I
            M_tmp = mnode->Amoment * mnode->J;
            M_tmp(0, 0) += 1;
            M_tmp(1, 1) += 1;
            M_tmp(2, 2) += 1;
            mnode->J = M_tmp.transpose();

            // Compute step strain tensor  de = J'*J - I
            ChMatrix33<> mtensor = M_tmp * mnode->J;
            mtensor(0, 0) -= 1;
            mtensor(1, 1) -= 1;
            mtensor(2, 2) -= 1;

            mnode->t_strain.ConvertFromMatrix(mtensor);  // store 'step strain' de, change in total strain

            ChStrainTensor<> strainplasticflow;
            material->ComputeReturnMapping(strainplasticflow,   // dEp, flow of elastic strain (correction)
                                           nodes[j]->t_strain,  // increment of total strain
                                           nodes[j]->e_strain,  // last elastic strain
                                           nodes[j]->p_strain   // last plastic strain
                                           );
            ChStrainTensor<> proj_e_strain = nodes[j]->e_strain - strainplasticflow + nodes[j]->t_strain;
            GetMaterial()->ComputeElasticStress(mnode->e_stress, proj_e_strain);
            mnode->e_stress.ConvertToMatrix(mtensor);

            /*
            // Compute elastic stress tensor  sigma= C*epsilon
            //   NOTE: it should be better to perform stress computation on corrected e_strain, _after_ the
            //   return mapping (see later), but for small timestep it could be the same.
            ChStrainTensor<> guesstot_e_strain; // anticipate the computation of total strain for anticipating strains
            guesstot_e_strain.MatrAdd(mnode->e_strain, mnode->t_strain);
            GetMaterial()->ComputeElasticStress(mnode->e_stress, guesstot_e_strain);
            mnode->e_stress.ConvertToMatrix(mtensor);
            */

            // Precompute 2*v*J*sigma*A^-1
            mnode->FA = (2 * mnode->volume) * mnode->J * mtensor * mnode->Amoment;
        }
    }

    // 4- Per-edge force transfer from stress, and add also viscous forces

    edges->AccumulateStep2();

    // 5- Per-node load force

    #pragma omp parallel for
    for (int j = 0; j < nodes.size(); j++) {
        // particle gyroscopic force:
        // none.

        // add gravity
        ChVector<> Gforce = GetSystem()->Get_G_acc() * nodes[j]->GetMass();
        ChVector<> TotForce = nodes[j]->UserForce + Gforce;

        std::shared_ptr<ChNodePeridynamics> mnode(nodes[j]);
        assert(mnode);

        R.segment(off + 3 * j, 3) += c * TotForce.eigen();
    }
}

void ChMatterPeridynamics::IntLoadResidual_Mv(const unsigned int off,      ///< offset in R residual
                                          ChVectorDynamic<>& R,        ///< result: the R residual, R += c*M*v
                                          const ChVectorDynamic<>& w,  ///< the w vector
                                          const double c               ///< a scaling factor
                                          ) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        R(off + 3 * j) += c * nodes[j]->GetMass() * w(off + 3 * j);
        R(off + 3 * j + 1) += c * nodes[j]->GetMass() * w(off + 3 * j + 1);
        R(off + 3 * j + 2) += c * nodes[j]->GetMass() * w(off + 3 * j + 2);
    }
}

void ChMatterPeridynamics::IntToDescriptor(const unsigned int off_v,
                                       const ChStateDelta& v,
                                       const ChVectorDynamic<>& R,
                                       const unsigned int off_L,
                                       const ChVectorDynamic<>& L,
                                       const ChVectorDynamic<>& Qc) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->variables.Get_qb() = v.segment(off_v + 3 * j, 3);
        nodes[j]->variables.Get_fb() = R.segment(off_v + 3 * j, 3);
    }
}

void ChMatterPeridynamics::IntFromDescriptor(const unsigned int off_v,
                                         ChStateDelta& v,
                                         const unsigned int off_L,
                                         ChVectorDynamic<>& L) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        v.segment(off_v + 3 * j, 3) = nodes[j]->variables.Get_qb();
    }
}

void ChMatterPeridynamics::InjectVariables(ChSystemDescriptor& mdescriptor) {
    // variables.SetDisabled(!IsActive());
    for (unsigned int j = 0; j < nodes.size(); j++) {
        mdescriptor.InsertVariables(&(nodes[j]->variables));
    }
}

void ChMatterPeridynamics::VariablesFbReset() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->variables.Get_fb().setZero();
    }
}

void ChMatterPeridynamics::VariablesFbLoadForces(double factor) {
    // COMPUTE THE FORCES HERE

    // First, find if any ChProximityContainerPeridynamics object is present
    // in the system,

    std::shared_ptr<ChProximityContainerPeridynamics> edges;
    for (auto otherphysics : GetSystem()->Get_otherphysicslist()) {
        if ((edges = std::dynamic_pointer_cast<ChProximityContainerPeridynamics>(otherphysics)))
            break;
    }
    assert(edges);  // If using a ChMatterPeridynamics, you must add also a ChProximityContainerPeridynamics.

    // 1- Per-node initialization

    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->J.setZero();
        nodes[j]->Amoment.setZero();
        nodes[j]->t_strain.setZero();
        nodes[j]->e_stress.setZero();
        nodes[j]->UserForce = VNULL;
        nodes[j]->density = 0;
    }

    // 2- Per-edge initialization and accumulation of values in particles's J, Amoment, m_v, density

    edges->AccumulateStep1();

    // 3- Per-node inversion of A and computation of strain stress

    for (unsigned int j = 0; j < nodes.size(); j++) {
        std::shared_ptr<ChNodePeridynamics> mnode(nodes[j]);
        assert(mnode);

        // node volume is v=mass/density
        if (mnode->density > 0)
            mnode->volume = mnode->GetMass() / mnode->density;
        else
            mnode->volume = 0;

        // Compute A inverse
        ChMatrix33<> M_tmp = mnode->Amoment;

        if (std::abs(M_tmp.determinant()) < 0.00003) {
            mnode->Amoment.setZero();     // deactivate if not possible to invert
            mnode->e_strain.setZero();  // detach
        } else {
            mnode->Amoment = M_tmp.inverse();

            // Compute J = ( A^-1 * [dwg | dwg | dwg] )' + I
            M_tmp = mnode->Amoment * mnode->J;
            M_tmp(0, 0) += 1;
            M_tmp(1, 1) += 1;
            M_tmp(2, 2) += 1;
            mnode->J = M_tmp.transpose();

            // Compute step strain tensor  de = J'*J - I
            ChMatrix33<> mtensor = M_tmp * mnode->J;
            mtensor(0, 0) -= 1;
            mtensor(1, 1) -= 1;
            mtensor(2, 2) -= 1;

            mnode->t_strain.ConvertFromMatrix(mtensor);  // store 'step strain' de, change in total strain

            ChStrainTensor<> strainplasticflow;
            material->ComputeReturnMapping(strainplasticflow,   // dEp, flow of elastic strain (correction)
                                           nodes[j]->t_strain,  // increment of total strain
                                           nodes[j]->e_strain,  // last elastic strain
                                           nodes[j]->p_strain   // last plastic strain
                                           );
            ChStrainTensor<> proj_e_strain = nodes[j]->e_strain - strainplasticflow + nodes[j]->t_strain;
            GetMaterial()->ComputeElasticStress(mnode->e_stress, proj_e_strain);
            mnode->e_stress.ConvertToMatrix(mtensor);

            /*
            // Compute elastic stress tensor  sigma= C*epsilon
            //   NOTE: it should be better to perform stress computation on corrected e_strain, _after_ the
            //   return mapping (see later), but for small timestep it could be the same.
            ChStrainTensor<> guesstot_e_strain; // anticipate the computation of total strain for anticipating strains
            guesstot_e_strain.MatrAdd(mnode->e_strain, mnode->t_strain);
            GetMaterial()->ComputeElasticStress(mnode->e_stress, guesstot_e_strain);
            mnode->e_stress.ConvertToMatrix(mtensor);
            */

            // Precompute 2*v*J*sigma*A^-1
            mnode->FA = (2 * mnode->volume) * mnode->J * mtensor * mnode->Amoment;
        }
    }

    // 4- Per-edge force transfer from stress, and add also viscous forces

    edges->AccumulateStep2();

    // 5- Per-node load force

    for (unsigned int j = 0; j < nodes.size(); j++) {
        // particle gyroscopic force:
        // none.

        // add gravity
        ChVector<> Gforce = GetSystem()->Get_G_acc() * nodes[j]->GetMass();
        ChVector<> TotForce = nodes[j]->UserForce + Gforce;

        std::shared_ptr<ChNodePeridynamics> mnode(nodes[j]);
        assert(mnode);

        mnode->variables.Get_fb() += factor * TotForce.eigen();
    }
}

void ChMatterPeridynamics::VariablesFbIncrementMq() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->variables.Compute_inc_Mb_v(nodes[j]->variables.Get_fb(), nodes[j]->variables.Get_qb());
    }
}

void ChMatterPeridynamics::VariablesQbLoadSpeed() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        // set current speed in 'qb', it can be used by the solver when working in incremental mode
        nodes[j]->variables.Get_qb() = nodes[j]->GetPos_dt().eigen();
    }
}

void ChMatterPeridynamics::VariablesQbSetSpeed(double step) {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        ChVector<> old_pos_dt = nodes[j]->GetPos_dt();

        // from 'qb' vector, sets body speed, and updates auxiliary data
        nodes[j]->SetPos_dt(nodes[j]->variables.Get_qb());

        // Compute accel. by BDF (approximate by differentiation);
        if (step) {
            nodes[j]->SetPos_dtdt((nodes[j]->GetPos_dt() - old_pos_dt) / step);
        }
    }
}

void ChMatterPeridynamics::VariablesQbIncrementPosition(double dt_step) {
    // if (!IsActive())
    //	return;

    for (unsigned int j = 0; j < nodes.size(); j++) {
        std::shared_ptr<ChNodePeridynamics> mnode(nodes[j]);
        assert(mnode);

        // Integrate plastic flow
        ChStrainTensor<> strainplasticflow;
        material->ComputeReturnMapping(strainplasticflow,   // dEp, flow of elastic strain (correction)
                                       nodes[j]->t_strain,  // increment of total strain
                                       nodes[j]->e_strain,  // last elastic strain
                                       nodes[j]->p_strain   // last plastic strain
                                       );
        double dtpfact = dt_step * material->Get_flow_rate();
        if (dtpfact > 1.0)
            dtpfact = 1.0;  // clamp if dt is larger than plastic flow duration

        nodes[j]->p_strain += strainplasticflow * dtpfact;

        // Increment total elastic tensor and proceed for next step
        nodes[j]->pos_ref = nodes[j]->pos;
        nodes[j]->e_strain += nodes[j]->t_strain;
        //	nodes[j]->e_strain.MatrDec(strainplasticflow*dtpfact);
        nodes[j]->t_strain.setZero();  // unuseful? will be overwritten anyway
    }

    for (unsigned int j = 0; j < nodes.size(); j++) {
        // Updates position with incremental action of speed contained in the
        // 'qb' vector:  pos' = pos + dt * speed   , like in an Eulero step.

        ChVector<> newspeed(nodes[j]->variables.Get_qb());

        // ADVANCE POSITION: pos' = pos + dt * vel
        nodes[j]->SetPos(nodes[j]->GetPos() + newspeed * dt_step);
    }
}

void ChMatterPeridynamics::SetNoSpeedNoAcceleration() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->SetPos_dt(VNULL);
        nodes[j]->SetPos_dtdt(VNULL);
    }
}

void ChMatterPeridynamics::Update(bool update_assets) {
    ChMatterPeridynamics::Update(GetChTime(), update_assets);
}

void ChMatterPeridynamics::Update(double mytime, bool update_assets) {
    // Inherit time changes of parent class
    ChPhysicsItem::Update(mytime, update_assets);

    // TrySleeping();    // See if the body can fall asleep; if so, put it to sleeping
    // ClampSpeed();     // Apply limits (if in speed clamping mode) to speeds.
}

// collision stuff
void ChMatterPeridynamics::SetCollide(bool mcoll) {
    if (mcoll == do_collide)
        return;

    if (mcoll) {
        do_collide = true;
        if (GetSystem()) {
            for (unsigned int j = 0; j < nodes.size(); j++) {
                GetSystem()->GetCollisionSystem()->Add(nodes[j]->GetCollisionModel());
            }
        }
    } else {
        do_collide = false;
        if (GetSystem()) {
            for (unsigned int j = 0; j < nodes.size(); j++) {
                GetSystem()->GetCollisionSystem()->Remove(nodes[j]->GetCollisionModel());
            }
        }
    }
}

void ChMatterPeridynamics::SyncCollisionModels() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->GetCollisionModel()->SyncPosition();
    }
}

void ChMatterPeridynamics::AddCollisionModelsToSystem(ChCollisionSystem* coll_sys) const {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        if (nodes[j]->GetCollisionModel())
            coll_sys->Add(nodes[j]->GetCollisionModel());
    }
}

void ChMatterPeridynamics::RemoveCollisionModelsFromSystem(ChCollisionSystem* coll_sys) const {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        if (nodes[j]->GetCollisionModel())
            coll_sys->Remove(nodes[j]->GetCollisionModel());
    }
}

////

/*
void ChMatterPeridynamics::UpdateParticleCollisionModels() {
    for (unsigned int j = 0; j < nodes.size(); j++) {
        nodes[j]->GetCollisionModel()->Clear();
        nodes[j]->GetCollisionModel()->Build();
    }
}
*/

//////// FILE I/O

void ChMatterPeridynamics::ArchiveOut(ChArchiveOut& marchive) {
    // version number
    marchive.VersionWrite<ChMatterPeridynamics>();

    // serialize the parent class data too
    ChIndexedNodes::ArchiveOut(marchive);

    // serialize all member data:
    //***TODO
}

void ChMatterPeridynamics::ArchiveIn(ChArchiveIn& marchive) {
    // version number
    /*int version =*/ marchive.VersionRead<ChMatterPeridynamics>();

    // deserialize the parent class data too
    ChIndexedNodes::ArchiveIn(marchive);

    // deserialize all member data:
    //***TODO
}


// -----------------------------------------------------------------------------------------------------


ChNodePeri::ChNodePeri()
    : pos_ref(VNULL), UserForce(VNULL), volume(0.01), h_rad(0.2), coll_rad(0.0006) {

    SetMass(0.01);
}

ChNodePeri::ChNodePeri(const ChNodePeri& other) : ChNodeXYZ(other) {

    //auto cshape = chrono_types::make_shared<ChCollisionShapePoint>(other.container->GetMaterialSurface(), VNULL, other.coll_rad);
    auto cshape = chrono_types::make_shared<ChCollisionShapeSphere>(other.GetCollisionModel()->GetShape(0).first->GetMaterial(),other.coll_rad);

    collision_model->AddShape(cshape);

    pos_ref = other.pos_ref;
    UserForce = other.UserForce;
    h_rad = other.h_rad;
    SetCollisionRadius(other.coll_rad);
    SetMass(other.GetMass());
    volume = other.volume;
    F = other.F;
    is_boundary = other.is_boundary;
    is_colliding = other.is_colliding;
    is_elastic = other.is_elastic;
    is_requiring_bonds = other.is_requiring_bonds;

    variables = other.variables;
}

ChNodePeri::~ChNodePeri() {
}

void ChNodePeri::SetHorizonRadius(double mr) {
    h_rad = mr;
    double aabb_rad = h_rad / 2;  // to avoid too many pairs: bounding boxes hemisizes will sum..  __.__--*--
    if (auto mshape = std::dynamic_pointer_cast<ChCollisionModelBullet>(GetCollisionModel()->GetShape(0).first))
        mshape->SetSphereRadius(coll_rad, ChMax(0.0, aabb_rad - coll_rad));
}

void ChNodePeri::SetCollisionRadius(double mr) {
    coll_rad = mr;
    double aabb_rad = h_rad / 2;  // to avoid too many pairs: bounding boxes hemisizes will sum..  __.__--*--
    if (auto mshape = std::dynamic_pointer_cast<ChCollisionModelBullet>(GetCollisionModel()->GetShape(0).first))
        mshape->SetSphereRadius(coll_rad, ChMax(0.0, aabb_rad - coll_rad));
}

void ChNodePeri::ContactForceLoadResidual_F(    const ChVector<>& F,
                                                const ChVector<>& T,
                                                const ChVector<>& abs_point,
                                                ChVectorDynamic<>& R) {
    R.segment(NodeGetOffsetW(),3) += F.eigen();
}

void ChNodePeri::ComputeJacobianForContactPart(    const ChVector<>& abs_point,
                                                   ChMatrix33<>& contact_plane,
                                                   type_constraint_tuple& jacobian_tuple_N,
                                                   type_constraint_tuple& jacobian_tuple_U,
                                                   type_constraint_tuple& jacobian_tuple_V,
                                                   bool second) {
    ChMatrix33<> Jx1 = contact_plane.transpose();
    if (!second)
        Jx1 *= -1;

    jacobian_tuple_N.Get_Cq().segment(0,3) = Jx1.row(0);
    jacobian_tuple_U.Get_Cq().segment(0,3) = Jx1.row(1);
    jacobian_tuple_V.Get_Cq().segment(0,3) = Jx1.row(2);
}

}  // end namespace fea
}  // end namespace chrono
