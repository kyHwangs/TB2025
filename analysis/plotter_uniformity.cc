#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TH2D.h"
#include "TDirectory.h"
#include "TLine.h"
#include "TText.h"
#include "TBox.h"
#include "TArrow.h"
#include "TH1.h"
#include "TLegend.h"



void SetPlotFrame(TH1D* plot) {

	// plot->SetStats(kFALSE);            
	// plot->SetMarkerStyle(20);      
	// plot->SetMarkerSize(0.);      
	// plot->SetLineWidth(0);

	// plot->GetYaxis()->SetTitle("N_{evts}" );
	plot->GetYaxis()->SetTitleFont(42);     
	plot->GetYaxis()->SetTitleSize(0.04);   
	plot->GetYaxis()->SetTitleOffset(1.2); 
	plot->GetYaxis()->SetLabelFont(42);
	plot->GetYaxis()->SetLabelSize(0.03); 

	// plot->GetXaxis()->SetTitle( xTitle.c_str() ); 
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

TH1D* GetHist1D(TFile* fFile, std::string fDir, std::string fName) {
  // std::cout << "Directory " << fDir << " not found" << std::endl;
  // std::cout << "Histogram " << Form("%s/%s", fDir.c_str(), fName.c_str()) << " not found" << std::endl;

  TDirectory* dir = fFile->GetDirectory(fDir.c_str());
  if (!dir) {
    // std::cout << "Directory " << fDir << " not found" << std::endl;
    return nullptr;
  }

  auto fHist_temp = (TH1D*)dir->Get((TString)Form("%s", fName.c_str()));

  if (!fHist_temp) {
    // std::cout << "Histogram " << Form("%s/%s", fDir.c_str(), fName.c_str()) << " not found" << std::endl;
    return nullptr;
  }

  return fHist_temp;
}

TH2D* GetHist2D(TFile* fFile, std::string fDir, std::string fName) {
  // std::cout << "Directory " << fDir << " not found" << std::endl;
  // std::cout << "Histogram " << Form("%s", fName.c_str()) << " not found" << std::endl;

  TDirectory* dir = fFile->GetDirectory(fDir.c_str());
  if (!dir) {
    // std::cout << "Directory " << fDir << " not found" << std::endl;
    return nullptr;
  }

  auto fHist_temp = (TH2D*)dir->Get((TString)Form("%s", fName.c_str()));

  if (!fHist_temp) {
    // std::cout << "Histogram " << Form("%s/%s", fDir.c_str(), fName.c_str()) << " not found" << std::endl;
    return nullptr;
  }

  fHist_temp->SetStats(kFALSE);

  return fHist_temp;
}

int plotter_uniformity(std::string fSuffix) {
  gStyle->SetOptFit(1);

  TFile* fFile = new TFile((TString)Form("UNIFORMITY/%s/TOTAL.root", fSuffix.c_str()), "READ");
  
  std::vector<int> fColorVec = {
    TColor::GetColor("#5790fc"),
    TColor::GetColor("#f89c20"),
    TColor::GetColor("#e42536"),
    TColor::GetColor("#964a8b"),
    TColor::GetColor("#9c9ca1"),
    TColor::GetColor("#7a21dd")
  };

  std::vector<float> tHorizontalList = {0., 24.3, 48.5, 72.8, 97.};
  std::vector<float> tVerticalList = {50., 25., 0.};
  TGraph* fBeamPosition = new TGraph();
  for (int i = 0; i < 5; i++)
    for (int j = 0; j < 3; j++)
      fBeamPosition->SetPoint(fBeamPosition->GetN(), tHorizontalList.at(i), tVerticalList.at(j));


  fBeamPosition->SetMarkerStyle(20);
  fBeamPosition->SetMarkerSize(2);
  fBeamPosition->SetMarkerColor(kRed);;

  TGraph* fTowerBoundary = new TGraph();
  fTowerBoundary->SetPoint(0, -24.2, 9999);
  fTowerBoundary->SetPoint(1, -24.2, -9999);
  fTowerBoundary->SetPoint(2, 24.3, -9999);
  fTowerBoundary->SetPoint(3, 24.3, 9999);
  fTowerBoundary->SetPoint(4, 72.8, 9999);
  fTowerBoundary->SetPoint(5, 72.8, -9999);
  fTowerBoundary->SetPoint(6, 120.3, -9999);
  fTowerBoundary->SetPoint(7, 120.3, 9999);
  fTowerBoundary->SetPoint(8, 9999, 9999);
  fTowerBoundary->SetPoint(9, 9999, 75);
  fTowerBoundary->SetPoint(10, -9999, 75);
  fTowerBoundary->SetPoint(11, -9999, 25);
  fTowerBoundary->SetPoint(12, 9999, 25);
  fTowerBoundary->SetPoint(13, 9999, -25);
  fTowerBoundary->SetPoint(14, -9999, -25);
  fTowerBoundary->SetLineColor(kBlack);
  fTowerBoundary->SetLineWidth(3);
  fTowerBoundary->SetLineStyle(kDashed);

  TH2D* fHistLDWC_Total = GetHist2D(fFile, "Uniformity_Total", "LDWC_Uniformity_Total");
  TH2D* fHistFDWC_Total = GetHist2D(fFile, "Uniformity_Total", "FDWC_Uniformity_Total");

  TH1D* fHistCerenTotal = GetHist1D(fFile, "Uniformity_Total", "Ceren_Uniformity_Total");
  fHistCerenTotal->SetLineColor(kBlue);
  fHistCerenTotal->SetLineWidth(2);
  TH1D* fHistScintTotal = GetHist1D(fFile, "Uniformity_Total", "Scint_Uniformity_Total");
  fHistScintTotal->SetLineColor(kRed);
  fHistScintTotal->SetLineWidth(2);

  TH2D* fHistCeren = new TH2D("CEREN", ";X [mm];Y [mm]", 40, -50., 150., 30, -50., 100.); fHistCeren->SetStats(kFALSE);
  TH2D* fHistScint = new TH2D("SCINT", ";X [mm];Y [mm]", 40, -50., 150., 30, -50., 100.); fHistScint->SetStats(kFALSE);
  TH2D* fHistEntry = new TH2D("ENTRY", ";X [mm];Y [mm]", 40, -50., 150., 30, -50., 100.); fHistEntry->SetStats(kFALSE);

  fHistCeren->GetZaxis()->SetTitle("GeV");
  fHistScint->GetZaxis()->SetTitle("GeV");
  fHistEntry->GetZaxis()->SetTitle("nEvents");
  fHistLDWC_Total->GetZaxis()->SetTitle("nEvents");
  fHistFDWC_Total->GetZaxis()->SetTitle("nEvents");

  TH1* fHistCerenSection = new TH1D("CEREN_SECTION", ";GeV;nSection", 80, 0., 160);
  TH1* fHistScintSection = new TH1D("SCINT_SECTION", ";GeV;nSection", 80, 0., 160);

  TCanvas* c2 = new TCanvas("c2", "c2", 1000, 1000);

 for (int iY = 0; iY < 20; iY++) {
    for (int iX = 0; iX < 30; iX++) {

      std::string type_name = Form("Uniformity_%d_%d", iX, iY);
      TH1D* fCerenHist = GetHist1D(fFile, type_name, "Ceren_" + type_name);
      TH1D* fScintHist = GetHist1D(fFile, type_name, "Scint_" + type_name);

      if (fCerenHist == nullptr)
        std::cout << "Ceren " << type_name << " not found" << std::endl;
      
        if (fScintHist == nullptr)
        std::cout << "Scint " << type_name << " not found" << std::endl;

      if (fCerenHist != nullptr && fScintHist != nullptr)
        std::cout << iX << " " << iY << " " << iX + 6 << " " << iY + 6 << " " << fScintHist->GetEntries() << " " << fCerenHist->GetMean() << " " << fScintHist->GetMean() << std::endl;

      if (fCerenHist != nullptr) fHistCeren->SetBinContent(iX + 6, iY + 6, fCerenHist->GetMean());
      if (fScintHist != nullptr) fHistScint->SetBinContent(iX + 6, iY + 6, fScintHist->GetMean());
      if (fScintHist != nullptr) fHistEntry->SetBinContent(iX + 6, iY + 6, fScintHist->GetEntries());

      if (fCerenHist != nullptr) fHistCerenSection->Fill(fCerenHist->GetMean());
      if (fScintHist != nullptr) fHistScintSection->Fill(fScintHist->GetMean());

      if (fCerenHist != nullptr) {
        fCerenHist->GetXaxis()->SetRangeUser(0., 160);
        c2->cd();
        fCerenHist->Draw("Hist");
        c2->SaveAs(Form("UNIFORMITY/%s/PLOT/SECTION/CEREN_Uniformity_%d_%d.pdf", fSuffix.c_str(), iX, iY));
      }

      if (fScintHist != nullptr) {
        fScintHist->GetXaxis()->SetRangeUser(0., 160);
        c2->cd();
        fScintHist->Draw("Hist");
        c2->SaveAs(Form("UNIFORMITY/%s/PLOT/SECTION/SCINT_Uniformity_%d_%d.pdf", fSuffix.c_str(), iX, iY));
      }
    }
  }

  // fFile->Close();

  // TFile* fFileOutput = new TFile("UNIFORMITY/test/PLOT/PLOT.root", "RECREATE");
  // fFileOutput->cd();

  // fHistCeren->Write();
  // fHistScint->Write();
  // fHistEntry->Write();

  // fFileOutput->Close();

  if (!fHistLDWC_Total) std::cout << "No data - fHistLDWC_Total" << std::endl;
  if (!fHistCeren) std::cout << "No data - fHistCeren" << std::endl;
  if (!fHistScint) std::cout << "No data - fHistScint" << std::endl;
  if (!fHistEntry) std::cout << "No data - fHistEntry" << std::endl;

  fHistCeren->GetZaxis()->SetRangeUser(50., 120.);
  fHistScint->GetZaxis()->SetRangeUser(50., 120.);

  TF1* fGausCeren = new TF1("fGausCeren", "gaus", 50, 140); 
  fGausCeren->SetLineColor(kBlue);
  fGausCeren->SetLineWidth(2);
  fHistCerenSection->SetOption("p"); 
  fHistCerenSection->Fit(fGausCeren,"RM+ same");

  fHistCerenSection->SetLineWidth(2);
  fHistCerenSection->SetLineColor(kBlack);

  TF1* fGausScint = new TF1("fGausScint", "gaus", 50, 140); 
  fGausScint->SetLineColor(kRed);
  fGausScint->SetLineWidth(2);
  fHistScintSection->SetOption("p"); 
  fHistScintSection->Fit(fGausScint,"RM+ same");

  fHistScintSection->SetLineWidth(2);
  fHistScintSection->SetLineColor(kBlack);
  
  TCanvas* c = new TCanvas("c", "c", 1500, 1050);
  c->SetRightMargin(0.15);

  c->cd();
  fHistLDWC_Total->Draw("colz");
  fTowerBoundary->Draw("L same");
  fBeamPosition->Draw("P same");
  c->SaveAs(Form("UNIFORMITY/%s/PLOT/LDWC_Uniformity_Total.pdf", fSuffix.c_str()));

  c->cd();
  fHistFDWC_Total->Draw("colz");
  fTowerBoundary->Draw("L same");
  fBeamPosition->Draw("P same");
  c->SaveAs(Form("UNIFORMITY/%s/PLOT/FDWC_Uniformity_Total.pdf", fSuffix.c_str()));

  c->cd();
  fHistCeren->Draw("colz");
  fTowerBoundary->Draw("L same");
  fBeamPosition->Draw("P same");
  c->SaveAs(Form("UNIFORMITY/%s/PLOT/CEREN_Uniformity_Total.pdf", fSuffix.c_str()));

  c->cd();
  fHistScint->Draw("colz");
  fTowerBoundary->Draw("L same");
  fBeamPosition->Draw("P same");
  c->SaveAs(Form("UNIFORMITY/%s/PLOT/SCINT_Uniformity_Total.pdf", fSuffix.c_str()));
  
  c->cd();
  fHistEntry->Draw("colz");
  fTowerBoundary->Draw("L same");
  fBeamPosition->Draw("P same");
  c->SaveAs(Form("UNIFORMITY/%s/PLOT/ENTRY_Uniformity_Total.pdf", fSuffix.c_str()));

  c2->cd();
  fHistCerenTotal->Draw("Hist");
  c2->SaveAs(Form("UNIFORMITY/%s/PLOT/CEREN_Uniformity_Total_1D.pdf", fSuffix.c_str()));

  c2->cd();
  fHistScintTotal->Draw("Hist");
  c2->SaveAs(Form("UNIFORMITY/%s/PLOT/SCINT_Uniformity_Total_1D.pdf", fSuffix.c_str()));

  c2->cd();
  fHistCerenSection->Draw("Hist");
  fGausCeren->Draw("same");
  c2->SaveAs(Form("UNIFORMITY/%s/PLOT/CEREN_SECTION.pdf", fSuffix.c_str()));

  c2->cd();
  fHistScintSection->Draw("Hist");
  fGausScint->Draw("same");
  c2->SaveAs(Form("UNIFORMITY/%s/PLOT/SCINT_SECTION.pdf", fSuffix.c_str()));

  return 0;
}