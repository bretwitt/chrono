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

#ifndef CHLINKMATE_H
#define CHLINKMATE_H

#include "chrono/physics/ChLink.h"
#include "chrono/physics/ChLinkMask.h"
#include "chrono/solver/ChKblockGeneric.h"

namespace chrono {

/// Base class for all 'simple' constraints between
/// two frames attached to two bodies. These constraints
/// can correspond to the typical 'mating' conditions that
/// are created in assemblies of 3D CAD tools (parallel
/// axis, or face-to-face, etc.).
/// Note that most of the ChLinkMate constraints can be
/// done also with the constraints inherited from ChLinkLock...
/// but in case of links of the ChLinkLock class they
/// reference two ChMarker objects, that can also move, but
/// this could be an unnecessary complication in most cases.

class ChApi ChLinkMate : public ChLink {
  public:
    ChLinkMate() {}
    ChLinkMate(const ChLinkMate& other) : ChLink(other) {}
    virtual ~ChLinkMate() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMate* Clone() const override { return new ChLinkMate(*this); }

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMate, 0)

// -----------------------------------------------------------------------------

/// Generic mate constraint, where one can select which DOFs must be constrained
/// between two frames attached to the two bodies.

class ChApi ChLinkMateGeneric : public ChLinkMate {
  public:
    using ChConstraintVectorX = Eigen::Matrix<double, Eigen::Dynamic, 1, Eigen::ColMajor, 6, 1>;

    ChLinkMateGeneric(bool mc_x = true,
                      bool mc_y = true,
                      bool mc_z = true,
                      bool mc_rx = true,
                      bool mc_ry = true,
                      bool mc_rz = true);

    ChLinkMateGeneric(const ChLinkMateGeneric& other);

    virtual ~ChLinkMateGeneric() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateGeneric* Clone() const override { return new ChLinkMateGeneric(*this); }

    /// Get the link coordinate system, expressed relative to Body2 (the 'master'
    /// body). This represents the 'main' reference of the link: reaction forces
    /// are expressed in this coordinate system.
    /// (It is the coordinate system of the contact plane relative to Body2)
    virtual ChCoordsys<> GetLinkRelativeCoords() override { return frame2.GetCsys(); }

    /// Get the reference frame (expressed in and relative to the absolute frame) of the visual model.
    /// For a ChLinkMate, this returns the absolute coordinate system of the second body.
    virtual ChFrame<> GetVisualModelFrame(unsigned int nclone = 0) override { return frame2 >> *GetBody2(); }

    /// Access the coordinate system considered attached to body1.
    /// Its position is expressed in the coordinate system of body1.
    ChFrame<>& GetFrame1() { return frame1; }

    /// Access the coordinate system considered attached to body2.
    /// Its position is expressed in the coordinate system of body2.
    ChFrame<>& GetFrame2() { return frame2; }

    /// Get the translational Lagrange multipliers, expressed in the master frame F2
    ChVector3d GetLagrangeMultiplier_f() { return gamma_f; }

    /// Get the rotational Lagrange multipliers, expressed in a ghost frame determined by the
    /// projection matrix (this->P) for rho_F1(F2)
    ChVector3d GetLagrangeMultiplier_m() { return gamma_m; }

    bool IsConstrainedX() const { return c_x; }
    bool IsConstrainedY() const { return c_y; }
    bool IsConstrainedZ() const { return c_z; }
    bool IsConstrainedRx() const { return c_rx; }
    bool IsConstrainedRy() const { return c_ry; }
    bool IsConstrainedRz() const { return c_rz; }

    /// Sets which movements (of frame 1 respect to frame 2) are constrained
    void SetConstrainedCoords(bool mc_x, bool mc_y, bool mc_z, bool mc_rx, bool mc_ry, bool mc_rz);

    /// Initialize the generic mate, given the two bodies to be connected, and the absolute position of
    /// the mate (the two frames to connect on the bodies will be initially coincindent to that frame).
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            ChFrame<> mabsframe                   ///< mate frame, in abs. coordinate
    );

