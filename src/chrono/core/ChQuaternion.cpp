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

#include <cmath>

#include "chrono/core/ChQuaternion.h"
#include "chrono/core/ChMatrix33.h"
#include "chrono/utils/ChUtils.h"

namespace chrono {

const ChQuaterniond QNULL(0., 0., 0., 0.);
const ChQuaterniond QUNIT(1., 0., 0., 0.);

const ChQuaterniond Q_ROTATE_Y_TO_X(CH_C_SQRT_1_2,				0,				0, -CH_C_SQRT_1_2 );
const ChQuaterniond Q_ROTATE_Y_TO_Z(CH_C_SQRT_1_2, CH_C_SQRT_1_2,				0,				0 );
const ChQuaterniond Q_ROTATE_X_TO_Y(CH_C_SQRT_1_2,				0,				0,	CH_C_SQRT_1_2 );
const ChQuaterniond Q_ROTATE_X_TO_Z(CH_C_SQRT_1_2,				0, -CH_C_SQRT_1_2,				0 );
const ChQuaterniond Q_ROTATE_Z_TO_Y(CH_C_SQRT_1_2,-CH_C_SQRT_1_2,				0,				0 );
const ChQuaterniond Q_ROTATE_Z_TO_X(CH_C_SQRT_1_2,				0,  CH_C_SQRT_1_2,				0 );

const ChQuaterniond Q_FLIP_AROUND_X(0., 1., 0., 0.);
const ChQuaterniond Q_FLIP_AROUND_Y(0., 0., 1., 0.);
const ChQuaterniond Q_FLIP_AROUND_Z(0., 0., 0., 1.);

static const double FD_STEP = 1e-4;

// -----------------------------------------------------------------------------
// QUATERNION OPERATIONS

double Qlength(const ChQuaterniond& q) {
    return (sqrt(pow(q.e0(), 2) + pow(q.e1(), 2) + pow(q.e2(), 2) + pow(q.e3(), 2)));
}

ChQuaterniond Qscale(const ChQuaterniond& q, double fact) {
    ChQuaterniond result;
    result.e0() = q.e0() * fact;
    result.e1() = q.e1() * fact;
    result.e2() = q.e2() * fact;
    result.e3() = q.e3() * fact;
    return result;
}

ChQuaterniond Qadd(const ChQuaterniond& qa, const ChQuaterniond& qb) {
    ChQuaterniond result;
    result.e0() = qa.e0() + qb.e0();
    result.e1() = qa.e1() + qb.e1();
    result.e2() = qa.e2() + qb.e2();
    result.e3() = qa.e3() + qb.e3();
    return result;
}

ChQuaterniond Qsub(const ChQuaterniond& qa, const ChQuaterniond& qb) {
    ChQuaterniond result;
    result.e0() = qa.e0() - qb.e0();
    result.e1() = qa.e1() - qb.e1();
    result.e2() = qa.e2() - qb.e2();
    result.e3() = qa.e3() - qb.e3();
    return result;
}

// Return the norm two of the quaternion. Euler's parameters have norm = 1
ChQuaterniond Qnorm(const ChQuaterniond& q) {
    double invlength;
    invlength = 1 / (Qlength(q));
    return Qscale(q, invlength);
}

// Return the conjugate of the quaternion [s,v1,v2,v3] is [s,-v1,-v2,-v3]
ChQuaterniond Qconjugate(const ChQuaterniond& q) {
    ChQuaterniond res;
    res.e0() = q.e0();
    res.e1() = -q.e1();
    res.e2() = -q.e2();
    res.e3() = -q.e3();
    return (res);
}

// Return the product of two quaternions. It is non-commutative (like cross product in vectors).
ChQuaterniond Qcross(const ChQuaterniond& qa, const ChQuaterniond& qb) {
    ChQuaterniond res;
    res.e0() = qa.e0() * qb.e0() - qa.e1() * qb.e1() - qa.e2() * qb.e2() - qa.e3() * qb.e3();
    res.e1() = qa.e0() * qb.e1() + qa.e1() * qb.e0() - qa.e3() * qb.e2() + qa.e2() * qb.e3();
    res.e2() = qa.e0() * qb.e2() + qa.e2() * qb.e0() + qa.e3() * qb.e1() - qa.e1() * qb.e3();
    res.e3() = qa.e0() * qb.e3() + qa.e3() * qb.e0() - qa.e2() * qb.e1() + qa.e1() * qb.e2();
    return (res);
}

// Get the quaternion from an angle of rotation and an axis, defined in _abs_ coords.
// The axis is supposed to be fixed, i.e. it is constant during rotation.
// The 'axis' vector must be normalized.
ChQuaterniond Q_from_AngAxis(double angle, const ChVector3d& axis) {
    ChQuaterniond quat;
    double halfang;
    double sinhalf;

    halfang = (angle * 0.5);
    sinhalf = sin(halfang);

    quat.e0() = cos(halfang);
    quat.e1() = axis.x() * sinhalf;
    quat.e2() = axis.y() * sinhalf;
    quat.e3() = axis.z() * sinhalf;
    return (quat);
}

// Get the quaternion from a source vector and a destination vector which specifies
// the rotation from one to the other.  The vectors do not need to be normalized.
ChQuaterniond Q_from_Vect_to_Vect(const ChVector3d& fr_vect, const ChVector3d& to_vect) {
    const double ANGLE_TOLERANCE = 1e-6;
    ChQuaterniond quat;
    double halfang;
    double sinhalf;
    ChVector3d axis;

    double lenXlen = fr_vect.Length() * to_vect.Length();
    axis = fr_vect % to_vect;
    double sinangle = ChClamp(axis.Length() / lenXlen, -1.0, +1.0);
    double cosangle = ChClamp(fr_vect ^ to_vect / lenXlen, -1.0, +1.0);

    // Consider three cases: Parallel, Opposite, non-collinear
    if (std::abs(sinangle) == 0.0 && cosangle > 0) {
        // fr_vect & to_vect are parallel
        quat.e0() = 1.0;
        quat.e1() = 0.0;
        quat.e2() = 0.0;
        quat.e3() = 0.0;
    } else if (std::abs(sinangle) < ANGLE_TOLERANCE && cosangle < 0) {
        // fr_vect & to_vect are opposite, i.e. ~180 deg apart
        axis = fr_vect.GetOrthogonalVector() + (-to_vect).GetOrthogonalVector();
        axis.Normalize();
        quat.e0() = 0.0;
        quat.e1() = ChClamp(axis.x(), -1.0, +1.0);
        quat.e2() = ChClamp(axis.y(), -1.0, +1.0);
        quat.e3() = ChClamp(axis.z(), -1.0, +1.0);
    } else {
        // fr_vect & to_vect are not co-linear case
        axis.Normalize();
        halfang = 0.5 * std::atan2(sinangle, cosangle);
        sinhalf = sin(halfang);

        quat.e0() = cos(halfang);
        quat.e1() = sinhalf* axis.x();
        quat.e2() = sinhalf* axis.y();
        quat.e3() = sinhalf* axis.z();
    }
    return (quat);
}

ChQuaterniond Q_from_AngZ(double angleZ) {
    return Q_from_AngAxis(angleZ, VECT_Z);
}
ChQuaterniond Q_from_AngX(double angleX) {
    return Q_from_AngAxis(angleX, VECT_X);
}
ChQuaterniond Q_from_AngY(double angleY) {
    return Q_from_AngAxis(angleY, VECT_Y);
}

ChQuaterniond Q_from_NasaAngles(const ChVector3d& mang) {
    ChQuaterniond mq;
    double c1 = cos(mang.z() / 2);
    double s1 = sin(mang.z() / 2);
    double c2 = cos(mang.x() / 2);
    double s2 = sin(mang.x() / 2);
    double c3 = cos(mang.y() / 2);
    double s3 = sin(mang.y() / 2);
    double c1c2 = c1 * c2;
    double s1s2 = s1 * s2;
    mq.e0() = c1c2 * c3 + s1s2 * s3;
    mq.e1() = c1c2 * s3 - s1s2 * c3;
    mq.e2() = c1 * s2 * c3 + s1 * c2 * s3;
    mq.e3() = s1 * c2 * c3 - c1 * s2 * s3;
    return mq;
}

ChVector3d Q_to_NasaAngles(const ChQuaterniond& q1) {
    ChVector3d mnasa;
    double sqw = q1.e0() * q1.e0();
    double sqx = q1.e1() * q1.e1();
    double sqy = q1.e2() * q1.e2();
    double sqz = q1.e3() * q1.e3();
    // heading
    mnasa.z() = atan2(2.0 * (q1.e1() * q1.e2() + q1.e3() * q1.e0()), (sqx - sqy - sqz + sqw));
    // bank
    mnasa.y() = atan2(2.0 * (q1.e2() * q1.e3() + q1.e1() * q1.e0()), (-sqx - sqy + sqz + sqw));
    // attitude
    mnasa.x() = asin(-2.0 * (q1.e1() * q1.e3() - q1.e2() * q1.e0()));
    return mnasa;
}

ChQuaterniond Q_from_Euler123(const ChVector3d& ang) {
	ChQuaterniond q;
    double t0 = cos(ang.z() * 0.5);
	double t1 = sin(ang.z() * 0.5);
	double t2 = cos(ang.x() * 0.5);
	double t3 = sin(ang.x() * 0.5);
	double t4 = cos(ang.y() * 0.5);
	double t5 = sin(ang.y() * 0.5);

	q.e0() = t0 * t2 * t4 + t1 * t3 * t5;
	q.e1() = t0 * t3 * t4 - t1 * t2 * t5;
	q.e2() = t0 * t2 * t5 + t1 * t3 * t4;
	q.e3() = t1 * t2 * t4 - t0 * t3 * t5;
	
	return q;
}

ChVector3d Q_to_Euler123(const ChQuaterniond& mq) {
	ChVector3d euler;
    double sq0 = mq.e0() * mq.e0();
    double sq1 = mq.e1() * mq.e1();
    double sq2 = mq.e2() * mq.e2();
    double sq3 = mq.e3() * mq.e3();
    // roll
    euler.x() = atan2(2 * (mq.e2() * mq.e3() + mq.e0() * mq.e1()), sq3 - sq2 - sq1 + sq0);
    // pitch
    euler.y() = -asin(2 * (mq.e1() * mq.e3() - mq.e0() * mq.e2()));
    // yaw
    euler.z() = atan2(2 * (mq.e1() * mq.e2() + mq.e3() * mq.e0()), sq1 + sq0 - sq3 - sq2);
	
	return euler;
}

void Q_to_AngAxis(const ChQuaterniond& quat, double& angle, ChVector3d& axis) {
    if (std::abs(quat.e0()) < 0.99999999) {
        double arg = acos(quat.e0());
        double invsine = 1 / sin(arg);
        ChVector3d vtemp;
        vtemp.x() = invsine * quat.e1();
        vtemp.y() = invsine * quat.e2();
        vtemp.z() = invsine * quat.e3();
        angle = 2 * arg;
        axis = Vnorm(vtemp);
    } else {
        axis.x() = 1;
        axis.y() = 0;
        axis.z() = 0;
        angle = 0;
    }
}

// Get the quaternion time derivative from the vector of angular speed, with w specified in _absolute_ coords.
ChQuaterniond Qdt_from_Wabs(const ChVector3d& w, const ChQuaterniond& q) {
    ChQuaterniond qw;
    double half = 0.5;

    qw.e0() = 0;
    qw.e1() = w.x();
    qw.e2() = w.y();
    qw.e3() = w.z();

    return Qscale(Qcross(qw, q), half);  // {q_dt} = 1/2 {0,w}*{q}
}

// Get the quaternion time derivative from the vector of angular speed, with w specified in _local_ coords.
ChQuaterniond Qdt_from_Wrel(const ChVector3d& w, const ChQuaterniond& q) {
    ChQuaterniond qw;
    double half = 0.5;

    qw.e0() = 0;
    qw.e1() = w.x();
    qw.e2() = w.y();
    qw.e3() = w.z();

    return Qscale(Qcross(q, qw), half);  // {q_dt} = 1/2 {q}*{0,w_rel}
}

// Get the quaternion first derivative from the vector of angular acceleration with a specified in _absolute_ coords.
ChQuaterniond Qdtdt_from_Aabs(const ChVector3d& a,
                                     const ChQuaterniond& q,
                                     const ChQuaterniond& q_dt) {
    ChQuaterniond ret;
    ret.Qdtdt_from_Aabs(a, q, q_dt);
    return ret;
}

//	Get the quaternion second derivative from the vector of angular acceleration with a specified in _relative_ coords.
ChQuaterniond Qdtdt_from_Arel(const ChVector3d& a,
                                     const ChQuaterniond& q,
                                     const ChQuaterniond& q_dt) {
    ChQuaterniond ret;
    ret.Qdtdt_from_Arel(a, q, q_dt);
    return ret;
}

// Get the time derivative from a quaternion, a speed of rotation and an axis, defined in _abs_ coords.
ChQuaterniond Qdt_from_AngAxis(const ChQuaterniond& quat, double angle_dt, const ChVector3d& axis) {
    ChVector3d W;

    W = Vmul(axis, angle_dt);

    return Qdt_from_Wabs(W, quat);
}

// Get the second time derivative from a quaternion, an angular acceleration and an axis, defined in _abs_ coords.
ChQuaterniond Qdtdt_from_AngAxis(double angle_dtdt,
                                        const ChVector3d& axis,
                                        const ChQuaterniond& q,
                                        const ChQuaterniond& q_dt) {
    ChVector3d Acc;

    Acc = Vmul(axis, angle_dtdt);

    return Qdtdt_from_Aabs(Acc, q, q_dt);
}

// Check if two quaternions are equal
bool Qequal(const ChQuaterniond& qa, const ChQuaterniond& qb) {
    return qa == qb;
}

// Check if quaternion is not null
bool Qnotnull(const ChQuaterniond& qa) {
    return (qa.e0() != 0) || (qa.e1() != 0) || (qa.e2() != 0) || (qa.e3() != 0);
}

// Given the imaginary (vectorial) {e1 e2 e3} part of a quaternion,
// find the entire quaternion q = {e0, e1, e2, e3}.
// Note: singularities are possible.
ChQuaterniond ImmQ_complete(const ChVector3d& qimm) {
    ChQuaterniond mq;
    mq.e1() = qimm.x();
    mq.e2() = qimm.y();
    mq.e3() = qimm.z();
    mq.e0() = sqrt(1 - mq.e1() * mq.e1() - mq.e2() * mq.e2() - mq.e3() * mq.e3());
    return mq;
}

// Given the imaginary (vectorial) {e1 e2 e3} part of a quaternion time derivative,
// find the entire quaternion q = {e0, e1, e2, e3}.
// Note: singularities are possible.
ChQuaterniond ImmQ_dt_complete(const ChQuaterniond& mq, const ChVector3d& qimm_dt) {
    ChQuaterniond mqdt;
    mqdt.e1() = qimm_dt.x();
    mqdt.e2() = qimm_dt.y();
    mqdt.e3() = qimm_dt.z();
    mqdt.e0() = (-mq.e1() * mqdt.e1() - mq.e2() * mqdt.e2() - mq.e3() * mqdt.e3()) / mq.e0();
    return mqdt;
}

// Given the imaginary (vectorial) {e1 e2 e3} part of a quaternion second time derivative,
// find the entire quaternion q = {e0, e1, e2, e3}.
// Note: singularities are possible.
ChQuaterniond ImmQ_dtdt_complete(const ChQuaterniond& mq,
                                        const ChQuaterniond& mqdt,
                                        const ChVector3d& qimm_dtdt) {
    ChQuaterniond mqdtdt;
    mqdtdt.e1() = qimm_dtdt.x();
    mqdtdt.e2() = qimm_dtdt.y();
    mqdtdt.e3() = qimm_dtdt.z();
    mqdtdt.e0() = (-mq.e1() * mqdtdt.e1() - mq.e2() * mqdtdt.e2() - mq.e3() * mqdtdt.e3() - mqdt.e0() * mqdt.e0() -
                   mqdt.e1() * mqdt.e1() - mqdt.e2() * mqdt.e2() - mqdt.e3() * mqdt.e3()) /
                  mq.e0();
    return mqdtdt;
}

// -----------------------------------------------------------------------------

// Get the X axis of a coordsystem, given the quaternion which
// represents the alignment of the coordsystem.
ChVector3d VaxisXfromQuat(const ChQuaterniond& quat) {
    ChVector3d res;
    res.x() = (pow(quat.e0(), 2) + pow(quat.e1(), 2)) * 2 - 1;
    res.y() = ((quat.e1() * quat.e2()) + (quat.e0() * quat.e3())) * 2;
    res.z() = ((quat.e1() * quat.e3()) - (quat.e0() * quat.e2())) * 2;
    return res;
}

}  // end namespace chrono
