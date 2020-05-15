/*
 * Copyright 2019 Gianluca Frison, Dimitris Kouzoupis, Robin Verschueren,
 * Andrea Zanelli, Niels van Duijkeren, Jonathan Frey, Tommaso Sartor,
 * Branimir Novoselnik, Rien Quirynen, Rezart Qelibari, Dang Doan,
 * Jonas Koenemann, Yutao Chen, Tobias Schöls, Jonas Schlagenhauf, Moritz Diehl
 *
 * This file is part of acados.
 *
 * The 2-Clause BSD License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.;
 */

// standard
#include <stdio.h>
#include <stdlib.h>

// acados
#include "acados_c/external_function_interface.h"
#include "acados_c/sim_interface.h"
#include "acados_c/external_function_interface.h"

#include "acados/sim/sim_common.h"
#include "acados/utils/external_function_generic.h"
#include "acados/utils/print.h"


// example specific
#include "{{ model.name }}_model/{{ model.name }}_model.h"
#include "acados_sim_solver_{{ model.name }}.h"


// ** global data **
sim_config  * {{ model.name }}_sim_config;
sim_in      * {{ model.name }}_sim_in;
sim_out     * {{ model.name }}_sim_out;
void        * {{ model.name }}_sim_dims;
sim_opts    * {{ model.name }}_sim_opts;
sim_solver  * {{ model.name }}_sim_solver;

{% if solver_options.integrator_type == "ERK" %}
external_function_param_casadi * sim_forw_vde_casadi;
external_function_param_casadi * sim_expl_ode_fun_casadi;
{% elif solver_options.integrator_type == "IRK" %}
external_function_param_casadi * sim_impl_dae_fun;
external_function_param_casadi * sim_impl_dae_fun_jac_x_xdot_z;
external_function_param_casadi * sim_impl_dae_jac_x_xdot_u_z;
{% elif solver_options.integrator_type == "GNSF" -%}
external_function_param_casadi * sim_gnsf_phi_fun;
external_function_param_casadi * sim_gnsf_phi_fun_jac_y;
external_function_param_casadi * sim_gnsf_phi_jac_y_uhat;
external_function_param_casadi * sim_gnsf_f_lo_jac_x1_x1dot_u_z;
external_function_param_casadi * sim_gnsf_get_matrices_fun;
{%- endif %}


