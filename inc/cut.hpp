#ifndef H_CUT
#define H_CUT

#include <string>
#include <ostream>

class Cut{
public:
  Cut(const std::string &cut = "1");
  Cut(const char *cut);

  Cut & Replace(const Cut &orig, const Cut &rep);
  Cut & RmCutOn(const Cut &to_rm, const Cut &rep = Cut());

  Cut & operator &= (const Cut &cut);
  Cut & operator |= (const Cut &cut);
  Cut & operator += (const Cut &cut);
  Cut & operator -= (const Cut &cut);
  Cut & operator *= (const Cut &cut);
  Cut & operator /= (const Cut &cut);
  Cut & operator %= (const Cut &cut);
  Cut & operator ^= (const Cut &cut);
  Cut & operator <<= (const Cut &cut);
  Cut & operator >>= (const Cut &cut);

  explicit operator std::string() const;
  explicit operator const char *() const;

  bool operator<(const Cut &cut) const;
  friend std::ostream & operator<<(std::ostream &stream, const Cut &cut);

private:
  std::string cut_;

  void Clean();

  using CompType = std::tuple<const std::string &>;
  CompType ComparisonTuple() const{
    return CompType(cut_);
  }
};

Cut operator&(Cut a, Cut b);
Cut operator&&(Cut a, Cut b);
Cut operator|(Cut a, Cut b);
Cut operator||(Cut a, Cut b);
Cut operator+(Cut a, Cut b);
Cut operator-(Cut a, Cut b);
Cut operator*(Cut a, Cut b);
Cut operator/(Cut a, Cut b);
Cut operator%(Cut a, Cut b);
Cut operator^(Cut a, Cut b);
Cut operator<<(Cut a, Cut b);
Cut operator>>(Cut a, Cut b);

#endif
