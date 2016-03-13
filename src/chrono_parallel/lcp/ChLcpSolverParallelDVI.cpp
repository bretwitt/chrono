#include "chrono_parallel/lcp/ChLcpSolverParallel.h"

using namespace chrono;
#define xstr(s) str(s)
#define str(s) #s

#define CLEAR_RESERVE_RESIZE(M, nnz, rows, cols)                                             \
    {                                                                                        \
        uint current = M.capacity();                                                         \
        clear(M);                                                                            \
        if (current < nnz) {                                                                 \
            M.reserve(nnz * 1.5);                                                            \
            LOG(INFO) << "Increase Capacity of: " << str(M) << " " << current << " " << nnz; \
        }                                                                                    \
        M.resize(rows, cols, false);                                                         \
    }

void ChLcpSolverParallelDVI::RunTimeStep() {
    // Compute the offsets and number of constrains depending on the solver mode
    if (data_manager->settings.solver.solver_mode == NORMAL) {
        data_manager->rigid_rigid->offset = 1;
        data_manager->num_unilaterals = 1 * data_manager->num_rigid_contacts;
    } else if (data_manager->settings.solver.solver_mode == SLIDING) {
        data_manager->rigid_rigid->offset = 3;
        data_manager->num_unilaterals = 3 * data_manager->num_rigid_contacts;
    } else if (data_manager->settings.solver.solver_mode == SPINNING) {
        data_manager->rigid_rigid->offset = 6;
        data_manager->num_unilaterals = 6 * data_manager->num_rigid_contacts;
    }

    uint num_3dof_3dof = data_manager->node_container->GetNumConstraints();
    uint num_tet_constraints = data_manager->fea_container->GetNumConstraints();

    // Get the number of 3dof constraints, from the 3dof container in use right now

    // This is the total number of constraints
    data_manager->num_constraints =
        data_manager->num_unilaterals + data_manager->num_bilaterals + num_3dof_3dof + num_tet_constraints;
    LOG(INFO) << "ChLcpSolverParallelDVI::RunTimeStep S num_constraints: " << data_manager->num_constraints;
    // Generate the mass matrix and compute M_inv_k
    ComputeInvMassMatrix();
    // ComputeMassMatrix();

    data_manager->host_data.gamma.resize(data_manager->num_constraints);
    data_manager->host_data.gamma.reset();

    // Perform any setup tasks for all constraint types
    data_manager->rigid_rigid->Setup(data_manager);
    data_manager->bilateral->Setup(data_manager);
    data_manager->node_container->Setup(data_manager->num_unilaterals + data_manager->num_bilaterals);
    data_manager->fea_container->Setup(data_manager->num_unilaterals + data_manager->num_bilaterals + num_3dof_3dof);

    // Clear and reset solver history data and counters
    solver->current_iteration = 0;
    bilateral_solver->current_iteration = 0;
    data_manager->measures.solver.total_iteration = 0;
    data_manager->measures.solver.maxd_hist.clear();
    data_manager->measures.solver.maxdeltalambda_hist.clear();
    // Set pointers to constraint objects and perform setup actions for solver

    solver->Setup(data_manager);
    bilateral_solver->Setup(data_manager);

    ComputeD();
    ComputeE();
    ComputeR();
    ComputeN();

    data_manager->system_timer.start("ChLcpSolverParallel_Solve");

    data_manager->node_container->PreSolve();
    data_manager->fea_container->PreSolve();

    ShurProductFull.Setup(data_manager);
    ShurProductBilateral.Setup(data_manager);
    ProjectFull.Setup(data_manager);

    PerformStabilization();

    if (data_manager->settings.solver.solver_mode == NORMAL || data_manager->settings.solver.solver_mode == SLIDING ||
        data_manager->settings.solver.solver_mode == SPINNING) {
        if (data_manager->settings.solver.max_iteration_normal > 0) {
            data_manager->settings.solver.local_solver_mode = NORMAL;
            SetR();
            LOG(INFO) << "ChLcpSolverParallelDVI::RunTimeStep - Solve Normal";
            data_manager->measures.solver.total_iteration +=
                solver->Solve(ShurProductFull,                                     //
                              ProjectFull,                                         //
                              data_manager->settings.solver.max_iteration_normal,  //
                              data_manager->num_constraints,                       //
                              data_manager->host_data.R,                           //
                              data_manager->host_data.gamma);                      //
        }
    }
    if (data_manager->settings.solver.solver_mode == SLIDING || data_manager->settings.solver.solver_mode == SPINNING) {
        if (data_manager->settings.solver.max_iteration_sliding > 0) {
            data_manager->settings.solver.local_solver_mode = SLIDING;
            SetR();
            LOG(INFO) << "ChLcpSolverParallelDVI::RunTimeStep - Solve Sliding";
            data_manager->measures.solver.total_iteration +=
                solver->Solve(ShurProductFull,                                      //
                              ProjectFull,                                          //
                              data_manager->settings.solver.max_iteration_sliding,  //
                              data_manager->num_constraints,                        //
                              data_manager->host_data.R,                            //
                              data_manager->host_data.gamma);                       //
        }
    }
    if (data_manager->settings.solver.solver_mode == SPINNING) {
        if (data_manager->settings.solver.max_iteration_spinning > 0) {
            data_manager->settings.solver.local_solver_mode = SPINNING;
            SetR();
            LOG(INFO) << "ChLcpSolverParallelDVI::RunTimeStep - Solve Spinning";
            data_manager->measures.solver.total_iteration +=
                solver->Solve(ShurProductFull,                                       //
                              ProjectFull,                                           //
                              data_manager->settings.solver.max_iteration_spinning,  //
                              data_manager->num_constraints,                         //
                              data_manager->host_data.R,                             //
                              data_manager->host_data.gamma);                        //
        }
    }

    //    DynamicVector<real> temp(data_manager->num_rigid_bodies * 6, 0.0);
    //    DynamicVector<real> output(data_manager->num_rigid_contacts * 3, 0.0);
    //
    //    // DynamicVector<real> temp(data_manager->num_fluid_bodies * 3, 0.0);
    //    // DynamicVector<real> output(data_manager->num_fluid_bodies, 0.0);
    //
    //    /////
    //    double t1 = 0;
    //    {
    //        ChTimer<> timer;
    //        timer.start();
    //        rigid_rigid.Dx(data_manager->host_data.gamma, temp);
    //        rigid_rigid.D_Tx(temp, output);
    //        // data_manager->node_container->Dx(data_manager->host_data.gamma, temp);
    //        // data_manager->node_container->D_Tx(temp, output);
    //        timer.stop();
    //        t1 = timer();
    //    }
    //    ChTimer<> timer;
    //    timer.start();
    //    DynamicVector<real> compare =
    //        data_manager->host_data.D_T * data_manager->host_data.D * data_manager->host_data.gamma;
    //    timer.stop();
    //    std::cout << "time1: " << t1 << " time2: " << timer() << std::endl;
    //    /////

    data_manager->Fc_current = false;
    data_manager->node_container->PostSolve();
    data_manager->fea_container->PostSolve();

    data_manager->system_timer.stop("ChLcpSolverParallel_Solve");

    ComputeImpulses();
    for (int i = 0; i < data_manager->measures.solver.maxd_hist.size(); i++) {
        AtIterationEnd(data_manager->measures.solver.maxd_hist[i], data_manager->measures.solver.maxdeltalambda_hist[i],
                       i);
    }
    tot_iterations = data_manager->measures.solver.maxd_hist.size();

    LOG(TRACE) << "ChLcpSolverParallelDVI::RunTimeStep E solve: "
               << data_manager->system_timer.GetTime("ChLcpSolverParallel_Solve")
               << " shur: " << data_manager->system_timer.GetTime("ShurProduct")
               //<< " residual: " << data_manager->measures.solver.residual
               //<< " objective: " << data_manager->measures.solver.maxdeltalambda_hist.back()
               << " iterations: " << tot_iterations;
}