int {{ model.name }}_acados_sim_create()
{
    // initialize
    int nx = {{ dims.nx }};
    int nu = {{ dims.nu }};
    int nz = {{ dims.nz }};

    {#// double Tsim = {{ solver_options.tf / dims.N }};#}
    double Tsim = {{ solver_options.Tsim }};

    {% if solver_options.integrator_type == "IRK" %}
    sim_impl_dae_fun = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_impl_dae_fun_jac_x_xdot_z = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_impl_dae_jac_x_xdot_u_z = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));

    // external functions (implicit model)
    sim_impl_dae_fun->casadi_fun  = &{{ model.name }}_impl_dae_fun;
    sim_impl_dae_fun->casadi_work = &{{ model.name }}_impl_dae_fun_work;
    sim_impl_dae_fun->casadi_sparsity_in = &{{ model.name }}_impl_dae_fun_sparsity_in;
    sim_impl_dae_fun->casadi_sparsity_out = &{{ model.name }}_impl_dae_fun_sparsity_out;
    sim_impl_dae_fun->casadi_n_in = &{{ model.name }}_impl_dae_fun_n_in;
    sim_impl_dae_fun->casadi_n_out = &{{ model.name }}_impl_dae_fun_n_out;
    external_function_param_casadi_create(sim_impl_dae_fun, {{ dims.np }});

    sim_impl_dae_fun_jac_x_xdot_z->casadi_fun = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z;
    sim_impl_dae_fun_jac_x_xdot_z->casadi_work = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z_work;
    sim_impl_dae_fun_jac_x_xdot_z->casadi_sparsity_in = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z_sparsity_in;
    sim_impl_dae_fun_jac_x_xdot_z->casadi_sparsity_out = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z_sparsity_out;
    sim_impl_dae_fun_jac_x_xdot_z->casadi_n_in = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z_n_in;
    sim_impl_dae_fun_jac_x_xdot_z->casadi_n_out = &{{ model.name }}_impl_dae_fun_jac_x_xdot_z_n_out;
    external_function_param_casadi_create(sim_impl_dae_fun_jac_x_xdot_z, {{ dims.np }});

    // external_function_param_casadi impl_dae_jac_x_xdot_u_z;
    sim_impl_dae_jac_x_xdot_u_z->casadi_fun = &{{ model.name }}_impl_dae_jac_x_xdot_u_z;
    sim_impl_dae_jac_x_xdot_u_z->casadi_work = &{{ model.name }}_impl_dae_jac_x_xdot_u_z_work;
    sim_impl_dae_jac_x_xdot_u_z->casadi_sparsity_in = &{{ model.name }}_impl_dae_jac_x_xdot_u_z_sparsity_in;
    sim_impl_dae_jac_x_xdot_u_z->casadi_sparsity_out = &{{ model.name }}_impl_dae_jac_x_xdot_u_z_sparsity_out;
    sim_impl_dae_jac_x_xdot_u_z->casadi_n_in = &{{ model.name }}_impl_dae_jac_x_xdot_u_z_n_in;
    sim_impl_dae_jac_x_xdot_u_z->casadi_n_out = &{{ model.name }}_impl_dae_jac_x_xdot_u_z_n_out;
    external_function_param_casadi_create(sim_impl_dae_jac_x_xdot_u_z, {{ dims.np }});

    {% elif solver_options.integrator_type == "ERK" %}
    // explicit ode
    sim_forw_vde_casadi = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_expl_ode_fun_casadi = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));

    sim_forw_vde_casadi->casadi_fun = &{{ model.name }}_expl_vde_forw;
    sim_forw_vde_casadi->casadi_n_in = &{{ model.name }}_expl_vde_forw_n_in;
    sim_forw_vde_casadi->casadi_n_out = &{{ model.name }}_expl_vde_forw_n_out;
    sim_forw_vde_casadi->casadi_sparsity_in = &{{ model.name }}_expl_vde_forw_sparsity_in;
    sim_forw_vde_casadi->casadi_sparsity_out = &{{ model.name }}_expl_vde_forw_sparsity_out;
    sim_forw_vde_casadi->casadi_work = &{{ model.name }}_expl_vde_forw_work;
    external_function_param_casadi_create(sim_forw_vde_casadi, {{ dims.np }});

    sim_expl_ode_fun_casadi->casadi_fun = &{{ model.name }}_expl_ode_fun;
    sim_expl_ode_fun_casadi->casadi_n_in = &{{ model.name }}_expl_ode_fun_n_in;
    sim_expl_ode_fun_casadi->casadi_n_out = &{{ model.name }}_expl_ode_fun_n_out;
    sim_expl_ode_fun_casadi->casadi_sparsity_in = &{{ model.name }}_expl_ode_fun_sparsity_in;
    sim_expl_ode_fun_casadi->casadi_sparsity_out = &{{ model.name }}_expl_ode_fun_sparsity_out;
    sim_expl_ode_fun_casadi->casadi_work = &{{ model.name }}_expl_ode_fun_work;
    external_function_param_casadi_create(sim_expl_ode_fun_casadi, {{ dims.np }});

    {% elif solver_options.integrator_type == "GNSF" -%}
    sim_gnsf_phi_fun = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_gnsf_phi_fun_jac_y = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_gnsf_phi_jac_y_uhat = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_gnsf_f_lo_jac_x1_x1dot_u_z = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));
    sim_gnsf_get_matrices_fun = (external_function_param_casadi *) malloc(sizeof(external_function_param_casadi));

    sim_gnsf_phi_fun->casadi_fun = &{{ model.name }}_gnsf_phi_fun;
    sim_gnsf_phi_fun->casadi_n_in = &{{ model.name }}_gnsf_phi_fun_n_in;
    sim_gnsf_phi_fun->casadi_n_out = &{{ model.name }}_gnsf_phi_fun_n_out;
    sim_gnsf_phi_fun->casadi_sparsity_in = &{{ model.name }}_gnsf_phi_fun_sparsity_in;
    sim_gnsf_phi_fun->casadi_sparsity_out = &{{ model.name }}_gnsf_phi_fun_sparsity_out;
    sim_gnsf_phi_fun->casadi_work = &{{ model.name }}_gnsf_phi_fun_work;
    external_function_param_casadi_create(sim_gnsf_phi_fun, {{ dims.np }});

    sim_gnsf_phi_fun_jac_y->casadi_fun = &{{ model.name }}_gnsf_phi_fun_jac_y;
    sim_gnsf_phi_fun_jac_y->casadi_n_in = &{{ model.name }}_gnsf_phi_fun_jac_y_n_in;
    sim_gnsf_phi_fun_jac_y->casadi_n_out = &{{ model.name }}_gnsf_phi_fun_jac_y_n_out;
    sim_gnsf_phi_fun_jac_y->casadi_sparsity_in = &{{ model.name }}_gnsf_phi_fun_jac_y_sparsity_in;
    sim_gnsf_phi_fun_jac_y->casadi_sparsity_out = &{{ model.name }}_gnsf_phi_fun_jac_y_sparsity_out;
    sim_gnsf_phi_fun_jac_y->casadi_work = &{{ model.name }}_gnsf_phi_fun_jac_y_work;
    external_function_param_casadi_create(sim_gnsf_phi_fun_jac_y, {{ dims.np }});

    sim_gnsf_phi_jac_y_uhat->casadi_fun = &{{ model.name }}_gnsf_phi_jac_y_uhat;
    sim_gnsf_phi_jac_y_uhat->casadi_n_in = &{{ model.name }}_gnsf_phi_jac_y_uhat_n_in;
    sim_gnsf_phi_jac_y_uhat->casadi_n_out = &{{ model.name }}_gnsf_phi_jac_y_uhat_n_out;
    sim_gnsf_phi_jac_y_uhat->casadi_sparsity_in = &{{ model.name }}_gnsf_phi_jac_y_uhat_sparsity_in;
    sim_gnsf_phi_jac_y_uhat->casadi_sparsity_out = &{{ model.name }}_gnsf_phi_jac_y_uhat_sparsity_out;
    sim_gnsf_phi_jac_y_uhat->casadi_work = &{{ model.name }}_gnsf_phi_jac_y_uhat_work;
    external_function_param_casadi_create(sim_gnsf_phi_jac_y_uhat, {{ dims.np }});

    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_fun = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz;
    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_n_in = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz_n_in;
    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_n_out = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz_n_out;
    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_sparsity_in = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz_sparsity_in;
    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_sparsity_out = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz_sparsity_out;
    sim_gnsf_f_lo_jac_x1_x1dot_u_z->casadi_work = &{{ model.name }}_gnsf_f_lo_fun_jac_x1k1uz_work;
    external_function_param_casadi_create(sim_gnsf_f_lo_jac_x1_x1dot_u_z, {{ dims.np }});

    sim_gnsf_get_matrices_fun->casadi_fun = &{{ model.name }}_gnsf_get_matrices_fun;
    sim_gnsf_get_matrices_fun->casadi_n_in = &{{ model.name }}_gnsf_get_matrices_fun_n_in;
    sim_gnsf_get_matrices_fun->casadi_n_out = &{{ model.name }}_gnsf_get_matrices_fun_n_out;
    sim_gnsf_get_matrices_fun->casadi_sparsity_in = &{{ model.name }}_gnsf_get_matrices_fun_sparsity_in;
    sim_gnsf_get_matrices_fun->casadi_sparsity_out = &{{ model.name }}_gnsf_get_matrices_fun_sparsity_out;
    sim_gnsf_get_matrices_fun->casadi_work = &{{ model.name }}_gnsf_get_matrices_fun_work;
    external_function_param_casadi_create(sim_gnsf_get_matrices_fun, {{ dims.np }});
    {% endif %}

    // sim plan & config
    sim_solver_plan plan;
    plan.sim_solver = {{ solver_options.integrator_type }};

    // create correct config based on plan
    {{ model.name }}_sim_config = sim_config_create(plan);

    // sim dims
    {{ model.name }}_sim_dims = sim_dims_create({{ model.name }}_sim_config);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nx", &nx);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nu", &nu);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nz", &nz);
{% if solver_options.integrator_type == "GNSF" %}
    int gnsf_nx1 = {{ dims.gnsf_nx1 }};
    int gnsf_nz1 = {{ dims.gnsf_nz1 }};
    int gnsf_nout = {{ dims.gnsf_nout }};
    int gnsf_ny = {{ dims.gnsf_ny }};
    int gnsf_nuhat = {{ dims.gnsf_nuhat }};

    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nx1", &gnsf_nx1);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nz1", &gnsf_nz1);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nout", &gnsf_nout);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "ny", &gnsf_ny);
    sim_dims_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims, "nuhat", &gnsf_nuhat);
{% endif %}

    // sim opts
    {{ model.name }}_sim_opts = sim_opts_create({{ model.name }}_sim_config, {{ model.name }}_sim_dims);
    int tmp_int = {{ solver_options.sim_method_num_stages }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "num_stages", &tmp_int);
    tmp_int = {{ solver_options.sim_method_num_steps }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "num_steps", &tmp_int);
    tmp_int = {{ solver_options.sim_method_newton_iter }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "newton_iter", &tmp_int);
    bool tmp_bool = {{ solver_options.sim_method_jac_reuse }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "jac_reuse", &tmp_bool);

