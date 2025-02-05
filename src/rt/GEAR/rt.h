/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2020 Mladen Ivkovic (mladen.ivkovic@hotmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#ifndef SWIFT_RT_GEAR_H
#define SWIFT_RT_GEAR_H

#include "rt_debugging.h"
#include "rt_gradients.h"
#include "rt_properties.h"
#include "rt_slope_limiters_cell.h"
#include "rt_stellar_emission_rate.h"
#include "rt_thermochemistry.h"

/**
 * @file src/rt/GEAR/rt.h
 * @brief Main header file for the GEAR M1 Closure radiative transfer scheme.
 */

/**
 * @brief Initialisation of the RT density loop related particle data.
 * Note: during initalisation (space_init), rt_reset_part and rt_init_part
 * are both called individually.
 */
__attribute__((always_inline)) INLINE static void rt_init_part(
    struct part* restrict p) {

  rt_gradients_init(p);
  rt_slope_limit_cell_init(p);
}

/**
 * @brief Reset of the RT hydro particle data not related to the density.
 * Note: during initalisation (space_init), rt_reset_part and rt_init_part
 * are both called individually. Also, if debugging checks are active, an
 * extra call to rt_reset_part is made in space_convert_rt_quantities() after
 * the zeroth time step is finished.
 */
__attribute__((always_inline)) INLINE static void rt_reset_part(
    struct part* restrict p) {

#ifdef SWIFT_RT_DEBUG_CHECKS
  /* reset this here as well as in the rt_debugging_checks_end_of_step()
   * routine to test task dependencies are done right */
  p->rt_data.debug_iact_stars_inject = 0;

  p->rt_data.debug_calls_iact_gradient = 0;
  p->rt_data.debug_calls_iact_transport = 0;
  /* skip this for GEAR */
  /* p->rt_data.debug_injection_check = 0; */
  p->rt_data.debug_calls_iact_gradient_interaction = 0;
  p->rt_data.debug_calls_iact_transport_interaction = 0;

  p->rt_data.debug_injection_done = 0;
  p->rt_data.debug_gradients_done = 0;
  p->rt_data.debug_transport_done = 0;
  p->rt_data.debug_thermochem_done = 0;
#endif
}

/**
 * @brief First initialisation of the RT hydro particle data.
 */
__attribute__((always_inline)) INLINE static void rt_first_init_part(
    struct part* restrict p) {

  /* Don't reset conserved quantities here! ICs will be overwritten */
  rt_init_part(p);
  rt_reset_part(p);
  for (int g = 0; g < RT_NGROUPS; g++) {
    p->rt_data.density[g].energy = 0.f;
    p->rt_data.density[g].flux[0] = 0.f;
    p->rt_data.density[g].flux[1] = 0.f;
    p->rt_data.density[g].flux[2] = 0.f;
  }
#ifdef SWIFT_RT_DEBUG_CHECKS
  p->rt_data.debug_radiation_absorbed_tot = 0ULL;
#endif
}

/**
 * @brief Initialisation of the RT density loop related star particle data.
 * Note: during initalisation (space_init), rt_reset_spart and rt_init_spart
 * are both called individually.
 */
__attribute__((always_inline)) INLINE static void rt_init_spart(
    struct spart* restrict sp) {}

/**
 * @brief Reset of the RT star particle data not related to the density.
 * Note: during initalisation (space_init), rt_reset_spart and rt_init_spart
 * are both called individually. Also, if debugging checks are active, an
 * extra call to rt_reset_spart is made in space_convert_rt_quantities() after
 * the zeroth time step is finished.
 */
__attribute__((always_inline)) INLINE static void rt_reset_spart(
    struct spart* restrict sp) {

  for (int g = 0; g < RT_NGROUPS; g++) {
    sp->rt_data.emission_this_step[g] = 0.f;
  }

#ifdef SWIFT_RT_DEBUG_CHECKS
  /* reset this here as well as in the rt_debugging_checks_end_of_step()
   * routine to test task dependencies are done right */
  sp->rt_data.debug_iact_hydro_inject = 0;

  sp->rt_data.debug_emission_rate_set = 0;
  /* skip this for GEAR */
  /* sp->rt_data.debug_injection_check = 0; */

  for (int g = 0; g < RT_NGROUPS; g++) {
    sp->rt_data.debug_injected_energy[g] = 0.f;
  }
#endif
}

/**
 * @brief First initialisation of the RT star particle data.
 */
__attribute__((always_inline)) INLINE static void rt_first_init_spart(
    struct spart* restrict sp) {

  rt_init_spart(sp);
  rt_reset_spart(sp);
#ifdef SWIFT_RT_DEBUG_CHECKS
  sp->rt_data.debug_radiation_emitted_tot = 0ULL;
  for (int g = 0; g < RT_NGROUPS; g++) {
    sp->rt_data.debug_injected_energy_tot[g] = 0.f;
  }
#endif
}

/**
 * @brief Split the RT data of a particle into n pieces
 *
 * @param p The #part.
 * @param n The number of pieces to split into.
 */
__attribute__((always_inline)) INLINE static void rt_split_part(struct part* p,
                                                                double n) {
  error("RT can't run with split particles for now.");
}

/**
 * @brief Exception handle a hydro part not having any neighbours in ghost task
 *
 * @param p The #part.
 */
__attribute__((always_inline)) INLINE static void rt_part_has_no_neighbours(
    struct part* p) {
  message("WARNING: found particle without neighbours");
};

/**
 * @brief Exception handle a star part not having any neighbours in ghost task
 *
 * @param p The #part.
 */
__attribute__((always_inline)) INLINE static void rt_spart_has_no_neighbours(
    struct spart* sp) {

  /* Reset energy to be injected so that global statistics
   * checks still work */
  for (int g = 0; g < RT_NGROUPS; g++) {
    sp->rt_data.emission_this_step[g] = 0.f;
  }
  message("WARNING: found star without neighbours");
};

/**
 * @brief  This function finalises the injection step.
 *
 * @param p particle to work on
 * @param props struct #rt_props that contains global RT properties
 */
__attribute__((always_inline)) INLINE static void
rt_injection_update_photon_density(struct part* restrict p,
                                   struct rt_props* props) {

  const float V = p->geometry.volume;
  const float Vinv = 1. / V;
  for (int g = 0; g < RT_NGROUPS; g++) {
    p->rt_data.density[g].energy = p->rt_data.conserved[g].energy * Vinv;
    p->rt_data.density[g].flux[0] = p->rt_data.conserved[g].flux[0] * Vinv;
    p->rt_data.density[g].flux[1] = p->rt_data.conserved[g].flux[1] * Vinv;
    p->rt_data.density[g].flux[2] = p->rt_data.conserved[g].flux[2] * Vinv;
  }

#ifdef SWIFT_RT_DEBUG_CHECKS
  p->rt_data.debug_injection_done += 1;
#endif
}

/**
 * @brief Compute the photon emission rates for this stellar particle
 *        This function is called every time the spart is being reset
 *        (during start-up and during stars ghost if spart is active)
 *        and assumes that the photon emission rate is an intrinsic
 *        stellar property, i.e. doesn't depend on the environment.
 *
 * @param sp star particle to work on
 * @param time current system time
 * @param star_age age of the star *at the end of the step*
 * @param dt star time step
 * @param rt_props RT properties struct
 * @param phys_const physical constants struct
 * @param internal_units struct holding internal units
 */
__attribute__((always_inline)) INLINE static void
rt_compute_stellar_emission_rate(struct spart* restrict sp, double time,
                                 double star_age, double dt,
                                 const struct rt_props* rt_props,
                                 const struct phys_const* phys_const,
                                 const struct unit_system* internal_units) {

  /* Skip initial fake time-step */
  if (dt == 0.0l) return;

  if (time == 0.l) {
    /* if function is called before the first actual step, time is still
     * at zero unless specified otherwise in parameter file.*/
    star_age = dt;
  }

  /* now get the emission rates */
  double star_age_begin_of_step = star_age - dt;
  star_age_begin_of_step = max(0.l, star_age_begin_of_step);
  rt_set_stellar_emission_rate(sp, star_age_begin_of_step, star_age, rt_props,
                               phys_const, internal_units);
}

/**
 * @brief finishes up the gradient computation
 *
 * @param p particle to work on
 */
__attribute__((always_inline)) INLINE static void rt_end_gradient(
    struct part* restrict p) {

#ifdef SWIFT_RT_DEBUG_CHECKS
  if (p->rt_data.debug_injection_done != 1)
    error(
        "Called finalise gradient on particle "
        "where injection count = %d",
        p->rt_data.debug_injection_done);

  if (p->rt_data.debug_calls_iact_gradient == 0)
    error(
        "Called finalise gradient on particle "
        "with iact gradient count = %d",
        p->rt_data.debug_calls_iact_gradient);
  if (p->rt_data.debug_calls_iact_gradient_interaction == 0)
    message(
        "WARNING: Called finalise gradient on particle "
        "with iact gradient count from rt_iact = %d",
        p->rt_data.debug_calls_iact_gradient_interaction);

  p->rt_data.debug_gradients_done += 1;
#endif

  rt_finalize_gradient_part(p);
}

/**
 * @brief finishes up the transport step
 *
 * @param p particle to work on
 */
__attribute__((always_inline)) INLINE static void rt_finalise_transport(
    struct part* restrict p) {

#ifdef SWIFT_RT_DEBUG_CHECKS
  if (p->rt_data.debug_injection_done != 1)
    error(
        "Trying to do finalise_transport when "
        "injection count is %d",
        p->rt_data.debug_injection_done);

  if (p->rt_data.debug_gradients_done != 1)
    error(
        "Trying to do finalise_transport when "
        "rt_finalise_gradient count is %d",
        p->rt_data.debug_gradients_done);

  if (p->rt_data.debug_calls_iact_gradient == 0)
    error(
        "Called finalise transport on particle "
        "with iact gradient count = %d",
        p->rt_data.debug_calls_iact_gradient);

  if (p->rt_data.debug_calls_iact_transport == 0)
    error(
        "Called finalise transport on particle "
        "with iact transport count = %d",
        p->rt_data.debug_calls_iact_transport);
  if (p->rt_data.debug_calls_iact_transport_interaction == 0)
    message(
        "WARNING: Called finalise transport on particle "
        "with iact transport count from rt_iact = %d",
        p->rt_data.debug_calls_iact_transport_interaction);

  p->rt_data.debug_transport_done += 1;
#endif
}

/**
 * @brief Do the thermochemistry on a particle.
 *
 * This function wraps around rt_do_thermochemistry function.
 *
 * @param p particle to work on
 */
__attribute__((always_inline)) INLINE static void rt_tchem(
    struct part* restrict p) {

  rt_do_thermochemistry(p);
}

/**
 * @brief Clean the allocated memory inside the RT properties struct.
 *
 * @param props the #rt_props.
 */
__attribute__((always_inline)) INLINE static void rt_clean(
    struct rt_props* props) {}

#endif /* SWIFT_RT_GEAR_H */
