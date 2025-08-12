#include "TBplotengine.h"
#include "GuiTypes.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TPaveStats.h"

TBplotengine::TBplotengine(const YAML::Node fConfig_, int fRunNum_, bool fLive_, bool fDraw_, TButility fUtility_)
: fConfig(fConfig_), fRunNum(fRunNum_), fLive(fLive_), fDraw(fDraw_), fUtility(fUtility_), fCaseName(""), fAuxCut(false)
{}

void TBplotengine::init() {

  fIsFirst = true;
  fUsingAUX = false;
  gStyle->SetPalette(kRainBow);

  std::vector<int> fColorVec = {
    TColor::GetColor("#5790fc"),
    TColor::GetColor("#f89c20"),
    TColor::GetColor("#e42536"),
    TColor::GetColor("#964a8b"),
    TColor::GetColor("#9c9ca1"),
    TColor::GetColor("#7a21dd")
  };

  if (fCaseName == "single") {

    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
      fLeg = new TLegend(0.7, 0.2, 0.9, 0.5);
     	fLeg->SetFillStyle(0);
     	fLeg->SetBorderSize(0);
     	fLeg->SetTextFont(42);
    }


    for (int i = 0; i < fNametoPlot.size(); i++) {
      // std::string aName = fUtility.GetName(aCID);
      std::string aName = fNametoPlot.at(i);
      TBcid aCID = fUtility.GetCID(aName);
      fCIDtoPlot_Ceren.push_back(aCID);

      TButility::mod_info aInfo = fUtility.GetInfo(aCID);
      // std::cout << aName << " "; aCID.print();

      if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
        std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC) {
         if (aName.find("LC") != std::string::npos) fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 1200, -10000., 50000.));
         else fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 440, -30000., 300000.));
        }

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 1152, -512., 4096.));

        fPlotter_Ceren.at(i).hist1D->SetLineColor(fColorVec.at(i));
        fPlotter_Ceren.at(i).hist1D->SetLineWidth(2);

      } else if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, 0, 0));
        fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";Bin;ADC", 1000, 0.5, 1000.5));
        fPlotter_Ceren.at(i).hist1D->SetLineColor(fColorVec.at(i));
        fPlotter_Ceren.at(i).hist1D->SetLineWidth(2);
        fPlotter_Ceren.at(i).hist1D->SetStats(0);

      } else if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, 0, 0));
        fPlotter_Ceren.at(i).SetPlot(new TH2D((TString)(aName), (TString)"Run " + std::to_string(fRunNum) + ";Bin;ADC", 1024, 0., 1024., 4096, 0., 4096.));
        fPlotter_Ceren.at(i).hist2D->SetStats(0);

      } else {
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo));
      }
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
      fMainFrame = new TH1D("frame", (TString)"Run " + std::to_string(fRunNum) + ";Bin;ADC", 1000, 0.5, 1000.5);
      fMainFrame->SetStats(0);
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC) {
      fMainFrame = new TH1D("frame", (TString)"Run " + std::to_string(fRunNum) + ";IntADC;nEvents", 440, -30000., 300000.);
      fMainFrame->SetStats(0);
    }

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
      fMainFrame = new TH1D("frame", (TString)"Run " + std::to_string(fRunNum) + ";PeakADC;nEvents", 288, -512., 4096.);
      fMainFrame->SetStats(0);
    }

    fCanvas = new TCanvas("fCanvasPlot", "fCanvasPlot", 1400, 1400);

    Draw();
  } else if (fCaseName == "heatmap") {

    fCanvas = new TCanvas("fCanvasPlot", "fCanvasPlot", 2700, 1000);
    fCanvas->Divide(2, 1);

    auto tPadLeft = fCanvas->cd(1);
    tPadLeft->SetRightMargin(0.13);

    auto tPadRight = fCanvas->cd(2);
    tPadRight->SetRightMargin(0.13);

    init_2D();
  } else if (fCaseName == "module") {

    fCanvas = new TCanvas("fCanvasPlot", "fCanvasPlot", 1400, 1400);
    if (fModule == "M11") fCanvas->Divide(3, 3);
    else                  fCanvas->Divide(2, 2);

    init_single_module();
  } else if (fCaseName == "full") {

    fCanvasFull.push_back(new TCanvas("fCanvasHeatmap", "fCanvasHeatmap", 2700, 1000));
    fCanvasFull.at(0)->Divide(2, 1);

    auto tPadLeft = fCanvasFull.at(0)->cd(1);
    tPadLeft->SetRightMargin(0.13);

    auto tPadRight = fCanvasFull.at(0)->cd(2);
    tPadRight->SetRightMargin(0.13);

    for (int i = 1; i <= 9; i++) {
      std::string aCanvasName = Form("fCanvas_Module%d", i);
      fCanvasFull.push_back(new TCanvas((TString)aCanvasName, (TString)aCanvasName, 1400, 1400));
      fCanvasFull.at(i)->Divide(2, 2);
    }

    init_Generic();
  }
}