{% if problem_class == "SIM" %}
    // options that are not available to AcadosOcpSolver
    //  (in OCP they will be determined by other options, like exact_hessian)
    tmp_bool = {{ solver_options.sens_forw }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "sens_forw", &tmp_bool);
    tmp_bool = {{ solver_options.sens_adj }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "sens_adj", &tmp_bool);
    tmp_bool = {{ solver_options.sens_algebraic }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "sens_algebraic", &tmp_bool);
    tmp_bool = {{ solver_options.sens_hess }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "sens_hess", &tmp_bool);
    tmp_bool = {{ solver_options.output_z }};
    sim_opts_set({{ model.name }}_sim_config, {{ model.name }}_sim_opts, "output_z", &tmp_bool);
{% endif %}

    // sim in / out
    {{ model.name }}_sim_in  = sim_in_create({{ model.name }}_sim_config, {{ model.name }}_sim_dims);
    {{ model.name }}_sim_out = sim_out_create({{ model.name }}_sim_config, {{ model.name }}_sim_dims);
    sim_in_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims,
               {{ model.name }}_sim_in, "T", &Tsim);

    // model functions
{%- if solver_options.integrator_type == "IRK" %}
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "impl_ode_fun", sim_impl_dae_fun);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "impl_ode_fun_jac_x_xdot", sim_impl_dae_fun_jac_x_xdot_z);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "impl_ode_jac_x_xdot_u", sim_impl_dae_jac_x_xdot_u_z);
{%- elif solver_options.integrator_type == "ERK" %}
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "expl_vde_for", sim_forw_vde_casadi);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "expl_ode_fun", sim_expl_ode_fun_casadi);
{%- elif solver_options.integrator_type == "GNSF" %}
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "phi_fun", sim_gnsf_phi_fun);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "phi_fun_jac_y", sim_gnsf_phi_fun_jac_y);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "phi_jac_y_uhat", sim_gnsf_phi_jac_y_uhat);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "f_lo_jac_x1_x1dot_u_z", sim_gnsf_f_lo_jac_x1_x1dot_u_z);
    {{ model.name }}_sim_config->model_set({{ model.name }}_sim_in->model,
                 "gnsf_get_matrices_fun", sim_gnsf_get_matrices_fun);
{%- endif %}

    // sim solver
    {{ model.name }}_sim_solver = sim_solver_create({{ model.name }}_sim_config,
                                               {{ model.name }}_sim_dims, {{ model.name }}_sim_opts);

    /* initialize parameter values */
    {% if dims.np > 0 %}
    // initialize parameters to nominal value
    double p[{{ dims.np }}];
    {% for i in range(end=dims.np) %}
    p[{{ i }}] = {{ parameter_values[i] }};
    {%- endfor %}

