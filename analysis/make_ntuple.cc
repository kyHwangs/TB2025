
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>

#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include "TH2D.h"

#include "TBmonit.h"
#include "TBmid.h"
#include "TBevt.h"
#include "TBread.h"
#include "TButility.h"
#include "TBaux.h"
#include "TFile.h"
#include "TBconfig.h"
#include "TBread.h"
#include "TBobject.h"

void GetFormattedRamInfo() {

  // Total physical memory
  int64_t physical_memory;
  size_t length = sizeof(physical_memory);
  sysctlbyname("hw.memsize", &physical_memory, &length, NULL, 0);
  double total_memory_GB = static_cast<double>(physical_memory) / (1024 * 1024 * 1024);

  // Memory usage by this process
  task_basic_info_data_t info;
  mach_msg_type_number_t info_count = TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &info_count) == KERN_SUCCESS) {
      double process_memory_GB = static_cast<double>(info.resident_size) / (1024 * 1024 * 1024);

      // system memory usage
      vm_size_t page_size;
      mach_port_t mach_port = mach_host_self();
      vm_statistics64_data_t vm_stats;
      mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
      if (host_page_size(mach_port, &page_size) == KERN_SUCCESS &&
          host_statistics64(mach_port, HOST_VM_INFO, (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
          double free_memory_GB = static_cast<double>(vm_stats.free_count * page_size) / (1024 * 1024 * 1024);
          double used_memory_GB = total_memory_GB - free_memory_GB;


          printf("%.1f GB / %.1f GB (%0.2f %%) | Current Process: %.2f MB (%.2f %%)",
            used_memory_GB, total_memory_GB, (used_memory_GB / total_memory_GB * 100),
            process_memory_GB * 1024., (process_memory_GB / total_memory_GB * 100));
      }
  }
}

class TBCalcTemplate {
public:

  TBCalcTemplate(TBcid fCID_, int fINIT_, int fFIN_) : fCID(fCID_), fINIT(fINIT_), fFIN(fFIN_) {}
  ~TBCalcTemplate() {}

  TBcid GetCID() {
    return fCID;
  }

  void SetWave(std::vector<short> tWave) {

    std::vector<double> tCorrectedWave = {};
    
    double tPed = 0.;
    for (int i = 1; i < 101; i++)
      tPed += static_cast<double>(tWave.at(i)) / 100.;
  
    for (int i = fINIT; i < fFIN; i++)
      tCorrectedWave.push_back(static_cast<double>(tPed - tWave.at(i)));

    fCorrectedWave = tCorrectedWave;
  }

  void Clear() {
    fCorrectedWave.clear();
  }

  double GetPeakADC() {

    return  *std::max_element(fCorrectedWave.begin(), fCorrectedWave.end());
  }

  double GetIntADC() {

    return std::accumulate(fCorrectedWave.begin(), fCorrectedWave.end(), 0.);
  }

private:
  TBcid fCID;
  int fINIT;
  int fFIN;

  std::vector<double> fCorrectedWave;

};

int main(int argc, char* argv[]) {

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  TH2D* fDWC1 = new TH2D("DWC1", "DWC 1 position;X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);
  // fDWC1->SetStats(0);

  TH2D* fDWC2 = new TH2D("DWC2", "DWC 2 position;X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);
  // fDWC2->SetStats(0);

  ANSI_CODE ANSI = ANSI_CODE();
    
  std::vector<std::vector<double>> fIntADC_CEREN;
  std::vector<std::vector<double>> fPeakADC_CEREN;
  std::vector<std::vector<double>> fIntADC_SCINT;
  std::vector<std::vector<double>> fPeakADC_SCINT;

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fIntADC_CEREN.push_back(tTEMPVEC);
  }

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fPeakADC_CEREN.push_back(tTEMPVEC);
  }

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fIntADC_SCINT.push_back(tTEMPVEC);
  }

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fPeakADC_SCINT.push_back(tTEMPVEC);
  }

  std::vector<double> fIntADC_AUX = {0., 0., 0., 0., 0.}; // CC1 CC2 PC MC TC
  std::vector<double> fPeakADC_AUX = {0., 0., 0., 0., 0.};

  std::vector<float> fDWCPOS = {0., 0., 0., 0.};

  std::vector<double> fPeakADC_LC = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<double> fIntADC_LC = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<int> fLC_ID = {2, 4, 8, 10, 3, 5, 7, 9, 11, 12, 13, 19, 14, 15, 16, 20}; 

  TTree* fTree = new TTree("data", "data");
  for (int iModule = 0; iModule < 9; iModule++) {
    for (int iTower = 0; iTower < 4; iTower++) {
      fTree->Branch((TString)Form("IntADC_M%d_T%d_C", iModule + 1, iTower + 1), &(fIntADC_CEREN[iModule][iTower]));
      fTree->Branch((TString)Form("PeakADC_M%d_T%d_C", iModule + 1, iTower + 1), &(fPeakADC_CEREN[iModule][iTower]));
      fTree->Branch((TString)Form("IntADC_M%d_T%d_S", iModule + 1, iTower + 1), &(fIntADC_SCINT[iModule][iTower]));
      fTree->Branch((TString)Form("PeakADC_M%d_T%d_S", iModule + 1, iTower + 1), &(fPeakADC_SCINT[iModule][iTower]));
    }
  }

  for (int i = 0; i < fLC_ID.size(); i++) {
    fTree->Branch((TString)Form("PeakADC_LC%d", fLC_ID.at(i)), &(fPeakADC_LC.at(i)));
    fTree->Branch((TString)Form("IntADC_LC%d", fLC_ID.at(i)), &(fIntADC_LC.at(i)));
  }
  
  fTree->Branch((TString)Form("IntADC_CC1"), &(fIntADC_AUX[0]));
  fTree->Branch((TString)Form("IntADC_CC2"), &(fIntADC_AUX[1]));
  fTree->Branch((TString)Form("IntADC_PC"), &(fIntADC_AUX[2]));
  fTree->Branch((TString)Form("IntADC_MC"), &(fIntADC_AUX[3]));
  fTree->Branch((TString)Form("IntADC_TC"), &(fIntADC_AUX[4]));

  fTree->Branch((TString)Form("PeakADC_CC1"), &(fPeakADC_AUX[0]));
  fTree->Branch((TString)Form("PeakADC_CC2"), &(fPeakADC_AUX[1]));
  fTree->Branch((TString)Form("PeakADC_PC"), &(fPeakADC_AUX[2]));
  fTree->Branch((TString)Form("PeakADC_MC"), &(fPeakADC_AUX[3]));
  fTree->Branch((TString)Form("PeakADC_TC"), &(fPeakADC_AUX[4]));

  fTree->Branch((TString)Form("DWC1X"), &(fDWCPOS[0]));
  fTree->Branch((TString)Form("DWC1Y"), &(fDWCPOS[1]));
  fTree->Branch((TString)Form("DWC2X"), &(fDWCPOS[2]));
  fTree->Branch((TString)Form("DWC2Y"), &(fDWCPOS[3]));

  TBconfig* fConfig = new TBconfig("./config_general.yml");
  const YAML::Node fConfig_YAML = fConfig->GetConfig();
  const YAML::Node fConfig_RANGE = fConfig->GetConfig()["ModuleConfig"];

  std::string fBaseDir = fConfig_YAML["BaseDirectory"].as<std::string>();
  std::string fMapping = fConfig_YAML["Mapping"].as<std::string>();

  TButility* fUtility = new TButility(fMapping);

  int fRunNum = 0;
  fObj->GetVariable("RunNumber", &fRunNum);

  std::vector<TBcid> fCIDtoPlot_CEREN = {};
  std::vector<TBcid> fCIDtoPlot_SCINT = {};

  std::vector<std::vector<TBCalcTemplate*>> fCalcTemplate_CEREN = {};
  std::vector<std::vector<TBCalcTemplate*>> fCalcTemplate_SCINT = {}; 

  for (int iModule = 0; iModule < 9; iModule++) {

    std::vector<TBCalcTemplate*> tTEMPVEC_CEREN = {};
    std::vector<TBCalcTemplate*> tTEMPVEC_SCINT = {};

    for (int iTower = 0; iTower < 4; iTower++) {
    
      std::string aNameCEREN = (std::string)Form("M%d-T%d-C", iModule + 1, iTower + 1);
      fCIDtoPlot_CEREN.push_back(fUtility->GetCID(aNameCEREN));
      std::vector<int> interval_CEREN = fConfig_RANGE[aNameCEREN].as<std::vector<int>>();
      tTEMPVEC_CEREN.push_back(new TBCalcTemplate(fUtility->GetCID(aNameCEREN), interval_CEREN.at(0), interval_CEREN.at(1))); 
    
      std::string aNameSCINT = (std::string)Form("M%d-T%d-S", iModule + 1, iTower + 1);
      fCIDtoPlot_SCINT.push_back(fUtility->GetCID(aNameSCINT));
      std::vector<int> interval_SCINT = fConfig_RANGE[aNameSCINT].as<std::vector<int>>();
      tTEMPVEC_SCINT.push_back(new TBCalcTemplate(fUtility->GetCID(aNameSCINT), interval_SCINT.at(0), interval_SCINT.at(1))); 

      // std::cout << aNameCEREN << " " << interval_CEREN.at(0) << " " << interval_CEREN.at(1) << std::endl;
      // std::cout << aNameSCINT << " " << interval_SCINT.at(0) << " " << interval_SCINT.at(1) << std::endl;
    }

    fCalcTemplate_CEREN.push_back(tTEMPVEC_CEREN);
    fCalcTemplate_SCINT.push_back(tTEMPVEC_SCINT);
  }

  std::vector<TBcid> fCIDtoPlot_LC = {};
  std::vector<TBCalcTemplate*> fCalcTemplate_LC = {};

  for (int i = 0; i < fLC_ID.size(); i++) {
    std::string aNameLC = (std::string)Form("LC%d", fLC_ID.at(i));
    fCIDtoPlot_LC.push_back(fUtility->GetCID(aNameLC));
    std::vector<int> interval_LC = fConfig_RANGE[aNameLC].as<std::vector<int>>();
    fCalcTemplate_LC.push_back(new TBCalcTemplate(fUtility->GetCID(aNameLC), interval_LC.at(0), interval_LC.at(1)));
  }

  // std::cout << "fCalcTemplate_CEREN.size() = " << fCalcTemplate_CEREN.size() << std::endl;
  // std::cout << "fCalcTemplate_SCINT.size() = " << fCalcTemplate_SCINT.size() << std::endl;

  // for (int iModule = 0; iModule < 9; iModule++) {
  //   std::cout << "fCalcTemplate_CEREN.at(" << iModule << ").size() = " << fCalcTemplate_CEREN.at(iModule).size() << std::endl;
  //   std::cout << "fCalcTemplate_SCINT.at(" << iModule << ").size() = " << fCalcTemplate_SCINT.at(iModule).size() << std::endl;
  // }

  std::vector<TBcid> fCIDtoPlot_AUX = {};

  fCIDtoPlot_AUX.push_back(fUtility->GetCID("CC1"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("CC2"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("PS"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("MC"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("TC"));

  std::vector<TBCalcTemplate*> fCalcTemplate_AUX = {};

  fCalcTemplate_AUX.push_back(new TBCalcTemplate(fUtility->GetCID("CC1"),  fConfig_RANGE["CC1"].as<std::vector<int>>().at(0), fConfig_RANGE["CC1"].as<std::vector<int>>().at(1)));
  fCalcTemplate_AUX.push_back(new TBCalcTemplate(fUtility->GetCID("CC2"),  fConfig_RANGE["CC2"].as<std::vector<int>>().at(0), fConfig_RANGE["CC2"].as<std::vector<int>>().at(1)));
  fCalcTemplate_AUX.push_back(new TBCalcTemplate(fUtility->GetCID("PS"),  fConfig_RANGE["PS"].as<std::vector<int>>().at(0), fConfig_RANGE["PS"].as<std::vector<int>>().at(1)));
  fCalcTemplate_AUX.push_back(new TBCalcTemplate(fUtility->GetCID("MC"),  fConfig_RANGE["MC"].as<std::vector<int>>().at(0), fConfig_RANGE["MC"].as<std::vector<int>>().at(1)));
  fCalcTemplate_AUX.push_back(new TBCalcTemplate(fUtility->GetCID("TC"),  fConfig_RANGE["TC"].as<std::vector<int>>().at(0), fConfig_RANGE["TC"].as<std::vector<int>>().at(1)));

  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC1R"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC1L"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC1U"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC1D"));

  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC2R"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC2L"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC2U"));
  fCIDtoPlot_AUX.push_back(fUtility->GetCID("DWC2D"));

  TBaux fAux = TBaux(fConfig->GetConfig()["AUX"], fRunNum, false, false, false, *fUtility);
  fAux.SetMethod("PeakADC");
  // fAux.init();
  fAux.SetRange(fConfig->GetConfig()["ModuleConfig"]);

  auto fUniqueMID_module = fUtility->GetUniqueMID(fCIDtoPlot_CEREN, fCIDtoPlot_SCINT);
  auto fUniqueMID_aux = fUtility->GetUniqueMID(fCIDtoPlot_AUX);
  auto fUniqueMID_lc = fUtility->GetUniqueMID(fCIDtoPlot_LC);
  auto fUniqueMID_lc_aux = fUtility->GetUniqueMID(fUniqueMID_lc, fUniqueMID_aux);
  auto tUniqueMID = fUtility->GetUniqueMID(fUniqueMID_module, fUniqueMID_lc_aux);
  
  TBread<TBwaveform> readerWave = TBread<TBwaveform>(fRunNum, -1, -1, false, fBaseDir, tUniqueMID);
  int fMaxEvent = readerWave.GetMaxEvent();

  // fMaxEvent = 1000;

  auto time_begin = std::chrono::system_clock::now();
  for (int i = 0; i < fMaxEvent; i++) {
    if (i > 0 && i % 10 == 0) {

      auto time_taken = std::chrono::system_clock::now() - time_begin; // delete
      float percent_done = 1. * (float)(i) / (float)(fMaxEvent);
      auto time_left = time_taken * (1 / percent_done - 1);
      std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
      std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
      std::cout << "\r\033[F" //+ ANSI.HIGHLIGHTED_GREEN + ANSI.BLACK
                << " " << i << " / " << fMaxEvent << " events  " << minutes_left.count() << ":";
      printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);
      GetFormattedRamInfo();

      std::cout << ANSI.END << std::endl;
    }

    TBevt<TBwaveform> anEvent = readerWave.GetAnEvent();
    
    for (int iModule = 0; iModule < 9; iModule++) {
      for (int iTower = 0; iTower < 4; iTower++) {
        fCalcTemplate_CEREN.at(iModule).at(iTower)->SetWave(anEvent.GetData(fCalcTemplate_CEREN.at(iModule).at(iTower)->GetCID()).waveform());
        fCalcTemplate_SCINT.at(iModule).at(iTower)->SetWave(anEvent.GetData(fCalcTemplate_SCINT.at(iModule).at(iTower)->GetCID()).waveform());

        fIntADC_CEREN.at(iModule).at(iTower) = fCalcTemplate_CEREN.at(iModule).at(iTower)->GetIntADC();
        fPeakADC_CEREN.at(iModule).at(iTower) = fCalcTemplate_CEREN.at(iModule).at(iTower)->GetPeakADC();

        fIntADC_SCINT.at(iModule).at(iTower) = fCalcTemplate_SCINT.at(iModule).at(iTower)->GetIntADC();
        fPeakADC_SCINT.at(iModule).at(iTower) = fCalcTemplate_SCINT.at(iModule).at(iTower)->GetPeakADC();

        fCalcTemplate_CEREN.at(iModule).at(iTower)->Clear();
        fCalcTemplate_SCINT.at(iModule).at(iTower)->Clear();
      }
    }

    for (int iLC = 0; iLC < fLC_ID.size(); iLC++) {
      fCalcTemplate_LC.at(iLC)->SetWave(anEvent.GetData(fCalcTemplate_LC.at(iLC)->GetCID()).waveform());
      fPeakADC_LC.at(iLC) = fCalcTemplate_LC.at(iLC)->GetPeakADC();
      fIntADC_LC.at(iLC) = fCalcTemplate_LC.at(iLC)->GetIntADC();
      fCalcTemplate_LC.at(iLC)->Clear();
    }
    
    fCalcTemplate_AUX.at(0)->SetWave(anEvent.GetData(fCalcTemplate_AUX.at(0)->GetCID()).waveform());
    fCalcTemplate_AUX.at(1)->SetWave(anEvent.GetData(fCalcTemplate_AUX.at(1)->GetCID()).waveform());
    fCalcTemplate_AUX.at(2)->SetWave(anEvent.GetData(fCalcTemplate_AUX.at(2)->GetCID()).waveform());
    fCalcTemplate_AUX.at(3)->SetWave(anEvent.GetData(fCalcTemplate_AUX.at(3)->GetCID()).waveform());
    fCalcTemplate_AUX.at(4)->SetWave(anEvent.GetData(fCalcTemplate_AUX.at(4)->GetCID()).waveform());

    fIntADC_AUX.at(0) = fCalcTemplate_AUX.at(0)->GetIntADC();
    fIntADC_AUX.at(1) = fCalcTemplate_AUX.at(1)->GetIntADC();
    fIntADC_AUX.at(2) = fCalcTemplate_AUX.at(2)->GetIntADC();
    fIntADC_AUX.at(3) = fCalcTemplate_AUX.at(3)->GetIntADC();
    fIntADC_AUX.at(4) = fCalcTemplate_AUX.at(4)->GetIntADC();

    fPeakADC_AUX.at(0) = fCalcTemplate_AUX.at(0)->GetPeakADC();
    fPeakADC_AUX.at(1) = fCalcTemplate_AUX.at(1)->GetPeakADC();
    fPeakADC_AUX.at(2) = fCalcTemplate_AUX.at(2)->GetPeakADC();
    fPeakADC_AUX.at(3) = fCalcTemplate_AUX.at(3)->GetPeakADC();
    fPeakADC_AUX.at(4) = fCalcTemplate_AUX.at(4)->GetPeakADC();
    
    std::vector<std::vector<float>> tDWCwaves;

    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC1R")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC1L")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC1U")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC1D")).pedcorrectedWaveform());

    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC2R")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC2L")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC2U")).pedcorrectedWaveform());
    tDWCwaves.push_back(anEvent.GetData(fUtility->GetCID("DWC2D")).pedcorrectedWaveform());
  
    auto pos = fAux.GetPosition(tDWCwaves);
    fDWCPOS.at(0) = pos.at(0);
    fDWCPOS.at(1) = pos.at(1);
    fDWCPOS.at(2) = pos.at(2);
    fDWCPOS.at(3) = pos.at(3);

    fDWC1->Fill(fDWCPOS.at(0), fDWCPOS.at(1));
    fDWC2->Fill(fDWCPOS.at(2), fDWCPOS.at(3));

    fTree->Fill();
  }

  TFile* fFile = new TFile(Form("./RAW/LCCALIB/Run%d.root", fRunNum), "RECREATE");
  fFile->cd();
  fTree->Write();
  fDWC1->Write();
  fDWC2->Write();
  fFile->Close();

  return 0;
}