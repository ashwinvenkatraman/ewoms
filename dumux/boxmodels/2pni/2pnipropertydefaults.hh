// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*****************************************************************************
 *   Copyright (C) 2008 by Klaus Mosthaf                                     *
 *   Copyright (C) 2008-2009 by Andreas Lauser                               *
 *   Copyright (C) 2008 Bernd Flemisch                                       *
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
 * \ingroup Properties
 * \ingroup BoxProperties
 *  \ingroup TwoPNIBoxModel
 * \file
 *
 * \brief Defines the default values for most of the properties
 *        required by the non-isothermal two-phase box model.
 */

#ifndef DUMUX_2PNI_PROPERTY_DEFAULTS_HH
#define DUMUX_2PNI_PROPERTY_DEFAULTS_HH

#include "2pniproperties.hh"
#include "2pnimodel.hh"
#include "2pniproblem.hh"
#include "2pnilocalresidual.hh"
#include "2pnivolumevariables.hh"
#include "2pnifluxvariables.hh"
#include "2pniboundaryratevector.hh"
#include "2pniindices.hh"

#include <dumux/material/heatconduction/dummyheatconductionlaw.hh>

namespace Dumux
{
namespace Properties
{
//////////////////////////////////////////////////////////////////
// Property values
//////////////////////////////////////////////////////////////////

SET_INT_PROP(BoxTwoPNI, NumEq, 3); //!< set the number of equations to 3

//! Use the 2pni local jacobian operator for the 2pni model
SET_TYPE_PROP(BoxTwoPNI,
              LocalResidual,
              TwoPNILocalResidual<TypeTag>);

//! the Model property
SET_TYPE_PROP(BoxTwoPNI, Model, TwoPNIModel<TypeTag>);

//! The type of the base base class for actual problems
SET_TYPE_PROP(BoxTwoPNI, BaseProblem, TwoPNIProblem<TypeTag>);

//! the TwoPFluidState property
SET_PROP(BoxTwoP, TwoPFluidState)
{ private:
    typedef typename GET_PROP_TYPE(TypeTag, Scalar) Scalar;
    typedef typename GET_PROP_TYPE(TypeTag, FluidSystem) FluidSystem;
public:
    typedef Dumux::ImmiscibleFluidState<Scalar,
                                        FluidSystem,
                                        /*enableEnthalpy=*/true> type;
};

//! the VolumeVariables property
SET_TYPE_PROP(BoxTwoPNI, VolumeVariables, TwoPNIVolumeVariables<TypeTag>);

//! the FluxVariables property
SET_TYPE_PROP(BoxTwoPNI, FluxVariables, TwoPNIFluxVariables<TypeTag>);

//! extract the type parameter objects for the heat conduction law
//! from the law itself
SET_TYPE_PROP(BoxTwoPNI,
              HeatConductionLawParams,
              typename GET_PROP_TYPE(TypeTag, HeatConductionLaw)::Params);

//! The indices required by the non-isothermal two-phase model
SET_TYPE_PROP(BoxTwoPNI, Indices, TwoPNIIndices<0>);

//! The boundary rate vector for the 2pni model.
SET_TYPE_PROP(BoxTwoPNI, BoundaryRateVector, TwoPNIBoundaryRateVector<TypeTag>);

}
}

#endif
