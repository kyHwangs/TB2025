
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
    HistogramCollection(std::string fName_, int fEnergy_) {
      fName = fName_;
      fEnergy = fEnergy_;
      fIsFitted = false;

      fFitRangeMap = {
        {20, {0, 40}},
        {40, {10, 70}},
        {60, {20, 100}},
        {80, {30, 130}},
        {100, {40, 160}},
        {120, {40, 180}}
      };

      fHistCeren = new TH1D((TString)Form("Ceren_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);
      fHistScint = new TH1D((TString)Form("Scint_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);
      fHistScintLC = new TH1D((TString)Form("ScintLC_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);
      fHistSum = new TH1D((TString)Form("Sum_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -40., 400.);
      fHistDRCorr = new TH1D((TString)Form("DRCorr_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);
      fHistDRCorrLC = new TH1D((TString)Form("DRCorrLC_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);

      fHistScintVsCeren = new TH2D((TString)Form("ScintVsCeren_%s_%dGeV", fName.c_str(), fEnergy), ";Scint E [GeV];Ceren E [GeV]", 220, -20., 200., 220, -20., 200.);
      fHistScintLCVsCeren = new TH2D((TString)Form("ScintLCVsCereLC_%s_%dGeV", fName.c_str(), fEnergy), ";Scint E [GeV];Ceren E [GeV]", 220, -20., 200., 220, -20., 200.);

      if (fEnergy >= 100) {
        fHistCeren->Rebin(2);
        fHistScint->Rebin(2);
        fHistSum->Rebin(2);
        fHistDRCorr->Rebin(2);
      }

      fHistPS = new TH1D((TString)Form("PS_%s_%dGeV", fName.c_str(), fEnergy), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistMC = new TH1D((TString)Form("MC_%s_%dGeV", fName.c_str(), fEnergy), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistLC = new TH1D((TString)Form("LC_%s_%dGeV", fName.c_str(), fEnergy), ";GeV;nEvents", 440, -20., 200.);
      
      fHistCC1 = new TH1D((TString)Form("CC1_%s_%dGeV", fName.c_str(), fEnergy), ";PeakADC;nEvents", 1024, 0., 4096.);
      fHistCC2 = new TH1D((TString)Form("CC2_%s_%dGeV", fName.c_str(), fEnergy), ";PeakADC;nEvents", 1024, 0., 4096.);

      fHistFDWC = new TH2D((TString)Form("FDWC_%s_%dGeV", fName.c_str(), fEnergy), ";X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);
      fHistLDWC = new TH2D((TString)Form("LDWC_%s_%dGeV", fName.c_str(), fEnergy), ";X [mm];Y [mm]", 200, -50., 50., 200, -50., 50.);

      fHistDWCX = new TH2D((TString)Form("DWCX_%s_%dGeV", fName.c_str(), fEnergy), ";DWC 1 X [mm];DWC 2 X [mm]", 200, -50., 50., 200, -50., 50.);
      fHistDWCY = new TH2D((TString)Form("DWCY_%s_%dGeV", fName.c_str(), fEnergy), ";DWC 1 Y [mm];DWC 2 Y [mm]", 200, -50., 50., 200, -50., 50.);
    
      fHistDWCX_1D = new TH1D((TString)Form("DWCX_1D_%s_%dGeV", fName.c_str(), fEnergy), ";DWC1 X - DWC2 X [mm];nEvents", 200, -50., 50.);
      fHistDWCY_1D = new TH1D((TString)Form("DWCY_1D_%s_%dGeV", fName.c_str(), fEnergy), ";DWC1 Y - DWC2 Y [mm];nEvents", 200, -50., 50.);
    }

    void Fit() {

      auto [init, fin] = fFitRangeMap[fEnergy];
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;
      std::cout << "init: " << init << ", fin: " << fin << std::endl;

      fGausDRCorr = new TF1((TString)Form("fGausDRCorr_%s_%dGeV", fName.c_str(), fEnergy), "gaus", init, fin); 
      fGausDRCorr->SetLineColor(kBlack);
      fHistDRCorr->SetOption("p"); 
      fHistDRCorr->Fit(fGausDRCorr,"R");

      fGausDRCorrLC = new TF1((TString)Form("fGausDRCorrLC_%s_%dGeV", fName.c_str(), fEnergy), "gaus", init, fin); 
      fGausDRCorrLC->SetLineColor(kBlack);
      fHistDRCorrLC->SetOption("p"); 
      fHistDRCorrLC->Fit(fGausDRCorrLC,"R");


      fIsFitted = true;
    }

    double DualReadoutCorrection(double eC, double eS) { return (eS - 0.291 * eC) / (1 - 0.221); }

    void Fill_CC(double value1, double value2) {
      fHistCC1->Fill(value1);
      fHistCC2->Fill(value2);
    }

    void Fill_PS(double value) {
      fHistPS->Fill(value);
    }
    void Fill_MC(double value) {
      fHistMC->Fill(value);
    }
    void Fill_LC(double value) {
      fHistLC->Fill(value);
    }

    void Fill_Tower(double value_ceren, double value_scint, double value_scintLC) {
      fHistCeren->Fill(value_ceren);
      fHistScint->Fill(value_scint);
      fHistScintLC->Fill(value_scintLC);
      fHistSum->Fill(value_ceren + value_scint);
      fHistDRCorr->Fill(DualReadoutCorrection(value_ceren, value_scint));
      fHistDRCorrLC->Fill(DualReadoutCorrection(value_ceren, value_scintLC));
      fHistScintVsCeren->Fill(value_scint, value_ceren);
      fHistScintLCVsCeren->Fill(value_scintLC, value_ceren);
    }

    void Fill_DWC(std::vector<float> fDWC) {

      fHistFDWC->Fill(fDWC.at(0), fDWC.at(1));
      fHistLDWC->Fill(fDWC.at(2), fDWC.at(3));

      fHistDWCX->Fill(fDWC.at(0), fDWC.at(2));
      fHistDWCY->Fill(fDWC.at(1), fDWC.at(3));

      fHistDWCX_1D->Fill(fDWC.at(0) - fDWC.at(2));
      fHistDWCY_1D->Fill(fDWC.at(1) - fDWC.at(3));
    }

    void Fill(double ps, double mc, double LC, double ceren, double scint, double scintLC, std::vector<float> fDWC) {
      Fill_PS(ps); 
      Fill_MC(mc);
      Fill_LC(LC);
      Fill_Tower(ceren, scint, scintLC);
      Fill_DWC(fDWC); 
    }

    void Save(TFile* fFile) {
      fFile->cd();

      fFile->mkdir(fName.c_str());
      fFile->cd(fName.c_str());

      fHistCeren->Write();
      fHistScint->Write();
      fHistScintLC->Write();
      fHistDRCorr->Write();
      if (fIsFitted) fGausDRCorr->Write();
      fHistDRCorrLC->Write();
      if (fIsFitted) fGausDRCorrLC->Write();

      fHistScintVsCeren->Write();
      fHistScintLCVsCeren->Write();

      fHistPS->Write();
      fHistMC->Write();
      fHistLC->Write();
      fHistFDWC->Write();
      fHistLDWC->Write();
      fHistDWCX->Write();
      fHistDWCY->Write();
      fHistDWCX_1D->Write();
      fHistDWCY_1D->Write();
    }

  private:
    std::string fName;
    int fEnergy;

    std::map<int, std::pair<int, int>> fFitRangeMap;

    TF1* fGausDRCorr;
    TF1* fGausDRCorrLC;

    TH1D* fHistCeren;
    TH1D* fHistScint;
    TH1D* fHistScintLC;

    TH1D* fHistSum;
    TH1D* fHistDRCorr;
    TH1D* fHistDRCorrLC;

    TH2D* fHistScintVsCeren;
    TH2D* fHistScintLCVsCeren;
    
    TH2D* fHistFDWC;
    TH2D* fHistLDWC;
    TH2D* fHistDWCX;
    TH2D* fHistDWCY;
    TH1D* fHistDWCX_1D;
    TH1D* fHistDWCY_1D;

    TH1D* fHistPS;
    TH1D* fHistMC;
    TH1D* fHistLC;
    TH1D* fHistCC1;
    TH1D* fHistCC2;

    bool fIsFitted;
};

static std::vector<double> fCalibIndADCLC = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

static std::vector<double> fCalibPeakADCLC = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
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

  if (std::abs(fDWC.at(0)) > 5.)
    return false;

  if (std::abs(fDWC.at(1)) > 5.)
    return false;
  
  if (std::abs(fDWC.at(2)) > 5.)
    return false;
  
  if (std::abs(fDWC.at(3)) > 5.)
    return false;

  if (std::abs(fDWC.at(0) - fDWC.at(2)) > 4.)
    return false;

  if (std::abs(fDWC.at(1) - fDWC.at(3)) > 4.)
    return false;

  return true;
}


double GetResolutionError(double fResMean, double fResStdDev, double fResMeanErr, double fResStdDevErr) {

  return std::sqrt( (fResStdDevErr / fResStdDev)*(fResStdDevErr / fResStdDev) + (fResMeanErr / fResMean)*(fResMeanErr / fResMean) ) * (fResStdDev / fResMean);
}

void SetPlotFrame(TH1D* plot) {

	plot->SetStats(kFALSE);            
	plot->SetMarkerStyle(20);      
	plot->SetMarkerSize(0.);      
	plot->SetLineWidth(0);

	// plot->GetYaxis()->SetTitle( "#scale[1.4]{#font[42]{N_{evts} / 1 GeV}}" );
	plot->GetYaxis()->SetTitleFont(42);     
	plot->GetYaxis()->SetTitleSize(0.04);   
	plot->GetYaxis()->SetTitleOffset(1.2); 
	plot->GetYaxis()->SetLabelFont(42);
	plot->GetYaxis()->SetLabelSize(0.03); 

	// plot->GetXaxis()->SetTitle( "LHE dimuon mass [GeV]" ); 
	plot->GetXaxis()->SetTitleFont(42);               
	plot->GetXaxis()->SetTitleSize(0.04);           
	plot->GetXaxis()->SetTitleOffset( 1.0 );  

	plot->GetXaxis()->SetLabelFont(42);               
	plot->GetXaxis()->SetLabelSize(0.03);             
	plot->GetXaxis()->SetLabelOffset(0.01);           
	plot->GetXaxis()->SetLabelColor(1);               
	plot->GetXaxis()->SetMoreLogLabels();             
	plot->GetXaxis()->SetNoExponent(); 

}

void MakeReconstruction(std::string fSuffix, int fEnergy, std::vector<std::string> fModule) {
    
  ANSI_CODE ANSI = ANSI_CODE();

  std::vector<std::pair<int, int>> fModuleList = {};

  if (fModule.size() == 0) {

    for (int i = 0; i < 9; i++)
      for (int j = 0; j < 4; j++)
        fModuleList.push_back(std::make_pair(i + 1, j + 1));

  } else {

    for (int i = 0; i < fModule.size(); i++)
      fModuleList.push_back(std::make_pair((int)(fModule.at(i).at(1) - '0'), (int)(fModule.at(i).at(4) - '0')));

  }

  HistogramCollection* fHistSet_noCut = new HistogramCollection("noCut", fEnergy);
  HistogramCollection* fHistSet_DWC   = new HistogramCollection("DWC", fEnergy);
  HistogramCollection* fHistSet_PS    = new HistogramCollection("PS", fEnergy);
  HistogramCollection* fHistSet_MC    = new HistogramCollection("MC", fEnergy);
  HistogramCollection* fHistSet_CC    = new HistogramCollection("CC", fEnergy);

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
  std::vector<float> fLC = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<double> fPeakADC_LC = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<double> fIntADC_LC = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  std::vector<int> fLC_ID = {2, 4, 8, 10, 3, 5, 7, 9, 11, 12, 13, 19, 14, 15, 16, 20}; 

  TFile* fRawFile = new TFile(Form("./RAW/M5T2_Hadron/MERGED/h_%dGeV.root", fEnergy), "READ");
  
  TTree* fTree = (TTree*)fRawFile->Get("data");
  // fTree->SetDirectory(0);

  for (int iModule = 0; iModule < 9; iModule++) {
    for (int iTower = 0; iTower < 4; iTower++) {
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_C", iModule + 1, iTower + 1), &(fIntADC_CEREN[iModule][iTower]));
      fTree->SetBranchAddress((TString)Form("IntADC_M%d_T%d_S", iModule + 1, iTower + 1), &(fIntADC_SCINT[iModule][iTower]));
    }
  }

  for (int i = 0; i < fLC_ID.size(); i++) {
    fTree->SetBranchAddress((TString)Form("PeakADC_LC%d", fLC_ID.at(i)), &(fPeakADC_LC.at(i)));
    fTree->SetBranchAddress((TString)Form("IntADC_LC%d", fLC_ID.at(i)), &(fIntADC_LC.at(i)));
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
    double tEnergyScintLC = 0.;
    double tLC = 0;

    for (auto [iModule, iTower] : fModuleList) {
      // if (fIntADC_CEREN[iModule - 1][iTower - 1] > 0) tEnergyCeren += fIntADC_CEREN[iModule - 1][iTower - 1] * fScaleCeren * fCalibCeren[iModule - 1][iTower - 1];
      tEnergyCeren += fIntADC_CEREN[iModule - 1][iTower - 1] * fScaleCeren * fCalibCeren[iModule - 1][iTower - 1];
      
      // if (fIntADC_SCINT[iModule - 1][iTower - 1] > 0) tEnergyScint += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];
      tEnergyScint += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];

      // if (fIntADC_SCINT[iModule - 1][iTower - 1] > 0) tEnergyScintLC += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];
      tEnergyScintLC += fIntADC_SCINT[iModule - 1][iTower - 1] * fScaleScint * fCalibScint[iModule - 1][iTower - 1];
    }

    for (int iLC = 0; iLC < fLC_ID.size(); iLC++) {
      // if (fIntADC_LC.at(iLC) > 0) tLC += fPeakADC_LC.at(iLC) * fCalibPeakADCLC.at(iLC);
      tLC += fPeakADC_LC.at(iLC) * fCalibPeakADCLC.at(iLC);

      // if (fIntADC_LC.at(iLC) > 0) tLC += fIntADC_LC.at(iLC) * fCalibIndADCLC.at(iLC);
      // tLC += fIntADC_LC.at(iLC) * fCalibIndADCLC.at(iLC);
    }

    tEnergyScintLC += tLC;

    fHistSet_noCut->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], tLC, tEnergyCeren, tEnergyScint, tEnergyScintLC, fDWCPOS);
    fHistSet_noCut->Fill_CC(fPeakADC_AUX[0], fPeakADC_AUX[1]);

    if (!IsPassingDWC(fDWCPOS))
      continue;

    fHistSet_DWC->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], tLC, tEnergyCeren, tEnergyScint, tEnergyScintLC, fDWCPOS);
    fHistSet_DWC->Fill_CC(fPeakADC_AUX[0], fPeakADC_AUX[1]);

    if (fPeakADC_AUX[2] < 80. || fPeakADC_AUX[2] > 202.)
      continue;

    fHistSet_PS->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], tLC, tEnergyCeren, tEnergyScint, tEnergyScintLC, fDWCPOS);
    fHistSet_PS->Fill_CC(fPeakADC_AUX[0], fPeakADC_AUX[1]);

    if (fPeakADC_AUX[3] > 38.)
      continue;

    fHistSet_MC->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], tLC, tEnergyCeren, tEnergyScint, tEnergyScintLC, fDWCPOS);
    fHistSet_MC->Fill_CC(fPeakADC_AUX[0], fPeakADC_AUX[1]);

    if (fPeakADC_AUX[0] > 86.)
      continue;

    if (fPeakADC_AUX[1] < 136.)
      continue;

    fHistSet_CC->Fill(fPeakADC_AUX[2], fPeakADC_AUX[3], tLC, tEnergyCeren, tEnergyScint, tEnergyScintLC, fDWCPOS);
    fHistSet_CC->Fill_CC(fPeakADC_AUX[0], fPeakADC_AUX[1]);
  }

  fRawFile->Close();
  
  fHistSet_CC->Fit();

  TFile* fFile = new TFile(Form("./PLOT/%s/ROOT/h_%dGeV.root", fSuffix.c_str(), fEnergy), "RECREATE");

  fHistSet_noCut->Save(fFile);
  fHistSet_DWC->Save(fFile);
  fHistSet_PS->Save(fFile);
  fHistSet_MC->Save(fFile);
  fHistSet_CC->Save(fFile);

  fFile->Close();
}


void MakeResolution(std::string fSuffix, std::vector<int> fEnergy) {

  std::string fBaseDir = Form("./PLOT/%s/ROOT/", fSuffix.c_str());

  std::vector<double> fRes_Xaxis;
  std::vector<double> fRes_XaxisError;

  std::vector<double> fRes_Yaxis_Ceren;
  std::vector<double> fRes_YaxisError_Ceren;
  
  std::vector<double> fRes_Yaxis_Scint;
  std::vector<double> fRes_YaxisError_Scint;
  
  std::vector<double> fRes_Yaxis_Summed;
  std::vector<double> fRes_YaxisError_Summed;

  std::vector<double> fLin_Xaxis;
  std::vector<double> fLin_XaxisError;
  
  std::vector<double> fLin_Xaxis_Ceren;
  std::vector<double> fLin_Xaxis_Scint;
  std::vector<double> fLin_Xaxis_Summed;

  std::vector<double> fLin_Yaxis_Ceren;
  std::vector<double> fLin_YaxisError_Ceren;

  std::vector<double> fLin_Yaxis_Scint;
  std::vector<double> fLin_YaxisError_Scint;

  std::vector<double> fLin_Yaxis_Summed;
  std::vector<double> fLin_YaxisError_Summed;


  for (int iEnergy = 0; iEnergy < fEnergy.size(); iEnergy++) {
    // std::cout << fEnergy.at(iEnergy) << std::endl;
    // std::cout << Form("%se_%dGeV.root", fBaseDir.c_str(), fEnergy.at(iEnergy)) << std::endl;
    TFile* fFile = new TFile(Form("%sh_%dGeV.root", fBaseDir.c_str(), fEnergy.at(iEnergy)), "READ");
        
    TString fCerenName = (TString)Form("MC/Ceren_MC_%dGeV", fEnergy.at(iEnergy));
    TString fScintName = (TString)Form("MC/Scint_MC_%dGeV", fEnergy.at(iEnergy));
    TString fSummedName = (TString)Form("MC/Sum_MC_%dGeV", fEnergy.at(iEnergy));

    TH1D* fCeren = (TH1D*)fFile->Get(fCerenName);
    TH1D* fScint = (TH1D*)fFile->Get(fScintName);
    TH1D* fSummed = (TH1D*)fFile->Get(fSummedName);

    TF1* fGausCeren = (TF1*)fFile->Get((TString)Form("MC/fGausCeren_MC_%dGeV", fEnergy.at(iEnergy)));
    TF1* fGausScint = (TF1*)fFile->Get((TString)Form("MC/fGausScint_MC_%dGeV", fEnergy.at(iEnergy)));
    TF1* fGausSummed = (TF1*)fFile->Get((TString)Form("MC/fGausSum_MC_%dGeV", fEnergy.at(iEnergy)));

    fRes_Xaxis.push_back(1. / std::sqrt(fEnergy.at(iEnergy)));
    fRes_XaxisError.push_back(0.);

    fRes_Yaxis_Ceren.push_back(fGausCeren->GetParameter(2) / fGausCeren->GetParameter(1));
    fRes_YaxisError_Ceren.push_back(GetResolutionError(fGausCeren->GetParameter(1), fGausCeren->GetParameter(2), fGausCeren->GetParError(1), fGausCeren->GetParError(2)));

    fRes_Yaxis_Scint.push_back(fGausScint->GetParameter(2) / fGausScint->GetParameter(1));
    fRes_YaxisError_Scint.push_back(GetResolutionError(fGausScint->GetParameter(1), fGausScint->GetParameter(2), fGausScint->GetParError(1), fGausScint->GetParError(2)));

    fRes_Yaxis_Summed.push_back(fGausSummed->GetParameter(2) / fGausSummed->GetParameter(1));
    fRes_YaxisError_Summed.push_back(GetResolutionError(fGausSummed->GetParameter(1), fGausSummed->GetParameter(2), fGausSummed->GetParError(1), fGausSummed->GetParError(2)));

    fLin_Xaxis.push_back(fEnergy.at(iEnergy));
    fLin_XaxisError.push_back(0.);

    fLin_Xaxis_Ceren.push_back(fEnergy.at(iEnergy));

    fLin_Xaxis_Scint.push_back(fEnergy.at(iEnergy));

    fLin_Xaxis_Summed.push_back(fEnergy.at(iEnergy));


    fLin_Yaxis_Ceren.push_back(fGausCeren->GetParameter(1) / (double)fEnergy.at(iEnergy));
    fLin_YaxisError_Ceren.push_back(fGausCeren->GetParError(1) / (double)fEnergy.at(iEnergy));

    fLin_Yaxis_Scint.push_back(fGausScint->GetParameter(1) / (double)fEnergy.at(iEnergy));
    fLin_YaxisError_Scint.push_back(fGausScint->GetParError(1) / (double)fEnergy.at(iEnergy));

    fLin_Yaxis_Summed.push_back(fGausSummed->GetParameter(1) / (2 * (double)fEnergy.at(iEnergy)));
    fLin_YaxisError_Summed.push_back(fGausSummed->GetParError(1) / (2 * (double)fEnergy.at(iEnergy)));
  }


	TGraphErrors* fLinGrCeren = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis_Ceren[0]), &(fLin_Yaxis_Ceren[0]), &(fLin_XaxisError[0]), &(fLin_YaxisError_Ceren[0])); 
	TGraphErrors* fLinGrScint = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis_Scint[0]), &(fLin_Yaxis_Scint[0]), &(fLin_XaxisError[0]), &(fLin_YaxisError_Scint[0])); 
  TGraphErrors* fLinGrSummed = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis_Summed[0]), &(fLin_Yaxis_Summed[0]), &(fLin_XaxisError[0]), &(fLin_YaxisError_Summed[0])); 
  
  fLinGrCeren->SetTitle(""); 
	fLinGrCeren->SetMarkerColor(kBlue); 
  fLinGrCeren->SetMarkerSize(1.2); 
  fLinGrCeren->SetMarkerStyle(20); 
  fLinGrCeren->SetLineWidth(2); 
  fLinGrCeren->SetLineColor(kBlue); 
  
  fLinGrScint->SetTitle(""); 
	fLinGrScint->SetMarkerColor(kRed); 
  fLinGrScint->SetMarkerSize(1.2); 
  fLinGrScint->SetMarkerStyle(20); 
  fLinGrScint->SetLineWidth(2); 
  fLinGrScint->SetLineColor(kRed); 

  fLinGrSummed->SetTitle(""); 
	fLinGrSummed->SetMarkerColor(kBlack); 
  fLinGrSummed->SetMarkerSize(1.2); 
  fLinGrSummed->SetMarkerStyle(20); 
  fLinGrSummed->SetLineWidth(2); 
  fLinGrSummed->SetLineColor(kBlack); 

  TGraphErrors* fResNoiseGrCeren = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis[0]), &(fRes_Yaxis_Ceren[0]), &(fLin_XaxisError[0]), &(fRes_YaxisError_Ceren[0])); 

	TGraphErrors* fResNoiseGrScint = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis[0]), &(fRes_Yaxis_Scint[0]), &(fLin_XaxisError[0]), &(fRes_YaxisError_Scint[0])); 
	
  TGraphErrors* fResNoiseGrSummed = new TGraphErrors(fLin_Xaxis.size(), &(fLin_Xaxis[0]), &(fRes_Yaxis_Summed[0]), &(fLin_XaxisError[0]), &(fRes_YaxisError_Summed[0])); 
  
  fResNoiseGrCeren->SetTitle(""); 
  fResNoiseGrCeren->SetStats(0); 
	fResNoiseGrCeren->SetMarkerColor(kBlue); 
  fResNoiseGrCeren->SetMarkerSize(0.9); 
  fResNoiseGrCeren->SetMarkerStyle(20); 
  fResNoiseGrCeren->SetLineWidth(2); 
  fResNoiseGrCeren->SetLineColor(kBlue); 
  
  fResNoiseGrScint->SetTitle(""); 
  fResNoiseGrScint->SetStats(0); 
	fResNoiseGrScint->SetMarkerColor(kRed); 
  fResNoiseGrScint->SetMarkerSize(0.9); 
  fResNoiseGrScint->SetMarkerStyle(20); 
  fResNoiseGrScint->SetLineWidth(2); 
  fResNoiseGrScint->SetLineColor(kRed); 

  fResNoiseGrSummed->SetTitle(""); 
  fResNoiseGrSummed->SetStats(0); 
	fResNoiseGrSummed->SetMarkerColor(kBlack); 
  fResNoiseGrSummed->SetMarkerSize(0.9); 
  fResNoiseGrSummed->SetMarkerStyle(20); 
  fResNoiseGrSummed->SetLineWidth(2); 
  fResNoiseGrSummed->SetLineColor(kBlack); 

  TString fResNoiseFitFunc = "TMath::Sqrt([0]*[0] + ([1]/x)*([1]/x) + ([2]/TMath::Sqrt(x))*([2]/TMath::Sqrt(x)))";
  
  TF1* fResNoiseFitCeren = new TF1("fResNoiseFitCeren", fResNoiseFitFunc, 5., 125.);  
  fResNoiseFitCeren->SetParLimits(0, 0., 999.);
  fResNoiseFitCeren->SetParLimits(1, 0., 999.);
  fResNoiseFitCeren->SetParLimits(2, 0., 999.);
  fResNoiseGrCeren->Fit(fResNoiseFitCeren,"R"); fResNoiseFitCeren->SetLineWidth(2); fResNoiseFitCeren->SetLineColor(kBlue);
	
  TF1* fResNoiseFitScint = new TF1("fResNoiseFitScint", fResNoiseFitFunc, 5., 125.);  
  fResNoiseFitScint->SetParLimits(0, 0., 999.);
  fResNoiseFitScint->SetParLimits(1, 0., 999.);
  fResNoiseFitScint->SetParLimits(2, 0., 999.);
  fResNoiseGrScint->Fit(fResNoiseFitScint,"R"); fResNoiseFitScint->SetLineWidth(2); fResNoiseFitScint->SetLineColor(kRed);
	
  TF1* fResNoiseFitSummed = new TF1("fResNoiseFitSummed", fResNoiseFitFunc, 5., 125.);  
  fResNoiseFitSummed->SetParLimits(0, 0., 999.);
  fResNoiseFitSummed->SetParLimits(1, 0., 999.);
  fResNoiseFitSummed->SetParLimits(2, 0., 999.);
  fResNoiseGrSummed->Fit(fResNoiseFitSummed,"R"); fResNoiseFitSummed->SetLineWidth(2); fResNoiseFitSummed->SetLineColor(kBlack);

  TGraphErrors* fResGrCeren = new TGraphErrors(fRes_Xaxis.size(), &(fRes_Xaxis[0]), &(fRes_Yaxis_Ceren[0]), &(fRes_XaxisError[0]), &(fRes_YaxisError_Ceren[0])); 

	TGraphErrors* fResGrScint = new TGraphErrors(fRes_Xaxis.size(), &(fRes_Xaxis[0]), &(fRes_Yaxis_Scint[0]), &(fRes_XaxisError[0]), &(fRes_YaxisError_Scint[0])); 
	
  TGraphErrors* fResGrSummed = new TGraphErrors(fRes_Xaxis.size(), &(fRes_Xaxis[0]), &(fRes_Yaxis_Summed[0]), &(fRes_XaxisError[0]), &(fRes_YaxisError_Summed[0])); 
  
  fResGrCeren->SetTitle(""); 
  fResGrCeren->SetStats(0); 
	fResGrCeren->SetMarkerColor(kBlue); 
  fResGrCeren->SetMarkerSize(0.9); 
  fResGrCeren->SetMarkerStyle(20); 
  fResGrCeren->SetLineWidth(2); 
  fResGrCeren->SetLineColor(kBlue); 
  
  fResGrScint->SetTitle(""); 
  fResGrScint->SetStats(0); 
	fResGrScint->SetMarkerColor(kRed); 
  fResGrScint->SetMarkerSize(0.9); 
  fResGrScint->SetMarkerStyle(20); 
  fResGrScint->SetLineWidth(2); 
  fResGrScint->SetLineColor(kRed); 

  fResGrSummed->SetTitle(""); 
  fResGrSummed->SetStats(0); 
	fResGrSummed->SetMarkerColor(kBlack); 
  fResGrSummed->SetMarkerSize(0.9); 
  fResGrSummed->SetMarkerStyle(20); 
  fResGrSummed->SetLineWidth(2); 
  fResGrSummed->SetLineColor(kBlack); 
  
  TF1* fResFitCeren = new TF1("fResFitCeren", "[0]+[1]*x", 0., 3.5);  
  // fResFitCeren->SetParLimits(0, 0., 999.);
  // fResFitCeren->SetParLimits(1, 0., 999.);
  fResGrCeren->Fit(fResFitCeren,"R M &0 same"); fResFitCeren->SetLineWidth(2); fResFitCeren->SetLineColor(kBlue);
	
  TF1* fResFitScint = new TF1("fResFitScint", "[0]+[1]*x", 0., 3.5);  
  // fResFitScint->SetParLimits(0, 0., 999.);
  // fResFitScint->SetParLimits(1, 0., 999.);
  fResGrScint->Fit(fResFitScint,"R M &0 same"); fResFitScint->SetLineWidth(2); fResFitScint->SetLineColor(kRed);
	
  TF1* fResFitSummed = new TF1("fResFitSummed", "[0]+[1]*x", 0., 3.5);  
  // fResFitSummed->SetParLimits(0, 0., 999.);
  // fResFitSummed->SetParLimits(1, 0., 999.);
  fResGrSummed->Fit(fResFitSummed,"R M &0 same"); fResFitSummed->SetLineWidth(2); fResFitSummed->SetLineColor(kBlack);


  TH1D* fLinFrame = new TH1D("fLinFrame", ";#font[42]{Energy [GeV]};#font[42]{Response}", 1, 0., 130.);
  SetPlotFrame(fLinFrame);
  fLinFrame->GetYaxis()->SetRangeUser(0.7, 1.5);

  TGraph* fLineOne = new TGraph();
  fLineOne->SetPoint(0, -10., 1.);
  fLineOne->SetPoint(1, 130., 1.);
  fLineOne->SetLineWidth(2);
  fLineOne->SetLineColor(kGreen);
  fLineOne->SetLineStyle(kDashed);
  
  TGraph* fLineOnePlus = new TGraph();
  fLineOnePlus->SetPoint(0, -10., 1.1);
  fLineOnePlus->SetPoint(1, 130., 1.1);
  fLineOnePlus->SetLineWidth(2);
  fLineOnePlus->SetLineColor(kGray);
  fLineOnePlus->SetLineStyle(kDashed);


  TH1D* fResFrame = new TH1D("fResFrame", ";#font[42]{1/#sqrt{E}};#font[42]{#sigma/E}", 1, 0., 0.35);
  SetPlotFrame(fResFrame);
  fResFrame->GetYaxis()->SetRangeUser(0., 0.6);

  TH1D* fResNoiseFrame = new TH1D("fResNoiseFrame", ";#font[42]{Energy [GeV]};#font[42]{#sigma/E}", 1, 0., 130.);
  SetPlotFrame(fResNoiseFrame);
  fResNoiseFrame->GetYaxis()->SetRangeUser(0., 0.6);

  TLegend* fLegendLin = new TLegend(0.16, 0.78, 0.9, 0.9);
  fLegendLin->SetNColumns(3);
  fLegendLin->SetFillStyle(0);
  fLegendLin->SetBorderSize(0);
  fLegendLin->SetTextFont( 42 );
  fLegendLin->SetTextSize( 0.028 );
  fLegendLin->AddEntry(fLinGrCeren, (TString)("#font[42]{#color[" + std::to_string(fLinGrCeren->GetLineColor()) + "]{Cerenkov}}"), "lep");
  fLegendLin->AddEntry(fLinGrScint, (TString)("#font[42]{#color[" + std::to_string(fLinGrScint->GetLineColor()) + "]{Scintillation}}"), "lep");
  fLegendLin->AddEntry(fLinGrSummed, (TString)("#font[42]{#color[" + std::to_string(fLinGrSummed->GetLineColor()) + "]{Summed}}"), "lep");

  TLegend* fLegendRes = new TLegend(0.08, 0.65, 0.55, 0.9);
  fLegendRes->SetFillStyle(0);
  fLegendRes->SetBorderSize(0);
  fLegendRes->SetTextFont( 42 );
  fLegendRes->SetTextSize( 0.028 );

  TString fStringCeren = (TString)Form("#font[42]{#color[%d]{Cerenkov    #splitline{%.3f^{#pm%.3f} / #sqrt{E} + %.3f^{#pm%.3f}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}", 
    fLinGrCeren->GetLineColor(), fResFitCeren->GetParameter(1), fResFitCeren->GetParError(1), fResFitCeren->GetParameter(0), fResFitCeren->GetParError(0), fResFitCeren->GetChisquare(), fResFitCeren->GetNDF());
  
  TString fStringScint = (TString)Form("#font[42]{#color[%d]{Scintillation  #splitline{%.3f^{#pm%.3f} / #sqrt{E} + %.3f^{#pm%.3f}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}", 
    fLinGrScint->GetLineColor(), fResFitScint->GetParameter(1), fResFitScint->GetParError(1), fResFitScint->GetParameter(0), fResFitScint->GetParError(0), fResFitScint->GetChisquare(), fResFitScint->GetNDF());
  
  TString fStringSummed = (TString)Form("#font[42]{#color[%d]{Summed      #splitline{%.3f^{#pm%.3f} / #sqrt{E} + %.3f^{#pm%.3f}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}", 
    fLinGrSummed->GetLineColor(), fResFitSummed->GetParameter(1), fResFitSummed->GetParError(1), fResFitSummed->GetParameter(0), fResFitSummed->GetParError(0), fResFitSummed->GetChisquare(), fResFitSummed->GetNDF());
  
  fLegendRes->AddEntry(fLinGrCeren, fStringCeren, "lep");
  fLegendRes->AddEntry(fLinGrScint, fStringScint, "lep");
  fLegendRes->AddEntry(fLinGrSummed, fStringSummed, "lep");

  float fCerenNoiseConst = fResNoiseFitCeren->GetParameter(0);
  float fCerenNoiseConstError = fResNoiseFitCeren->GetParError(0);
  float fCerenNoiseLinear = fResNoiseFitCeren->GetParameter(1);
  float fCerenNoiseLinearError = fResNoiseFitCeren->GetParError(1);
  float fCerenNoiseSqrt = fResNoiseFitCeren->GetParameter(2);
  float fCerenNoiseSqrtError = fResNoiseFitCeren->GetParError(2);

  float fScintNoiseConst = fResNoiseFitScint->GetParameter(0);
  float fScintNoiseConstError = fResNoiseFitScint->GetParError(0);
  float fScintNoiseLinear = fResNoiseFitScint->GetParameter(1);
  float fScintNoiseLinearError = fResNoiseFitScint->GetParError(1);
  float fScintNoiseSqrt = fResNoiseFitScint->GetParameter(2);
  float fScintNoiseSqrtError = fResNoiseFitScint->GetParError(2);

  float fSummedNoiseConst = fResNoiseFitSummed->GetParameter(0);
  float fSummedNoiseConstError = fResNoiseFitSummed->GetParError(0);
  float fSummedNoiseLinear = fResNoiseFitSummed->GetParameter(1);
  float fSummedNoiseLinearError = fResNoiseFitSummed->GetParError(1);
  float fSummedNoiseSqrt = fResNoiseFitSummed->GetParameter(2);
  float fSummedNoiseSqrtError = fResNoiseFitSummed->GetParError(2);

  if (fCerenNoiseConst < 0) fCerenNoiseConst = std::abs(fCerenNoiseConst);
  if (fCerenNoiseLinear < 0) fCerenNoiseLinear = std::abs(fCerenNoiseLinear);
  if (fCerenNoiseSqrt < 0) fCerenNoiseSqrt = std::abs(fCerenNoiseSqrt);
  
  if (fScintNoiseConst < 0) fScintNoiseConst = std::abs(fScintNoiseConst);
  if (fScintNoiseLinear < 0) fScintNoiseLinear = std::abs(fScintNoiseLinear);
  if (fScintNoiseSqrt < 0) fScintNoiseSqrt = std::abs(fScintNoiseSqrt);

  if (fSummedNoiseConst < 0) fSummedNoiseConst = std::abs(fSummedNoiseConst);
  if (fSummedNoiseLinear < 0) fSummedNoiseLinear = std::abs(fSummedNoiseLinear);
  if (fSummedNoiseSqrt < 0) fSummedNoiseSqrt = std::abs(fSummedNoiseSqrt);
  

  TString fCerenNoiseTerm = (TString)Form("#font[42]{#color[%d]{Cerenkov    #splitline{%.3f^{#pm%.3f} #oplus %.3f^{#pm%.3f} / E #oplus %.3f^{#pm%.3f} / #sqrt{E}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}",
    fLinGrCeren->GetLineColor(), fCerenNoiseConst, fCerenNoiseConstError, fCerenNoiseLinear, fCerenNoiseLinearError, fCerenNoiseSqrt, fCerenNoiseSqrtError, fResNoiseFitCeren->GetChisquare(), fResNoiseFitCeren->GetNDF());

  TString fScintNoiseTerm = (TString)Form("#font[42]{#color[%d]{Scintillation  #splitline{%.3f^{#pm%.3f} #oplus %.3f^{#pm%.3f} / E #oplus %.3f^{#pm%.3f} / #sqrt{E}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}",
    fLinGrScint->GetLineColor(), fScintNoiseConst, fScintNoiseConstError, fScintNoiseLinear, fScintNoiseLinearError, fScintNoiseSqrt, fScintNoiseSqrtError, fResNoiseFitScint->GetChisquare(), fResNoiseFitScint->GetNDF());

  TString fSummedNoiseTerm = (TString)Form("#font[42]{#color[%d]{Summed      #splitline{%.3f^{#pm%.3f} #oplus %.3f^{#pm%.3f} / E #oplus %.3f^{#pm%.3f} / #sqrt{E}}{#scale[0.7]{#chi^{2}/NDF = %.3f/%d}} }}",
    fLinGrSummed->GetLineColor(), fSummedNoiseConst, fSummedNoiseConstError, fSummedNoiseLinear, fSummedNoiseLinearError, fSummedNoiseSqrt, fSummedNoiseSqrtError, fResNoiseFitSummed->GetChisquare(), fResNoiseFitSummed->GetNDF());
  
  TLegend* fLegendNoise = new TLegend(0.13, 0.66, 0.50, 0.88);
  fLegendNoise->SetFillStyle(0);
  fLegendNoise->SetBorderSize(0);
  fLegendNoise->SetTextFont( 42 );
  fLegendNoise->SetTextSize( 0.027 );
  fLegendNoise->AddEntry(fResNoiseGrCeren, fCerenNoiseTerm, "lep");
  fLegendNoise->AddEntry(fResNoiseGrScint, fScintNoiseTerm, "lep");
  fLegendNoise->AddEntry(fResNoiseGrSummed, fSummedNoiseTerm, "lep");

  TCanvas* fCanvas = new TCanvas("fCanvas", "fCanvas", 1000, 1000);
  
  fCanvas->cd();
  fLinFrame->Draw();
  fLineOne->Draw("l same");
  fLineOnePlus->Draw("l same");
  fLinGrCeren->Draw("P same");
  fLinGrScint->Draw("P same");
  fLinGrSummed->Draw("P same");
  fLegendLin->Draw("SAME");
  fCanvas->SaveAs((TString)Form("./PLOT/%s/RESOLUTION/Lin_Response.pdf", fSuffix.c_str()));

  fCanvas->Clear();
  fCanvas->cd();
  fResFrame->Draw();
  fResGrCeren->Draw("P same");
  fResFitCeren->Draw("L same"); 
  fResGrScint->Draw("P same");
  fResFitScint->Draw("L same");
  fResGrSummed->Draw("P same");
  fResFitSummed->Draw("L same");
  fLegendRes->Draw("SAME");
  fCanvas->SaveAs((TString)Form("./PLOT/%s/RESOLUTION/Res_Resolution.pdf", fSuffix.c_str()));

  fCanvas->Clear();
  fCanvas->cd();
  fResNoiseFrame->Draw();
  fResNoiseGrCeren->Draw("P same");
  fResNoiseFitCeren->Draw("L same");  
  fResNoiseGrScint->Draw("P same");
  fResNoiseFitScint->Draw("L same");
  fResNoiseGrSummed->Draw("P same");
  fResNoiseFitSummed->Draw("L same");
  fLegendNoise->Draw("SAME");
  fCanvas->SaveAs((TString)Form("./PLOT/%s/RESOLUTION/ResNoise_Resolution.pdf", fSuffix.c_str()));
  

}



