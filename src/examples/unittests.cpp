#undef NDEBUG
#define CATCH_CONFIG_MAIN  // This tell CATCH to provide a main() - only do this in one cpp file
#define DIPOLEPARTICLE
#include <catch/catch.hpp>
#include <faunus/faunus.h>
#include <faunus/ewald.h>

using namespace Faunus;

TEST_CASE( "Test Quantize", "[Quantize]" ) {
    Quantize<float> Q(0.5, -0.5);
    Q = 0.61;
    CHECK( double(Q) == Approx(0.5) );
    CHECK( int(Q) == 2 );
    //CHECK( Q.frombin(2) == Approx(0.5) );
    CHECK( int(Q(-0.5)) == 0 );
}

/* check tabulator */
template<typename Ttabulator>
void checkTabulator(Ttabulator t) {
  double tol=0.01;
  t.setRange(0.9, 100);
  t.setTolerance(tol,tol);
  std::function<double(double)> f = [](double x) { return 1/x; };
  auto data = t.generate(f); 
  for (double x=1.0; x<100; x+=1) {
    double error = fabs( f(x) - t.eval(data,x) );
    CHECK(error>0);
    CHECK(error<tol);
  }
}

TEST_CASE("Spline table", "Spline")
{
  checkTabulator(Tabulate::Hermite<double>());
  checkTabulator(Tabulate::AndreaIntel<double>());
  checkTabulator(Tabulate::Andrea<double>());
  checkTabulator(Tabulate::Linear<double>());

  PointParticle a,b;
  a.charge=1;
  b.charge=-1;
  a.radius=b.radius=2;
  InputMap mcp("unittests.json");
  auto js = mcp.at("energy").at("nonbonded");
  atom.include(mcp);
  Potential::Coulomb pot_org( js );
  Potential::PotentialTabulate<Potential::Coulomb> pot_tab( js );

  CHECK( pot_org.bjerrumLength() == Approx(560.455786334) );

  double error = fabs( pot_org(a,b,25)-pot_tab(a,b,25) ) ;
  CHECK(error>0);
  CHECK(error<0.01);

  // Check if negative potential operator works
  auto minus = Potential::Coulomb( js ) - Potential::Coulomb( js );
  CHECK( abs(minus(a,b,7)) < 1e-6 );
}

/*
 * Check various copying operations
 * between particle types
 */
template<typename Tparticle>
void checkParticle() {
  Tparticle p;
  p.clear();
  p.mw=1.0; // set some property - should not be overridden!
  Point::Tvec vec(1,0,0);
  p=vec;
  CHECK( p.norm() == Approx(1) );
  p+=vec;
  CHECK( p.norm() == Approx(2) );
  p*=0.5;
  CHECK( p.norm() == Approx(1) );

  CHECK( p.mw == Approx(1) ); // mw should not be overridden!

  p=Point::Tvec(2,3,5);
  CHECK( p.len() == Approx( sqrt(4+9+25) ) );

  CHECK( p.mw == Approx(1) ); // mw should not be overridden!

  PointParticle a;
  a.clear();
  a.x()=10.;
  a.charge=1.0;
  p=a; 
  CHECK( p.x() == Approx(a.x()) );
  CHECK( p.charge == Approx(a.charge) );
  CHECK( p.mw == Approx(0) );
}

TEST_CASE("Particles", "...")
{
  checkParticle<PointParticle>();
  checkParticle<DipoleParticle>();
  checkParticle<CigarParticle>();
}

TEST_CASE("Polar Test","Ion-induced dipole test (polarization)") 
{
  using namespace Faunus::Potential;
  typedef CombinedPairPotential<Coulomb,IonDipole> Tpair;
  typedef Space<Geometry::Cuboid, DipoleParticle> Tspace;

  InputMap in("unittests.json");
  Tspace spc(in);
  Energy::NonbondedVector<Tspace,Tpair> pot(in);
  Move::PolarizeMove<Move::AtomicTranslation<Tspace> > mv(pot,spc,in.at("moves").at("atomtranslate"));

  CHECK( mv.getThreshold() == Approx(0.0011) );
  CHECK( mv.getMaxIterations() == 39 );

  auto m = spc.molList().find( "multipoles" );
  spc.insert( m->id, m->getRandomConformation() );

  spc.p[0] = Point(0,0,0);
  spc.p[1] = Point(0,0,4);
  spc.trial = spc.p;

  CHECK( pc::T() == 298 );
  CHECK( spc.p.size() == 2 );

  CHECK( mv.move(1) == Approx(-5.695030454893824) ); // check energy change
  CHECK( spc.p[1].muscalar() == Approx(0.1625) ); // check induced moment
}

