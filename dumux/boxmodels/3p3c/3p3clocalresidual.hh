// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2011-2012 by Holger Class                                 *
 *   Copyright (C) 2008-2009 by Klaus Mosthaf                                *
 *   Copyright (C) 2008-2009 by Bernd Flemisch                               *
 *   Copyright (C) 2009-2010 by Andreas Lauser                               *
 *   Institute for Modelling Hydraulic and Environmental Systems             *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief Element-wise calculation of the Jacobian matrix for problems
 *        using the two-phase two-component box model.
 */
#ifndef DUMUX_3P3C_LOCAL_RESIDUAL_HH
#define DUMUX_3P3C_LOCAL_RESIDUAL_HH

#include "3p3cvolumevariables.hh"
#include "3p3cfluxvariables.hh"
#include "3p3cnewtoncontroller.hh"
#include "3p3cproperties.hh"

#include <dumux/boxmodels/common/boxmodel.hh>
#include <dumux/common/math.hh>

namespace Dumux
{
template<class TypeTag>
class ThreePThreeCLocalResidual: public GET_PROP_TYPE(TypeTag, BaseLocalResidual)
{
protected:
    typedef typename GET_PROP_TYPE(TypeTag, LocalResidual) Implementation;
    typedef typename GET_PROP_TYPE(TypeTag, GridView) GridView;
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, EqVector) EqVector;
    typedef typename GET_PROP_TYPE(TypeTag, RateVector) RateVector;
    typedef typename GET_PROP_TYPE(TypeTag, ThreePThreeCIndices) Indices;

    enum
    {
        dim = GridView::dimension,

        numPhases = GET_PROP_VALUE(TypeTag, NumPhases),
        numComponents = GET_PROP_VALUE(TypeTag, NumComponents),

        conti0EqIdx = Indices::conti0EqIdx
    };

