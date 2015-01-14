#include <faunus/faunus.h>
using namespace Faunus;

#ifdef CUBOID
typedef Geometry::Cuboid Tgeometry;   // specify geometry - here cube w. periodic boundaries
#else
typedef Geometry::Sphere Tgeometry;   // sphere with hard boundaries
#endif
typedef Potential::CoulombHS Tpairpot;// particle pair potential: primitive model

typedef Space<Tgeometry,PointParticle> Tspace;

int main() {
  cout << textio::splash();           // show faunus banner and credits
  
  InputMap mcp("polymers.json");      // open user input file
  MCLoop loop(mcp);                   // class for handling mc loops
  EnergyDrift sys;                    // class for tracking system energy drifts
  UnitTest test(mcp);                 // class for unit testing

  Tspace spc(mcp);                    // instantiate space and load molecules/atoms

  // Create Space and a Hamiltonian with three terms
  auto pot = Energy::Nonbonded<Tspace,Tpairpot>(mcp)
    + Energy::ExternalPressure<Tspace>(mcp) + Energy::Bonded<Tspace>();

  Move::Propagator<Tspace> mv( mcp, pot, spc );

  Analysis::PolymerShape shape;
  Analysis::RadialDistribution<> rdf(0.2);

  spc.load("state");                                // load old config. from disk (if any)
  sys.init( Energy::systemEnergy(spc,pot,spc.p)  ); // store initial total system energy

  auto pol = spc.findMolecules("polymer");          // all molecules named "polymer" 

  cout << atom.info() << spc.info() << pot.info() << textio::header("MC Simulation Begins!");

  while ( loop[0] ) {  // Markov chain 
    while ( loop[1] ) {
      sys += mv.move();

      for (auto i : pol)                              // sample gyration radii etc. 
        shape.sample(*i,spc);

      for (auto i = pol.begin(); i != pol.end(); ++i )// sample cm-cm g(r)
        for (auto j = i; ++j != pol.end(); )
          rdf( spc.geo.dist( (**i).cm, (**j).cm) )++;

    } // end of micro loop

    sys.checkDrift(Energy::systemEnergy(spc,pot,spc.p)); // compare energy sum with current
    cout << loop.timing();

  } // end of macro loop

  // save to disk
  rdf.save("rdf_p2p.dat");
  spc.save("state");
  FormatPQR::save("confout.pqr", spc.p);

  // unit tests
  mv.test(test);
  sys.test(test);

  // print information
  cout << loop.info() << mv.info() << shape.info()
    << sys.info() << test.info();

  return test.numFailed();
}
/** 
 * @page example_polymers Example: Polymers
 *
 * This will simulate an arbitrary number of linear polymers in the NPT ensemble
 * with explicit salt particles and implicit solvent (dielectric continuum).
 * We include the following Monte Carlo moves:
 *
 * - salt translation
 * - monomer translation
 * - polymer translation and rotation
 * - polymer crankshaft and pivot rotations
 * - isobaric volume move (NPT ensemble)
 *
 * ![Hardsphere polyelectrolytes with counter ions](polymers.png)
 *
 * Run this example from the `examples` directory:
 *
 * ~~~~~~~~~~~~~~~~~~~
 * $ make
 * $ cd src/examples
 * $ ./polymers.run
 * ~~~~~~~~~~~~~~~~~~~
 *
 * polymers.cpp
 * ============
 *
 * @includelineno examples/polymers.cpp
 */

