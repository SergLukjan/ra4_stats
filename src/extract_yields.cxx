#include "extract_yields.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "TFile.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TColor.h"

#include "RooArgList.h"
#include "RooRealVar.h"

#include "utilities.hpp"
#include "styles.hpp"

using namespace std;

int main(int argc, char *argv[]){
  for(int argi = 1; argi < argc; ++argi){
    execute("export blah=$(pwd); cd ~/cmssw/CMSSW_7_1_5/src; eval `scramv1 runtime -sh`; cd $blah; combine -M MaxLikelihoodFit --saveWorkspace "+string(argv[argi]));
    
    styles style("RA4");
    style.setDefaultStyle();
    
    TFile w_file("MaxLikelihoodFitResult.root","read");
    if(!w_file.IsOpen()) continue;
    RooWorkspace *w = static_cast<RooWorkspace*>(w_file.Get("MaxLikelihoodFitResult"));
    if(w == nullptr) continue;
    
    TFile fit_file("mlfit.root","read");
    if(!fit_file.IsOpen()) continue;
    RooFitResult *fit_b = static_cast<RooFitResult*>(fit_file.Get("fit_b"));
    RooFitResult *fit_s = static_cast<RooFitResult*>(fit_file.Get("fit_s"));
    
    if(fit_b != nullptr){
      PrintDebug(*w, *fit_b, ChangeExtension(argv[argi], "_bkg_debug.tex"));
      PrintTable(*w, *fit_b, ChangeExtension(argv[argi], "_bkg_table.tex"));
      MakeYieldPlot(*w, *fit_b, ChangeExtension(argv[argi], "_bkg_plot.pdf"));
      if(!Contains(argv[argi], "nokappa")) MakeCorrectionPlot(*w, *fit_b, ChangeExtension(argv[argi], "_bkg_correction.pdf"));
    }
    if(fit_s != nullptr){
      PrintDebug(*w, *fit_s, ChangeExtension(argv[argi], "_sig_debug.tex"));
      PrintTable(*w, *fit_s, ChangeExtension(argv[argi], "_sig_table.tex"));
      MakeYieldPlot(*w, *fit_s, ChangeExtension(argv[argi], "_sig_plot.pdf"));
      if(!Contains(argv[argi], "nokappa")) MakeCorrectionPlot(*w, *fit_s, ChangeExtension(argv[argi], "_sig_correction.pdf"));
    }
  }
}

string GetSignalName(const RooWorkspace &w){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nsig_BLK_") continue;
    TIter iter2(arg->getVariables()->createIterator());
    int size2 = arg->getVariables()->getSize();
    TObject *obj2;
    int i2 = 0;
    while((obj2 = iter2()) && i2 < size2){
      ++i2;
      RooAbsArg *arg2 = static_cast<RooAbsArg*>(obj2);
      if(arg2 == nullptr) continue;
      string name2 = arg2->GetName();
      auto pos2 = name2.find("_PRC_");
      if(pos2 != string::npos){
        iter2.Reset();
        iter.Reset();
        return name2.substr(pos2+5);
      }
    }
    iter2.Reset();
  }
  iter.Reset();
  return "signal";
}

string TexFriendly(const string &s){
  string out;
  for(size_t i = 0; i < s.size(); ++i){
    if(s.at(i) == '_'){
      out += "\\_";
    }else{
      out += s.at(i);
    }
  }
  return out;
}

void PrintDebug(RooWorkspace &w,
                const RooFitResult &f,
                const string &file_name){
  SetVariables(w, f);
  
  vector<string> var_names = GetVarNames(w);
  vector<string> func_names = GetFuncNames(w);

  ofstream out(file_name);
  out << "\\documentclass{article}\n";
  out << "\\usepackage{amsmath,graphicx,rotating,longtable}\n";
  out << "\\begin{document}\n";
  out << "\\begin{longtable}{rr}\n";
  out << "\\hline\\hline\n";
  out << "Variable & Fit Value\\\\\n";
  out << "\\hline\n";
  out << fixed << setprecision(4);
  for(const auto &var: var_names){
    RooRealVar *varo = w.var(var.c_str());
    if(!varo->isConstant()){
      out << TexFriendly(var) << " & $" << varo->getVal() << "\\pm" << varo->getPropagatedError(f) << "$\\\\\n";
    }else{
      out << TexFriendly(var) << " & $" << varo->getVal() << "$\\\\\n";
    }
  }
  for(const auto &func: func_names){
    RooAbsReal *funco = w.function(func.c_str());
    if(!funco->isConstant()){
      out << TexFriendly(func) << " & $" << funco->getVal() << "\\pm" << funco->getPropagatedError(f) << "$\\\\\n";
    }else{
      out << TexFriendly(func) << " & $" << funco->getVal() << "$\\\\\n";
    }
  }

  out << "\\hline\\hline\n";
  out << "\\end{longtable}\n";
  out << "\\end{document}\n";
  out << endl;
  out.close();
  cout<<"Saved "<<file_name.c_str()<<endl;
}

