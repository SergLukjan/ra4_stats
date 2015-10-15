#include "make_workspace.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <initializer_list>
#include <vector>
#include <string>

#include <unistd.h>
#include <getopt.h>

#include "bin.hpp"
#include "process.hpp"
#include "utilities.hpp"
#include "systematic.hpp"
#include "cut.hpp"

#include "workspace_generator.hpp"

using namespace std;

namespace{
  double lumi = 4.;
  bool blinded = true;
  bool do_syst = true;
}

int main(int argc, char *argv[]){
  cout << fixed << setprecision(2);
  GetOptions(argc, argv);
  //Define processes. Try to minimize splitting
  Process ttbar{"ttbar", {
      {"archive/2015_09_28_ana/skim/*TTJets*Lept*.root/tree"}
    }};
  Process other{"other", {
      {"archive/2015_09_28_ana/skim/*DYJetsToLL*.root/tree"},
        {"archive/2015_09_28_ana/skim/*QCD_Pt*.root/tree"},
	  {"archive/2015_09_28_ana/skim/*_ST_*.root/tree"},
	    {"archive/2015_09_28_ana/skim/*WJetsToLNu*.root/tree"},
	      {"archive/2015_09_28_ana/skim/*_WWTo*.root/tree"},
		{"archive/2015_09_28_ana/skim/*ggZH_HToBB*.root/tree"},
		  {"archive/2015_09_28_ana/skim/*ttHJetTobb*.root/tree"}
    }};
  Process signal_nc{"signal_nc", {
      {"archive/2015_09_28_ana/skim/*T1tttt*1500*100*.root/tree"}
    }};
  Process signal_c{"signal_c", {
      {"archive/2015_09_28_ana/skim/*T1tttt*1200*800*.root/tree"}
    }};
  Process data{"data", {}};

  //Make list of all backgrounds. Backgrounds assumed to be orthogonal
  set<Process> backgrounds{ttbar, other};

  //Baseline selection applied to all bins and processes
  Cut baseline{"ht>500&&met>200&njets>=7&&nbm>=2&&(nels+nmus)==1"};

  //Declare bins
  //Method 2
  Bin r1_lowmet_lownb{"r1_lowmet_lownb", "mt<=140&&mj<=400&&met<=400&&nbm<=2"};
  Bin r1_lowmet_highnb{"r1_lowmet_highnb", "mt<=140&&mj<=400&&met<=400&&nbm>2"};
  Bin r1_highmet{"r1_highmet", "mt<=140&&mj<=400&&met>400"};

  Bin r2_lowmet_lownj_lownb{"r2_lowmet_lownj_lownb", "mt<=140&&mj>400&&met<=400&&njets<=8&&nbm<=2"};
  Bin r2_lowmet_lownj_highnb{"r2_lowmet_lownj_highnb", "mt<=140&&mj>400&&met<=400&&njets<=8&&nbm>2"};
  Bin r2_lowmet_highnj_lownb{"r2_lowmet_highnj_lownb", "mt<=140&&mj>400&&met<=400&&njets>8&&nbm<=2"};
  Bin r2_lowmet_highnj_highnb{"r2_lowmet_highnj_highnb", "mt<=140&&mj>400&&met<=400&&njets>8&&nbm>2"};
  Bin r2_highmet_lownj{"r2_highmet_lownj", "mt<=140&&mj>400&&met>400&&njets<=8"};
  Bin r2_highmet_highnj{"r2_highmet_highnj", "mt<=140&&mj>400&&met>400&&njets>8"};

  Bin r3_lowmet_lownb{"r3_lowmet_lownb", "mt>140&&mj<=400&&met<=400&&nbm<=2"};
  Bin r3_lowmet_highnb{"r3_lowmet_highnb", "mt>140&&mj<=400&&met<=400&&nbm>2"};
  Bin r3_highmet{"r3_highmet", "mt>140&&mj<=400&&met>400"};

  Bin r4_lowmet_lownj_lownb{"r4_lowmet_lownj_lownb", "mt>140&&mj>400&&met<=400&&njets<=8&&nbm<=2"};
  Bin r4_lowmet_lownj_highnb{"r4_lowmet_lownj_highnb", "mt>140&&mj>400&&met<=400&&njets<=8&&nbm>2"};
  Bin r4_lowmet_highnj_lownb{"r4_lowmet_highnj_lownb", "mt>140&&mj>400&&met<=400&&njets>8&&nbm<=2"};
  Bin r4_lowmet_highnj_highnb{"r4_lowmet_highnj_highnb", "mt>140&&mj>400&&met<=400&&njets>8&&nbm>2"};
  Bin r4_highmet_lownj{"r4_highmet_lownj", "mt>140&&mj>400&&met>400&&njets<=8"};
  Bin r4_highmet_highnj{"r4_highmet_highnj", "mt>140&&mj>400&&met>400&&njets>8"};

  //Method 1
  Bin m1_r1_lowmet_lownj{"m1_r1_lowmet_lownj", "mt<=140&&mj<=600&&met<=400&&njets<=8"};
  Bin m1_r1_lowmet_highnj{"m1_r1_lowmet_highnj", "mt<=140&&mj<=600&&met<=400&&njets>8"};
  Bin m1_r1_highmet_lownj{"m1_r1_highmet_lownj", "mt<=140&&mj<=600&&met>400&&njets<=8"};
  Bin m1_r1_highmet_highnj{"m1_r1_highmet_highnj", "mt<=140&&mj<=600&&met>400&&njets>8"};

  Bin m1_r2_lowmet_lownj{"m1_r2_lowmet_lownj", "mt<=140&&mj>600&&met<=400&&njets<=8"};
  Bin m1_r2_lowmet_highnj{"m1_r2_lowmet_highnj", "mt<=140&&mj>600&&met<=400&&njets>8"};
  Bin m1_r2_highmet_lownj{"m1_r2_highmet_lownj", "mt<=140&&mj>600&&met>400&&njets<=8"};
  Bin m1_r2_highmet_highnj{"m1_r2_highmet_highnj", "mt<=140&&mj>600&&met>400&&njets>8"};

  Bin m1_r3_lowmet_lownj{"m1_r3_lowmet_lownj", "mt>140&&mj<=600&&met<=400&&njets<=8"};
  Bin m1_r3_lowmet_highnj{"m1_r3_lowmet_highnj", "mt>140&&mj<=600&&met<=400&&njets>8"};
  Bin m1_r3_highmet_lownj{"m1_r3_highmet_lownj", "mt>140&&mj<=600&&met>400&&njets<=8"};
  Bin m1_r3_highmet_highnj{"m1_r3_highmet_highnj", "mt>140&&mj<=600&&met>400&&njets>8"};

  Bin m1_r4_lowmet_lownj{"m1_r4_lowmet_lownj", "mt>140&&mj>600&&met<=400&&njets<=8"};
  Bin m1_r4_lowmet_highnj{"m1_r4_lowmet_highnj", "mt>140&&mj>600&&met<=400&&njets>8"};
  Bin m1_r4_highmet_lownj{"m1_r4_highmet_lownj", "mt>140&&mj>600&&met>400&&njets<=8"};
  Bin m1_r4_highmet_highnj{"m1_r4_highmet_highnj", "mt>140&&mj>600&&met>400&&njets>8"};

  //Method David
  Bin r1{"r1", "mt<=140&&mj<=400"};
  Bin r2{"r2", "mt<=140&&mj>400"};
  Bin r3{"r3", "mt>140&&mj<=400"};
  Bin r4{"r4", "mt>140&&mj>400"};

  //Specify ABCD constraints
  set<Block> blocks_m2{
    {"lowmet_lownb", {{r1_lowmet_lownb, r2_lowmet_lownj_lownb, r2_lowmet_highnj_lownb},
          {r3_lowmet_lownb, r4_lowmet_lownj_lownb, r4_lowmet_highnj_lownb}}},
      {"lowmet_highnb", {{r1_lowmet_highnb, r2_lowmet_lownj_highnb, r2_lowmet_highnj_highnb},
            {r3_lowmet_highnb, r4_lowmet_lownj_highnb, r4_lowmet_highnj_highnb}}},
        {"highmet", {{r1_highmet, r2_highmet_lownj, r2_highmet_highnj},
              {r3_highmet, r4_highmet_lownj, r4_highmet_highnj}}}
  };

  set<Block> blocks_m1{
    {"lowmet_lownj", {{m1_r1_lowmet_lownj, m1_r2_lowmet_lownj},
          {m1_r3_lowmet_lownj, m1_r4_lowmet_lownj}}},
      {"lowmet_highnj", {{m1_r1_lowmet_highnj, m1_r2_lowmet_highnj},
            {m1_r3_lowmet_highnj, m1_r4_lowmet_highnj}}},
        {"highmet_lownj", {{m1_r1_highmet_lownj, m1_r2_highmet_lownj},
              {m1_r3_highmet_lownj, m1_r4_highmet_lownj}}},
          {"highmet_highnj", {{m1_r1_highmet_highnj, m1_r2_highmet_highnj},
                {m1_r3_highmet_highnj, m1_r4_highmet_highnj}}}
  };

  vector<Block> blocks_david{
    {"all", {{r1, r2}, {r3, r4}}}
  };

  ostringstream oss;
  oss << (10.*lumi) << flush;
  string lumi_string = oss.str();
  auto decimal = lumi_string.find('.');
  while( decimal != string::npos ) {
    lumi_string.erase(decimal,1);
    decimal = lumi_string.find('.');
  }
  string no_syst = do_syst ? "" : "_nosyst";

  WorkspaceGenerator wg2(baseline, blocks_m2, backgrounds, signal_nc, data, "txt/systematics/method2.txt");
  wg2.SetDoDilepton(true);
  wg2.SetDoSystematics(true);
  wg2.WriteToFile("method2nc.root");
  WorkspaceGenerator wga(baseline, blocks_m2, backgrounds, signal_nc, data, "txt/systematics/method2.txt");
  wga.SetDoDilepton(true);
  wga.SetDoSystematics(false);
  wga.WriteToFile("method2nc_nosyst.root");
  WorkspaceGenerator wgb(baseline, blocks_m2, backgrounds, signal_nc, data, "txt/systematics/method2.txt");
  wgb.SetDoDilepton(false);
  wgb.SetDoSystematics(true);
  wgb.WriteToFile("method2nc_nodilep.root");
  WorkspaceGenerator wgc(baseline, blocks_m2, backgrounds, signal_nc, data, "txt/systematics/method2.txt");
  wgc.SetDoDilepton(false);
  wgc.SetDoSystematics(false);
  wgc.WriteToFile("method2nc_statonly.root");
  WorkspaceGenerator wgd(baseline, blocks_m2, backgrounds, signal_nc, data, "txt/systematics/method2.txt");
  wgd.SetDoDilepton(true);
  wgd.SetDoSystematics(true);
  wgd.SetKappaCorrected(false);
  wgd.WriteToFile("method2nc_nokappa.root");
  WorkspaceGenerator wg2c(baseline, blocks_m2, backgrounds, signal_c, data, "txt/systematics/method2.txt");
  wg2c.WriteToFile("method2c.root");
  WorkspaceGenerator wg1(baseline, blocks_m1, backgrounds, signal_nc, data, "txt/systematics/method1.txt");
  wg1.WriteToFile("method1nc.root");
  WorkspaceGenerator wg1c(baseline, blocks_m1, backgrounds, signal_c, data, "txt/systematics/method1.txt");
  wg1c.WriteToFile("method1c.root");
}

void GetOptions(int argc, char *argv[]){
  while(true){
    static struct option long_options[] = {
      {"lumi", required_argument, 0, 'l'},
      {"unblind", no_argument, 0, 'u'},
      {"no_syst", no_argument, 0, 0},
      {0, 0, 0, 0}
    };

    char opt = -1;
    int option_index;
    opt = getopt_long(argc, argv, "l:u", long_options, &option_index);
    if( opt == -1) break;

    string optname;
    switch(opt){
    case 'l':
      lumi = atof(optarg);
      break;
    case 'u':
      blinded = false;
      break;
    case 0:
      optname = long_options[option_index].name;
      if(optname == "no_syst"){
	do_syst = false;
      }else{
        printf("Bad option! Found option name %s\n", optname.c_str());
      }
      break;
    default:
      printf("Bad option! getopt_long returned character code 0%o\n", opt);
      break;
    }
  }
}