void TBplotengine::init_single_module() {

  // std::cout << "TBplotengine::init_single_module()" << " " << fModule << std::endl;

  if (fModule != "M11") {
    for (int i = 1; i <= 4; i++) {
      std::string aCName = fModule + "-T" + std::to_string(i) + "-C";
      TBcid aCCID = fUtility.GetCID(aCName);
      TButility::mod_info aCInfo = fUtility.GetInfo(aCName);

      fCIDtoPlot_Ceren.push_back(aCCID);
      std::vector<int> aCinterval = fConfig[aCName].as<std::vector<int>>();
      fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCCID, aCName, aCInfo, aCinterval.at(0), aCinterval.at(1)));

      if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
        fPlotter_Ceren.at(i - 1).SetPlot(new TH1D((TString)(aCName), ";IntADC;nEvents", 440, -30000., 300000.));

      if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
        fPlotter_Ceren.at(i - 1).SetPlot(new TH1D((TString)(aCName), ";PeakADC;nEvents", 288, -512., 4096.));

      fPlotter_Ceren.at(i - 1).hist1D->SetLineWidth(2);
      fPlotter_Ceren.at(i - 1).hist1D->SetLineColor(kBlue);

      std::string aSName = fModule + "-T" + std::to_string(i) + "-S";
      TBcid aSCID = fUtility.GetCID(aSName);
      TButility::mod_info aSInfo = fUtility.GetInfo(aSName);

      fCIDtoPlot_Scint.push_back(aSCID);
      std::vector<int> aSinterval = fConfig[aSName].as<std::vector<int>>();
      fPlotter_Scint.push_back(TBplotengine::PlotInfo(aSCID, aSName, aSInfo, aSinterval.at(0), aSinterval.at(1)));

      if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
        fPlotter_Scint.at(i - 1).SetPlot(new TH1D((TString)(aSName), ";IntADC;nEvents", 440, -30000., 300000.));

      if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
        fPlotter_Scint.at(i - 1).SetPlot(new TH1D((TString)(aSName), ";PeakADC;nEvents", 288, -512., 4096.));

      fPlotter_Scint.at(i - 1).hist1D->SetLineWidth(2);
      fPlotter_Scint.at(i - 1).hist1D->SetLineColor(kRed);

      if (i == 1) {
        fPlotter_Ceren.at(0).hist1D->SetTitle((TString)"Run " + std::to_string(fRunNum));
        fPlotter_Scint.at(0).hist1D->SetTitle((TString)"Run " + std::to_string(fRunNum));
      }
    }
  } else {
    for (int i = 1; i <= 9; i++) {
      if (i == 5 ) {

        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(TBcid(-1, -1), "M11-T5-C", TButility::mod_info(-1, -1, -1), -1, -1));
        fPlotter_Scint.push_back(TBplotengine::PlotInfo(TBcid(-1, -1), "M11-T5-S", TButility::mod_info(-1, -1, -1), -1, -1));
      } else {

        std::string aCName = fModule + "-T" + std::to_string(i) + "-C";
        TBcid aCCID = fUtility.GetCID(aCName);
        TButility::mod_info aCInfo = fUtility.GetInfo(aCName);

        fCIDtoPlot_Ceren.push_back(aCCID);
        std::vector<int> aCinterval = fConfig[aCName].as<std::vector<int>>();
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCCID, aCName, aCInfo, aCinterval.at(0), aCinterval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
          fPlotter_Ceren.at(i - 1).SetPlot(new TH1D((TString)(aCName), ";IntADC;nEvents", 440, -30000., 300000.));

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Ceren.at(i - 1).SetPlot(new TH1D((TString)(aCName), ";PeakADC;nEvents", 288, -512., 4096.));

        fPlotter_Ceren.at(i - 1).hist1D->SetLineWidth(2);
        fPlotter_Ceren.at(i - 1).hist1D->SetLineColor(kBlue);

        std::string aSName = fModule + "-T" + std::to_string(i) + "-S";
        TBcid aSCID = fUtility.GetCID(aSName);
        TButility::mod_info aSInfo = fUtility.GetInfo(aSName);

        fCIDtoPlot_Scint.push_back(aSCID);
        std::vector<int> aSinterval = fConfig[aSName].as<std::vector<int>>();
        fPlotter_Scint.push_back(TBplotengine::PlotInfo(aSCID, aSName, aSInfo, aSinterval.at(0), aSinterval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
          fPlotter_Scint.at(i - 1).SetPlot(new TH1D((TString)(aSName), ";IntADC;nEvents", 440, -30000., 300000.));

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Scint.at(i - 1).SetPlot(new TH1D((TString)(aSName), ";PeakADC;nEvents", 288, -512., 4096.));

        fPlotter_Scint.at(i - 1).hist1D->SetLineWidth(2);
        fPlotter_Scint.at(i - 1).hist1D->SetLineColor(kRed);

        if (i == 1) {
          fPlotter_Ceren.at(0).hist1D->SetTitle((TString)"Run " + std::to_string(fRunNum));
          fPlotter_Scint.at(0).hist1D->SetTitle((TString)"Run " + std::to_string(fRunNum));
        }
      }
    }
  }

  // std::cout << fCIDtoPlot_Ceren.size() << std::endl;
  // for (int i = 0; i < fPlotter_Ceren.size(); i++) {
  //   std::cout << i << " " << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << " ";
  //   fPlotter_Ceren.at(i).cid.print();
  // }

  // std::cout << fCIDtoPlot_Scint.size() << std::endl;
  // for (int i = 0; i < fPlotter_Scint.size(); i++) {
  //   std::cout << i << " " << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << " ";
  //   fPlotter_Scint.at(i).cid.print();
  // }

  Draw();
}