void PrintTable(RooWorkspace &w,
                const RooFitResult &f,
                const string &file_name){
  SetVariables(w, f);
  
  string sig_name = GetSignalName(w);
  vector<string> prc_names = GetProcessNames(w);
  vector<string> bin_names = GetPlainBinNames(w);

  ofstream out(file_name);
  out << fixed << setprecision(4);
  out << "\\documentclass{article}\n";
  out << "\\usepackage{amsmath,graphicx,rotating}\n";
  out << "\\usepackage[landscape]{geometry}\n";
  out << "\\begin{document}\n";
  out << "\\begin{table}\n";
  out << "\\centering\n";
  out << "\\resizebox{\\textwidth}{!}{\n";
  out << "\\begin{tabular}{r";
  for(size_t i = 0; i < prc_names.size() + 8; ++i) out << "r";
  out << "}\n";
  out << "\\hline\\hline\n";
  out << "Bin & ";
  for(const auto &prc_name: prc_names){
    out << prc_name << " & ";
  }
  out << "Bkgnd. Tot. & Bkgnd. Pred. & Signal & Sig. Pred. & Tot. Pred. & Observed & $\\lambda$\\\\\n";
  out << "\\hline\n";
  for(const auto &bin_name: bin_names){
    out << TexFriendly(bin_name) << " & ";
    for(const auto &prc_name: prc_names){
      out << GetMCYield(w, bin_name, prc_name) << " & ";
    }
    out << "$" << GetMCTotal(w, bin_name) << "\\pm" << GetMCTotalErr(w, f, bin_name) <<  "$ & ";
    out << "$" << GetBkgPred(w, bin_name) << "\\pm" << GetBkgPredErr(w, f, bin_name) <<  "$ & ";
    out << GetMCYield(w, bin_name, sig_name) << " & ";
    out << "$" << GetSigPred(w, bin_name) << "\\pm" << GetSigPredErr(w, f, bin_name) <<  "$ & ";
    out << "$" << GetTotPred(w, bin_name) << "\\pm" << GetTotPredErr(w, f, bin_name) <<  "$ & ";
    out << GetObserved(w, bin_name) << " & ";
    out << "$" << GetLambda(w, bin_name) << "\\pm" << GetLambdaErr(w, f, bin_name) <<  "$\\\\\n";
  }
  out << "\\hline\\hline\n";
  out << "\\end{tabular}\n";
  out << "}\n";
  out << "\\end{table}\n";
  out << "\\end{document}\n";
  out << endl;
  out.close();
  cout<<"Saved "<<file_name.c_str()<<endl;
}

double GetMCYield(const RooWorkspace &w,
                  const string &bin_name,
                  const string &prc_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,8) != "ymc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(!(Contains(name, "_PRC_"+prc_name))) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetMCTotal(const RooWorkspace &w,
                  const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,8) != "ymc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetMCTotalErr(const RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,8) != "ymc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getPropagatedError(f);
  }
  iter.Reset();
  return -1.;
}

double GetBkgPred(const RooWorkspace &w,
                  const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nbkg_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetBkgPredErr(const RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nbkg_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getPropagatedError(f);
  }
  iter.Reset();
  return -1.;
}

double GetSigPred(const RooWorkspace &w,
                  const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nsig_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetSigPredErr(const RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nsig_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getPropagatedError(f);
  }
  iter.Reset();
  return -1.;
}