    /// Initialize the generic mate, given the two bodies to be connected, the positions of the two frames
    /// to connect on the bodies (each expressed in body or abs. coordinates).
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            ChFrame<> mframe1,                    ///< slave frame 1 (rel. or abs.)
                            ChFrame<> mframe2                     ///< master frame 2 (rel. or abs.)
    );

    /// Initialization based on passing two vectors (point + dir) on the two bodies, which will represent the Z axes of
    /// the two frames (X and Y will be built from the Z vector via Gram Schmidt orthonormalization).
    /// Note: It is safer and recommended to check whether the final result of the master frame F2
    /// is as your expectation since it could affect the output result of the joint, such as the reaction
    /// forces/torques, etc.
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< origin of slave frame 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< origin of master frame 2 (rel. or abs.)
                            const ChVector3d& dir1,                    ///< X axis of slave plane 1 (rel. or abs.)
                            const ChVector3d& dir2                     ///< X axis of master plane 2 (rel. or abs.)
    );

    //
    // UPDATING FUNCTIONS
    //

    /// Update link state. This is called automatically by the solver at each time step.
    /// Update constraint jacobian and frames.
    /// Derived classes must call this parent method and then take care of updating their own assets.
    virtual void Update(double mtime, bool update_assets = true) override;

    /// If some constraint is redundant, return to normal state
    virtual int RestoreRedundant() override;

    /// User can use this to enable/disable all the constraint of
    /// the link as desired.
    virtual void SetDisabled(bool mdis) override;

    /// Ex:3rd party software can set the 'broken' state via this method
    virtual void SetBroken(bool mon) override;

    /// Set this as true to compute the tangent stiffness matrix (Kc) of this constraint.
    /// It is false by default to keep consistent as previous code.
    void SetUseTangentStiffness(bool useKc);

    virtual int GetNumConstraints() override { return m_num_constr; }
    virtual int GetNumConstraintsBilateral() override { return m_num_constr_bil; }
    virtual int GetNumConstraintsUnilateral() override { return m_num_constr_uni; }

    // LINK VIOLATIONS
    // Get the constraint violations, i.e. the residual of the constraint equations and their time derivatives (TODO)

    /// Link violation (residuals of the link constraint equations).
    virtual ChVectorDynamic<> GetConstraintViolation() const override { return C; }

    //
    // STATE FUNCTIONS
    //

    // (override/implement interfaces for global state vectors, see ChPhysicsItem for comments.)
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

    //
    // SOLVER INTERFACE
    //

    virtual void InjectConstraints(ChSystemDescriptor& mdescriptor) override;
    virtual void ConstraintsBiReset() override;
    virtual void ConstraintsBiLoad_C(double factor = 1, double recovery_clamp = 0.1, bool do_clamp = false) override;
    virtual void ConstraintsBiLoad_Ct(double factor = 1) override;
    virtual void ConstraintsLoadJacobians() override;
    virtual void ConstraintsFetch_react(double factor = 1) override;

    /// Tell to a system descriptor that there are item(s) of type
    /// ChKblock in this object (for further passing it to a solver)
    virtual void InjectKRMmatrices(ChSystemDescriptor& descriptor) override;

    /// Add the current stiffness K matrix in encapsulated ChKblock item(s), if any.
    /// The K matrices are load with scaling values Kfactor.
    virtual void KRMmatricesLoad(double Kfactor, double Rfactor, double Mfactor) override;

    //
    // SERIALIZATION
    //

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;

  protected:
    void SetupLinkMask();
    void ChangedLinkMask();

    ChFrame<> frame1;
    ChFrame<> frame2;

    bool c_x;
    bool c_y;
    bool c_z;
    bool c_rx;
    bool c_ry;
    bool c_rz;

    int m_num_constr;    ///< number of constraints
    int m_num_constr_bil;  ///< number of bilateral constraints
    int m_num_constr_uni;  ///< number of unilateral constraints

    ChLinkMask mask;

    ChConstraintVectorX C;  ///< residuals

    // The projection matrix from Lagrange multiplier to reaction torque
    ChMatrix33<> P;

    ChVector3d gamma_f;  ///< store the translational Lagrange multipliers
    ChVector3d gamma_m;  ///< store the rotational Lagrange multipliers

    std::unique_ptr<ChKblockGeneric> Kmatr = nullptr;  ///< the tangent stiffness matrix of constraint
};