TEST_CASE("Ewald Test","Ion-Ion- and Dipole-Dipole-interaction") 
{
  // Check against values from article: http://dx.doi.org/10.1063/1.481216
  typedef Space<Geometry::Cuboid,DipoleParticle> Tspace;
  InputMap in("unittests.json");    
  auto pot = Energy::NonbondedEwald<Tspace,Potential::HardSphere,true,false,true>(in);
  Space<Geometry::Cuboid,DipoleParticle> spc(in);         
  spc.p.resize(4);
  Group g(0,3);
  spc.groupList().push_back(&g);
  
  spc.p[0] = Point(0,0,0);
  spc.p[1] = Point(1,0,0);
  spc.p[2] = Point(0,0,1);
  spc.p[3] = Point(1,0,1);
  for(unsigned int i = 0; i < spc.p.size(); i++) {
      spc.p[i].charge = 0.0;
      spc.p[i].muscalar() = 0.0;
  }
  spc.p[0].charge = 1.0;
  spc.p[1].charge = -1.0;
  spc.trial = spc.p;
  pot.setSpace(spc); // Updates vectors in Ewald
  
  // Extra check. Checks that 'updateChange' and 'update' is working OK
  spc.p[0] = Point(0.5,0,0); // Move particle 
  spc.trial = spc.p;
  pot.setSpace(spc);
  spc.trial[0] = Point(0,0,0); // Move particle back
  Tspace::Change c;
  c.mvGroup[0].push_back(0); // Add particle 0 in group 0 to moved particles
  pot.updateChange(c);
  pot.update(true);
  spc.p = spc.trial;
  
  double ureal = pot.pairpot(spc.p[0],spc.p[1],spc.p[1]-spc.p[0]);
  double usurf_reci = pot.external(spc.p);
  double uself = pot.g_external(spc.p, g);
  pc::setT(298.0);
  double lB = pc::lB(1.0);//560.7378949512486; // Scale with 'lB' in order to get right energy at 298 K 
  
  CHECK(ureal == Approx(-0.205903210732068*lB));
  CHECK(usurf_reci == Approx(0.215125076289333*lB));  // reciprocal energy in addition to surface energy
  CHECK(uself == Approx(-1.009253008808064*lB)); 
  CHECK(Energy::systemEnergy(spc,pot,spc.p) == Approx(-1.0000311*lB));  // Total ion-Ion interaction energy
  
  spc.p[0].charge = 0.0;
  spc.p[1].charge = 0.0;
  spc.p[2].muscalar() = 1.0;
  spc.p[3].muscalar() = 1.0;
  spc.p[2].mu() = Point(1,0,0);
  spc.p[3].mu() = Point(1,0,0);
  spc.trial = spc.p;
  pot.setSpace(spc); // Updates vectors in Ewald
  ureal = pot.pairpot(spc.p[2],spc.p[3],spc.p[2]-spc.p[3]);
  usurf_reci = pot.external(spc.p);
  uself = pot.g_external(spc.p, g);
  
  CHECK(ureal == Approx(-2.044358213791836*lB));
  CHECK(usurf_reci == Approx(0.582251578315622*lB)); // reciprocal energy in addition to surface energy
  CHECK(uself == Approx(-0.538268271364301*lB));
  CHECK(Energy::systemEnergy(spc,pot,spc.p) == Approx(-2.0003749*lB));  // Total dipole-dipole interaction energy
}

