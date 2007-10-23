/*
 * Point and Particle class.
 *
 * Coordinates, radii etc. are handled
 * by these classes. Functions for calc.
 * distances, overlap etc. are provided.
 *
 * M. Lund, 2002
 *
 */

#ifndef _point_h
#define _point_h

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "slump.h"

#ifdef SWIG
%module point
%{
#include "point.h"
%}
#endif

using std::ostream;
using namespace std;

//**********************************
//! Class for cartesian coordinates
//**********************************
class point {
  public:
    double x,y,z;               ///< The point coordinates
    point();                    ///< Constructor, zero data.
    void clear();               ///< Zero all data.
    double len();               
    inline double sqdist(point &);  ///< Squared distance
    inline double invdist(point &); ///< Inverse distance (fast)
    inline double dist(point &);    ///< Distance to another point
    double dot(point &);            ///< Angle to another point
    point operator-();              ///< Sign reversal
    point operator*(point);     ///< Multiply two vectors
    point operator*(double);    ///< Scale vector
    point operator+(point);     ///< Add two vectors
    point operator-(point);     ///< Substract vector
    point operator+(double);    ///< Displace x,y,z by value
    void operator+=(point);
    friend ostream &operator<<(ostream &, point &); /// Print x,y,z
    string str();
};

//***********************************************
//! Class for storing and manipulating particles
//***********************************************
class particle : public point {
  public:
    enum type {FIRST=0,GLY,ALA,VAL,LEU,ILE,PHE,TRP,TYR,HIS,SER,THR,MET,CYS,
      ASP,GLN,GLU,ASN,LYS,ARG,PRO,UNK,NTR,CTR,NA,K,CL,BR,I,ION,CATION,ANION,GHOST,
      RNH3,RNH4,RCOOH,RCOO,LAST};

    double charge;         ///< Charge number
    double radius;         ///< Radius
    float mw;              ///< Molecular weight
    type id;               ///< Particle identifier
    particle();            ///< Constructor, zero data.
    inline bool overlap(particle &);    //!< Test for overlap between particles
    inline double potential(point &);   //!< Potential from particle in point
    void operator=(point);         //!< Copy coordinates from a point
};

/*! \brief Class for spherical coordinates
 *  \author Mikael Lund
 *  \date Canberra, 2005-2006
 */
class spherical : private slump {
 public:
  double r,     //!< Radial
         theta, //!< Zenith angle [0:pi]
         phi;   //!< Azimuthal angle [0:2*pi]
  spherical(double=0,double=0,double=0);
  point cartesian();                            //!< Convert to cartesian coordinates
  void operator=(point &);                      //!< Convert from cartesian coordinates
  void random_angles();                         //!< Randomize angles
};

inline void spherical::operator=(point &p) {
  r=p.len();
  theta=acos(p.z/r);
  phi=asin( p.y/sqrt(p.x*p.x+p.y*p.y) );
  if (p.x<0)
    phi=acos(-1.) - phi;
}

inline point spherical::cartesian() {
  point::point p;
  p.x=r*sin(theta)*cos(phi);
  p.y=r*sin(theta)*sin(phi);
  p.z=r*cos(theta);
  return p;
}

//! \todo This function is not completed
inline void spherical::random_angles() {
  r=1.0 ;
}

inline double point::sqdist(point &p) {
  double dx,dy,dz;
  dx=x-p.x;
  dy=y-p.y;
  dz=z-p.z;
  return dx*dx + dy*dy + dz*dz;
}

/*!
 * Note: On MacOS X this function can use the PPC
 * intrinsic reciprocal square root estimate to
 * increase performance. Use -DMACOSX during
 * compilation to envoke this.
 */
inline double point::invdist(point &p) { return 1./sqrt( sqdist(p) );}
inline double point::dist(point &p) { return sqrt(sqdist(p)); }

/*!
 * Calculates electrostatic potential in a point according to
 * \f$ \phi = \frac{charge}{R}\f$
 * where R is the distance to between the point and the particle.
 * Note: Not multiplied with the Bjerrum length!
 */
inline double particle::potential(point &p) { return charge / dist(p); }

inline bool particle::overlap(particle &p) {
  double r=radius+p.radius;
  return (sqdist(p) < r*r) ? true : false;
}

#endif