CH_CLASS_VERSION(ChLinkMateGeneric, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of plane-to-plane type. 
/// The planes are defined by the X and Y axes of the two frames i.e. the two Z axes are parallel.
/// An offset distance can be provided.

class ChApi ChLinkMatePlanar : public ChLinkMateGeneric {
  protected:
    bool m_flipped;
    double m_distance;

  public:
    ChLinkMatePlanar() : ChLinkMateGeneric(false, false, true, true, true, false), m_flipped(false), m_distance(0) {}
    ChLinkMatePlanar(const ChLinkMatePlanar& other);
    ~ChLinkMatePlanar() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMatePlanar* Clone() const override { return new ChLinkMatePlanar(*this); }

    /// Tell if the two normals must be opposed (flipped=true) or must have the same direction (flipped=false).
    void SetFlipped(bool doflip);

    /// Tell if the two normals are opposed (flipped=true) or have the same direction (flipped=false).
    bool IsFlipped() const { return m_flipped; }

    /// Set the distance between the two planes, in normal direction.
    void SetDistance(double distance) { m_distance = distance; }

    /// Get the requested distance between the two planes, in normal direction.
    double GetDistance() const { return m_distance; }

    /// Initialize the link by providing a point and a normal direction on each plane, each expressed in body or abs reference.
    /// Normals can be either aligned or opposed depending on the ::SetFlipped() method.
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< point on slave plane 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< point on master plane 2 (rel. or abs.)
                            const ChVector3d& norm1,                    ///< normal of slave plane 1 (rel. or abs.)
                            const ChVector3d& norm2                     ///< normal of master plane 2 (rel. or abs.)
                            ) override;

    /// Update link state. This is called automatically by the solver at each time step.
    /// Update constraint jacobian and frames.
    virtual void Update(double time, bool update_assets = true) override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMatePlanar, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of coaxial type. This correspond to the
/// typical cylinder-vs-cylinder mating used in 3D CAD assemblies.
/// The two coaxial axes are the Z axes of the two frames.

class ChApi ChLinkMateCylindrical : public ChLinkMateGeneric {
  protected:
    bool m_flipped;

  public:
    ChLinkMateCylindrical() : ChLinkMateGeneric(true, true, false, true, true, false), m_flipped(false) {}
    ChLinkMateCylindrical(const ChLinkMateCylindrical& other);
    virtual ~ChLinkMateCylindrical() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateCylindrical* Clone() const override { return new ChLinkMateCylindrical(*this); }

    /// Tell if the two axes must be opposed (flipped=true) or must have the same verse (flipped=false)
    void SetFlipped(bool doflip);
    bool IsFlipped() const { return m_flipped; }

    using ChLinkMateGeneric::Initialize;


    /// Specialized initialization for coaxial mate, given the two bodies to be connected, two points, two directions
    /// (each expressed in body or abs. coordinates).
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< point on slave axis 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< point on master axis 2 (rel. or abs.)
                            const ChVector3d& dir1,                     ///< direction of slave axis 1 (rel. or abs.)
                            const ChVector3d& dir2                      ///< direction of master axis 2 (rel. or abs.)
                            ) override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateCylindrical, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of revolute type.
/// The two revolute axes are the Z axes of the two frames.

class ChApi ChLinkMateRevolute : public ChLinkMateGeneric {
  protected:
    bool m_flipped;

  public:
    ChLinkMateRevolute() : ChLinkMateGeneric(true, true, true, true, true, false), m_flipped(false) {}
    ChLinkMateRevolute(const ChLinkMateRevolute& other);
    virtual ~ChLinkMateRevolute() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateRevolute* Clone() const override { return new ChLinkMateRevolute(*this); }

    using ChLinkMateGeneric::Initialize;

    /// Tell if the two axes must be opposed (flipped=true) or must have the same verse (flipped=false)
    void SetFlipped(bool doflip);
    bool IsFlipped() const { return m_flipped; }

    /// Specialized initialization for revolute mate, given the two bodies to be connected, two points, two directions
    /// (each expressed in body or abs. coordinates). These two directions are the Z axes of slave frame F1 and master
    /// frame F2
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
        std::shared_ptr<ChBodyFrame> body2,                      ///< second body to link
        bool pos_are_relative,                                    ///< true: following pos. are relative to bodies
        const ChVector3d& point1,                                          ///< point on slave axis 1 (rel. or abs.)
        const ChVector3d& point2,                                          ///< point on master axis 2 (rel. or abs.)
        const ChVector3d& dir1,                                         ///< direction of slave axis 1 (rel. or abs.)
        const ChVector3d& dir2                                          ///< direction of master axis 2 (rel. or abs.)
    ) override;

    /// Get relative angle of slave frame with respect to master frame.
    double GetRelativeAngle();

    /// Get relative angular velocity of slave frame with respect to master frame.
    double GetRelativeAngle_dt();

    /// Get relative angular acceleration of slave frame with respect to master frame.
    double GetRelativeAngle_dtdt();

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateRevolute, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of prismatic type.
/// Allowed relative movements are along the Z axes of the two frames.

class ChApi ChLinkMatePrismatic : public ChLinkMateGeneric {
  protected:
    bool m_flipped;

  public:
    ChLinkMatePrismatic() : ChLinkMateGeneric(true, true, false, true, true, true), m_flipped(false) {}
    ChLinkMatePrismatic(const ChLinkMatePrismatic& other);
    virtual ~ChLinkMatePrismatic() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMatePrismatic* Clone() const override { return new ChLinkMatePrismatic(*this); }

    using ChLinkMateGeneric::Initialize;

    /// Tell if the two axes must be opposed (flipped=true) or must have the same verse (flipped=false)
    void SetFlipped(bool doflip);
    bool IsFlipped() const { return m_flipped; }

    /// Specialized initialization for prismatic mate, given the two bodies to be connected, two points, two directions
    /// (each expressed in body or abs. coordinates). These two directions are the X axes of slave frame F1 and master
    /// frame F2
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< point on slave axis 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< point on master axis 2 (rel. or abs.)
                            const ChVector3d& dir1,                     ///< direction of slave axis 1 (rel. or abs.)
                            const ChVector3d& dir2                      ///< direction of master axis 2 (rel. or abs.)
                            ) override;

    /// Get relative position of slave frame with respect to master frame.
    double GetRelativePos();

    /// Get relative velocity of slave frame with respect to master frame.
    double GetRelativePosDer();

    /// Get relative acceleration of slave frame with respect to master frame.
    double GetRelativePosDer2();

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMatePrismatic, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of spherical type. This correspond to the
/// typical point-on-point or spherical joint mating used in 3D CAD assemblies.

class ChApi ChLinkMateSpherical : public ChLinkMateGeneric {
  public:
    ChLinkMateSpherical() : ChLinkMateGeneric(true, true, true, false, false, false) {}
    ChLinkMateSpherical(const ChLinkMateSpherical& other);
    virtual ~ChLinkMateSpherical() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateSpherical* Clone() const override { return new ChLinkMateSpherical(*this); }

    using ChLinkMateGeneric::Initialize;

    /// Specialized initialization for coincident mate, given the two bodies to be connected, and two points
    /// (each expressed in body or abs. coordinates).
    void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                    std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                    bool pos_are_relative,                ///< true: following pos. are relative to bodies.
                    ChVector3d point1,                      ///< point slave 1 (rel. or abs.)
                    ChVector3d point2                       ///< point master 2 (rel. or abs.)
    );
};

