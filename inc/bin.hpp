#ifndef H_BIN
#define H_BIN

#include <string>
#include <vector>
#include <tuple>

#include "systematic.hpp"

class Bin{
  typedef std::vector<Systematic> SystCollection;
public:
  Bin(const std::string &name, const std::string &cut,
      const SystCollection &systematics = SystCollection());

  const std::string Name() const;
  Bin & Name(const std::string &name);

  const std::string & Cut() const;
  Bin & Cut(const std::string &cut);

  const SystCollection & Systematics() const;
  Bin & Systematics(const SystCollection &systematics);
  Bin & AddSystematic(const Systematic &systematic);
  Bin & AddSystematics(const SystCollection &systematic);
  bool HasSystematic(const Systematic &systematic) const;
  Bin & RemoveSystematic(const Systematic &systematic);
  Bin & RemoveSystematics();
  Bin & SetSystematicStrength(const std::string &name, double strength);

  bool operator<(const Bin &b) const;

private:
  std::string name_, cut_;
  SystCollection systematics_;

  auto ComparisonTuple() const{
    return make_tuple(cut_, systematics_);
  }
};

#endif