void TBplotengine::init_2D() {

  if (fModule == "MCPPMT") init_MCPPMT();
  if (fModule == "SiPM")   init_SiPM();
  if (fModule == "Generic") init_Generic();

}

void TBplotengine::init_Generic() {

  std::vector<std::string> plotVec = {

    "M1-T1",
    "M1-T2",
    "M1-T3",
    "M1-T4",

    "M2-T1",
    "M2-T2",
    "M2-T3",
    "M2-T4",

    "M3-T1",
    "M3-T2",
    "M3-T3",
    "M3-T4",

    "M4-T1",
    "M4-T2",
    "M4-T3",
    "M4-T4",

    "M5-T1",
    "M5-T2",
    "M5-T3",
    "M5-T4",

    "M6-T1",
    "M6-T2",
    "M6-T3",
    "M6-T4",

    "M7-T1",
    "M7-T2",
    "M7-T3",
    "M7-T4",

    "M8-T1",
    "M8-T2",
    "M8-T3",
    "M8-T4",

    "M9-T1",
    "M9-T2",
    "M9-T3",
    "M9-T4"
  };


  for (int i = 0; i < plotVec.size(); i++) {

    std::string aCName = plotVec.at(i) + "-C";
    TBcid aCCID = fUtility.GetCID(aCName);
    TButility::mod_info aCInfo = fUtility.GetInfo(aCName);

    fCIDtoPlot_Ceren.push_back(aCCID);

    std::vector<int> intervalC = fConfig[aCName].as<std::vector<int>>();
    fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCCID, aCName, aCInfo, intervalC.at(0), intervalC.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aCName), ";IntADC;nEvents", 440, -30000., 300000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aCName), ";PeakADC;nEvents", 288, -512., 4096.));


    fPlotter_Ceren.at(i).hist1D->SetLineWidth(2);
    fPlotter_Ceren.at(i).hist1D->SetLineColor(kBlue);

    std::string aSName = plotVec.at(i) + "-S";
    TBcid aSCID = fUtility.GetCID(aSName);
    TButility::mod_info aSInfo = fUtility.GetInfo(aSName);

    fCIDtoPlot_Scint.push_back(aSCID);

    std::vector<int> intervalS = fConfig[aSName].as<std::vector<int>>();
    fPlotter_Scint.push_back(TBplotengine::PlotInfo(aSCID, aSName, aSInfo, intervalS.at(0), intervalS.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Scint.at(i).SetPlot(new TH1D((TString)(aSName), ";IntADC;nEvents", 440, -30000., 300000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Scint.at(i).SetPlot(new TH1D((TString)(aSName), ";PeakADC;nEvents", 288, -512., 4096.));

    fPlotter_Scint.at(i).hist1D->SetLineWidth(2);
    fPlotter_Scint.at(i).hist1D->SetLineColor(kRed);

  }

  f2DHistCeren = new TH2D("CERENKOV", "CERENKOV;;", 6, 0.5, 6.5, 6, 0.5, 6.5);
  f2DHistCeren->SetStats(0);

  f2DHistScint = new TH2D("SCINTILLATION", "SCINTILLATION;;", 6, 0.5, 6.5, 6, 0.5, 6.5);
  f2DHistScint->SetStats(0);

  for (int i = 1; i <= 6; i++) {
    f2DHistCeren->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistCeren->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistScint->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistScint->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
  }

  Draw();
}

void TBplotengine::init_MCPPMT() {

  for (int i = 0; i < 64; i++) {
    std::string aName = "C" + std::to_string(i + 1);
    TBcid aCID = fUtility.GetCID(aName);
    TButility::mod_info aInfo = fUtility.GetInfo(aName);

    fCIDtoPlot_Ceren.push_back(aCID);

    std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
    fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 440, -30000., 300000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Ceren.at(i).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 288, -512., 4096.));
  }

  f2DHistCeren = new TH2D("CERENKOV", "CERENKOV;;", 8, 0.5, 8.5, 8, 0.5, 8.5);
  f2DHistCeren->SetStats(0);

  for (int i = 0; i < 64; i++) {
    std::string aName = "S" + std::to_string(i + 1);
    TBcid aCID = fUtility.GetCID(aName);
    TButility::mod_info aInfo = fUtility.GetInfo(aCID);

    fCIDtoPlot_Scint.push_back(aCID);

    std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
    fPlotter_Scint.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
      fPlotter_Scint.at(i).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 440, -30000., 300000.));

    if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
      fPlotter_Scint.at(i).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 288, -512., 4096.));
  }

  f2DHistScint = new TH2D("SCINTILLATION", "SCINTILLATION;;", 8, 0.5, 8.5, 8, 0.5, 8.5);
  f2DHistScint->SetStats(0);

  for (int i = 1; i <= 8; i++) {
    f2DHistCeren->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistCeren->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistScint->GetXaxis()->SetBinLabel(i, std::to_string(i).c_str());
    f2DHistScint->GetYaxis()->SetBinLabel(i, std::to_string(i).c_str());
  }

  // std::cout << fCIDtoPlot_Ceren.size() << std::endl;
  // for (int i = 0; i < fPlotter_Ceren.size(); i++) {
  //   std::cout << i << " " << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << " ";
  //   fPlotter_Ceren.at(i).cid.print();
  // }

  // std::cout << fCIDtoPlot_Scint.size() << std::endl;
  // for (int i = 0; i < fPlotter_Scint.size(); i++) {
  //   std::cout << i << " " << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << " ";
  //   fPlotter_Scint.at(i).cid.print();
  // }

  Draw();
}