int main(int argc, char* argv[]) {
  gStyle->SetOptFit(1);

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  std::string fSuffix = "";
  fObj->GetVariable("suffix", &fSuffix);

  std::vector<int> fEnregy = {};
  fObj->GetVector("energy", &fEnregy);

  std::vector<std::string> fModule = {};
  fObj->GetVector("module", &fModule);

  std::vector<std::string> fIgnore = {};
  fObj->GetVector("ignore", &fIgnore);

  if (fModule.size() == 0 && fIgnore.size() != 0) {
    for (int i = 0; i < 9; i++) {
      for (int j = 0; j < 4; j++) {
        std::string fModuleName = Form("M%d-T%d", i + 1, j + 1);

        if (std::find(fIgnore.begin(), fIgnore.end(), fModuleName) == fIgnore.end())
          fModule.push_back(fModuleName);
      }
    }
  }

  fs::path fDirBase(Form("./PLOT/%s", fSuffix.c_str()));
  if ( !fs::exists(fDirBase) ) fs::create_directory(fDirBase);
  
  fs::path fDirPlot(Form("./PLOT/%s/PLOT", fSuffix.c_str()));
  if ( !fs::exists(fDirPlot) ) fs::create_directory(fDirPlot);
  
  fs::path fDirRoot(Form("./PLOT/%s/ROOT", fSuffix.c_str()));
  if ( !fs::exists(fDirRoot) ) fs::create_directory(fDirRoot);

  fs::path fDirRes(Form("./PLOT/%s/RESOLUTION", fSuffix.c_str()));
  if ( !fs::exists(fDirRes) ) fs::create_directory(fDirRes);

  std::ofstream fConfigFile(Form("./PLOT/%s/CONIFG.txt", fSuffix.c_str()));
  if (fConfigFile.is_open()) {
      fConfigFile << "CONFIG: " << fSuffix << std::endl;
      
      fConfigFile << "MODULE: " << std::endl;
      if (fModule.size() == 0 && fIgnore.size() == 0) {
        fConfigFile << "    ALL" << std::endl;
      } else {
        for (int i = 0; i < fModule.size(); i++)
          fConfigFile << "    " << fModule.at(i) << std::endl;

      }
      
      fConfigFile << "RUN: " << std::endl;
        for (int i = 0; i < fEnregy.size(); i++)
          fConfigFile << "    " << fEnregy.at(i) << " GeV" << std::endl;
      
      fConfigFile.close();
  }

  for (int i = 0; i < fEnregy.size(); i++)
    MakeReconstruction(fSuffix, fEnregy.at(i), fModule);

  // MakeResolution(fSuffix, fEnregy);

    
  return 1;
}