TEST_CASE("Groups", "Check group range and size properties")
{
  Group g(2,5);           // first, last particle

  CHECK(g.front()==2);
  CHECK(g.back()==5);
  CHECK(g.size()==4);

  g.resize(0);
  CHECK(g.empty());

  g.resize(1000);
  CHECK(g.size()==1000);

  g.setfront(1);
  g.setback(10);
  CHECK(g.front()==1);
  CHECK(g.back()==10);
  CHECK(g.size()==10);

  int cnt=0;
  for (auto i : g) {
    cnt++;
    CHECK(g.find(i));
  }
  CHECK(cnt==g.size());
  CHECK(!g.find(0));
  CHECK(!g.find(11));

  // check random
  int min=1e6, max=-1e6;
  for (int n=0; n<1e3; n++) {
    int i=g.random();
    if (i<min) min=i;
    if (i>max) max=i;
    CHECK( g.find(i) );
  }
  CHECK(min==g.front());
  CHECK(max==g.back());
}

TEST_CASE("Math", "Checks mathematical functions")
{
  CHECK( pc::infty == -std::log(0) );
  CHECK( exp_cawley(1) == Approx(std::exp(1)).epsilon(0.05) );
  CHECK( invsqrtQuake(20.) == Approx(1/std::sqrt(20.)).epsilon(0.05) );
}

TEST_CASE("Space", "Space tests")
{
  //::atom.includefile("unittests.json");
  //InputMap in("unittests.input");
  //typedef Space<Geometry::Cuboid, PointParticle> Tspace;
  //Tspace spc(in);
  //PointParticle a;
  //spc.insert(a);
}

TEST_CASE("Geometries", "Geometry tests")
{
  Geometry::Sphere geoSph(1000);
  Geometry::Cylinder geoCyl(1000,1000);
  Point a(0,0,sqrt(16)), b(0,sqrt(64),0);
  double x = geoSph.sqdist(a,b);
  double y = geoCyl.sqdist(a,b);
  CHECK( x==Approx(16+64) );
  CHECK( x==Approx(y) );
}

TEST_CASE("Random numbers", "Check random number generator")
{
  int min=10, max=0, N=1e7;
  double x=0;
  for (int i=0; i<N; i++) {
    int j = slump.range(0,9);
    if (j<min) min=j;
    if (j>max) max=j;
    x+=j;
  }
  CHECK( min==0 );
  CHECK( max==9 );
  CHECK( std::fabs(x/N) == Approx(4.5).epsilon(0.05) );
}

TEST_CASE("Quaternion", "Check vector rotation")
{
  Geometry::QuaternionRotate qrot;
  Geometry::Cylinder geo(1000,1000);
  Point a(0,0,0);
  a.x()=1.;
  qrot.setAxis( geo, Point(0,0,0), Point(0,1,0), pc::pi/2); // rotate around y-axis
  a = qrot(a); // rot. 90 deg.
  CHECK( a.x() == Approx(0.0) );
  a = qrot(a); // rot. 90 deg.
  CHECK( a.x() == Approx(-1.0) );
}

TEST_CASE("Tables and averages","Check table of averages")
{
  typedef Table2D<float,Average<float> > Ttable;
  Ttable table(0.1, Ttable::XYDATA);
  table(2.1)+=1;
  table(2.1)+=3;
  CHECK( table(2.1).avg() == Approx(2.0) );
}

TEST_CASE("String literals","Check unit conversion")
{
  using namespace ChemistryUnits;
  pc::setT(298.15);
  CHECK( 1.0e-10_m == 1 );
  CHECK( (1/1.0_Debye) == Approx(4.8032) );
  CHECK( 1.0_Debye == Approx( 3.33564e-30_Cm ) );
  CHECK( 1.0_Debye == Approx( 0.20819434_eA ) );
  CHECK( 360.0_deg == Approx( 2*std::acos(-1) ) );
  CHECK( (1.0_mol / 1.0_liter) == Approx( 1.0_molar ) );
  CHECK( 1.0_bar == Approx( 0.987_atm ) );
  CHECK( 1.0_atm == Approx( 101325._Pa) );
  CHECK( 1.0_kT == Approx( 2.47897_kJmol ) );
  CHECK( 1.0_hartree == Approx( 2625.499_kJmol ) );
}

