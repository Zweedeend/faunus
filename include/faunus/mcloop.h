#ifndef FAU_MCLOOP_H
#define FAU_MCLOOP_H

#ifndef SWIG

#include <faunus/common.h>
#include <faunus/textio.h>
#include <chrono>

#endif

namespace Faunus {

  /**
   * @brief Counter with arbitrary levels
   *
   * Example:
   * ~~~~
   * Counter cnt( {2,10} );
   * while (cnt[0])
   *   while (cnt[1])
   *     std::cout cnt.count(0) << " " << cnt.count(1) << "\n";
   * ~~~~
   */
  template<typename T=int>
  class Counter {
  protected:
    typedef std::vector<T> Tvec;
    Tvec l, cnt;
    T inner;

  public:
    Counter() {};

    Counter(const Tvec &levels) { set(levels); }

    /** @brief Set levels, for example `set( {10,100} );` */
    void set(const Tvec &levels) {
      l = cnt = levels;
      std::fill(cnt.begin(), cnt.end(), 0);
      inner = 0;
    }


    bool operator[](T level) {
      // For backwards compatibility.
      // The fact that a lookup will also increase the counter is not intuitive.
      return step(level);
    }

    /** @brief Increase counter for level and check if maximum is not reached */
    bool step(T level) {
      assert(level < T(cnt.size()));
      if (cnt[level]++ < l[level]) {  // Wow, side effect!
        if (level == int(cnt.size()) - 1) { // if level is the would refer to the last element of cnt
          inner++;
        }
        return true;  // maximum is not reached, go ahead
      }
      cnt[level] = 0; // overflow, this is desired behavior
      return false;  // maximum is not reached, go ahead
    }

    /** @brief Number of counts of a specific level */
    T count(T level) const { return cnt[level]; }

    /** @brief Total number of counts in innermost loop */
    T innerCount() const { return inner; }
  };

  /**
   * @brief As `Counter` but with timing at the zeroth level
   */
  template<typename T=int, typename base=Counter<T> >
  class TimedCounter : public base {
  protected:
    std::chrono::steady_clock::time_point start;

    int duration() const {
      auto now = std::chrono::steady_clock::now();
      return std::chrono::duration_cast<
          std::chrono::milliseconds>(now - start).count();
    } // milliseconds

    /** @brief milliseconds / 0'th level step */
    int speed() const { return duration() / base::count(0); }

  public:
    TimedCounter(const typename base::Tvec &levels) : base(levels) {}

    TimedCounter() {}

    bool operator[](T level) {
      if (level == 0 && base::cnt[0] == 0)
        start = std::chrono::steady_clock::now();
      return base::operator[](level);
    }

    /** @brief Formatted string with estimated time of arrival (ETA) */
    std::string eta() const {
      std::time_t rawtime = std::time(NULL)
                            + int(speed() * (base::l[0] - base::cnt[0]) / 1000);
      return std::asctime(std::localtime(&rawtime));
    }
  };

  /**
   * @brief Two level loop book-keeping
   * 
   * This class simply keeps track of an outer and inner
   * Markov chain loop. After each macro step an
   * estimated time of the simulation will be evaluated.
   *
   * The constructor will look in the json section `mcloop`
   * for the keywords:
   * 
   * Key     | Description
   * :-------| :----------------------------
   * `macro` | Number of steps in outer loop
   * `micro` | Number of steps in inner loop
   */
  class MCLoop : public TimedCounter<int> {
  private:
    typedef TimedCounter<int> base;
  public:
    inline MCLoop(Tmjson &j, string sec = "system") {
      auto _j = j[sec]["mcloop"];
      base::set({_j["macro"] | 10, _j["micro"] | 1000});
    }

    inline std::string timing() const {
      using namespace textio;
      std::ostringstream o;
      o << indent(SUB) << "Steps: " << std::right << std::setw(4) << base::count(0)
        << " / " << std::left << std::setw(10) << base::innerCount()
        << indent(SUB) << "Macrosteps/min: " << std::left << std::setw(6)
        << int(1. / (base::speed() / 1000. / 60))
        << "  ETA: " << eta() << std::flush;
      return o.str();
    }

    inline std::string info() const {
      using namespace textio;
      std::ostringstream o;
      int s = duration() * 1e-3; // in seconds
      o << header("MC Steps and Time")
        << pad(SUB, 25, "Steps (macro micro tot)") << base::l[0] << "\u2219"
        << base::l[1] << " = " << base::l[0] * base::l[1] << "\n"
        << pad(SUB, 25, "Time elapsed") << s / 60. << " min = " << s / 3600. << " h\n";
      return o.str();
    }

    bool macroCnt() { return base::operator[](0); }

    bool microCnt() { return base::operator[](1); }
  };

}// namespace
#endif