double GetTotPred(const RooWorkspace &w,
                  const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nexp_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetTotPredErr(const RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nexp_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getPropagatedError(f);
  }
  iter.Reset();
  return -1.;
}

double GetObserved(const RooWorkspace &w,
                   const string &bin_name){
  TIter iter(w.allVars().createIterator());
  int size = w.allVars().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nobs_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetLambda(const RooWorkspace &w,
                 const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,12) != "kappamc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  iter.Reset();
  return -1.;
}

double GetLambdaErr(const RooWorkspace &w,
                    const RooFitResult &f,
                    const string &bin_name){
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,12) != "kappamc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getPropagatedError(f);
  }
  iter.Reset();
  return -1.;
}

RooRealVar * SetVariables(RooWorkspace &w,
                          const RooFitResult &f){
  bool set_r = false;
  RooArgList pars = f.floatParsFinal();
  for(int ipar = 0; ipar < pars.getSize(); ++ipar){
    RooRealVar *fit_var = static_cast<RooRealVar*>(pars.at(ipar));
    if(fit_var == nullptr) continue;
    RooRealVar *w_var = w.var(fit_var->GetName());
    if(w_var == nullptr) continue;
    w_var->removeRange();
    w_var->setVal(fit_var->getVal());
    w_var->setError(fit_var->getError());
    if(fit_var->GetName() == string("r")) set_r = true;
  }
  RooRealVar *r_var = static_cast<RooRealVar*>(w.var("r"));
  if(r_var != nullptr){
    if(!set_r){
      r_var->setVal(0);
      r_var->setConstant(true);
    }else{
      r_var->setConstant(false);
    }
  }
  return r_var;
}

void MakeYieldPlot(RooWorkspace &w,
                   const RooFitResult &f,
                   const string &file_name){
  RooRealVar *r_var = SetVariables(w, f);

  vector<string> bin_names = GetBinNames(w);
  vector<string> prc_names = GetProcessNames(w);

  vector<vector<double> > component_yields = GetComponentYields(w, bin_names, prc_names);

  vector<TH1D> histos = MakeBackgroundHistos(component_yields, bin_names, prc_names);
  TH1D signal = MakeTotalHisto(w, f, bin_names);
  TGraphErrors band = MakeErrorBand(signal);
  TH1D obs = MakeObserved(w, bin_names);

  SetBounds(obs, signal, histos);

  TCanvas c;
  c.cd();
  TPad bot_pad("bot_pad", "bot_pad", 0., 0., 1., 0.4);
  bot_pad.SetFillColor(0); bot_pad.SetFillStyle(4000);
  bot_pad.SetMargin(0.1, 0., 0.5, 0.);
  bot_pad.Draw();
  c.cd();
  TPad mid_pad("mid_pad", "mid_pad", 0., 0.4, 1., 0.85);
  mid_pad.SetFillColor(0); mid_pad.SetFillStyle(4000);
  mid_pad.SetMargin(0.1, 0., 0.0, 0.);
  mid_pad.Draw();
  c.cd();
  TPad top_pad("top_pad", "top_pad", 0., 0.85, 1., 1.0);
  top_pad.SetFillColor(0); top_pad.SetFillStyle(4000);
  top_pad.SetMargin(0.1, 0., 0.0, 0.);
  top_pad.Draw();

  double font_size = 0.1;
  double offset = 0.5;

  mid_pad.cd();
  mid_pad.SetLogy();
  signal.SetTitleSize(font_size, "Y");
  signal.SetTitleOffset(offset, "Y");
  signal.SetFillColor(2);
  signal.SetFillStyle(1001);
  signal.SetLineColor(2);
  signal.SetLineStyle(1);
  signal.SetLineWidth(0);
  signal.Draw("hist");
  for(auto h = histos.rbegin(); h!= histos.rend(); ++h){
    h->Draw("same");
  }
  band.Draw("02 same");
  obs.Draw("e1p same");
  signal.Draw("same axis");

  top_pad.cd();
  TLegend l(0.1, 0., 1., 1.);
  l.SetNColumns(3);
  l.SetFillColor(0); l.SetFillStyle(4000);
  l.SetBorderSize(0);
  l.AddEntry(&obs, "Observed", "lep");
  l.AddEntry(&signal, "Signal", "f");
  ostringstream oss;
  oss << setprecision(4) << fixed;
  oss << "r=";
  if(r_var == nullptr){
    oss << "???";
  }else if(r_var->isConstant()){
    oss << r_var->getVal() << " (fixed)";
  }else{
    oss << r_var->getVal() << "#pm" << r_var->getPropagatedError(f);
    cout<<"Signal strength: "<<r_var->getVal() << "#pm" << r_var->getPropagatedError(f)<<endl;
  }
  oss << flush;
  l.AddEntry(&obs, oss.str().c_str(), "");
  for(auto h = histos.crbegin(); h != histos.crend(); ++h){
    l.AddEntry(&(*h), h->GetName(), "f");
  }
  l.Draw("same");

  bot_pad.cd();
  TGraphErrors obs_rat = MakeRatio(obs, signal);
  TGraphErrors pred_rat = MakeRatio(signal, signal);
  TH1D dumb = obs;
  dumb.SetLineColor(0);
  dumb.SetLineWidth(0);
  dumb.SetFillColor(0);
  dumb.SetFillStyle(4000);
  dumb.SetMinimum(0.);
  dumb.SetMaximum(2.);
  dumb.SetTitle(";;Obs/Pred ");
  dumb.GetXaxis()->LabelsOption("V");
  dumb.SetTitleSize(font_size, "Y");
  dumb.SetTitleOffset(offset, "Y");
  dumb.Draw();
  pred_rat.SetFillColor(kGray);
  pred_rat.SetFillStyle(3001);
  pred_rat.Draw("02 same");
  obs_rat.Draw("0 same");
  c.Print(file_name.c_str());
}