{%- if solver_options.integrator_type == "ERK" %}
    sim_forw_vde_casadi[0].set_param(sim_forw_vde_casadi, p);
    sim_expl_ode_fun_casadi[0].set_param(sim_expl_ode_fun_casadi, p);
{%- elif solver_options.integrator_type == "IRK" %}
    sim_impl_dae_fun[0].set_param(sim_impl_dae_fun, p);
    sim_impl_dae_fun_jac_x_xdot_z[0].set_param(sim_impl_dae_fun_jac_x_xdot_z, p);
    sim_impl_dae_jac_x_xdot_u_z[0].set_param(sim_impl_dae_jac_x_xdot_u_z, p);
{%- elif solver_options.integrator_type == "GNSF" %}
    sim_gnsf_phi_fun[0].set_param(sim_gnsf_phi_fun, p);
    sim_gnsf_phi_fun_jac_y[0].set_param(sim_gnsf_phi_fun_jac_y, p);
    sim_gnsf_phi_jac_y_uhat[0].set_param(sim_gnsf_phi_jac_y_uhat, p);
    sim_gnsf_f_lo_jac_x1_x1dot_u_z[0].set_param(sim_gnsf_f_lo_jac_x1_x1dot_u_z, p);
    sim_gnsf_get_matrices_fun[0].set_param(sim_gnsf_get_matrices_fun, p);
{% endif %}
    {% endif %}{# if dims.np #}

    /* initialize input */
    // x
    double x0[{{ dims.nx }}];
    for (int ii = 0; ii < {{ dims.nx }}; ii++)
        x0[ii] = 0.0;

    sim_in_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims,
               {{ model.name }}_sim_in, "x", x0);


    // u
    double u0[{{ dims.nu }}];
    for (int ii = 0; ii < {{ dims.nu }}; ii++)
        u0[ii] = 0.0;

    sim_in_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims,
               {{ model.name }}_sim_in, "u", u0);

    // S_forw
    double S_forw[{{ dims.nx * (dims.nx + dims.nu) }}];
    for (int ii = 0; ii < {{ dims.nx * (dims.nx + dims.nu) }}; ii++)
        S_forw[ii] = 0.0;
    for (int ii = 0; ii < {{ dims.nx }}; ii++)
        S_forw[ii + ii * {{ dims.nx }} ] = 1.0;


    sim_in_set({{ model.name }}_sim_config, {{ model.name }}_sim_dims,
               {{ model.name }}_sim_in, "S_forw", S_forw);

    int status = sim_precompute({{ model.name }}_sim_solver, {{ model.name }}_sim_in, {{ model.name }}_sim_out);

    return status;
}