CH_CLASS_VERSION(ChLinkMateSpherical, 0)

// -----------------------------------------------------------------------------

/// Mate constraining distance of origin of frame 2 respect to Z axis of frame 1.

class ChApi ChLinkMateDistanceZ : public ChLinkMateGeneric {
  protected:
    double m_distance;

  public:
    ChLinkMateDistanceZ() : ChLinkMateGeneric(false, false, true, false, false, false), m_distance(0) {}
    ChLinkMateDistanceZ(const ChLinkMateDistanceZ& other);
    virtual ~ChLinkMateDistanceZ() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateDistanceZ* Clone() const override { return new ChLinkMateDistanceZ(*this); }

    /// Set the distance of the two constrainted frames along the Z axis of frame 2.
    void SetDistance(double distance) { m_distance = distance; }

    /// Get the imposed distance on Z of frame 2.
    double GetDistance() const { return m_distance; }

    using ChLinkMateGeneric::Initialize;

    /// Initialize the link by providing two points and a direction along which the distance must be considered.
    /// \a dir2 will be the Z axis of both frame 1 and 2.
    void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                    std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                    bool pos_are_relative,                ///< true: following pos. are relative to bodies
                    ChVector3d point1,                      ///< point slave 1 (rel. or abs.)
                    ChVector3d point2,                      ///< point master 2 (rel. or abs.)
                    ChVector3d dir2                      ///< direction of master axis 2 (rel. or abs.)
    );

    /// Update link state. This is called automatically by the solver at each time step.
    /// Update constraint jacobian and frames.
    virtual void Update(double mtime, bool update_assets = true) override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateDistanceZ, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of parallel type. This correspond to the
