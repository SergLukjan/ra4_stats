#include "yield_manager.hpp"

#include <iostream>
#include <sstream>
#include <array>

#include "bin.hpp"
#include "process.hpp"
#include "cut.hpp"
#include "utilities.hpp"

using namespace std;

map<YieldKey, GammaParams> YieldManager::yields_ = map<YieldKey, GammaParams>();
const double YieldManager::store_lumi_ = 4.;

YieldManager::YieldManager(double lumi):
  local_lumi_(lumi),
  verbose_(false){
}

GammaParams YieldManager::GetYield(const YieldKey &key) const{
  if(!HaveYield(key)) ComputeYield(key);

  double factor = local_lumi_/store_lumi_;
  if(GetProcess(key).IsData()) factor = 1.;

  return factor*yields_.at(key);
}

GammaParams YieldManager::GetYield(const Bin &bin,
                                   const Process &process,
                                   const Cut &cut) const{
  return GetYield(YieldKey(bin, process, cut));
}

const double & YieldManager::Luminosity() const{
  return local_lumi_;
}

double & YieldManager::Luminosity(){
  return local_lumi_;
}

bool YieldManager::HaveYield(const YieldKey &key) const{
  return yields_.find(key) != yields_.end();
}

void YieldManager::ComputeYield(const YieldKey &key) const{
  const Bin &bin = GetBin(key);
  const Process &process = GetProcess(key);
  const Cut &cut = GetCut(key);

  GammaParams gps;

  if(HaveYield(key)){
    if(verbose_){
      cout << "Using known yield for " << key << endl;
    }
    gps = GetYield(key);
  }else if(process.GetEntries() == 0){
    if(verbose_){
      cout << "No entries found for " << key << endl;
    }
    gps.SetNEffectiveAndWeight(0., 0.);
  }else{
    if(verbose_){
      cout << "Computing yield for " << key << endl;
    }
    ostringstream oss;
    oss << local_lumi_ << flush;
    Cut lumi_weight = process.IsData() ? Cut() : 
      (Contains(process.Name(), "sig")?Cut(oss.str()+"*weight*eff_trig/w_btag"):Cut(oss.str()+"*weight*eff_trig/w_btag"));

    array<Cut, 5> cuts;
    cuts.at(0) = lumi_weight*(cut && bin.Cut() && process.Cut());
    cuts.at(1) = lumi_weight*(cut && process.Cut());
    cuts.at(2) = lumi_weight*(process.Cut());
    cuts.at(3) = lumi_weight;
    cuts.at(4) = Cut();

    for(size_t icut = 0; icut < cuts.size() && gps.Weight()<=0.; ++icut){
      if(icut > 0 && !process.CountZeros()){
        gps.SetNEffectiveAndWeight(0., 0.);
        break;
      }
      Cut &this_cut = cuts.at(icut);
      if(verbose_){
        cout << "Trying cut " << this_cut << endl;
      }
      GammaParams temp_gps = process.GetYield(this_cut);
      //// Averaging signal yields cutting on met and met_tru, as prescripted by SUSY group
      //// https://twiki.cern.ch/twiki/bin/viewauth/CMS/SUSRecommendationsICHEP16#Special_treatment_of_MET_uncerta
      if(Contains(process.Name(), "sig")){
	string mettru_s = this_cut.GetCut(); 
	ReplaceAll(mettru_s, "met_calo", "XXXYYYZZZ_calo");
	ReplaceAll(mettru_s, "met", "met_tru");
	ReplaceAll(mettru_s, "XXXYYYZZZ_calo", "met_calo");
	Cut mettru_cut(mettru_s); 
	GammaParams mettru_gps = process.GetYield(mettru_cut);
	if(verbose_) cout<<"Yields: met "<<temp_gps.Yield()<<", met_tru "<<mettru_gps.Yield();
	temp_gps += mettru_gps;
	temp_gps *= 0.5;
	if(verbose_) cout<<", average "<<temp_gps.Yield()<<" for bin "<<bin.Name()<<endl;
      } // If it is signal
      if(icut == 0) gps = temp_gps;
      else gps.SetNEffectiveAndWeight(0., temp_gps.Weight());
    }
  }

  if(verbose_){
    cout << "Found yield=" << gps << '\n' << endl;
  }
  double factor = store_lumi_/local_lumi_;
  if(process.IsData()) factor = 1.;
  yields_[key] = factor*gps;
}