void TBplotengine::init_SiPM() {
  // std::cout << "init_SiPM" << std::endl;
  for (int i = 21; i <= 40; i++) {
    for (int j = 21; j <= 40; j++) {

      std::string aName = std::to_string(i) + "-" + std::to_string(j);
      TBcid aCID = fUtility.GetCID(aName);
      TButility::mod_info aInfo = fUtility.GetInfo(aName);

      if (aInfo.isCeren == -1)
        continue;

      if (aInfo.isCeren) {

        fCIDtoPlot_Ceren.push_back(aCID);

        std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
        fPlotter_Ceren.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
          fPlotter_Ceren.at(fPlotter_Ceren.size() - 1).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 440, -30000., 300000.));

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Ceren.at(fPlotter_Ceren.size() - 1).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 288, -512., 4096.));

      } else {

        fCIDtoPlot_Scint.push_back(aCID);

        std::vector<int> interval = fConfig[aName].as<std::vector<int>>();
        fPlotter_Scint.push_back(TBplotengine::PlotInfo(aCID, aName, aInfo, interval.at(0), interval.at(1)));

        if (fCalcInfo == TBplotengine::CalcInfo::kIntADC)
          fPlotter_Scint.at(fPlotter_Scint.size() - 1).SetPlot(new TH1D((TString)(aName), ";IntADC;nEvents", 440, -30000., 300000.));

        if (fCalcInfo == TBplotengine::CalcInfo::kPeakADC)
          fPlotter_Scint.at(fPlotter_Scint.size() - 1).SetPlot(new TH1D((TString)(aName), ";PeakADC;nEvents", 288, -512., 4096.));

      }
    }
  }

  f2DHistCeren = new TH2D("CERENKOV", "CERENKOV;;", 20, 0.5, 20.5, 20, 0.5, 20.5);
  f2DHistCeren->SetStats(0);

  f2DHistScint = new TH2D("SCINTILLATION", "SCINTILLATION;;", 20, 0.5, 20.5, 20, 0.5, 20.5);
  f2DHistScint->SetStats(0);

  for (int i = 1; i <= 20; i++) {
    f2DHistCeren->GetXaxis()->SetBinLabel(i, std::to_string(i + 20).c_str());
    f2DHistCeren->GetYaxis()->SetBinLabel(i, std::to_string(i + 20).c_str());
    f2DHistScint->GetXaxis()->SetBinLabel(i, std::to_string(i + 20).c_str());
    f2DHistScint->GetYaxis()->SetBinLabel(i, std::to_string(i + 20).c_str());
  }

  // std::cout << fCIDtoPlot_Ceren.size() << std::endl;
  // for (int i = 0; i < fPlotter_Ceren.size(); i++) {
  //   std::cout << i << " " << fPlotter_Ceren.at(i).name << " " << fPlotter_Ceren.at(i).info.row << " " << fPlotter_Ceren.at(i).info.col << " ";
  //   fPlotter_Ceren.at(i).cid.print();
  // }

  // std::cout << fCIDtoPlot_Scint.size() << std::endl;
  // for (int i = 0; i < fPlotter_Scint.size(); i++) {
  //   std::cout << i << " " << fPlotter_Scint.at(i).name << " " << fPlotter_Scint.at(i).info.row << " " << fPlotter_Scint.at(i).info.col << " ";
  //   fPlotter_Scint.at(i).cid.print();
  // }

  Draw();
}

