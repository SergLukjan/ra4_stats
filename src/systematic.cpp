#include "systematic.hpp"

#include <string>

using namespace std;

Systematic::Systematic(const string &name,
		       double strength):
  name_(name),
  strength_(strength){
}

const std::string & Systematic::Name() const{
  return name_;
}

Systematic & Systematic::Name(const std::string &name){
  name_ = name;
  return *this;
}

double Systematic::Strength() const{
  return strength_;
}

Systematic & Systematic::Strength(double strength){
  strength_ = strength;
  return *this;
}

bool Systematic::operator<(const Systematic &systematic) const{
  return ComparisonTuple()<systematic.ComparisonTuple();
}

bool Systematic::operator==(const Systematic &systematic) const{
  return ComparisonTuple()==systematic.ComparisonTuple();
}
