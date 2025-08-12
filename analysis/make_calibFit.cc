
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

#include "TH1.h"
#include "TF1.h"

int make_calibFit() {

  TFile* fFile = new TFile("./CALIB/CALIB.root", "READ");
  TFile* fOutput = new TFile("./CALIB/CALIB_Fit.root", "RECREATE");

  std::vector<std::vector<double>> fCerenCalib = {};
  std::vector<std::vector<double>> fScintCalib = {};

  for (int iModule = 1; iModule <= 9; iModule++) {
    std::vector<double> fCerenCalib_Module = {0., 0., 0., 0.};
    fCerenCalib.push_back(fCerenCalib_Module);

    std::vector<double> fScintCalib_Module = {0., 0., 0., 0.};
    fScintCalib.push_back(fScintCalib_Module);
  }
  
  std::vector<int> fCerenRange = {25000, 55000};
  std::vector<int> fScintRange = {50000, 100000};
  
  for (int iModule = 1; iModule <= 9; iModule++) {
    for (int iTower = 1; iTower <= 4; iTower++) {
      std::string fCerenName = "M" + std::to_string(iModule) + "-T" + std::to_string(iTower) + "-C";
      auto tCerenHist = (TH1D*)fFile->Get(fCerenName.c_str())->Clone((TString)fCerenName + "Fit");

      TF1* fCerenFit = new TF1((TString)"fCerenFit_" + fCerenName, "gaus", fCerenRange[0], fCerenRange[1]);
      tCerenHist->Fit(fCerenFit, "R");

      fOutput->cd();
      tCerenHist->Write();
      fCerenFit->Write();

      std::string fScintName = "M" + std::to_string(iModule) + "-T" + std::to_string(iTower) + "-S";
      auto tScintHist = (TH1D*)fFile->Get(fScintName.c_str())->Clone((TString)fScintName + "Fit");

      TF1* fScintFit = new TF1((TString)"fScintFit_" + fScintName, "gaus", fScintRange[0], fScintRange[1]);
      tScintHist->Fit(fScintFit, "R");

      fOutput->cd();
      tScintHist->Write();
      fScintFit->Write();

      fCerenCalib[iModule - 1][iTower - 1] = fCerenFit->GetParameter(1);
      fScintCalib[iModule - 1][iTower - 1] = fScintFit->GetParameter(1);
    }
  }

  std::cout << "Cerenkov Calibration:" << std::endl;
  for (int iModule = 1; iModule <= 9; iModule++) {
    std::cout << " M" << iModule << " ";
    for (int iTower = 1; iTower <= 4; iTower++) {
      std::cout <<  60. / fCerenCalib[iModule - 1][iTower - 1] << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "Scintillator Calibration:" << std::endl;
  for (int iModule = 1; iModule <= 9; iModule++) {
    std::cout << " M1 ";
    for (int iTower = 1; iTower <= 4; iTower++) {
      std::cout <<  60. / fScintCalib[iModule - 1][iTower - 1] << " ";
    }
    std::cout << std::endl;
  }

  return 1;
}