int {{ model.name }}_acados_sim_solve()
{
    // integrate dynamics using acados sim_solver
    int status = sim_solve({{ model.name }}_sim_solver,
                           {{ model.name }}_sim_in, {{ model.name }}_sim_out);
    if (status != 0)
        printf("error in {{ model.name }}_acados_sim_solve()! Exiting.\n");

    return status;
}


int {{ model.name }}_acados_sim_free()
{
    // free memory
    sim_solver_destroy({{ model.name }}_sim_solver);
    sim_in_destroy({{ model.name }}_sim_in);
    sim_out_destroy({{ model.name }}_sim_out);
    sim_opts_destroy({{ model.name }}_sim_opts);
    sim_dims_destroy({{ model.name }}_sim_dims);
    sim_config_destroy({{ model.name }}_sim_config);

    // free external function
{%- if solver_options.integrator_type == "IRK" %}
    external_function_param_casadi_free(sim_impl_dae_fun);
    external_function_param_casadi_free(sim_impl_dae_fun_jac_x_xdot_z);
    external_function_param_casadi_free(sim_impl_dae_jac_x_xdot_u_z);
{%- elif solver_options.integrator_type == "ERK" %}
    external_function_param_casadi_free(sim_forw_vde_casadi);
    external_function_param_casadi_free(sim_expl_ode_fun_casadi);
{%- elif solver_options.integrator_type == "GNSF" %}
    external_function_param_casadi_free(sim_gnsf_phi_fun);
    external_function_param_casadi_free(sim_gnsf_phi_fun_jac_y);
    external_function_param_casadi_free(sim_gnsf_phi_jac_y_uhat);
    external_function_param_casadi_free(sim_gnsf_f_lo_jac_x1_x1dot_u_z);
    external_function_param_casadi_free(sim_gnsf_get_matrices_fun);
{% endif %}

    return 0;
}