    typedef typename GET_PROP_TYPE(TypeTag, VolumeVariables) VolumeVariables;
    typedef typename GET_PROP_TYPE(TypeTag, ElementContext) ElementContext;

public:
    /*!
     * \brief Evaluate the storage term [kg/m^3] in a single phase.
     *
     * \param element The element
     * \param phaseIdx The index of the fluid phase
     */
    void addPhaseStorage(EqVector &storage,
                         const ElementContext &elemCtx,
                         int timeIdx,
                         int phaseIdx)
    {
        for (int scvIdx = 0; scvIdx < elemCtx.numScv(); ++scvIdx) {
            const VolumeVariables &volVars = elemCtx.volVars(scvIdx, timeIdx);

            const auto &fs = volVars.fluidState();

            // compute storage term of all components within all phases
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                int eqIdx = conti0EqIdx + compIdx;
                storage[eqIdx] +=
                    fs.density(phaseIdx)
                    * fs.massFraction(phaseIdx, compIdx)
                    * fs.saturation(phaseIdx)
                    * volVars.porosity()
                    * volVars.extrusionFactor()
                    * elemCtx.fvElemGeom(timeIdx).subContVol[scvIdx].volume;
            }
        }
    }

    /*!
     * \brief Evaluate the amount all conservation quantities
     *        (e.g. phase mass) within a sub-control volume.
     *
     * The result should be averaged over the volume (e.g. phase mass
     * inside a sub control volume divided by the volume)
     *
     *  \param result The mass of the component within the sub-control volume
     *  \param scvIdx The SCV (sub-control-volume) index
     *  \param usePrevSol Evaluate function with solution of current or previous time step
     */
    void computeStorage(EqVector &storage,
                        const ElementContext &elemCtx,
                        int scvIdx,
                        int timeIdx) const
    {
        const VolumeVariables &volVars =
            elemCtx.volVars(scvIdx, timeIdx);
        const auto &fs = volVars.fluidState();

        storage = 0;
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
        {
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                int eqIdx = conti0EqIdx + compIdx;
                storage[eqIdx] +=
                    fs.density(phaseIdx)
                    * fs.saturation(phaseIdx)
                    * fs.massFraction(phaseIdx, compIdx);
            }
        }
        storage *= volVars.porosity();
    }

    /*!
     * \brief Evaluates the total flux of all conservation quantities
     *        over a face of a subcontrol volume.
     */
    void computeFlux(RateVector &flux,
                     const ElementContext &elemCtx,
                     int scvfIdx,
                     int timeIdx) const
    {
        flux = 0.0;
        asImp_().computeAdvectiveFlux(flux, elemCtx, scvfIdx, timeIdx);
        Valgrind::CheckDefined(flux);

        asImp_().computeDiffusiveFlux(flux, elemCtx, scvfIdx, timeIdx);
        Valgrind::CheckDefined(flux);
    }

    /*!
     * \brief Evaluates the advective mass flux of all components over
     *        a face of a subcontrol volume.
     *
     * \param flux The advective flux over the sub-control-volume face for each component
     * \param vars The flux variables at the current SCV
     */
    void computeAdvectiveFlux(RateVector &flux,
                              const ElementContext &elemCtx,
                              int scvfIdx,
                              int timeIdx) const
    {
        const auto &fluxVars = elemCtx.fluxVars(scvfIdx, timeIdx);
        const auto &evalPointFluxVars = elemCtx.evalPointFluxVars(scvfIdx, timeIdx);

        ////////
        // advective fluxes of all components in all phases
        ////////
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
        {
            // data attached to upstream and the downstream vertices
            // of the current phase
            const VolumeVariables &up = elemCtx.volVars(evalPointFluxVars.upstreamIdx(phaseIdx), timeIdx);
            const VolumeVariables &dn = elemCtx.volVars(evalPointFluxVars.downstreamIdx(phaseIdx), timeIdx);

            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
            {
                int eqIdx = conti0EqIdx + compIdx;

                flux[eqIdx] +=
                    fluxVars.filterVelocityNormal(phaseIdx)
                    *(fluxVars.upstreamWeight(phaseIdx)
                      * up.fluidState().density(phaseIdx)
                      * up.fluidState().massFraction(phaseIdx, compIdx)
                      +
                      fluxVars.downstreamWeight(phaseIdx)
                      * dn.fluidState().density(phaseIdx)
                      * dn.fluidState().massFraction(phaseIdx, compIdx));

                Valgrind::CheckDefined(flux[eqIdx]);
            }
        }

    }

    /*!
     * \brief Adds the diffusive mass flux of all components over
     *        a face of a subcontrol volume.
     *
     * \param flux The diffusive flux over the sub-control-volume face for each component
     * \param vars The flux variables at the current sub control volume face
     */
    void computeDiffusiveFlux(RateVector &flux,
                              const ElementContext &elemCtx,
                              int scvfIdx,
                              int timeIdx) const
    {
        const auto &fluxVars = elemCtx.fluxVars(scvfIdx, timeIdx);
        const auto &normal = elemCtx.fvElemGeom(timeIdx).subContVolFace[scvfIdx].normal;

        // add diffusive flux of gas component in liquid phase
        Scalar tmp = 0;
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            int compIdx = 1; // HACK
            const auto &xGrad = fluxVars.moleFracGrad(phaseIdx, compIdx);
            for (int i = 0; i < dim; ++i)
                tmp += xGrad[i] * normal[i];
            tmp *= -1;

            tmp *=
                fluxVars.porousDiffusionCoefficient(phaseIdx, compIdx) *
                fluxVars.molarDensity(phaseIdx);
        }
    }

    /*!
     * \brief Calculate the source term of the equation
     *
     * \param source The source/sink term [kg/m^3] in the sub control
     *               volume for each component
     */
    void computeSource(RateVector &source,
                       const ElementContext &elemCtx,
                       int scvIdx,
                       int timeIdx) const
    {
        Valgrind::SetUndefined(source);
        elemCtx.problem().source(source, elemCtx, scvIdx, timeIdx);
        Valgrind::CheckDefined(source);
    }

private:
    Implementation &asImp_()
    { return *static_cast<Implementation *> (this); }
    const Implementation &asImp_() const
    { return *static_cast<const Implementation *> (this); }
};

} // end namepace

#endif