double TBplotengine::GetPeakADC(std::vector<short> waveform, int xInit, int xFin) {
  double ped = 0;
  for (int i = 1; i < 101; i++)
    ped += (double)waveform.at(i) / 100.;

  std::vector<double> pedCorWave;
  for (int i = xInit; i < xFin; i++)
    pedCorWave.push_back(ped - (double)waveform.at(i));

  return *std::max_element(pedCorWave.begin(), pedCorWave.end());
}

double TBplotengine::GetIntADC(std::vector<short> waveform, int xInit, int xFin) {

  double ped = 0;
  for (int i = 1; i < 101; i++)
    ped += (double)waveform.at(i) / 100.;

  double intADC_ = 0;
  for (int i = xInit; i < xFin; i++)
    intADC_ += ped - (double)waveform.at(i);

  return intADC_;
}

double TBplotengine::GetIntADC_MCPPMTSingleChannel(std::vector<short> waveform, int xInit, int xFin) {

  double ped = 0;
  for (int i = 1; i < 41; i++)
    ped += (double)waveform.at(i) / 40.;

  double intADC_ = 0;
  for (int i = xInit; i < xFin; i++)
    intADC_ += ped - (double)waveform.at(i);

  return intADC_;
}

void TBplotengine::PrintInfo() {

}

void TBplotengine::Fill(TBevt<TBwaveform> anEvent) {

  if (fCaseName == "single") {
    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        double value = GetValue(anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform(), fPlotter_Ceren.at(i).xInit, fPlotter_Ceren.at(i).xFin);
        fPlotter_Ceren.at(i).hist1D->Fill(value);
      }
    } else if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        auto tWave = anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform();
        for (int j = 1; j <= 1000; j++) {
          fPlotter_Ceren.at(i).hist1D->Fill(j, tWave.at(j));
        }
        fPlotter_Ceren.at(i).xInit++;
      }
    } else if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        auto tWave = anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform();
        for (int j = 0; j < tWave.size(); j++) {
          fPlotter_Ceren.at(i).hist2D->Fill(j, tWave.at(j));
        }
      }
    }

  } else if (fCaseName == "heatmap" || fCaseName == "full") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {

      double value = GetValue(anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform(), fPlotter_Ceren.at(i).xInit, fPlotter_Ceren.at(i).xFin);
      fPlotter_Ceren.at(i).hist1D->Fill(value);
      // std::cout << i << " " <<  fPlotter_Ceren.at(i).cid.mid() << " " << fPlotter_Ceren.at(i).cid.channel() << std::endl;
    }

    for (int i = 0; i < fPlotter_Scint.size(); i++) {

      double value = GetValue(anEvent.GetData(fPlotter_Scint.at(i).cid).waveform(), fPlotter_Scint.at(i).xInit, fPlotter_Scint.at(i).xFin);
      fPlotter_Scint.at(i).hist1D->Fill(value);
      // std::cout << i << " " << fPlotter_Scint.at(i).cid.mid() << " " << fPlotter_Scint.at(i).cid.channel() << std::endl;
    }
  } else if (fCaseName == "module") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      if (i == 4) continue;
      double value = GetValue(anEvent.GetData(fPlotter_Ceren.at(i).cid).waveform(), fPlotter_Ceren.at(i).xInit, fPlotter_Ceren.at(i).xFin);
      fPlotter_Ceren.at(i).hist1D->Fill(value);
    }

    for (int i = 0; i < fPlotter_Scint.size(); i++) {
      if (i == 4) continue;
      double value = GetValue(anEvent.GetData(fPlotter_Scint.at(i).cid).waveform(), fPlotter_Scint.at(i).xInit, fPlotter_Scint.at(i).xFin);
      fPlotter_Scint.at(i).hist1D->Fill(value);
    }
  }
}

void TBplotengine::Draw() {

  if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      fLeg->AddEntry(fPlotter_Ceren.at(i).hist1D, fPlotter_Ceren.at(i).name.c_str(), "l");

  }

  if (fCaseName == "single") {
    fCanvas->cd();

    if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
      fPlotter_Ceren.at(0).hist2D->Draw("colz");

    } else {
      fMainFrame->Draw();
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist1D->Draw("sames");

      if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
        fLeg->Draw("same");
    }

  } else if (fCaseName == "heatmap") {
    fCanvas->cd(1);
    f2DHistCeren->Draw("colz text");

    fCanvas->cd(2);
    f2DHistScint->Draw("colz text");
  } else if (fCaseName == "module") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      if (i == 4) continue;

      fCanvas->cd(i + 1);
      fPlotter_Ceren.at(i).hist1D->Draw("Hist");
      fPlotter_Scint.at(i).hist1D->Draw("Hist & sames");
    }
  } else if (fCaseName == "full") {
    
    fCanvasFull.at(0)->cd(1);
    f2DHistCeren->Draw("colz text");
  
    fCanvasFull.at(0)->cd(2);
    f2DHistScint->Draw("colz text");

    for (int idx = 0; idx < fPlotter_Ceren.size(); idx++) {
      int iModule = idx / 4 + 1;
      int iTower = idx % 4 + 1;

      fCanvasFull.at(iModule)->cd(iTower);
      fPlotter_Ceren.at(idx).hist1D->Draw("Hist");
      fPlotter_Scint.at(idx).hist1D->Draw("Hist & sames");
    }
  }

  // if (fUsingAUX) gSystem->ProcessEvents();
  gSystem->Sleep(1000);
}