vector<string> GetVarNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allVars().createIterator());
  int size = w.allVars().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    if(obj == nullptr) continue;
    string name = obj->GetName();
    Append(names, name);
  }
  iter.Reset();
  sort(names.begin(), names.end());
  return names;
}

vector<string> GetFuncNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    if(obj == nullptr) continue;
    string name = obj->GetName();
    Append(names, name);
  }
  iter.Reset();
  sort(names.begin(), names.end());
  return names;
}

vector<string> GetBinNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nexp_BLK_") continue;
    string bin_name = name.substr(5);
    Append(names, bin_name);
  }
  iter.Reset();
  reverse(names.begin(), names.end());
  return names;
}

vector<string> GetPlainBinNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nexp_BLK_") continue;
    auto bpos = name.find("_BIN_");
    auto ppos = name.find("_PRC_");
    if(bpos == string::npos) continue;
    string bin_name = name.substr(bpos+5, ppos-bpos-5);
    Append(names, bin_name);
  }
  iter.Reset();
  reverse(names.begin(), names.end());
  return names;
}

vector<string> GetProcessNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allFunctions().createIterator());
  int size = w.allFunctions().getSize();
  TObject *obj;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    RooAbsArg *arg = static_cast<RooAbsArg*>(obj);
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "frac_BIN_") continue;
    auto prc_pos = name.find("_PRC_");
    if(prc_pos == string::npos) continue;
    string bin_name = name.substr(prc_pos+5);
    if(find(names.cbegin(), names.cend(), bin_name) != names.cend()) continue;
    Append(names, bin_name);
  }
  iter.Reset();
  return names;
}

vector<vector<double> > GetComponentYields(const RooWorkspace &w,
                                           const vector<string> &bin_names,
                                           const vector<string> &prc_names){
  vector<vector<double> > yields(bin_names.size());
  for(auto &bin: yields){
    bin = vector<double>(prc_names.size(), 0.);
  }
  for(size_t ibin = 0; ibin < yields.size(); ++ibin){
    const string &bin_name = bin_names.at(ibin);
    auto blk_pos = bin_name.find("_BIN_");
    if(blk_pos == string::npos) continue;
    string plain_name = bin_name.substr(blk_pos+5);
    for(size_t iprc = 0; iprc < yields.at(ibin).size(); ++iprc){
      const string &prc_name = prc_names.at(iprc);
      RooRealVar *nbkg_arg = static_cast<RooRealVar*>(w.function(("nbkg_"+bin_name).c_str()));
      if(nbkg_arg == nullptr) continue;
      RooRealVar *frac_arg = static_cast<RooRealVar*>(w.function(("frac_BIN_"+plain_name+"_PRC_"+prc_name).c_str()));
      if(frac_arg == nullptr) continue;
      yields.at(ibin).at(iprc) = nbkg_arg->getVal() * frac_arg->getVal();
    }
  }
  return yields;
}

