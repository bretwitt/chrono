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

#ifndef CHMATTERPERIBULKELASTIC_H
#define CHMATTERPERIBULKELASTIC_H


#include "chrono_peridynamics/ChApiPeridynamics.h"
#include "chrono_peridynamics/ChMatterPeridynamics.h"


namespace chrono {
namespace peridynamics {

class ChProximityContainerPeri;

/// @addtogroup chrono_peridynamics
/// @{




class  ChApiPeridynamics ChMatterDataPerBoundBulk : public ChMatterDataPerBound { 
public: 
    bool broken = false;
    double F_per_bond = 0;
};

class ChApiPeridynamics ChMatterPeriBulkElastic : public ChMatterPeri<ChMatterDataPerNode, ChMatterDataPerBoundBulk> {
public:
    double k_bulk = 100;
    double r = 10;
    double max_stretch = 0.08;

    ChMatterPeriBulkElastic() {};

    // Implement the function that adds the peridynamics force to each node, as a 
    // summation of all the effects of neighbouring nodes.
    virtual void ComputeForces() {
        // loop on bounds
        for (auto& bound : this->bounds) {
            ChMatterDataPerBoundBulk& mbound = bound.second;
            if (!mbound.broken) {
                ChVector<> old_vdist = mbound.nodeB->GetX0() - mbound.nodeA->GetX0();
                ChVector<>     vdist = mbound.nodeB->GetPos() - mbound.nodeA->GetPos();
                double     old_sdist = old_vdist.Length();
                double         sdist = vdist.Length();
                ChVector<>     vdir = vdist.GetNormalized();
                double         svel = Vdot(vdir, mbound.nodeB->GetPos_dt() - mbound.nodeA->GetPos_dt());

                double     stretch = (sdist - old_sdist) / old_sdist;

                double horizon = mbound.nodeA->GetHorizonRadius();
                double K_pih4 = 18.0 * k_bulk / (chrono::CH_C_PI * horizon * horizon * horizon * horizon); 

                ChVector force_val = 0.5 * K_pih4 * stretch;
                
                if (this->r > 0)
                    force_val += 0.5 * this->r * svel;

                mbound.nodeB->F_peridyn += -vdir * force_val * mbound.nodeA->volume;
                mbound.nodeA->F_peridyn += vdir * force_val * mbound.nodeB->volume;

                if (stretch > max_stretch) {
                    mbound.broken = true;
                    // the following will propagate the fracture geometry so that broken parts can collide
                    mbound.nodeA->is_boundary = true; 
                    mbound.nodeB->is_boundary = true;
                }
            }
            else {
                if ((mbound.nodeB->GetPos() - mbound.nodeA->GetPos()).Length() > mbound.nodeA->GetHorizonRadius())
                    bounds.erase(bound.first);
            }

        }
    }
};



class /*ChApiPeridynamics*/ ChVisualPeriBulkElastic : public ChGlyphs {
public:
    ChVisualPeriBulkElastic(std::shared_ptr<ChMatterPeriBulkElastic> amatter) : mmatter(amatter) { is_mutable = true; };
    virtual ~ChVisualPeriBulkElastic() {}

    // set true if you want those attributes to be attached to the glyps 
    // (ex for postprocessing in falsecolor or with vectors with the Blender addon)
    bool attach_velocity = 0;
    bool attach_acceleration = 0;

protected:
    virtual void Update(ChPhysicsItem* updater, const ChFrame<>& frame) {
        if (!mmatter)
            return;
        this->Reserve(mmatter->GetNnodes());
        unsigned int i = 0;
        for (const auto& anode : mmatter->GetMapOfNodes()) {
            this->SetGlyphPoint(i, anode.second.node->GetPos());
            ++i;
        }
        if (attach_velocity) {
            geometry::ChPropertyVector myprop;
            myprop.name = "velocity";
            this->AddProperty(myprop);
            auto mdata = &((geometry::ChPropertyVector*)(m_properties.back()))->data;
            mdata->resize(mmatter->GetNnodes());
            i = 0;
            for (const auto& anode : mmatter->GetMapOfNodes()) {
                mdata->at(i) = anode.second.node->GetPos_dt();
                ++i;
            }
        }
        if (attach_acceleration) {
            geometry::ChPropertyVector myprop;
            myprop.name = "acceleration";
            this->AddProperty(myprop);
            auto mdata = &((geometry::ChPropertyVector*)(m_properties.back()))->data;
            mdata->resize(mmatter->GetNnodes());
            i = 0;
            for (const auto& anode : mmatter->GetMapOfNodes()) {
                mdata->at(i) = anode.second.node->GetPos_dtdt();
                ++i;
            }
        }

    }

    std::shared_ptr<ChMatterPeriBulkElastic> mmatter;
};


class /*ChApiPeridynamics*/ ChVisualPeriBulkElasticBounds : public ChGlyphs {
public:
    ChVisualPeriBulkElasticBounds(std::shared_ptr<ChMatterPeriBulkElastic> amatter) : mmatter(amatter) { is_mutable = true; };
    virtual ~ChVisualPeriBulkElasticBounds() {}

    bool draw_broken = true;
    bool draw_unbroken = false;

protected:
    virtual void Update(ChPhysicsItem* updater, const ChFrame<>& frame) {
        if (!mmatter)
            return;
        this->Reserve(mmatter->GetNbounds());
        unsigned int i = 0;
        for (const auto& anode : mmatter->GetMapOfBounds()) {
            if (anode.second.broken && draw_broken)
                this->SetGlyphVector(i, anode.second.nodeA->GetPos(), anode.second.nodeB->GetPos()-anode.second.nodeA->GetPos(),ChColor(1,0,0));
            if (!anode.second.broken && draw_unbroken)
                this->SetGlyphVector(i, anode.second.nodeA->GetPos(), anode.second.nodeB->GetPos() - anode.second.nodeA->GetPos(),ChColor(0, 0, 1));
            ++i;
        }
    }

    std::shared_ptr<ChMatterPeriBulkElastic> mmatter;
};


/// @} chrono_peridynamics

}  // end namespace peridynamics
}  // end namespace chrono

#endif