void TBplotengine::Update() {

  if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      fPlotter_Ceren.at(i).hist1D->Scale(1./(float)fPlotter_Ceren.at(i).xInit);

  if (fCaseName == "single") {
    if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC || fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
      SetMaximum();

    fCanvas->cd();

    if (fCalcInfo == TBplotengine::CalcInfo::kOverlay) {
      fCanvas->cd();
      fPlotter_Ceren.at(0).hist2D->Draw("colz");

    } else {
      fCanvas->cd();
      fMainFrame->Draw();

      double stat_height = (1. - 0.2) / (double)fPlotter_Ceren.size();
      for (int i = 0; i < fPlotter_Ceren.size(); i++) {
        fCanvas->cd();
        fPlotter_Ceren.at(i).hist1D->Draw("Hist & sames");

        if (fIsFirst) {

          if (fCalcInfo == TBplotengine::CalcInfo::kIntADC || fCalcInfo == TBplotengine::CalcInfo::kPeakADC) {
            fCanvas->Update();
            // TPaveStats* stat = (TPaveStats*)fCanvas->GetPrimitive("stats");
            TPaveStats* stat = (TPaveStats*)fPlotter_Ceren.at(i).hist1D->FindObject("stats");
            // stat->SetName(fPlotter_Ceren.at(i).hist1D->GetName() + (TString)"_stat");
            stat->SetTextColor(fPlotter_Ceren.at(i).hist1D->GetLineColor());
            stat->SetY2NDC(1. - stat_height * i);
            stat->SetY1NDC(1 - stat_height * (i + 1));
            stat->SaveStyle();
          }
        }
      }
      if (fIsFirst) fIsFirst = false;
      if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc) {
        fCanvas->cd();
        fLeg->Draw("same");
      }
    }
  } else if (fCaseName == "heatmap") {

    f2DHistCeren->SetTitle((TString)"Run " + std::to_string(fRunNum) + " CERENKOV - " + std::to_string((int)fPlotter_Ceren.at(0).hist1D->GetEntries()));
    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      f2DHistCeren->SetBinContent(fPlotter_Ceren.at(i).info.row, fPlotter_Ceren.at(i).info.col, (int)fPlotter_Ceren.at(i).hist1D->GetMean());

      // std::cout << "CEREN - " << i
      //           << " " << fPlotter_Ceren.at(i).name
      //           << " " << fPlotter_Ceren.at(i).cid.mid()
      //           << " " << fPlotter_Ceren.at(i).cid.channel()
      //           << " " << fPlotter_Ceren.at(i).info.row
      //           << " " << fPlotter_Ceren.at(i).info.col
      //           << " " << fPlotter_Ceren.at(i).hist1D->GetMean() << std::endl;

    }

    f2DHistScint->SetTitle((TString)"Run " + std::to_string(fRunNum) + " SCINTILLATION - " + std::to_string((int)fPlotter_Scint.at(0).hist1D->GetEntries()));
    for (int i = 0; i < fPlotter_Scint.size(); i++) {
      f2DHistScint->SetBinContent(fPlotter_Scint.at(i).info.row, fPlotter_Scint.at(i).info.col, (int)fPlotter_Scint.at(i).hist1D->GetMean());

      // std::cout << "SCINT - " << i
      //           << " " << fPlotter_Scint.at(i).name
      //           << " " << fPlotter_Scint.at(i).cid.mid()
      //           << " " << fPlotter_Scint.at(i).cid.channel()
      //           << " " << fPlotter_Ceren.at(i).info.row
      //           << " " << fPlotter_Ceren.at(i).info.col
      //           << " " << fPlotter_Scint.at(i).hist1D->GetMean() << std::endl;

    }
    fCanvas->cd(1);
    f2DHistCeren->Draw("colz text");

    fCanvas->cd(2);
    f2DHistScint->Draw("colz text");

  } else if (fCaseName == "module") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      if (i == 4) continue;

      if (fPlotter_Ceren.at(i).hist1D->GetMaximum() > fPlotter_Ceren.at(i).hist1D->GetMaximum()) {

        fCanvas->cd(i + 1);
        fPlotter_Ceren.at(i).hist1D->Draw("Hist");

        if (fIsFirst) {
          fCanvas->Update();
          TPaveStats* stat_c = (TPaveStats*)fPlotter_Ceren.at(i).hist1D->FindObject("stats");
          stat_c->SetTextColor(4);
          stat_c->SetY2NDC(1.);
          stat_c->SetY1NDC(.8);
          stat_c->SaveStyle();
        }

        fCanvas->cd(i + 1);
        fPlotter_Scint.at(i).hist1D->Draw("Hist & sames");

        if (fIsFirst) {
          fCanvas->Update();
          TPaveStats* stat_s = (TPaveStats*)fPlotter_Scint.at(i).hist1D->FindObject("stats");
          stat_s->SetTextColor(2);
          stat_s->SetY2NDC(.8);
          stat_s->SetY1NDC(.6);
          stat_s->SaveStyle();
        }
      } else {

        fCanvas->cd(i + 1);
        fPlotter_Scint.at(i).hist1D->Draw("Hist");

        if (fIsFirst) {
          fCanvas->Update();
          TPaveStats* stat_s = (TPaveStats*)fPlotter_Scint.at(i).hist1D->FindObject("stats");
          stat_s->SetTextColor(2);
          stat_s->SetY2NDC(.8);
          stat_s->SetY1NDC(.6);
          stat_s->SaveStyle();
        }

        fCanvas->cd(i + 1);
        fPlotter_Ceren.at(i).hist1D->Draw("Hist & sames");

        if (fIsFirst) {
          fCanvas->Update();
          TPaveStats* stat_c = (TPaveStats*)fPlotter_Ceren.at(i).hist1D->FindObject("stats");
          stat_c->SetTextColor(4);
          stat_c->SetY2NDC(1.);
          stat_c->SetY1NDC(.8);
          stat_c->SaveStyle();
        }
      }
    }

    fIsFirst = false;
  } else if (fCaseName == "full") {

    f2DHistCeren->SetTitle((TString)"Run " + std::to_string(fRunNum) + " CERENKOV - " + std::to_string((int)fPlotter_Ceren.at(0).hist1D->GetEntries()));
    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      f2DHistCeren->SetBinContent(fPlotter_Ceren.at(i).info.row, fPlotter_Ceren.at(i).info.col, (int)fPlotter_Ceren.at(i).hist1D->GetMean());

    f2DHistScint->SetTitle((TString)"Run " + std::to_string(fRunNum) + " SCINTILLATION - " + std::to_string((int)fPlotter_Scint.at(0).hist1D->GetEntries()));
    for (int i = 0; i < fPlotter_Scint.size(); i++)
      f2DHistScint->SetBinContent(fPlotter_Scint.at(i).info.row, fPlotter_Scint.at(i).info.col, (int)fPlotter_Scint.at(i).hist1D->GetMean());

    fCanvasFull.at(0)->cd(1);
    f2DHistCeren->Draw("colz text");

    fCanvasFull.at(0)->cd(2);
    f2DHistScint->Draw("colz text");

    fCanvasFull.at(0)->Update();


    for (int idx = 0; idx < fPlotter_Ceren.size(); idx++) {
      int iModule = idx / 4 + 1;
      int iTower = idx % 4 + 1;
      
      if (fPlotter_Ceren.at(idx).hist1D->GetMaximum() > fPlotter_Scint.at(idx).hist1D->GetMaximum()) {

        fCanvasFull.at(iModule)->cd(iTower);
        fPlotter_Ceren.at(idx).hist1D->Draw("Hist");

        if (fIsFirst) {
          fCanvasFull.at(iModule)->Update();
          TPaveStats* stat_c = (TPaveStats*)fPlotter_Ceren.at(idx).hist1D->FindObject("stats");
          stat_c->SetTextColor(4);
          stat_c->SetY2NDC(1.);
          stat_c->SetY1NDC(.8);
          stat_c->SaveStyle();
        }

        fCanvasFull.at(iModule)->cd(iTower);
        fPlotter_Scint.at(idx).hist1D->Draw("Hist & sames");

        if (fIsFirst) {
          fCanvasFull.at(iModule)->Update();
          TPaveStats* stat_s = (TPaveStats*)fPlotter_Scint.at(idx).hist1D->FindObject("stats");
          stat_s->SetTextColor(2);
          stat_s->SetY2NDC(.8);
          stat_s->SetY1NDC(.6);
          stat_s->SaveStyle();
        }

      } else {

        fCanvasFull.at(iModule)->cd(iTower);
        fPlotter_Scint.at(idx).hist1D->Draw("Hist");

        if (fIsFirst) {
          fCanvasFull.at(iModule)->Update();
          TPaveStats* stat_s = (TPaveStats*)fPlotter_Scint.at(idx).hist1D->FindObject("stats");
          stat_s->SetTextColor(2);
          stat_s->SetY2NDC(.8);
          stat_s->SetY1NDC(.6);
          stat_s->SaveStyle();
        }

        fCanvasFull.at(iModule)->cd(iTower);
        fPlotter_Ceren.at(idx).hist1D->Draw("Hist & sames");

        if (fIsFirst) {
          fCanvasFull.at(iModule)->Update();
          TPaveStats* stat_c = (TPaveStats*)fPlotter_Ceren.at(idx).hist1D->FindObject("stats");
          stat_c->SetTextColor(4);
          stat_c->SetY2NDC(1.);
          stat_c->SetY1NDC(.8);
          stat_c->SaveStyle();
        }
      }
      fCanvasFull.at(iModule)->Update();
    }

    if (fIsFirst)fIsFirst = false;
  }

  if (fCaseName != "full") {
    fCanvas->cd();
    fCanvas->Update();
    if (fDraw) fCanvas->Pad()->Draw();
  }
  
  if (fCaseName == "full") {
    TString output = "./output/Run" + std::to_string(fRunNum) + "_" + fCaseName + "_" + fMethod + ".root";
    if (fAuxCut) output = "./output/Run" + std::to_string(fRunNum) + "_" + fCaseName + "_" + fMethod + "_AuxCut.root";
    TFile* outoutFile = new TFile(output, "RECREATE");
    outoutFile->cd();

    for (int i = 0; i < fCanvasFull.size(); i++)
      fCanvasFull.at(i)->Write();
    
    outoutFile->Close();
  } else {
    TString output = "./output/Run" + std::to_string(fRunNum) + "_" + fCaseName + "_" + fMethod + "_" + fModule + ".root"; 
    if (fAuxCut) output = "./output/Run" + std::to_string(fRunNum) + "_" + fCaseName + "_" + fMethod + "_" + fModule + "_AuxCut.root";
    TFile* outoutFile = new TFile(output, "RECREATE");
    outoutFile->cd();
    fCanvas->Write();
    outoutFile->Close();
  }

  // if (fUsingAUX) gSystem->ProcessEvents();
  // else           fApp->Run(false);

  // std::cout << fLive << " " << fUsingAUX << std::endl;

  if (fDraw) {
    if (fLive && fUsingAUX) gSystem->ProcessEvents();
    if (fLive && !fUsingAUX) gSystem->ProcessEvents();
    if (!fLive && fUsingAUX) gSystem->ProcessEvents();
    if (!fLive && !fUsingAUX) fApp->Run(false);
  }



  gSystem->Sleep(1000);

  if (fLive)
    if (fCalcInfo == TBplotengine::CalcInfo::kAvgTimeStruc)
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist1D->Scale((float)fPlotter_Ceren.at(i).xInit);


}