void ChLcpSolverParallelDVI::ComputeD() {
    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeD()";
    data_manager->system_timer.start("ChLcpSolverParallel_D");
    uint num_constraints = data_manager->num_constraints;
    if (num_constraints <= 0) {
        return;
    }

    uint num_shafts = data_manager->num_shafts;
    uint num_fluid_bodies = data_manager->num_fluid_bodies;
    uint num_dof = data_manager->num_dof;
    uint num_rigid_contacts = data_manager->num_rigid_contacts;
    uint num_rigid_fluid_contacts = data_manager->num_rigid_fluid_contacts;
    uint num_fluid_contacts = data_manager->num_fluid_contacts;
    uint num_bilaterals = data_manager->num_bilaterals;
    uint nnz_bilaterals = data_manager->nnz_bilaterals;

    int nnz_normal = 6 * 2 * num_rigid_contacts;
    int nnz_tangential = 6 * 4 * num_rigid_contacts;
    int nnz_spinning = 6 * 3 * num_rigid_contacts;

    int num_normal = 1 * num_rigid_contacts;
    int num_tangential = 2 * num_rigid_contacts;
    int num_spinning = 3 * num_rigid_contacts;

    uint num_fluid_fluid = data_manager->node_container->GetNumConstraints();
    uint nnz_fluid_fluid = data_manager->node_container->GetNumNonZeros();

    uint num_fem = data_manager->fea_container->GetNumConstraints();
    uint nnz_fem = data_manager->fea_container->GetNumNonZeros();

    CompressedMatrix<real>& D_T = data_manager->host_data.D_T;
    CompressedMatrix<real>& D = data_manager->host_data.D;
    CompressedMatrix<real>& M_invD = data_manager->host_data.M_invD;
    const CompressedMatrix<real>& M_inv = data_manager->host_data.M_inv;

    int nnz_total = nnz_bilaterals + nnz_fluid_fluid + nnz_fem;
    int num_rows = num_bilaterals + num_fluid_fluid + num_fem;

    switch (data_manager->settings.solver.solver_mode) {
        case NORMAL:
            nnz_total += nnz_normal;
            num_rows += num_normal;
            break;
        case SLIDING:
            nnz_total += nnz_normal + nnz_tangential;
            num_rows += num_normal + num_tangential;

            break;
        case SPINNING:
            nnz_total += nnz_normal + nnz_tangential + nnz_spinning;
            num_rows += num_normal + num_tangential + num_spinning;
            break;
    }

    CLEAR_RESERVE_RESIZE(D_T, nnz_total, num_rows, num_dof)
    // D is automatically reserved during transpose!
    // CLEAR_RESERVE_RESIZE(D, nnz_total, num_dof, num_rows)
    CLEAR_RESERVE_RESIZE(M_invD, nnz_total, num_dof, num_rows)

    data_manager->rigid_rigid->GenerateSparsity();
    data_manager->bilateral->GenerateSparsity();
    data_manager->node_container->GenerateSparsity();
    data_manager->fea_container->GenerateSparsity();

    data_manager->rigid_rigid->Build_D();
    data_manager->bilateral->Build_D();
    data_manager->node_container->Build_D();
    data_manager->fea_container->Build_D();

    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeD - D = trans(D_T)";
    // using the .transpose(); function will do in place transpose and copy
    data_manager->host_data.D = trans(D_T);
    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeD - M_inv * D";

    data_manager->host_data.M_invD = M_inv * data_manager->host_data.D;

    data_manager->system_timer.stop("ChLcpSolverParallel_D");
}

