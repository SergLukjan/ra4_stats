#include "extract_yields.hpp"

#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>

#include <getopt.h>

#include "TFile.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TColor.h"

#include "RooArgList.h"
#include "RooArgSet.h"
#include "RooRealVar.h"
#include "RooAbsData.h"

#include "utilities.hpp"
#include "styles.hpp"

using namespace std;

namespace{
  string file_wspace("empty");
  string name_wspace("w");
  bool table_clean(false);
  int toy_num(-1);
  bool r4_only(false);
  bool show_exp_sig(false);
}

int main(int argc, char *argv[]){
  GetOptionsExtract(argc, argv);

  if(file_wspace == "empty") ERROR("You need to specify the file containing the workspace with option -f");

  string workdir = MakeDir("extract_yields_");

  ostringstream command;
  command << "export blah=$(pwd); "
          << "cd ~/cmssw/CMSSW_7_1_5/src; "
          << "eval `scramv1 runtime -sh`; "
          << "cd $blah; "
          << "cp " << file_wspace << ' ' << workdir << "; "
          << "cd " << workdir << "; "
          << "combine -M MaxLikelihoodFit --saveWorkspace --saveWithUncertainties --minos=all --minimizerToleranceForMinos=0.000001 --minimizerStrategyForMinos=2 -w " << name_wspace << " --dataset data_obs";
  if(toy_num >= 0) command << "_" << toy_num;
  command << " " << StripPath(file_wspace) << "; "
          << "cd $blah; "
          << flush;
  cout << "Executing " << command.str() << endl;
  execute(command.str());

  styles style("RA4");
  style.setDefaultStyle();

  string w_name("higgsCombineTest.MaxLikelihoodFit.mH120.root");
  w_name = workdir+'/'+w_name;
  TFile w_file(w_name.c_str(),"read");
  if(!w_file.IsOpen()) ERROR("File "+w_name+" not produced");
  RooWorkspace *w = static_cast<RooWorkspace*>(w_file.Get(name_wspace.c_str()));
  if(w == nullptr) ERROR("Workspace "+name_wspace+" not found");

  string fit_name = "mlfit.root";
  string full_fit_name = workdir+'/'+fit_name;
  TFile fit_file(full_fit_name.c_str(),"read");
  if(!fit_file.IsOpen()) ERROR("Could not open "+full_fit_name);
  RooFitResult *fit_b = static_cast<RooFitResult*>(fit_file.Get("fit_b"));
  RooFitResult *fit_s = static_cast<RooFitResult*>(fit_file.Get("fit_s"));
  string toy_ext = "";
  if(toy_num >= 0) toy_ext = "_toy_" + to_string(toy_num);
  file_wspace = ChangeExtension(file_wspace, toy_ext+"_"+name_wspace+".root");
  if(fit_b != nullptr){
    PrintDebug(*w, *fit_b, ChangeExtension(file_wspace, "_bkg_debug.tex"));
    PrintTable(*w, *fit_b, ChangeExtension(file_wspace, "_bkg_table.tex"));
    MakeYieldPlot(*w, *fit_b, ChangeExtension(file_wspace, "_bkg_plot.pdf"), false);
    MakeYieldPlot(*w, *fit_b, ChangeExtension(file_wspace, "_bkg_plot_linear.pdf"), true);
    if(!Contains(file_wspace, "nokappa")) MakeCorrectionPlot(*w, *fit_b, ChangeExtension(file_wspace, "_bkg_correction.pdf"));
  }
  if(fit_s != nullptr){
    PrintDebug(*w, *fit_s, ChangeExtension(file_wspace, "_sig_debug.tex"));
    PrintTable(*w, *fit_s, ChangeExtension(file_wspace, "_sig_table.tex"));
    MakeYieldPlot(*w, *fit_s, ChangeExtension(file_wspace, "_sig_plot.pdf"), false);
    MakeYieldPlot(*w, *fit_s, ChangeExtension(file_wspace, "_sig_plot_linear.pdf"), true);
    if(!Contains(file_wspace, "nokappa")) MakeCorrectionPlot(*w, *fit_s, ChangeExtension(file_wspace, "_sig_correction.pdf"));
  }

  w_file.Close();
  fit_file.Close();

  execute("rm -rf "+workdir);
}

