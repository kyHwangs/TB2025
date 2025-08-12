
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


void MakeReconstruction(int fRunNumber) {
    
  ANSI_CODE ANSI = ANSI_CODE();


  std::vector<double> fPeakADC_AUX = {0., 0., 0., 0., 0.}; // CC1 CC2 PC MC TC
  std::vector<float> fDWCPOS = {0., 0., 0., 0.};

  std::vector<int> fLC_ID             = {14, 15, 16, 20}; 
  std::vector<double> fPeakADC_LC     = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<double> fIntADC_LC      = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<TH1D*> fHistPeakADC_LC = {};
  std::vector<TH1D*> fHistIntADC_LC = {};

  TFile* fRawFile = new TFile(Form("./RAW/LCCALIB/Run%d.root", fRunNumber), "READ");
  
  TTree* fTree = (TTree*)fRawFile->Get("data");

  for (int i = 0; i < fLC_ID.size(); i++) {
    fTree->SetBranchAddress((TString)Form("PeakADC_LC%d", fLC_ID.at(i)), &(fPeakADC_LC[i]));
    fTree->SetBranchAddress((TString)Form("IntADC_LC%d", fLC_ID.at(i)), &(fIntADC_LC[i]));

    fHistPeakADC_LC.push_back(new TH1D((TString)Form("PeakADC_LC%d", fLC_ID.at(i)), ";PeakADC;nEvents", 1024, 0., 4096.));
    fHistIntADC_LC.push_back(new TH1D((TString)Form("IntADC_LC%d", fLC_ID.at(i)), ";IntADC;nEvents", 440, -30000., 300000.));
    fHistPeakADC_LC.at(i)->SetDirectory(0);
    fHistIntADC_LC.at(i)->SetLineColor(0);
  }

  std::cout << fHistIntADC_LC.size() << std::endl;
  std::cout << fHistPeakADC_LC.size() << std::endl;
  std::cout << fLC_ID.size() << std::endl;

  fTree->SetBranchAddress((TString)Form("PeakADC_CC1"), &(fPeakADC_AUX[0]));
  fTree->SetBranchAddress((TString)Form("PeakADC_CC2"), &(fPeakADC_AUX[1]));
  fTree->SetBranchAddress((TString)Form("PeakADC_PC"), &(fPeakADC_AUX[2]));
  fTree->SetBranchAddress((TString)Form("PeakADC_MC"), &(fPeakADC_AUX[3]));
  fTree->SetBranchAddress((TString)Form("PeakADC_TC"), &(fPeakADC_AUX[4]));

  fTree->SetBranchAddress((TString)Form("DWC1X"), &(fDWCPOS[0]));
  fTree->SetBranchAddress((TString)Form("DWC1Y"), &(fDWCPOS[1]));
  fTree->SetBranchAddress((TString)Form("DWC2X"), &(fDWCPOS[2]));
  fTree->SetBranchAddress((TString)Form("DWC2Y"), &(fDWCPOS[3]));

  int fMaxEvent = fTree->GetEntries();
  
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

    if (!IsPassingDWC(fDWCPOS))
      continue;

    if (fPeakADC_AUX[2] < 80. || fPeakADC_AUX[2] > 202.)
      continue;

    if (fPeakADC_AUX[3] < 50.)
      continue;

    if (fPeakADC_AUX[0] > 50.)
      continue;

    for (int i = 0; i < fLC_ID.size(); i++) {
      fHistPeakADC_LC.at(i)->Fill(fPeakADC_LC[i]);
      fHistIntADC_LC.at(i)->Fill(fIntADC_LC[i]);
    }
  }

  fRawFile->Close();
  
  std::cout << fHistPeakADC_LC[0]->GetEntries() << std::endl;
  // std::cout << fHistIntADC_LC[0]->GetEntries() << std::endl;

  TFile* fFile = new TFile(Form("./CALIB_LC/Run%d.root", fRunNumber), "RECREATE");
  fFile->cd();

  for (int i = 0; i < fLC_ID.size(); i++) {
    fHistPeakADC_LC.at(i)->Write();
    // fHistIntADC_LC.at(i)->Write();
  }

  fFile->Close();
}

int main(int argc, char* argv[]) {
  gStyle->SetOptFit(1);

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  int fRunNum = 0;
  fObj->GetVariable("RunNumber", &fRunNum);

  MakeReconstruction(fRunNum);

  return 1;
}