void ChLcpSolverParallelDVI::ComputeE() {
    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeE()";
    data_manager->system_timer.start("ChLcpSolverParallel_E");
    if (data_manager->num_constraints <= 0) {
        return;
    }

    data_manager->host_data.E.resize(data_manager->num_constraints);
    reset(data_manager->host_data.E);

    data_manager->rigid_rigid->Build_E();
    data_manager->bilateral->Build_E();

    data_manager->fea_container->Build_E();
    data_manager->node_container->Build_E();

    data_manager->system_timer.stop("ChLcpSolverParallel_E");
}

void ChLcpSolverParallelDVI::ComputeR() {
    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeR()";
    data_manager->system_timer.start("ChLcpSolverParallel_R");
    if (data_manager->num_constraints <= 0) {
        return;
    }

    const DynamicVector<real>& M_invk = data_manager->host_data.M_invk;
    const CompressedMatrix<real>& D_T = data_manager->host_data.D_T;

    DynamicVector<real>& R = data_manager->host_data.R_full;
    DynamicVector<real>& b = data_manager->host_data.b;

    b.resize(data_manager->num_constraints);
    reset(b);

    R.resize(data_manager->num_constraints);
    reset(R);

    data_manager->rigid_rigid->Build_b();
    data_manager->bilateral->Build_b();
    data_manager->node_container->Build_b();
    data_manager->fea_container->Build_b();

    R = -b - D_T * M_invk;

    data_manager->system_timer.stop("ChLcpSolverParallel_R");
}