string GetSignalName(const RooWorkspace &w){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nsig_BLK_") continue;
    TIter iter2(arg->getVariables()->createIterator());
    int size2 = arg->getVariables()->getSize();
    RooAbsArg *arg2 = nullptr;
    int i2 = 0;
    while((arg2 = static_cast<RooAbsArg*>(iter2())) && i2 < size2){
      ++i2;
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
  out << "\\thispagestyle{empty}\n";
  out << "\\begin{document}\n";
  out << "\\begin{longtable}{rr}\n";
  out << "\\hline\\hline\n";
  out << "Variable & Fit Value\\\\\n";
  out << "\\hline\n";
  out << fixed << setprecision(2);
  for(const auto &var: var_names){
    RooRealVar *varo = w.var(var.c_str());
    if(varo == nullptr) continue;
    if(!varo->isConstant()){
      out << TexFriendly(var) << " & $" << varo->getVal() << "\\pm" << GetError(*varo, f) << "$\\\\\n";
    }else{
      out << TexFriendly(var) << " & $" << varo->getVal() << "$\\\\\n";
    }
  }
  for(const auto &func: func_names){
    RooAbsReal *funco = w.function(func.c_str());
    if(funco == nullptr) continue;
    if(!funco->isConstant()){
      out << TexFriendly(func) << " & $" << funco->getVal() << "\\pm" << GetError(*funco, f) << "$\\\\\n";
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

  bool dosig(Contains(file_name, "sig_table")), blind_all(Contains(file_name, "r4blinded"));
  bool blind_2b(Contains(file_name, "1bunblinded"));
  size_t digits(2), ncols(10);
  if(!dosig) ncols = 8;
  if(table_clean) {
    ncols--;
    digits = 1;
  }

  ofstream out(file_name);
  out << fixed << setprecision(digits);
  out << "\\documentclass{article}\n";
  out << "\\usepackage{amsmath,graphicx,rotating,longtable}\n";
  out << "\\thispagestyle{empty}\n";
  out << "\\begin{document}\n";
  out << "\\begin{longtable}{rr}\n";
  out << "\\centering\n";
  out << "\\resizebox{\\textwidth}{!}{\n";
  out << "\\begin{tabular}{l ";
  for(size_t i = 0; i < ncols-1; ++i) out << "r";
  out << "}\n";
  out << "\\hline\\hline\n";
  out << "Bin & ";
  for(const auto &prc_name: prc_names){
    out << prc_name << " & ";
  }
  out << "MC Bkg. "<<(dosig?"& Bkgnd. Pred. ":"")<<"& Signal "<<(dosig?"& Sig. Pred. ":"")
      <<"& Tot. Pred. & Obs.";
  if(!table_clean) out << " & $\\lambda$";
  out<<"\\\\\n";

  // out << "\\hline\n";
  for(const auto &bin_name: bin_names){
    if(Contains(bin_name, "r1")) {
      out << "\\hline\\hline"<<endl;
      if(Contains(bin_name, "lowmet")) out<<"\\multicolumn{"<<ncols<<"}{c}{$200<\\text{MET}\\leq 350$} \\\\ \\hline"<<endl;
      if(Contains(bin_name, "medmet")) out<<"\\multicolumn{"<<ncols<<"}{c}{$350<\\text{MET}\\leq 500$} \\\\ \\hline"<<endl;
      if(Contains(bin_name, "highmet")) out<<"\\multicolumn{"<<ncols<<"}{c}{$\\text{MET}>500$} \\\\ \\hline"<<endl;
    }
    string bin_tex(TexFriendly(bin_name));
    ReplaceAll(bin_tex, "lowmet\\_","");
    ReplaceAll(bin_tex, "medmet\\_","");
    ReplaceAll(bin_tex, "highmet\\_","");
    ReplaceAll(bin_tex, "lownj\\_","$n_j\\leq8$, ");
    ReplaceAll(bin_tex, "highnj\\_","$n_j\\geq9$, ");
    ReplaceAll(bin_tex, "allnb","all $n_j,n_b$");
    ReplaceAll(bin_tex, "1b","$n_b=1$");
    ReplaceAll(bin_tex, "2b","$n_b=2$");
    ReplaceAll(bin_tex, "3b","$n_b\\geq3$");
    for(int ind(1); ind<=4; ind++){
      ReplaceAll(bin_tex, "r"+to_string(ind)+"\\_","R"+to_string(ind)+": ");
      ReplaceAll(bin_tex, "r"+to_string(ind)+"c\\_","R"+to_string(ind)+": ");
      ReplaceAll(bin_tex, "d"+to_string(ind)+"\\_","D"+to_string(ind)+": ");
    }
    out << bin_tex << " & ";
    for(const auto &prc_name: prc_names){
      out << GetMCYield(w, bin_name, prc_name) << " & ";
    }
    out << "$" << GetMCTotal(w, bin_name);
    if(!table_clean) out << "\\pm" << GetMCTotalErr(w, f, bin_name);
    out <<  "$ & ";

    if(dosig) out << "$" << GetBkgPred(w, bin_name) << "\\pm" << GetBkgPredErr(w, f, bin_name) <<  "$ & ";
    out << GetMCYield(w, bin_name, sig_name) << " & ";
    if(dosig) out << "$" << GetSigPred(w, bin_name) << "\\pm" << GetSigPredErr(w, f, bin_name) <<  "$ & ";
    out << "$" << GetTotPred(w, bin_name) << "\\pm" << GetTotPredErr(w, f, bin_name) <<  "$ & ";
    if(Contains(bin_name,"4") && (blind_all || (!Contains(bin_name,"1b") && blind_2b))) out << "-- & ";
    else out << setprecision(0) << GetObserved(w, bin_name);
    out << setprecision(digits);
    if(!table_clean) out << "& $" << GetLambda(w, bin_name) << "\\pm" << GetLambdaErr(w, f, bin_name) <<  "$";
    out << "\\\\\n";
    if(Contains(bin_name, "r3") || Contains(bin_name, "d3")) out << "\\hline"<<endl;
  }
  out << "\\hline\\hline\n";
  out << "\\end{tabular}\n";
  out << "}\n";
  out << "\\end{longtable}\n";
  out << "\\end{document}\n";
  out << endl;
  out.close();
  cout<<"Saved "<<file_name.c_str()<<endl;
}

double GetMCYield(const RooWorkspace &w,
                  const string &bin_name,
                  const string &prc_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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

double GetMCTotalErr(RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,8) != "ymc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return GetError(*static_cast<RooAbsReal*>(arg), f);
  }
  iter.Reset();
  return -1.;
}

double GetBkgPred(const RooWorkspace &w,
                  const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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

double GetBkgPredErr(RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nbkg_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return GetError(*static_cast<RooAbsReal*>(arg), f);
  }
  iter.Reset();
  return -1.;
}

double GetSigPred(const RooWorkspace &w,
                  const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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

double GetSigPredErr(RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nsig_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return GetError(*static_cast<RooAbsReal*>(arg), f);
  }
  iter.Reset();
  return -1.;
}

double GetTotPred(const RooWorkspace &w,
                  const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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

double GetTotPredErr(RooWorkspace &w,
                     const RooFitResult &f,
                     const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nexp_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return GetError(*static_cast<RooAbsReal*>(arg), f);
  }
  iter.Reset();
  return -1.;
}

double GetObserved(const RooWorkspace &w,
                   const string &bin_name){
  RooAbsArg *arg = nullptr;
  const RooArgSet &vars = w.allVars();
  TIterator *iter_ptr = vars.createIterator();
  while(iter_ptr != nullptr && *(*iter_ptr) != nullptr){
    arg = static_cast<RooAbsArg*>((*iter_ptr)());
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,9) != "nobs_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return static_cast<RooRealVar*>(arg)->getVal();
  }
  return -1.;
}

double GetLambda(const RooWorkspace &w,
                 const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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

double GetLambdaErr(RooWorkspace &w,
                    const RooFitResult &f,
                    const string &bin_name){
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
    if(arg == nullptr) continue;
    string name = arg->GetName();
    if(name.substr(0,12) != "kappamc_BLK_") continue;
    if(!(Contains(name, "_BIN_"+bin_name))) continue;
    if(Contains(name, "_PRC_")) continue;
    return GetError(*static_cast<RooAbsReal*>(arg), f);
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
  vector<string> var_names = GetVarNames(w);
  vector<string> func_names = GetFuncNames(w);
  for(const auto &var: var_names){
    RooRealVar *varo = w.var(var.c_str());
    if(varo == nullptr) continue;
    if(!varo->isConstant()){
      varo->removeRange();
    }
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
                   const string &file_name,
		   bool linear){
  RooRealVar *r_var = SetVariables(w, f);

  vector<string> bin_names = GetBinNames(w, r4_only);
  vector<string> prc_names = GetProcessNames(w);

  vector<vector<double> > component_yields = GetComponentYields(w, bin_names, prc_names);

  vector<TH1D> histos = MakeBackgroundHistos(component_yields, bin_names, prc_names);
  TH1D signal = MakeTotalHisto(w, f, bin_names);
  TH1D exp_signal = MakeExpSignal(w, bin_names) + signal;
  exp_signal.SetLineColor(kRed+1);
  exp_signal.SetFillColor(0);
  exp_signal.SetFillStyle(0);
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
  if(!linear) mid_pad.SetLogy();
  mid_pad.Draw();
  c.cd();
  TPad top_pad("top_pad", "top_pad", 0., 0.85, 1., 1.0);
  top_pad.SetFillColor(0); top_pad.SetFillStyle(4000);
  top_pad.SetMargin(0.1, 0., 0.0, 0.);
  top_pad.Draw();

  double font_size = 0.1;
  double offset = 0.5;

  mid_pad.cd();
  signal.SetTitleSize(font_size, "Y");
  signal.SetTitleOffset(offset, "Y");
  signal.SetFillColor(kRed+1);
  signal.SetFillStyle(1001);
  signal.SetLineColor(2);
  signal.SetLineStyle(1);
  signal.SetLineWidth(0);
  if(linear) signal.SetMinimum(0.);
  else signal.SetMinimum(0.03);
  signal.Draw("hist");
  for(auto h = histos.rbegin(); h!= histos.rend(); ++h){
    if(linear) h->SetMinimum(0.);
    else h->SetMinimum(0.03);
    h->Draw("same");
  }

  double marker_size(1.4);
  obs.SetMarkerStyle(20); obs.SetMarkerSize(marker_size);
  band.Draw("02 same");
  if(linear) obs.SetMinimum(0.);
  else obs.SetMinimum(0.03);
  obs.Draw("e0 x0 p0 same");
  if(linear) signal.SetMinimum(0.);
  else signal.SetMinimum(0.03);
  signal.Draw("same axis");
  if(linear) exp_signal.SetMinimum(0.);
  else exp_signal.SetMinimum(0.03);
  if(Contains(file_name, "bkg")) exp_signal.Draw("hist same");

  top_pad.cd();
  TLegend l(0.1, 0., 1., 1.);
  l.SetNColumns(3);
  l.SetFillColor(0); l.SetFillStyle(4000);
  l.SetBorderSize(0);
  l.AddEntry(&obs, "Observed", "lep");
  if(r_var->isConstant()){
    l.AddEntry(&exp_signal, "Expected Signal", "l");
  }else{
    l.AddEntry(&signal, "Fitted Signal", "f");
  }
  ostringstream oss;
  oss << setprecision(2) << fixed;
  oss << "r=";
  if(r_var == nullptr){
    oss << "???";
  }else if(r_var->isConstant()){
    oss << r_var->getVal() << " (fixed)";
  }else{
    oss << r_var->getVal() << "#pm" << GetError(*r_var, f);
  }
  oss << flush;
  l.AddEntry(&obs, oss.str().c_str(), "");
  for(auto h = histos.crbegin(); h != histos.crend(); ++h){
    l.AddEntry(&(*h), h->GetName(), "f");
  }
  l.Draw("same");

  bot_pad.cd();
  TLine line; line.SetLineStyle(2);
  TGraphErrors obs_rat = MakeRatio(obs, signal);
  TGraphErrors pred_rat = MakeRatio(signal, signal);
  TH1D dumb = obs;
  obs_rat.SetMarkerStyle(20); obs_rat.SetMarkerSize(marker_size);
  obs_rat.SetMarkerColor(1);
  dumb.SetLineColor(0);
  dumb.SetLineWidth(0);
  dumb.SetFillColor(0);
  dumb.SetFillStyle(4000);
  dumb.SetMinimum(0.);
  dumb.SetMaximum(2.8);
  dumb.SetTitle(";;Obs/Pred ");
  dumb.GetXaxis()->LabelsOption("V");
  dumb.SetTitleSize(font_size, "Y");
  dumb.SetTitleOffset(offset, "Y");
  dumb.Draw();
  pred_rat.SetFillColor(kGray);
  pred_rat.SetFillStyle(3001);
  pred_rat.Draw("02 same");
  obs_rat.Draw("ep0 same");
  line.DrawLine(0.5, 1, 0.5+dumb.GetNbinsX(), 1);
  c.Print(file_name.c_str());
}

vector<string> GetVarNames(const RooWorkspace &w){
  vector<string> names;
  TIter iter(w.allVars().createIterator());
  int size = w.allVars().getSize();
  TObject *obj = nullptr;
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
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  TObject *obj = nullptr;
  int i = 0;
  while((obj = iter()) && i < size){
    ++i;
    if(obj == nullptr) continue;
    string name = obj->GetName();
    Append(names, name);
  }
  iter.Reset();
  //sort(names.begin(), names.end());
  return names;
}

void ManuallyAddBins(const RooWorkspace &w, vector<string> &names){
  RooArgSet funcs = w.allFunctions();
  vector<string> blocks = {"lowmet", "medmet", "highmet"};
  vector<string> regions = {"r1", "r2", "r3", "r4"};
  vector<string> njets = {"", "lownj", "highnj"};
  vector<string> nbs = {"allnb", "1b", "2b", "3b"};
  for(const auto &block: blocks){
    for(const auto &region: regions){
      for(const auto &nj: njets){
	for(const auto &nb: nbs){
	  string name = "nexp_BLK_"+block+"_BIN_"+region+"_"+block+"_"+(nj==""?string(""):nj+"_")+nb;
	  if(funcs.find(name.c_str())) names.push_back(name);
	}
      }
    }
  }
}

vector<string> GetBinNames(const RooWorkspace &w, bool r4_only){
  vector<string> func_names = GetFuncNames(w);
  vector<string> names;
  for(const auto &name: func_names){
    if(name.substr(0,9) != "nexp_BLK_") continue;
    if(!Contains(name, "r4") && r4_only) continue;
    string bin_name = name.substr(5);
    Append(names, bin_name);
  }
  //reverse(names.begin(), names.end());
  return names;
}

vector<string> GetPlainBinNames(const RooWorkspace &w){
  vector<string> func_names = GetFuncNames(w);
  vector<string> names;
  for(const auto &name: func_names){
    if(name.substr(0,9) != "nexp_BLK_") continue;
    auto bpos = name.find("_BIN_");
    auto ppos = name.find("_PRC_");
    if(bpos == string::npos) continue;
    string bin_name = name.substr(bpos+5, ppos-bpos-5);
    Append(names, bin_name);
  }
  //reverse(names.begin(), names.end());
  return names;
}

vector<string> GetProcessNames(const RooWorkspace &w){
  vector<string> names;
  RooArgSet funcs = w.allFunctions();
  TIter iter(funcs.createIterator());
  int size = funcs.getSize();
  RooAbsArg *arg = nullptr;
  int i = 0;
  while((arg = static_cast<RooAbsArg*>(iter())) && i < size){
    ++i;
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
  for(auto &h: histos) h.SetMinimum(0.03);

  for(size_t iprc = 0; iprc < histos.size(); ++iprc){
    TH1D &h = histos.at(iprc);
    h.SetName(prc_names.at(iprc).c_str());
    Int_t color = iprc==0 ? TColor::GetColor(9,186,1) :
      (iprc==1 ? TColor::GetColor(153,220,255) : static_cast<Int_t>(iprc+3));
    h.SetFillColor(color);
    h.SetLineColor(color);
    h.SetLineWidth(0);
    for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
      const string &name = bin_names.at(ibin);
      auto pos = name.find("_BIN_");
      if(pos == string::npos) continue;
      pos = name.find("4");
      if(pos != string::npos && Contains(file_wspace, "nor4")) continue;
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

TH1D MakeExpSignal(RooWorkspace &w,
		   const vector<string> &bin_names){
  TH1D h("", ";;Yield ", bin_names.size(), 0.5, bin_names.size()+0.5);
  h.SetFillColor(0);
  h.SetFillStyle(0);
  h.SetLineColor(kRed+1);
  h.SetLineStyle(2);
  h.SetMinimum(0.03);

  for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
    h.SetBinError(ibin+1, 0.);
    string name = bin_names.at(ibin);
    auto pos = name.find("_BIN_");
    name = name.substr(pos+5);
    h.GetXaxis()->SetBinLabel(ibin+1, name.c_str());
    if(pos != string::npos){
      h.SetBinContent(ibin+1, GetMCYield(w, name, "signal"));
    }else{
      h.SetBinContent(ibin+1, -1.);
    }
  }

  return h;
}

TH1D MakeTotalHisto(RooWorkspace &w,
                    const RooFitResult &f,
                    const vector<string> &bin_names){
  TH1D h("signal", ";;Yield ", bin_names.size(), 0.5, bin_names.size()+0.5);
  h.SetFillColor(kRed+1);
  h.SetLineColor(kRed+1);
  h.SetLineWidth(0);
  h.SetMinimum(0.03);

  for(size_t ibin = 0; ibin < bin_names.size(); ++ibin){
    const string &name = bin_names.at(ibin);
    auto pos = name.find("_BIN_");
    if(pos == string::npos) continue;
    string label = name.substr(pos+5);
    h.GetXaxis()->SetBinLabel(ibin+1, label.c_str());
    RooRealVar *var = static_cast<RooRealVar*>(w.function(("nexp_"+name).c_str()));
    if(var == nullptr) continue;
    h.SetBinContent(ibin+1, var->getVal());
    h.SetBinError(ibin+1, GetError(*var, f));
  }

  return h;
}

TH1D MakeObserved(const RooWorkspace &w,
                  const vector<string> &bin_names){
  TH1D h("observed", ";;Yield ", bin_names.size(), 0.5, bin_names.size()+0.5);
  h.SetBinErrorOption(TH1::kPoisson);
  h.SetLineColor(1);
  h.SetFillColor(0);
  h.SetFillStyle(4000);
  h.SetMinimum(0.03);

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
  if(!Contains(file_wspace, "nor4")){
    a.SetMinimum(hmin);
    a.SetMaximum(hmax);
    b.SetMinimum(hmin);
    b.SetMaximum(hmax);
    for(auto &c: cs){
      c.SetMinimum(hmin);
      c.SetMaximum(hmax);
    }
  } else {
    a.SetMaximum(hmax+1.1*sqrt(hmax));
    b.SetMaximum(hmax+1.1*sqrt(hmax));
    a.SetMinimum(0);
    b.SetMinimum(0);
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
  double xerror(0.5);
  if(&num != &den) xerror = 0;
  for(int bin = 1; bin <= num.GetNbinsX(); ++bin){
    double x = num.GetBinCenter(bin);
    double nc = num.GetBinContent(bin);
    double dc = den.GetBinContent(bin);
    double ne = num.GetBinError(bin);
    double big_num = 0.5*numeric_limits<float>::max();
    if(dc != 0.){
      g.SetPoint(bin, x, nc/dc);
      g.SetPointError(bin, xerror, ne/dc);
    }else if(nc == 0.){
      g.SetPoint(bin, x, 1.);
      g.SetPointError(bin, xerror, big_num);
    }else{
      g.SetPoint(bin, x, nc > 0. ? big_num : -big_num);
      g.SetPointError(bin, xerror, big_num);
    }
  }
  return g;
}

string StripPath(const string &full_path){
  auto pos = full_path.rfind("/");
  if(pos != string::npos){
    return full_path.substr(pos+1);
  }else{
    return full_path;
  }
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
    h.SetBinError(ibin+1, GetError(*w.function(("kappamc_"+bin).c_str()), f));
  }
  h.GetXaxis()->LabelsOption("V");
  h.Draw();
  c.SetMargin(0.1, 0.05, 1./3., 0.05);
  c.Print(file_name.c_str());
}

double GetError(const RooAbsReal &var,
                const RooFitResult &f){
  // Clone self for internal use
  RooAbsReal* cloneFunc = static_cast<RooAbsReal*>(var.cloneTree());
  RooArgSet* errorParams = cloneFunc->getObservables(f.floatParsFinal());
  RooArgSet* nset = cloneFunc->getParameters(*errorParams);

  // Make list of parameter instances of cloneFunc in order of error matrix
  RooArgList paramList;
  const RooArgList& fpf = f.floatParsFinal();
  vector<int> fpf_idx;
  for (int i=0; i<fpf.getSize(); i++) {
    RooAbsArg* par = errorParams->find(fpf[i].GetName());
    if (par) {
      paramList.add(*par);
      fpf_idx.push_back(i);
    }
  }

  vector<double> errors(paramList.getSize());
  for (Int_t ivar=0; ivar<paramList.getSize(); ivar++) {
    RooRealVar& rrv = static_cast<RooRealVar&>(fpf[fpf_idx[ivar]]);

    double cenVal = rrv.getVal();
    double errVal = rrv.getError();

    // Make Plus variation
    static_cast<RooRealVar*>(paramList.at(ivar))->setVal(cenVal+0.5*errVal);
    double up = cloneFunc->getVal(nset);

    // Make Minus variation
    static_cast<RooRealVar*>(paramList.at(ivar))->setVal(cenVal-0.5*errVal);
    double down = cloneFunc->getVal(nset);

    errors.at(ivar) = (up-down);

    static_cast<RooRealVar*>(paramList.at(ivar))->setVal(cenVal);
  }

  vector<double> right(errors.size());
  for(size_t i = 0; i < right.size(); ++i){
    right.at(i) = 0.;
    for(size_t j = 0; j < errors.size(); ++j){
      right.at(i) += f.correlation(paramList.at(i)->GetName(),paramList.at(j)->GetName())*errors.at(j);
    }
  }
  double sum = 0.;
  for(size_t i = 0; i < right.size(); ++i){
    sum += errors.at(i)*right.at(i);
  }

  if(cloneFunc != nullptr){
    delete cloneFunc;
    cloneFunc = nullptr;
  }
  if(errorParams != nullptr){
    delete errorParams;
    errorParams = nullptr;
  }
  if(nset != nullptr){
    delete nset;
    nset = nullptr;
  }

  return sqrt(sum);
}

void GetOptionsExtract(int argc, char *argv[]){
  while(true){
    static struct option long_options[] = {
      {"file_wspace", required_argument, 0, 'f'},
      {"name_wspace", required_argument, 0, 'w'},
      {"toy", required_argument, 0, 't'},
      {"table_clean", no_argument, 0, 'c'},
      {"r4_only", no_argument, 0, '4'},
      {"exp_sig", no_argument, 0, 's'},
      {0, 0, 0, 0}
    };

    char opt = -1;
    int option_index;
    opt = getopt_long(argc, argv, "f:w:t:c4s", long_options, &option_index);
    if( opt == -1) break;

    string optname;
    switch(opt){
    case 'w':
      name_wspace = optarg;
      break;
    case 'f':
      file_wspace = optarg;
      break;
    case 't':
      toy_num = atoi(optarg);
      break;
    case 'c':
      table_clean = true;
      break;
    case '4':
      r4_only = true;
      break;
    case 's':
      show_exp_sig = true;
      break;
    default:
      printf("Bad option! getopt_long returned character code 0%o\n", opt);
      break;
    }
  }
}