/// typical axis-is-parallel-to-axis (or edge to edge, etc.) mating
/// used in 3D CAD assemblies. The axes to be kept parallel are
/// the two Z axes of the two frames.

class ChApi ChLinkMateParallel : public ChLinkMateGeneric {
  protected:
    bool m_flipped;

  public:
    ChLinkMateParallel() : ChLinkMateGeneric(false, false, false, true, true, false), m_flipped(false) {}
    ChLinkMateParallel(const ChLinkMateParallel& other);
    virtual ~ChLinkMateParallel() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateParallel* Clone() const override { return new ChLinkMateParallel(*this); }

    /// Tell if the two axes must be opposed (flipped=true) or must have the same verse (flipped=false)
    void SetFlipped(bool doflip);
    bool IsFlipped() const { return m_flipped; }

    /// Specialized initialization for parallel mate, given the two bodies to be connected, two points and two
    /// directions (each expressed in body or abs. coordinates).
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< point on slave axis 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< point on master axis 2 (rel. or abs.)
                            const ChVector3d& dir1,                     ///< direction of slave axis 1 (rel. or abs.)
                            const ChVector3d& dir2                      ///< direction of master axis 2 (rel. or abs.)
                            ) override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateParallel, 0)

// -----------------------------------------------------------------------------

/// Mate constraint of orthogonal type. This correspond to the
/// typical axis-is-orthogonal-to-axis (or edge to edge, etc.) mating
/// used in 3D CAD assemblies. Then the two Z axes of the two frames
/// are aligned to the cross product of the two directions.

class ChApi ChLinkMateOrthogonal : public ChLinkMateGeneric {
  protected:
    ChVector3d m_reldir1;
    ChVector3d m_reldir2;

  public:
    ChLinkMateOrthogonal()
        : ChLinkMateGeneric(false, false, false, false, false, true), m_reldir1(VNULL), m_reldir2(VNULL) {}
    ChLinkMateOrthogonal(const ChLinkMateOrthogonal& other);
    virtual ~ChLinkMateOrthogonal() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateOrthogonal* Clone() const override { return new ChLinkMateOrthogonal(*this); }

    /// Specialized initialization for orthogonal mate, given the two bodies to be connected, two points and two
    /// directions (each expressed in body or abs. coordinates).
    virtual void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                            std::shared_ptr<ChBodyFrame> body2,  ///< second body to link
                            bool pos_are_relative,                ///< true: following pos. are relative to bodies
                            const ChVector3d& point1,                      ///< point on slave axis 1 (rel. or abs.)
                            const ChVector3d& point2,                      ///< point on master axis 2 (rel. or abs.)
                            const ChVector3d& dir1,                     ///< direction of slave axis 1 (rel. or abs.)
                            const ChVector3d& dir2                      ///< direction of master axis 2 (rel. or abs.)
                            ) override;

    /// Update link state. This is called automatically by the solver at each time step.
    /// Update constraint jacobian and frames.
    virtual void Update(double mtime, bool update_assets = true) override;

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateOrthogonal, 0)

// -----------------------------------------------------------------------------

/// Mate constraint that completely fix one frame's rotation and translation
/// respect to the other frame.

class ChApi ChLinkMateFix : public ChLinkMateGeneric {
  public:
    ChLinkMateFix() : ChLinkMateGeneric(true, true, true, true, true, true) {}
    ChLinkMateFix(const ChLinkMateFix& other);
    virtual ~ChLinkMateFix() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateFix* Clone() const override { return new ChLinkMateFix(*this); }

    using ChLinkMateGeneric::Initialize;

    /// Specialized initialization for "fix" mate, given the two bodies to be connected, the positions of the two
    /// auxiliary frames where the two bodies are connected are both automatically initialized as the current absolute
    /// position of body1.
    void Initialize(std::shared_ptr<ChBodyFrame> body1,  ///< first body to link
                    std::shared_ptr<ChBodyFrame> body2   ///< second body to link
    );
};

CH_CLASS_VERSION(ChLinkMateFix, 0)