void TBplotengine::SetMaximum() {

  float max = -999;
  for (int i = 0; i < fPlotter_Ceren.size(); i++) {
    if (max < fPlotter_Ceren.at(i).hist1D->GetMaximum()) {
      max = fPlotter_Ceren.at(i).hist1D->GetMaximum();
    }
  }

  fMainFrame->GetYaxis()->SetRangeUser(0., max * 1.2);
}

void TBplotengine::SaveAs(TString output = "")
{
  if (output == "")
    output = "./output/Run" + std::to_string(fRunNum) + "_" + fCaseName + "_" + fMethod + "_" + fModule + ".root";

  TFile* outoutFile = new TFile(output, "RECREATE");

  outoutFile->cd();
  if (fCaseName == "single") {
    if (fMethod == "Overlay") {
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist2D->Write();
    } else {
      for (int i = 0; i < fPlotter_Ceren.size(); i++)
        fPlotter_Ceren.at(i).hist1D->Write();
    }

  } else if (fCaseName == "heatmap") {
    f2DHistCeren->Write();
    f2DHistScint->Write();

    for (int i = 0; i < fPlotter_Ceren.size(); i++)
      fPlotter_Ceren.at(i).hist1D->Write();

    for (int i = 0; i < fPlotter_Scint.size(); i++)
      fPlotter_Scint.at(i).hist1D->Write();

  } else if (fCaseName == "module") {

    for (int i = 0; i < fPlotter_Ceren.size(); i++) {
      if (i == 4) continue;
      fPlotter_Ceren.at(i).hist1D->Write();
    }

    for (int i = 0; i < fPlotter_Scint.size(); i++) {
      if (i == 4) continue;
      fPlotter_Scint.at(i).hist1D->Write();
    }
  }

  outoutFile->Close();
}

std::vector<int> TBplotengine::GetUniqueMID() {
  if (fCaseName == "single") {

    return fUtility.GetUniqueMID(fCIDtoPlot_Ceren);
  } else if (fCaseName == "heatmap" || fCaseName == "module" || fCaseName == "full") {

    return fUtility.GetUniqueMID(fCIDtoPlot_Ceren, fCIDtoPlot_Scint);
  }

  return std::vector<int>{};
}
