#include "readGravitationalAcceleration.H"

Info<< "Creating combustion model\n" << endl;

autoPtr<combustionModels::rhoCombustionModel> combustion
(
    combustionModels::rhoCombustionModel::New(mesh)
);

rhoReactionThermo& thermo = combustion->thermo();
thermo.validate(args.executable(), "h", "e");

SLGThermo slgThermo(mesh, thermo);

basicSpecieMixture& composition = thermo.composition();
PtrList<volScalarField>& Y = composition.Y();

const word inertSpecie(thermo.lookup("inertSpecie"));

if (!composition.contains(inertSpecie))
{
    FatalErrorInFunction
        << "Specified inert specie '" << inertSpecie << "' not found in "
        << "species list. Available species:" << composition.species()
        << exit(FatalError);
}

volScalarField& p = thermo.p();

volScalarField rho
(
    IOobject
    (
        "rho",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    thermo.rho()
);

Info<< "\nReading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

#include "compressibleCreatePhi.H"

mesh.setFluxRequired(p.name());

dimensionedScalar rhoMax
(
    dimensionedScalar::lookupOrDefault
    (
        "rhoMax",
        simple.dict(),
        dimDensity,
        GREAT
    )
);

dimensionedScalar rhoMin
(
    dimensionedScalar::lookupOrDefault
    (
        "rhoMin",
        simple.dict(),
        dimDensity,
        0
    )
);

Info<< "Creating turbulence model\n" << endl;
autoPtr<compressible::turbulenceModel> turbulence
(
    compressible::turbulenceModel::New
    (
        rho,
        U,
        phi,
        thermo
    )
);

// Set the turbulence into the combustion model
combustion->setTurbulence(turbulence());

Info<< "Creating multi-variate interpolation scheme\n" << endl;
multivariateSurfaceInterpolationScheme<scalar>::fieldTable fields;

forAll(Y, i)
{
    fields.add(Y[i]);
}
fields.add(thermo.he());

volScalarField dQ
(
    IOobject
    (
        "dQ",
        runTime.timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    mesh,
    dimensionedScalar("dQ", dimEnergy/dimTime, 0.0)
);

#include "createMRF.H"
#include "createRadiationModel.H"
#include "createClouds.H"
