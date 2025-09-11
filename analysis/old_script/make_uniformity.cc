
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

std::pair<float, float> GetCenterPosition(int fCase) {

  fCase = fCase - 1;

  std::vector<float> tHorizontalList = {0., 24.3, 48.5, 72.8, 97.};
  std::vector<float> tVerticalList = {50., 25., 0.};

  std::cout << fCase << " " << fCase % 5 << " " << fCase / 5 << std::endl;

  return std::make_pair(tHorizontalList.at(fCase % 5), tVerticalList.at(fCase / 5));
}

class HistogramCollection {
  public:
    HistogramCollection(std::string fName_) {
      fName = fName_;
      fEntries = 0;

      fHistCeren = new TH1D((TString)Form("Ceren_%s", fName.c_str()), ";GeV;nEvents", 400, 0., 200.);
      fHistScint = new TH1D((TString)Form("Scint_%s", fName.c_str()), ";GeV;nEvents", 400, 0., 200.);

      fHistPS = new TH1D((TString)Form("PS_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistMC = new TH1D((TString)Form("MC_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistTC = new TH1D((TString)Form("TC_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistCC2 = new TH1D((TString)Form("CC2_%s", fName.c_str()), ";PeakADC;nEvents", 1024, 0., 4096.);

      fHistFDWC = new TH2D((TString)Form("FDWC_%s", fName.c_str()), ";X [mm];Y [mm]", 200, -50., 150., 150, -50., 100.);
      fHistLDWC = new TH2D((TString)Form("LDWC_%s", fName.c_str()), ";X [mm];Y [mm]", 200, -50., 150., 150, -50., 100.);
    }

    void Fill_PS(double value) {
      fHistPS->Fill(value);
    }
    void Fill_MC(double value) {
      fHistMC->Fill(value);
    }
    void Fill_CC2(double value) {
      fHistCC2->Fill(value);
    }
    void Fill_TC(double value) {
      fHistTC->Fill(value);
    }


    void Fill_Tower(double value_ceren, double value_scint) {
      fHistCeren->Fill(value_ceren);
      fHistScint->Fill(value_scint);
    }

    void Fill_DWC(std::vector<float> fDWC) {

      fHistFDWC->Fill(fDWC.at(0), fDWC.at(1));
      fHistLDWC->Fill(fDWC.at(2), fDWC.at(3));
    }

    void Fill(double ps, double mc, double tc, double cc2, double ceren, double scint, std::vector<float> fDWC) {
      Fill_PS(ps); 
      Fill_MC(mc);
      Fill_TC(tc);
      Fill_CC2(cc2);
      Fill_Tower(ceren, scint);
      Fill_DWC(fDWC); 
      fEntries++;
    }

    void Save(TFile* fFile) {

      if (fEntries == 0)
        return;

      fFile->cd();

      fFile->mkdir(fName.c_str());
      fFile->cd(fName.c_str());

      fHistCeren->Write();
      fHistScint->Write();

      fHistPS->Write();
      fHistMC->Write();
      fHistTC->Write();
      fHistCC2->Write();
      fHistFDWC->Write();
      fHistLDWC->Write();
    }

    std::string GetName() {
      return fName;
    }

  private:
    std::string fName;
    int fEntries;

    TH1D* fHistCeren;
    TH1D* fHistScint;
    
    TH2D* fHistFDWC;
    TH2D* fHistLDWC;

    TH1D* fHistPS;
    TH1D* fHistMC;
    TH1D* fHistTC;
    TH1D* fHistCC2;
};

static std::vector<std::vector<double>> fCalibCeren = {
  {0.001203555, 0.001212408, 0.001243669, 0.00120427},
  {0.001114422, 0.001210115, 0.001134603, 0.001228336},
  {0.001167599, 0.001298819, 0.001253319, 0.001245581},
  {0.00119433, 0.001002412, 0.001195593, 0.001178153},
  {0.001322633, 0.00120786, 0.001544522, 0.001155995},
  {0.001242049, 0.001142602, 0.001283866, 0.001343688},
  {0.00124939, 0.001223514, 0.001252899, 0.001233478},
  {0.001276618, 0.001222038, 0.001211442, 0.001226694},
  {0.00123999, 0.001236395, 0.001227406, 0.001174254}
};

static std::vector<std::vector<double>> fCalibScint = {
  {0.000608168, 0.000599951, 0.000632579, 0.00063748},
  {0.000603563, 0.000601509, 0.000624896, 0.000654066},
  {0.000604899, 0.000624518, 0.000621517, 0.000645067},
  {0.000631232, 0.000598297, 0.00061746, 0.000638033},
  {0.000628109, 0.000577394, 0.000777161, 0.000660589},
  {0.000642289, 0.000637514, 0.000651367, 0.000665772},
  {0.000631831, 0.000611742, 0.000592046, 0.000641848},
  {0.000625958, 0.000599988, 0.000612211, 0.000637865},
  {0.000631297, 0.000614992, 0.000635913, 0.000655751}
};

static double fScaleCeren =  60. / 62.53;
static double fScaleScint = 60. / 60.67;

// static std::vector<std::vector<double>> fCalibCeren = {
//   {0.0014847, 0.00149202, 0.00153191, 0.0014805}, 
//   {0.00137043, 0.00149163, 0.00139693, 0.00151534}, 
//   {0.00143401, 0.00159629, 0.00154317, 0.00153666}, 
//   {0.00146555, 0.00122406, 0.00147115, 0.00144988}, 
//   {0.00162606, 0.00148574, 0.00190002, 0.00142279}, 
//   {0.00152799, 0.00140535, 0.00156408, 0.00165267}, 
//   {0.00154709, 0.00150789, 0.00154376, 0.00151565}, 
//   {0.00157038, 0.00150369, 0.00149204, 0.00150807}, 
//   {0.00152505, 0.00152192, 0.00151984, 0.00144744}
// };

// static std::vector<std::vector<double>> fCalibScint = {
//   {0.000743043, 0.000738997, 0.000780472, 0.000784184},
//   {0.00074328, 0.000744483, 0.000771634, 0.000807684},
//   {0.00074252, 0.00076941, 0.000765835, 0.000795202},
//   {0.000777873, 0.000735694, 0.000759559, 0.000784774},
//   {0.00077507, 0.000712786, 0.000958204, 0.000814253},
//   {0.000789354, 0.000785964, 0.000800134, 0.000820156},
//   {0.000778996, 0.000753416, 0.000730685, 0.000791134},
//   {0.000773766, 0.000740452, 0.000755237, 0.000783835},
//   {0.000780019, 0.000758861, 0.00078369, 0.000806755}
// };

// static double fScaleCeren = 60. / 78.7;
// static double fScaleScint = 60. / 76.04;

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

void MakeReconstruction(std::string fSuffix, int fRunNumber, int fCase) {
    
  ANSI_CODE ANSI = ANSI_CODE();

  std::vector<std::string> fModule = {};
  std::vector<std::pair<int, int>> fModuleList = {};

  for (int i = 0; i < 9; i++)
    for (int j = 0; j < 4; j++)
      fModuleList.push_back(std::make_pair(i + 1, j + 1));

  HistogramCollection* fHistSetUniformityTotal = new HistogramCollection(Form("Uniformity_Total"));
  std::vector<std::vector<HistogramCollection*>> fHistSetUniformity;
  for (int iY = 0; iY < 20; iY++) {
    std::vector<HistogramCollection*> tTEMPVEC;
    for (int iX = 0; iX < 30; iX++)
      tTEMPVEC.push_back(new HistogramCollection(Form("Uniformity_%d_%d", iX, iY)));

    fHistSetUniformity.push_back(tTEMPVEC);
  }

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

  std::vector<double> fPeakADC_AUX = {0., 0., 0., 0., 0.};
  std::vector<float> fDWCPOS = {0., 0., 0., 0.};
  auto [tCenterX, tCenterY] = GetCenterPosition(fCase);

  TFile* fRawFile = new TFile(Form("./RAW/UNIFORMITY/Run%d.root", fRunNumber), "READ");
  
  TTree* fTree = (TTree*)fRawFile->Get("data");

  for (int iModule = 0; iModule < 9; iModule++) {
    for (int iTower = 0; iTower < 4; iTower++) {
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_C", iModule + 1, iTower + 1), &(fIntADC_CEREN[iModule][iTower]));
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_S", iModule + 1, iTower + 1), &(fIntADC_SCINT[iModule][iTower]));
    }
  }
  
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

    double tEnergyCeren = 0.;
    double tEnergyScint = 0.;
    for (auto [iModule, iTower] : fModuleList) {
      if (fIntADC_CEREN[iModule - 1][iTower - 1] > 0) tEnergyCeren += fIntADC_CEREN[iModule - 1][iTower - 1] * fScaleCeren * fCalibCeren[iModule - 1][iTower - 1];
      // tEnergyCeren += fIntADC_CEREN[iModule - 1][iTower - 1] * fScaleCeren * fCalibCeren[iModule - 1][iTower - 1];
      if (fIntADC_SCINT[iModule - 1][iTower - 1] > 0) tEnergyScint += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];
      // tEnergyScint += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];
    }

    if (!IsPassingDWC(fDWCPOS))
      continue;

    if (fPeakADC_AUX[2] < 424.)
      continue;

    if (fPeakADC_AUX[3] > 38.)
      continue;

    // if (fPeakADC_AUX[1] < 70.)
    //   continue;

    // auto [tCenterX, tCenterY] = GetCenterPosition(fCase);
    std::vector<float> tDWC = {fDWCPOS[0] + tCenterX, fDWCPOS[1] + tCenterY, fDWCPOS[2] + tCenterX, fDWCPOS[3] + tCenterY};

    fHistSetUniformityTotal->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fPeakADC_AUX[4], fPeakADC_AUX[1],tEnergyCeren, tEnergyScint, tDWC);

    int fX1Index = (int)((tDWC[0] + 25.) / 5.);
    int fY1Index = (int)((tDWC[1] + 25.) / 5.);

    int fX2Index = (int)((tDWC[2] + 25.) / 5.);
    int fY2Index = (int)((tDWC[3] + 25.) / 5.);

    if (fX1Index < 0 || fX1Index >= 30 || fY1Index < 0 || fY1Index >= 20)
      continue;

    if (fX2Index < 0 || fX2Index >= 30 || fY2Index < 0 || fY2Index >= 20)
      continue;

    fHistSetUniformity[fY2Index][fX2Index]->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], fPeakADC_AUX[4], fPeakADC_AUX[1],tEnergyCeren, tEnergyScint, tDWC);
  }

  fRawFile->Close();

  TFile* fFile = new TFile(Form("./UNIFORMITY/%s/ROOT/Run%d.root", fSuffix.c_str(), fRunNumber), "RECREATE");
  
  fHistSetUniformityTotal->Save(fFile);
  for (int iY = 0; iY < 20; iY++)
    for (int iX = 0; iX < 30; iX++)
      fHistSetUniformity[iY][iX]->Save(fFile);

  fFile->Close();
}


int main(int argc, char* argv[]) {
  gStyle->SetOptFit(1);

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  std::string fSuffix = "";
  fObj->GetVariable("suffix", &fSuffix);

  int fRunNumber = 0;
  fObj->GetVariable("RunNumber", &fRunNumber);

  int fCase = 0;
  fObj->GetVariable("Case", &fCase);

  fs::path fDirBase(Form("./UNIFORMITY/%s", fSuffix.c_str()));
  if ( !fs::exists(fDirBase) ) fs::create_directory(fDirBase);
  
  fs::path fDirPlot(Form("./UNIFORMITY/%s/PLOT", fSuffix.c_str()));
  if ( !fs::exists(fDirPlot) ) fs::create_directory(fDirPlot);
  
  fs::path fDirPlotSection(Form("./UNIFORMITY/%s/PLOT/SECTION", fSuffix.c_str()));
  if ( !fs::exists(fDirPlotSection) ) fs::create_directory(fDirPlotSection);

  fs::path fDirRoot(Form("./UNIFORMITY/%s/ROOT", fSuffix.c_str()));
  if ( !fs::exists(fDirRoot) ) fs::create_directory(fDirRoot);

  MakeReconstruction(fSuffix, fRunNumber, fCase);

  return 1;
}