vector<TH1D> MakeBackgroundHistos(const vector<vector<double> > &yields,
                                  const vector<string> &bin_names,
                                  const vector<string> &prc_names){
  if(yields.size() == 0){
    return vector<TH1D>();
  }
  vector<TH1D> histos(yields.at(0).size(),
                      TH1D("", ";;Yield ", yields.size(), 0.5, yields.size()+0.5));
  for(size_t ibin = 0; ibin < yields.size(); ++ibin){
    for(size_t iprc = 0; iprc < yields.at(ibin).size(); ++iprc){
      histos.at(iprc).SetBinContent(ibin+1, yields.at(ibin).at(iprc));
    }
  }

  for(size_t iprc = 0; iprc < histos.size(); ++iprc){
    TH1D &h = histos.at(iprc);
    h.SetName(prc_names.at(iprc).c_str());
    h.SetFillColor(iprc+3);
    h.SetLineColor(iprc+3);
    h.SetLineWidth(0);
    for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
      const string &name = bin_names.at(ibin);
      auto pos = name.find("_BIN_");
      if(pos == string::npos) continue;
      string label = name.substr(pos+5);
      h.GetXaxis()->SetBinLabel(ibin+1, label.c_str());
    }
  }
  sort(histos.begin(), histos.end(),
       [](const TH1D &a, const TH1D &b) -> bool{return a.Integral() < b.Integral();});

  for(size_t iprc = histos.size()-1; iprc < histos.size(); --iprc){
    TH1D &h = histos.at(iprc);
    for(size_t isum = iprc-1; isum < histos.size(); --isum){
      h.Add(&histos.at(isum));
    }
  }

  return histos;
}

TH1D MakeTotalHisto(const RooWorkspace &w,
                    const RooFitResult &f,
                    const vector<string> &bin_names){
  TH1D h("signal", ";;Yield ", bin_names.size(), 0.5, bin_names.size()+0.5);
  h.SetFillColor(2);
  h.SetLineColor(2);
  h.SetLineWidth(0);

  for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
    const string &name = bin_names.at(ibin);
    auto pos = name.find("_BIN_");
    if(pos == string::npos) continue;
    string label = name.substr(pos+5);
    h.GetXaxis()->SetBinLabel(ibin+1, label.c_str());
    RooRealVar *var = static_cast<RooRealVar*>(w.function(("nexp_"+name).c_str()));
    if(var == nullptr) continue;
    h.SetBinContent(ibin+1, var->getVal());
    h.SetBinError(ibin+1, var->getPropagatedError(f));
  }

  return h;
}

TH1D MakeObserved(const RooWorkspace &w,
                  const vector<string> &bin_names){
  TH1D h("observed", ";;Yield ", bin_names.size(), 0.5, bin_names.size()+0.5);
  h.SetLineColor(1);
  h.SetFillColor(0);
  h.SetFillStyle(4000);

  for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
    const string &name = bin_names.at(ibin);
    auto pos = name.find("_BIN_");
    if(pos == string::npos) continue;
    string label = name.substr(pos+5);
    h.GetXaxis()->SetBinLabel(ibin+1, label.c_str());
    RooRealVar *var = static_cast<RooRealVar*>(w.var(("nobs_"+name).c_str()));
    if(var == nullptr) continue;
    h.SetBinContent(ibin+1, var->getVal());
  }

  return h;
}

void SetBounds(TH1D &a,
               TH1D &b,
               std::vector<TH1D> &cs){
  double factor = 0.02;

  double hmax = GetMaximum(a, b, cs);
  double hmin = GetMinimum(a, b, cs);
  double lmax = log(hmax);
  double lmin = log(hmin);
  double log_diff = lmax-lmin;
  lmin -= factor*log_diff;
  lmax += factor*log_diff;
  hmin = exp(lmin);
  hmax = exp(lmax);
  a.SetMinimum(hmin);
  a.SetMaximum(hmax);
  b.SetMinimum(hmin);
  b.SetMaximum(hmax);
  for(auto &c: cs){
    c.SetMinimum(hmin);
    c.SetMaximum(hmax);
  }
}

