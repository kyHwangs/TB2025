
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include "TH2.h"
#include "TF1.h"

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
#include "TStyle.h"
#include "TGraphErrors.h"
#include "TCanvas.h"

namespace fs = std::__fs::filesystem;

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


class HistogramCollection {

public:
  HistogramCollection(std::string fName_) {
    fName = fName_;

    fHistCeren = new TH1D((TString)Form("Ceren_%s", fName.c_str()), ";IntADC;nEvents", 1550, -5000., 150000.);
    fHistScint = new TH1D((TString)Form("Scint_%s", fName.c_str()), ";IntADC;nEvents", 1550, -5000., 150000.);

    fHistPS = new TH1D((TString)Form("PS_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);
    fHistMC = new TH1D((TString)Form("MC_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);

    fHistFDWC = new TH2D((TString)Form("FDWC_%s", fName.c_str()), ";X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);
    fHistLDWC = new TH2D((TString)Form("LDWC_%s", fName.c_str()), ";X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);

    fHistDWCX = new TH2D((TString)Form("DWCX_%s", fName.c_str()), ";DWC 1 X [mm];DWC 2 X [mm]", 200, -50., 50., 200, -50., 50.);
    fHistDWCY = new TH2D((TString)Form("DWCY_%s", fName.c_str()), ";DWC 1 Y [mm];DWC 2 Y [mm]", 200, -50., 50., 200, -50., 50.);
  }

  void Fill_PS(double value) {
    fHistPS->Fill(value);
  }
  void Fill_MC(double value) {
    fHistMC->Fill(value);
  }

  void Fill_Tower(double value_ceren, double value_scint) {
    fHistCeren->Fill(value_ceren);
    fHistScint->Fill(value_scint);
  }

  void Fill_DWC(std::vector<float> fDWC) {

    fHistFDWC->Fill(fDWC.at(0), fDWC.at(1));
    fHistLDWC->Fill(fDWC.at(2), fDWC.at(3));

    fHistDWCX->Fill(fDWC.at(0), fDWC.at(2));
    fHistDWCY->Fill(fDWC.at(1), fDWC.at(3));
  }

  void Fill(double ps, double mc, double ceren, double scint, std::vector<float> fDWC) {
    Fill_PS(ps); 
    Fill_MC(mc);
    Fill_Tower(ceren, scint);
    Fill_DWC(fDWC); 
  }

  void Save(TFile* fFile) {
    fFile->cd();

    fFile->mkdir(fName.c_str());
    fFile->cd(fName.c_str());

    fHistCeren->Write();
    fHistScint->Write();
    fHistPS->Write();
    fHistMC->Write();
    fHistFDWC->Write();
    fHistLDWC->Write();
    fHistDWCX->Write();
    fHistDWCY->Write();
  }

private:
  std::string fName;

  TH1D* fHistCeren;
  TH1D* fHistScint;

  TH2D* fHistFDWC;
  TH2D* fHistLDWC;
  TH2D* fHistDWCX;
  TH2D* fHistDWCY;

  TH1D* fHistPS;
  TH1D* fHistMC;


};

bool IsPassingDWC(std::vector<float> fDWC) {

  // if (std::abs(fDWC.at(0)) > 5.)
  //   return false;

  // if (std::abs(fDWC.at(1)) > 5.)
  //   return false;
  
  // if (std::abs(fDWC.at(2)) > 5.)
  //   return false;
  
  // if (std::abs(fDWC.at(3)) > 5.)
  //   return false;

  if (std::abs(fDWC.at(0) - fDWC.at(2)) > 4.)
    return false;

  if (std::abs(fDWC.at(1) - fDWC.at(3)) > 4.)
    return false;

  return true;
}


bool IsInRange(float x, float y, int casetype) {

  switch (casetype) {
    case 1:
      return x > 3;
    case 2:
      return x <= 3. && x > -3.;
    case 3:
      return x <= -3.;

    case 4:
      return y > 3.;
    case 5:
      return y <= 3. && y > -3.;
    case 6:
      return y <= -3.;

    case 7:
      return y > 3. && x > 3.;
    case 8:
      return y > 3. && x <= 3. && x > -3.;
    case 9:
      return y > 3. && x <= -3.;

    case 10:
      return y <= 3. && y > -3. && x > 3.;
    case 11:
      return y <= 3. && y > -3. && x <= 3. && x > -3.;
    case 12:
      return y <= 3. && y > -3. && x <= -3.;

    case 13:
      return y <= -3. && x > 3.;
    case 14:
      return y <= -3. && x <= 3. && x > -3.;
    case 15:
      return y <= -3. && x <= -3.;

    case 16:
      return x > 0. && y > 0.;
    case 17:
      return x <= 0. && y > 0.;
    case 18:
      return x <= 0. && y <= 0.;
    case 19:
      return x > 0. && y <= 0.;

    case 20:
      return y <= -3. && y > -6. && x > 0.;
    case 21:
      return y <= -3. && y > -6. && x <= 0.;
    case 22:
      return y <= -6. && x > 0.;
    case 23:
      return y <= -6. && x <= 0.;

    case 24:
      return y > -3. && x > 0;
    case 25:
      return y > -3. && x <= 0;
    case 26:
      return y <= -3. && x > 0;
    case 27:
      return y <= -3. && x <= 0;

  }
}

int main(int argc, char* argv[]) {
    
  ANSI_CODE ANSI = ANSI_CODE();

  HistogramCollection* fHistSet    = new HistogramCollection("noCut");


  HistogramCollection* fHistSet_0    = new HistogramCollection("case0");

  HistogramCollection* fHistSet_1    = new HistogramCollection("case1");
  HistogramCollection* fHistSet_2    = new HistogramCollection("case2");
  HistogramCollection* fHistSet_3    = new HistogramCollection("case3");

  HistogramCollection* fHistSet_4    = new HistogramCollection("case4");
  HistogramCollection* fHistSet_5    = new HistogramCollection("case5");
  HistogramCollection* fHistSet_6    = new HistogramCollection("case6");

  HistogramCollection* fHistSet_7    = new HistogramCollection("case7");
  HistogramCollection* fHistSet_8    = new HistogramCollection("case8");
  HistogramCollection* fHistSet_9    = new HistogramCollection("case9");

  HistogramCollection* fHistSet_10   = new HistogramCollection("case10");
  HistogramCollection* fHistSet_11   = new HistogramCollection("case11");
  HistogramCollection* fHistSet_12   = new HistogramCollection("case12");

  HistogramCollection* fHistSet_13   = new HistogramCollection("case13");
  HistogramCollection* fHistSet_14   = new HistogramCollection("case14");
  HistogramCollection* fHistSet_15   = new HistogramCollection("case15");

  HistogramCollection* fHistSet_16   = new HistogramCollection("case16");
  HistogramCollection* fHistSet_17   = new HistogramCollection("case17");
  HistogramCollection* fHistSet_18   = new HistogramCollection("case18");
  HistogramCollection* fHistSet_19   = new HistogramCollection("case19");

  HistogramCollection* fHistSet_20   = new HistogramCollection("case20");
  HistogramCollection* fHistSet_21   = new HistogramCollection("case21");
  HistogramCollection* fHistSet_22   = new HistogramCollection("case22");
  HistogramCollection* fHistSet_23   = new HistogramCollection("case23");

  HistogramCollection* fHistSet_24   = new HistogramCollection("case24");
  HistogramCollection* fHistSet_25   = new HistogramCollection("case25"); 
  HistogramCollection* fHistSet_26   = new HistogramCollection("case26");
  HistogramCollection* fHistSet_27   = new HistogramCollection("case27");

  std::vector<std::vector<double>> fIntADC_CEREN;
  std::vector<std::vector<double>> fIntADC_SCINT;

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fIntADC_CEREN.push_back(tTEMPVEC);
  }

  for(int i = 0; i < 9; i++) {
    std::vector<double> tTEMPVEC = {0., 0., 0., 0.};
    fIntADC_SCINT.push_back(tTEMPVEC);
  }

  std::vector<double> fIntADC_AUX = {0., 0., 0., 0., 0.}; // CC1 CC2 PC MC TC
  std::vector<double> fPeakADC_AUX = {0., 0., 0., 0., 0.};

  std::vector<float> fDWCPOS = {0., 0., 0., 0.};

  TFile* fRawFile = new TFile(Form("./RAW/Run12196.root"), "READ");
  
  TTree* fTree = (TTree*)fRawFile->Get("data");
  // fTree->SetDirectory(0);
  // fTree->Print();

  double test = 0;
  // fTree->SetBranchAddress("test", &test);

  // fRawFile->Close();

  for (int iModule = 0; iModule < 9; iModule++) {
    for (int iTower = 0; iTower < 4; iTower++) {
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_C", iModule + 1, iTower + 1), &(fIntADC_CEREN[iModule][iTower]));
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_S", iModule + 1, iTower + 1), &(fIntADC_SCINT[iModule][iTower]));
    }
  }
  
  double fPS = 0;
  double fMC = 0;

  fTree->SetBranchAddress((TString)Form("PeakADC_CC1"), &(fPeakADC_AUX.at(0)));
  fTree->SetBranchAddress((TString)Form("PeakADC_CC2"), &(fPeakADC_AUX.at(1)));
  fTree->SetBranchAddress((TString)Form("PeakADC_PC"), &(fPeakADC_AUX.at(2)));
  fTree->SetBranchAddress((TString)Form("PeakADC_MC"), &(fPeakADC_AUX.at(3)));
  fTree->SetBranchAddress((TString)Form("PeakADC_TC"), &(fPeakADC_AUX.at(4)));

  fTree->SetBranchAddress((TString)Form("DWC1X"), &(fDWCPOS.at(0)));
  fTree->SetBranchAddress((TString)Form("DWC1Y"), &(fDWCPOS.at(1)));
  fTree->SetBranchAddress((TString)Form("DWC2X"), &(fDWCPOS.at(2)));
  fTree->SetBranchAddress((TString)Form("DWC2Y"), &(fDWCPOS.at(3)));

  int fMaxEvent = fTree->GetEntries();

  // fMaxEvent = 100;

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

    fTree->GetEntry(i);

    // std::cout << fPeakADC_AUX.at(2) << " " << fPeakADC_AUX.at(3) << std::endl;
    // std::cout << i << "  " << fPS << " " << fMC << std::endl;

    fHistSet->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);

    if (fPeakADC_AUX[2] < 424.)
      continue;

    if (fPeakADC_AUX[3] > 38.)
      continue;

    if (!IsPassingDWC(fDWCPOS))
      continue;

    fHistSet_0->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 1)) fHistSet_1->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 2)) fHistSet_2->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 3)) fHistSet_3->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 4)) fHistSet_4->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 5)) fHistSet_5->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 6)) fHistSet_6->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 7)) fHistSet_7->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 8)) fHistSet_8->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 9)) fHistSet_9->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 10)) fHistSet_10->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 11)) fHistSet_11->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 12)) fHistSet_12->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 13)) fHistSet_13->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 14)) fHistSet_14->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 15)) fHistSet_15->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);

    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 16)) fHistSet_16->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 17)) fHistSet_17->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 18)) fHistSet_18->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 19)) fHistSet_19->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 20)) fHistSet_20->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 21)) fHistSet_21->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 22)) fHistSet_22->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 23)) fHistSet_23->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);

    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 24)) fHistSet_24->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 25)) fHistSet_25->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 26)) fHistSet_26->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);
    if (IsInRange(fDWCPOS.at(2), fDWCPOS.at(3), 27)) fHistSet_27->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fIntADC_CEREN[6][0], fIntADC_SCINT[6][0], fDWCPOS);

  }

  fRawFile->Close();
  TFile* fFile = new TFile(Form("./TEST/Run12196.root"), "RECREATE");

  fHistSet->Save(fFile);
  fHistSet_0->Save(fFile);
  fHistSet_1->Save(fFile);
  fHistSet_2->Save(fFile);
  fHistSet_3->Save(fFile);
  fHistSet_4->Save(fFile);
  fHistSet_5->Save(fFile);
  fHistSet_6->Save(fFile);
  fHistSet_7->Save(fFile);
  fHistSet_8->Save(fFile);
  fHistSet_9->Save(fFile);
  fHistSet_10->Save(fFile);
  fHistSet_11->Save(fFile);
  fHistSet_12->Save(fFile);
  fHistSet_13->Save(fFile);
  fHistSet_14->Save(fFile);
  fHistSet_15->Save(fFile);

  fHistSet_16->Save(fFile);
  fHistSet_17->Save(fFile);
  fHistSet_18->Save(fFile);
  fHistSet_19->Save(fFile);

  fHistSet_20->Save(fFile);
  fHistSet_21->Save(fFile);
  fHistSet_22->Save(fFile);
  fHistSet_23->Save(fFile);

  fHistSet_24->Save(fFile);
  fHistSet_25->Save(fFile);
  fHistSet_26->Save(fFile);
  fHistSet_27->Save(fFile);

  fFile->Close();

  return 1;
}