void ChLcpSolverParallelDVI::ComputeN() {
    if (data_manager->settings.solver.compute_N == false) {
        return;
    }

    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeN";
    data_manager->system_timer.start("ChLcpSolverParallel_N");
    const CompressedMatrix<real>& D_T = data_manager->host_data.D_T;
    CompressedMatrix<real>& Nshur = data_manager->host_data.Nshur;
    const CompressedMatrix<real>& M_inv = data_manager->host_data.M_inv;
    Nshur = D_T * data_manager->host_data.M_invD;
    data_manager->system_timer.stop("ChLcpSolverParallel_N");
}

void ChLcpSolverParallelDVI::SetR() {
    LOG(INFO) << "ChLcpSolverParallelDVI::SetR()";
    if (data_manager->num_constraints <= 0) {
        return;
    }

    DynamicVector<real>& R = data_manager->host_data.R;
    const DynamicVector<real>& R_full = data_manager->host_data.R_full;

    uint num_rigid_contacts = data_manager->num_rigid_contacts;
    uint num_unilaterals = data_manager->num_unilaterals;
    uint num_bilaterals = data_manager->num_bilaterals;
    uint num_rigid_fluid = data_manager->num_rigid_fluid_contacts * 3;
    uint num_fluid_fluid = data_manager->num_fluid_contacts;
    uint num_fluid_bodies = data_manager->num_fluid_bodies;
    R.resize(data_manager->num_constraints);
    reset(R);

    if (data_manager->settings.solver.local_solver_mode == data_manager->settings.solver.solver_mode) {
        R = R_full;
    } else {
        subvector(R, num_unilaterals, num_bilaterals) = subvector(R_full, num_unilaterals, num_bilaterals);
        subvector(R, num_unilaterals + num_bilaterals, num_rigid_fluid) =
            subvector(R_full, num_unilaterals + num_bilaterals, num_rigid_fluid);

        // TODO: Set R in the associated 3dof container
        subvector(R, num_unilaterals + num_bilaterals + num_rigid_fluid, num_fluid_bodies) =
            subvector(R_full, num_unilaterals + num_bilaterals + num_rigid_fluid, num_fluid_bodies);

        switch (data_manager->settings.solver.local_solver_mode) {
            case BILATERAL: {
            } break;

            case NORMAL: {
                subvector(R, 0, num_rigid_contacts) = subvector(R_full, 0, num_rigid_contacts);
            } break;

            case SLIDING: {
                subvector(R, 0, num_rigid_contacts) = subvector(R_full, 0, num_rigid_contacts);
                subvector(R, num_rigid_contacts, num_rigid_contacts * 2) =
                    subvector(R_full, num_rigid_contacts, num_rigid_contacts * 2);
            } break;

            case SPINNING: {
                subvector(R, 0, num_rigid_contacts) = subvector(R_full, 0, num_rigid_contacts);
                subvector(R, num_rigid_contacts, num_rigid_contacts * 2) =
                    subvector(R_full, num_rigid_contacts, num_rigid_contacts * 2);
                subvector(R, num_rigid_contacts * 3, num_rigid_contacts * 3) =
                    subvector(R_full, num_rigid_contacts * 3, num_rigid_contacts * 3);
            } break;
        }
    }
}

void ChLcpSolverParallelDVI::ComputeImpulses() {
    LOG(INFO) << "ChLcpSolverParallelDVI::ComputeImpulses()";
    const DynamicVector<real>& M_invk = data_manager->host_data.M_invk;
    const CompressedMatrix<real>& M_inv = data_manager->host_data.M_inv;
    const DynamicVector<real>& gamma = data_manager->host_data.gamma;

    const DynamicVector<real>& hf = data_manager->host_data.hf;
    DynamicVector<real>& v = data_manager->host_data.v;

    if (data_manager->num_constraints > 0) {
        // Compute new velocity based on the lagrange multipliers
        v = v + M_inv * hf + data_manager->host_data.M_invD * gamma;
    } else {
        // When there are no constraints we need to still apply gravity and other
        // body forces!
        v = v + M_inv * hf;
    }
}

void ChLcpSolverParallelDVI::PreSolve() {
    // Currently not supported, might be added back in the future
}

void ChLcpSolverParallelDVI::ChangeSolverType(SOLVERTYPE type) {
    data_manager->settings.solver.solver_type = type;

    if (this->solver) {
        delete (this->solver);
    }
    switch (type) {
        case APGD:
            solver = new ChSolverAPGD();
            break;
        case APGDREF:
            solver = new ChSolverAPGDREF();
            break;
        case BB:
            solver = new ChSolverBB();
            break;
        case SPGQP:
            solver = new ChSolverSPGQP();
            break;
    }
}