int {{ model.name }}_acados_sim_update_params(double *p, int np)
{
    int status = 0;
    int casadi_np = {{ dims.np }};

{%- if solver_options.integrator_type == "ERK" %}
    sim_forw_vde_casadi[0].set_param(sim_forw_vde_casadi, p);
    sim_expl_ode_fun_casadi[0].set_param(sim_expl_ode_fun_casadi, p);
{%- elif solver_options.integrator_type == "IRK" %}
    sim_impl_dae_fun[0].set_param(sim_impl_dae_fun, p);
    sim_impl_dae_fun_jac_x_xdot_z[0].set_param(sim_impl_dae_fun_jac_x_xdot_z, p);
    sim_impl_dae_jac_x_xdot_u_z[0].set_param(sim_impl_dae_jac_x_xdot_u_z, p);
{%- elif solver_options.integrator_type == "GNSF" %}
    sim_gnsf_phi_fun[0].set_param(sim_gnsf_phi_fun, p);
    sim_gnsf_phi_fun_jac_y[0].set_param(sim_gnsf_phi_fun_jac_y, p);
    sim_gnsf_phi_jac_y_uhat[0].set_param(sim_gnsf_phi_jac_y_uhat, p);
    sim_gnsf_f_lo_jac_x1_x1dot_u_z[0].set_param(sim_gnsf_f_lo_jac_x1_x1dot_u_z, p);
    sim_gnsf_get_matrices_fun[0].set_param(sim_gnsf_get_matrices_fun, p);
{% endif %}

    return status;
}

/* getters pointers to C objects*/
sim_config * {{ model.name }}_acados_get_sim_config()
{
    return {{ model.name }}_sim_config;
};

sim_in * {{ model.name }}_acados_get_sim_in()
{
    return {{ model.name }}_sim_in;
};

sim_out * {{ model.name }}_acados_get_sim_out()
{
    return {{ model.name }}_sim_out;
};

void * {{ model.name }}_acados_get_sim_dims()
{
    return {{ model.name }}_sim_dims;
};

sim_opts * {{ model.name }}_acados_get_sim_opts()
{
    return {{ model.name }}_sim_opts;
};

sim_solver  * {{ model.name }}_acados_get_sim_solver()
{
    return {{ model.name }}_sim_solver;
};