// -----------------------------------------------------------------------------

/// Rack-pinion link between two body frames.
/// It correctly handles the direction of transmitted force
/// given the teeth pressure angle.

class ChApi ChLinkMateRackPinion : public ChLinkMateGeneric {

  protected:
    double R;         ///< primitive radius of the pinion
    double alpha;     ///< inclination of action line
    double beta;      ///< helix angle
    double phase;     ///< mounting phase angle
    bool checkphase;  ///< keep gear always on phase

    double a1;  ///< auxiliary

    ChVector3d contact_pt;

    ChFrame<double> local_pinion;  ///< pinion shaft pos & dir (frame Z axis), relative to body1
    ChFrame<double> local_rack;    ///< rack direction (frame X axis), relative to body2

  public:
    ChLinkMateRackPinion();
    ChLinkMateRackPinion(const ChLinkMateRackPinion& other);
    virtual ~ChLinkMateRackPinion() {}

    /// "Virtual" copy constructor (covariant return type).
    virtual ChLinkMateRackPinion* Clone() const override { return new ChLinkMateRackPinion(*this); }

    // Updates aux frames positions
    virtual void UpdateTime(double mytime) override;

    // data get/set

    /// Get the primitive radius of the pinion.
    double GetPinionRadius() const { return R; }
    /// Set the primitive radius of the pinion.
    void SetPinionRadius(double mR) { R = mR; }

    /// Get the pressure angle (usually 20 deg for typical gears)
    double GetAlpha() const { return alpha; }
    /// Set the pressure angle (usually 20 deg for typical gears)
    void SetAlpha(double mset) { alpha = mset; }

    /// Get the angle of teeth in bevel gears (0 deg for spur gears)
    double GetBeta() const { return beta; }
    /// Set the angle of teeth in bevel gears (0 deg for spur gears)
    void SetBeta(double mset) { beta = mset; }

    /// Get the initial phase of rotation of pinion respect to rack
    double GetPhase() const { return phase; }
    /// Set the initial phase of rotation of pinion respect to rack
    void SetPhase(double mset) { phase = mset; }

    /// If true, enforce check on exact phase between gears
    /// (otherwise after many simulation steps the phasing
    /// may be affected by numerical error accumulation).
    /// By default, it is turned off.
    /// Note that, to ensure the correct phasing during the many
    /// rotations, an algorithm will update an accumulator with total rotation
    /// values, which might be affected by loss of numerical precision
    /// after few thousands of revolutions; keep in mind this if you do
    /// real-time simulations which must run for many hours.
    void SetCheckphase(bool mset) { checkphase = mset; }
    bool GetCheckphase() const { return checkphase; }

    /// Get total rotation of 1st gear, respect to interaxis, in radians
    double Get_a1() const { return a1; }
    /// Reset the total rotations of a1 and a2.
    void Reset_a1() { a1 = 0; }

    /// Set pinion shaft position and direction, in body1-relative reference.
    /// The shaft direction is the Z axis of that frame.
    void SetPinionFrame(ChFrame<double> mf) { local_pinion = mf; }
    /// Get pinion shaft position and direction, in body1-relative reference.
    /// The shaft direction is the Z axis of that frame.
    ChFrame<double> GetPinionFrame() const { return local_pinion; }

    /// Set rack position and direction, in body2-relative reference.
    /// The rack direction is the X axis of that frame.
    void SetRackFrame(ChFrame<double> mf) { local_rack = mf; }
    /// Get rack position and direction, in body2-relative reference.
    /// The rack direction is the X axis of that frame.
    ChFrame<double> GetRackFrame() const { return local_rack; }

    /// Get pinion shaft direction in absolute reference
    ChVector3d GetAbsPinionDir();
    /// Get pinion position in absolute reference
    ChVector3d GetAbsPinionPos();

    /// Get rack direction in absolute reference
    ChVector3d GetAbsRackDir();
    /// Get rack position in absolute reference
    ChVector3d GetAbsRackPos();

    /// Method to allow serialization of transient data to archives.
    virtual void ArchiveOut(ChArchiveOut& archive_out) override;

    /// Method to allow deserialization of transient data from archives.
    virtual void ArchiveIn(ChArchiveIn& archive_in) override;
};

CH_CLASS_VERSION(ChLinkMateRackPinion,0)

}  // end namespace chrono

#endif
