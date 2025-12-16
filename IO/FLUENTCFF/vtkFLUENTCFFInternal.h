// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @namespace vtkFLUENTCFFInternal
 * @brief internal utilities for vtkFLUENTCFFReader
 *
 * Utility methods used by vtkFLUENTCFFReader to retrieve the matching array name.
 */
#ifndef vtkFLUENTCFFUtilities_h
#define vtkFLUENTCFFUtilities_h

#include <string>
#include <unordered_map>

namespace vtkFLUENTCFFInternal
{
/**
 * Retrieve the correct field name. If there is not match, returns the same string passed as an
 * argument
 */
std::string GetMatchingFieldName(const std::string& strSectionName);

/**
 * Remove the trailing index of the string if it ends by "_1" or any other digit
 */
bool RemoveTrailingIndex(std::string& fieldName);

/**
 * Remove the suffix passed in argument if it is prensent in the field name
 */
bool RemoveSuffixIfPresent(std::string& fieldName, const std::string& suffix);

const std::unordered_map<std::string, std::string> FieldsNamesMap = { { "R", "density" },
  { "P", "pressure" }, { "U", "u velocity" }, { "V", "v velocity" }, { "W", "w velocity" },
  { "T", "temperature" }, { "H", "enthalpy" }, { "K", "turb. kinetic energy" },
  { "NUT", "turbulent viscosity for Spalart-Allmaras" },
  { "D", "turb. kinetic energy dissipation rate" }, { "O", "specific dissipation rate" },
  { "YI", "species mass fraction" }, { "IGNITE", "ignition mass fraction" },
  { "PREMIXC_T", "premixed combustion temperature" },
  { "STORAGE_R", "value of variable nv (generic flow variable)" },
  { "POR", "porosity of fluid cell" },
  { "DUAL_ZN_POROSITY", "porosity of fluid at the dual cell zone region" },
  { "STRESS_RATE_MAG", "strain rate magnitude" }, { "DUDX", "velocity derivative" },
  { "DUDY", "velocity derivative" }, { "DUDZ", "velocity derivative" },
  { "DVDX", "velocity derivative" }, { "DVDY", "velocity derivative" },
  { "DVDZ", "velocity derivative" }, { "DWDX", "velocity derivative" },
  { "DWDY", "velocity derivative" }, { "DWDZ", "velocity derivative" },
  { "MU_L", "laminar viscosity" }, { "MU_T", "turbulent viscosity" },
  { "MU_EFF", "effective viscosity" }, { "K_L", "thermal conductivity" },
  { "K_T", "turbulent thermal conductivity" }, { "K_EFF", "effective thermal conductivity" },
  { "DIFF_L", "laminar species diffusivity" }, { "DIFF_EFF", "effective species diffusivity" },
  { "CP", "specific heat" }, { "RGAS", "universal gas constant/molecular weight" },
  { "FMEAN", "primary mean mixture fraction" }, { "FMEAN2", "secondary mean mixture fraction" },
  { "FVAR", "primary mixture fraction variance" },
  { "FVAR2", "secondary mixture fraction variance" }, { "PREMIXC", "reaction progress variable" },
  { "LAM_FLAME_SPEED", "laminar flame speed" }, { "SCAT_COEFF", "scattering coefficient" },
  { "ABS_COEFF", "absorption coefficient" }, { "CRITICAL_STRAIN_RATE", "critical strain rate" },
  { "LIQF", "liquid fraction in a cell" }, { "POLLUT", "pollutant species mass fraction" },
  { "RUU", "uu Reynolds stress" }, { "RVV", "vv Reynolds stress" }, { "RWW", "ww Reynolds stress" },
  { "RUV", "uv Reynolds stress" }, { "RVW", "vw Reynolds stress" }, { "RUW", "uw Reynolds stress" },
  { "VOF", "volume fraction" }, { "PHI", "potential value" },
  { "ELEC_COND", "electrical conductivity" },
  { "OVER_POTENTIAL", "over-potential for Butler-Volmer equation in electrolysis model (V)" },
  { "OSMOTIC_DRAG", "osmotic drag source terms (kg/m3 s)" },
  { "WATERCONTENT", "water content (-)" }, { "TRANSFER_CURRENT", "transfer current rate (A/m3)" },
  { "DUAL_ELEC_COND", "ionic conductivity" }, { "JOULE_HEATING", "Joule heating rate (W/m3)" },
  { "DPMS_MOM_S", "momentum source explicit" }, { "DPMS_MOM_AP", "momentum source implicit" },
  { "DPMS_WSWIRL_S", "momentum source swirl component explicit" },
  { "DPMS_WSWIRL_AP", "momentum source swirl component implicit" },
  { "DPMS_ENERGY", "energy source explicit" }, { "DPMS_ENERGY_AP", "energy source implicit" },
  { "DPMS_YI", "species mass source explicit" }, { "DPMS_YI_AP", "species mass source implicit" },
  { "DPMS_REACTION_RATE_POST", "reaction rates of particle" },
  { "DPMS_VAP_PER_MAT", "vaporization mass source" },
  { "DPMS_DEVOL_PER_MAT", "devolatilization mass source" },
  { "DPMS_BURN_PER_MAT", "burnout mass source" }, { "DPMS_PDF", "mass source of pdf stream 1" },
  { "DPMS_EMISS", "particles emissivity" }, { "DPMS_ABS", "particles absorption coefficient" },
  { "DPMS_SCAT", "particles scattering coefficient" }, { "DPMS_BURNOUT", "burnout mass source" },
  { "DPMS_CONCENTRATION", "concentration of particles" },
  { "DPMS_SURF_YI", "concentration of particle surface species" }, { "UDSI", "UDS variables" },
  { "UDSI", "UDS" }, { "UDSI_DIFF", "UDS diffusivity" }, { "DT_CG", "center of gravity vector" },
  { "DT_VEL_CG", "cg velocity vector" }, { "DT_OMEGA_CG", "angular velocity vector" },
  { "DT_THETA", "orientation of body-fixed axis vector" }, { "TP_POS", "position" },
  { "TP_VEL", "velocity" }, { "TP_DIAM", "diameter" }, { "TP_T", "temperature" },
  { "TP_RHO", "density" }, { "TP_MASS", "mass" }, { "TP_TIME", "current particle time" },
  { "TP_DT", "time step" }, { "TP_FLOW_RATE", "flow rate of particles in a stream" },
  { "TP_LMF", "liquid mass fraction" }, { "TP_LF", "liquid volume fraction" },
  { "TP_VF", "volatile fraction" }, { "TP_CF", "char mass fraction" },
  { "TP_VFF", "volatile fraction remaining" }, { "DPM_BOILING_TEMPERATURE", "boiling temperature" },
  { "DPM_DIFFUSION_COEFF", "Binary diffusion coefficient" },
  { "DPM_BINARY_DIFFUSIVITY", "Binary diffusion coefficient" },
  { "DPM_EMISSIVITY", "emissivity factor for the radiation model" },
  { "DPM_SCATT_FACTOR", "scattering factor for the radiation model" },
  { "DPM_KTC", "thermal conductivity" }, { "DPM_HEAT_OF_PYROLYSIS", "heat of pyrolysis" },
  { "DPM_HEAT_OF_REACTION", "heat of reaction" }, { "DPM_LATENT_HEAT", "latent heat" },
  { "DPM_LIQUID_SPECIFIC_HEAT",
    "specific heat of material used for liquid associated with particle" },
  { "DPM_MU", "dynamic viscosity of liquid part of particle" }, { "DPM_RHO", "particle density" },
  { "DPM_SPECIFIC_HEAT", "specific heat" },
  { "DPM_SWELLING_COEFF", "swelling coefficient for devolatilization" },
  { "DPM_SURFTEN", "surface tension of liquid part of particles" },
  { "DPM_VAPOR_PRESSURE", "vapor pressure of liquid part of particle" },
  { "DPM_VAPOR_TEMP", "vaporization temperature" },
  { "DPM_EVAPORATION_TEMPERATURE", "vaporization temperature" }, { "BF", "boundary face" },
  { "BF_V", "boundary face velocity" }, { "CFF_0", "CFF" }, { "DENSITY", "density of fluid" },
  { "DPM_PARTITION", "DPM domaine partition" }, { "DPMS_DS_BURNOUT", "DPM burnout rate source" },
  { "DPMS_DS_ENERGY", "DPM energy source" }, { "DPMS_DS_MASS", "DPM mass source" },
  { "DPMS_DS_MOM_S", "DPM momentum source" }, { "DPMS_DS_SPECIES", "DPM species source" },
  { "DPMS_DS_VAP_PER_MAT", "DPM vaporization per material" },
  { "DPMS_DS_WSWIRL_S", "DPM swirl source" }, { "DPMS_MASS", "DPM mass" },
  { "DPMS_SPECIES", "DPM species mass" }, { "DPMS_VAP_PER_MAT_0", "DPM vaporization per material" },
  { "ENERGY", "energy" }, { "MU_LAM", "laminar dynamic viscosity" },
  { "PHASE_MASS", "phase mass in multiphase flow" }, { "THIN_FILM", "thin film model quantity" },
  { "WALL_DIST", "distance to nearest wall" }, { "Y", "mass fraction" } };
};

#endif
