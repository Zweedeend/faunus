#ifndef MCLOOP_H
#define MCLOOP_H

#include "inputfile.h"
#include "countdown.h"

/*!
 * This class keeps track of the outer and inner
 * Markov chain loops. Also, after each macro step
 * it can estimate when the simulation will finish
 *
 * \author Mikael Lund
 * \date 2007
 */
class mcloop {
  private:
    countdown<unsigned int> cnt;
  public:
    unsigned int macro, micro;
    bool eq;
    mcloop(inputfile &);           //!< Setup
    string info();               //!< Shows setup
    string timing(unsigned int); //!< Show macrostep middle time and ETA.
};

/*!
 * \param in Inputfile class. "macrosteps" and "microsteps" will be searched for.
 */
mcloop::mcloop(inputfile &in)
  : cnt( in.getint("macrosteps",10))
{
  macro=in.getint("macrosteps",10);
  micro=in.getint("microsteps");
  eq=in.getboo("equilibration", false);
}

string mcloop::info() {
  ostringstream o;
  o << "# Steps (macro micro tot)  = "
    << macro << " " << micro << " " << macro*micro << endl;
  return o.str();
}

string mcloop::timing(unsigned int mac) {
  ostringstream o;
  o << "# Macrostep " << mac << " completed. ETA: "
    << cnt.eta(mac);
  return o.str();
}

#endif