double GetMaximum(const TH1D &a,
                  const TH1D &b,
                  const vector<TH1D> &cs){
  double the_max = GetMaximum(a);
  double this_max = GetMaximum(b);
  if(this_max > the_max) the_max = this_max;
  for(const auto &c: cs){
    this_max = GetMaximum(c);
    if(this_max > the_max) the_max = this_max;
  }
  return the_max;
}

double GetMinimum(const TH1D &a,
                  const TH1D &b,
                  const vector<TH1D> &cs){
  double the_min = GetMinimum(a, 0.1);
  double this_min = GetMinimum(b, 0.1);
  if(this_min < the_min) the_min = this_min;
  for(const auto &c: cs){
    this_min = GetMinimum(c, 0.1);
    if(this_min < the_min) the_min = this_min;
  }
  return the_min;
}

double GetMaximum(const TH1D &h, double y){
  double the_max = -numeric_limits<double>::max();
  for(int bin = 1; bin <= h.GetNbinsX(); ++bin){
    double content = h.GetBinContent(bin);
    if(content > the_max){
      if(content < y){
        the_max = content;
      }else{
        the_max = y;
      }
    }
  }
  return the_max;
}

double GetMinimum(const TH1D &h, double y){
  double the_min = numeric_limits<double>::max();
  for(int bin = 1; bin <= h.GetNbinsX(); ++bin){
    double content = h.GetBinContent(bin);
    if(content < the_min){
      if(content > y){
        the_min = content;
      }else{
        the_min = y;
      }
    }
  }
  return the_min;
}

TGraphErrors MakeErrorBand(const TH1D &h){
  TGraphErrors g(h.GetNbinsX());
  for(int bin = 1; bin <= h.GetNbinsX(); ++bin){
    g.SetPoint(bin, h.GetBinCenter(bin), h.GetBinContent(bin));
    g.SetPointError(bin, 0.5, h.GetBinError(bin));
  }
  g.SetFillColor(kGray);
  g.SetFillStyle(3001);
  return g;
}

TGraphErrors MakeRatio(const TH1D &num, const TH1D &den){
  TGraphErrors g(num.GetNbinsX());
  for(int bin = 1; bin <= num.GetNbinsX(); ++bin){
    double x = num.GetBinCenter(bin);
    double nc = num.GetBinContent(bin);
    double dc = den.GetBinContent(bin);
    double ne = num.GetBinError(bin);
    double big_num = 0.5*numeric_limits<float>::max();
    if(dc != 0.){
      g.SetPoint(bin, x, nc/dc);
      g.SetPointError(bin, 0.5, ne/dc);
    }else if(nc == 0.){
      g.SetPoint(bin, x, 1.);
      g.SetPointError(bin, 0.5, big_num);
    }else{
      g.SetPoint(bin, x, nc > 0. ? big_num : -big_num);
      g.SetPointError(bin, 0.5, big_num);
    }
  }
  return g;
}

void MakeCorrectionPlot(RooWorkspace &w,
			const RooFitResult &f,
			const string &file_name){
  SetVariables(w, f);

  vector<string> bin_names = GetBinNames(w);
  vector<string> prc_names = GetProcessNames(w);

  TCanvas c;
  c.cd();

  TH1D h("", ";;#lambda", bin_names.size(), 0.5, bin_names.size()+0.5);
  for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
    string bin = bin_names.at(ibin);
    auto pos = bin.find("_BIN_");
    string plain_bin = bin.substr(pos+5);
    h.GetXaxis()->SetBinLabel(ibin+1, plain_bin.c_str());
    h.SetBinContent(ibin+1, static_cast<RooRealVar*>(w.function(("kappamc_"+bin).c_str()))->getVal());
    h.SetBinError(ibin+1, static_cast<RooRealVar*>(w.function(("kappamc_"+bin).c_str()))->getPropagatedError(f));
  }
  h.GetXaxis()->LabelsOption("V");
  h.Draw();
  c.SetMargin(0.1, 0.05, 1./3., 0.05);
  c.Print(file_name.c_str());
}
