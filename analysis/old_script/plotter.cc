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

int plotter() {

  TFile* fFile = new TFile("TEST/Run12176.root");
  std::vector<int> fColorVec = {
    TColor::GetColor("#5790fc"),
    TColor::GetColor("#f89c20"),
    TColor::GetColor("#e42536"),
    TColor::GetColor("#964a8b"),
    TColor::GetColor("#9c9ca1"),
    TColor::GetColor("#7a21dd")
  };


  //////////////////////////////////////////////////////////////
  // VERTICAL
  //////////////////////////////////////////////////////////////

  TH1D* hist_Scint_case4 = (TH1D*)fFile->Get("case4/Scint_case4"); hist_Scint_case4->SetLineWidth(2); hist_Scint_case4->Rebin(10); hist_Scint_case4->SetName("Scint - Top"); hist_Scint_case4->SetLineColor(fColorVec[0]);
  TH1D* hist_Scint_case5 = (TH1D*)fFile->Get("case5/Scint_case5"); hist_Scint_case5->SetLineWidth(2); hist_Scint_case5->Rebin(10); hist_Scint_case5->SetName("Scint - Middle"); hist_Scint_case5->SetLineColor(fColorVec[1]);
  TH1D* hist_Scint_case6 = (TH1D*)fFile->Get("case6/Scint_case6"); hist_Scint_case6->SetLineWidth(2); hist_Scint_case6->Rebin(10); hist_Scint_case6->SetName("Scint - Bottom"); hist_Scint_case6->SetLineColor(fColorVec[2]);

  TH1D* hist_Ceren_case4 = (TH1D*)fFile->Get("case4/Ceren_case4"); hist_Ceren_case4->SetLineWidth(2); hist_Ceren_case4->Rebin(5); hist_Ceren_case4->SetName("Ceren - Top"); hist_Ceren_case4->SetLineColor(fColorVec[0]);
  TH1D* hist_Ceren_case5 = (TH1D*)fFile->Get("case5/Ceren_case5"); hist_Ceren_case5->SetLineWidth(2); hist_Ceren_case5->Rebin(5); hist_Ceren_case5->SetName("Ceren - Middle"); hist_Ceren_case5->SetLineColor(fColorVec[1]);
  TH1D* hist_Ceren_case6 = (TH1D*)fFile->Get("case6/Ceren_case6"); hist_Ceren_case6->SetLineWidth(2); hist_Ceren_case6->Rebin(5); hist_Ceren_case6->SetName("Ceren - Bottom"); hist_Ceren_case6->SetLineColor(fColorVec[2]);

  
  SetPlotFrame(hist_Scint_case4);
  hist_Scint_case4->GetYaxis()->SetRangeUser(0, hist_Scint_case6->GetMaximum() * 1.2);

  SetPlotFrame(hist_Ceren_case4);
  hist_Ceren_case4->GetYaxis()->SetRangeUser(0, hist_Ceren_case6->GetMaximum() * 1.2);
  hist_Ceren_case4->GetXaxis()->SetRangeUser(0, 70000);

  TCanvas* c1 = new TCanvas("c1", "c1", 1000, 1000);
  
  c1->cd();
  hist_Scint_case4->Draw("Hist"); 
  c1->Update();
  TPaveStats* stats_case4_scint = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case4_scint->SetName("Quadrant 1");
  stats_case4_scint->SetTextColor(fColorVec[0]);
  stats_case4_scint->SetY1NDC(.75); 
  stats_case4_scint->SetY2NDC(.9);

  hist_Scint_case5->Draw("Hist&sames"); 
  c1->Update();
  TPaveStats* stats_case5_scint = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case5_scint->SetName("Quadrant 2");
  stats_case5_scint->SetTextColor(fColorVec[1]);
  stats_case5_scint->SetY1NDC(.6); 
  stats_case5_scint->SetY2NDC(.75);

  hist_Scint_case6->Draw("Hist&sames"); 
  c1->Update();
  TPaveStats* stats_case6_scint = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case6_scint->SetName("Quadrant 3");
  stats_case6_scint->SetTextColor(fColorVec[2]);
  stats_case6_scint->SetY1NDC(.45); 
  stats_case6_scint->SetY2NDC(.6);

  c1->SaveAs("TEST/Scint_CutComparisonVertical.pdf");


  c1->cd();
  hist_Ceren_case4->Draw("Hist"); 
  c1->Update();
  TPaveStats* stats_case4_ceren = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case4_ceren->SetName("Quadrant 1");
  stats_case4_ceren->SetTextColor(fColorVec[0]);
  stats_case4_ceren->SetY1NDC(.75); 
  stats_case4_ceren->SetY2NDC(.9);

  hist_Ceren_case5->Draw("Hist&sames"); 
  c1->Update();
  TPaveStats* stats_case5_ceren = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case5_ceren->SetName("Quadrant 2");
  stats_case5_ceren->SetTextColor(fColorVec[1]);
  stats_case5_ceren->SetY1NDC(.6); 
  stats_case5_ceren->SetY2NDC(.75);

  hist_Ceren_case6->Draw("Hist&sames"); 
  c1->Update();
  TPaveStats* stats_case6_ceren = (TPaveStats*)c1->GetPrimitive("stats");
  stats_case6_ceren->SetName("Quadrant 3");
  stats_case6_ceren->SetTextColor(fColorVec[2]);
  stats_case6_ceren->SetY1NDC(.45); 
  stats_case6_ceren->SetY2NDC(.6);

  c1->SaveAs("TEST/Ceren_CutComparisonVertical.pdf");

  //////////////////////////////////////////////////////////////
  // QUADRANT
  //////////////////////////////////////////////////////////////

  // TH2D* hist_LDWC_case16 = (TH2D*)fFile->Get("case16/LDWC_case16"); hist_LDWC_case16->SetStats(0);
  // TH2D* hist_LDWC_case17 = (TH2D*)fFile->Get("case17/LDWC_case17"); hist_LDWC_case17->SetStats(0);
  // TH2D* hist_LDWC_case18 = (TH2D*)fFile->Get("case18/LDWC_case18"); hist_LDWC_case18->SetStats(0);
  // TH2D* hist_LDWC_case19 = (TH2D*)fFile->Get("case19/LDWC_case19"); hist_LDWC_case19->SetStats(0);

  // hist_LDWC_case16->GetYaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case17->GetYaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case18->GetYaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case19->GetYaxis()->SetRangeUser(-30., 30.);

  // hist_LDWC_case16->GetXaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case17->GetXaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case18->GetXaxis()->SetRangeUser(-30., 30.);
  // hist_LDWC_case19->GetXaxis()->SetRangeUser(-30., 30.);

  // TH1D* hist_Scint_case16 = (TH1D*)fFile->Get("case16/Scint_case16"); hist_Scint_case16->SetLineWidth(2); hist_Scint_case16->Rebin(10); hist_Scint_case16->SetName("Scint - Quadrant 1"); hist_Scint_case16->SetLineColor(fColorVec[0]);
  // TH1D* hist_Scint_case17 = (TH1D*)fFile->Get("case17/Scint_case17"); hist_Scint_case17->SetLineWidth(2); hist_Scint_case17->Rebin(10); hist_Scint_case17->SetName("Scint - Quadrant 2"); hist_Scint_case17->SetLineColor(fColorVec[1]);
  // TH1D* hist_Scint_case18 = (TH1D*)fFile->Get("case18/Scint_case18"); hist_Scint_case18->SetLineWidth(2); hist_Scint_case18->Rebin(10); hist_Scint_case18->SetName("Scint - Quadrant 3"); hist_Scint_case18->SetLineColor(fColorVec[2]);
  // TH1D* hist_Scint_case19 = (TH1D*)fFile->Get("case19/Scint_case19"); hist_Scint_case19->SetLineWidth(2); hist_Scint_case19->Rebin(10); hist_Scint_case19->SetName("Scint - Quadrant 4"); hist_Scint_case19->SetLineColor(fColorVec[3]);

  // TH1D* hist_Ceren_case16 = (TH1D*)fFile->Get("case16/Ceren_case16"); hist_Ceren_case16->SetLineWidth(2); hist_Ceren_case16->Rebin(5); hist_Ceren_case16->SetName("Ceren - Quadrant 1"); hist_Ceren_case16->SetLineColor(fColorVec[0]);
  // TH1D* hist_Ceren_case17 = (TH1D*)fFile->Get("case17/Ceren_case17"); hist_Ceren_case17->SetLineWidth(2); hist_Ceren_case17->Rebin(5); hist_Ceren_case17->SetName("Ceren - Quadrant 2"); hist_Ceren_case17->SetLineColor(fColorVec[1]);
  // TH1D* hist_Ceren_case18 = (TH1D*)fFile->Get("case18/Ceren_case18"); hist_Ceren_case18->SetLineWidth(2); hist_Ceren_case18->Rebin(5); hist_Ceren_case18->SetName("Ceren - Quadrant 3"); hist_Ceren_case18->SetLineColor(fColorVec[2]);
  // TH1D* hist_Ceren_case19 = (TH1D*)fFile->Get("case19/Ceren_case19"); hist_Ceren_case19->SetLineWidth(2); hist_Ceren_case19->Rebin(5); hist_Ceren_case19->SetName("Ceren - Quadrant 4"); hist_Ceren_case19->SetLineColor(fColorVec[3]);

  
  // SetPlotFrame(hist_Scint_case16);
  // hist_Scint_case16->GetYaxis()->SetRangeUser(0, hist_Scint_case17->GetMaximum() * 1.2);

  // SetPlotFrame(hist_Ceren_case16);
  // hist_Ceren_case16->GetYaxis()->SetRangeUser(0, hist_Ceren_case19->GetMaximum() * 1.2);
  // hist_Ceren_case16->GetXaxis()->SetRangeUser(0, 70000);

  // TCanvas* c1 = new TCanvas("c1", "c1", 1000, 1000);
  
  // c1->cd();
  // hist_LDWC_case16->Draw("colz");
  // c1->SaveAs("TEST/LDWC_case16.pdf");
  
  // c1->cd();
  // hist_LDWC_case17->Draw("colz");
  // c1->SaveAs("TEST/LDWC_case17.pdf");

  // c1->cd();
  // hist_LDWC_case18->Draw("colz");
  // c1->SaveAs("TEST/LDWC_case18.pdf");
  
  // c1->cd();
  // hist_LDWC_case19->Draw("colz");
  // c1->SaveAs("TEST/LDWC_case19.pdf");
  
  
  // c1->cd();
  // hist_Scint_case16->Draw("Hist"); 
  // c1->Update();
  // TPaveStats* stats_case4_scint = (TPaveStats*)c1->GetPrimitive("stats");
  // stats_case4_scint->SetName("Quadrant 1");
  // stats_case4_scint->SetTextColor(fColorVec[0]);
  // stats_case4_scint->SetY1NDC(.75); 
  // stats_case4_scint->SetY2NDC(.9);

  // hist_Scint_case17->Draw("Hist&sames"); 
  // c1->Update();
  // TPaveStats* stats_case5_scint = (TPaveStats*)c1->GetPrimitive("stats");
  // stats_case5_scint->SetName("Quadrant 2");
  // stats_case5_scint->SetTextColor(fColorVec[1]);
  // stats_case5_scint->SetY1NDC(.6); 
  // stats_case5_scint->SetY2NDC(.75);

  // hist_Scint_case18->Draw("Hist&sames"); 
  // c1->Update();
  // TPaveStats* stats_case6_scint = (TPaveStats*)c1->GetPrimitive("stats");
  // stats_case6_scint->SetName("Quadrant 3");
  // stats_case6_scint->SetTextColor(fColorVec[2]);
  // stats_case6_scint->SetY1NDC(.45); 
  // stats_case6_scint->SetY2NDC(.6);

  // hist_Scint_case19->Draw("Hist&sames"); 
  // c1->Update();
  // TPaveStats* stats_case7_scint = (TPaveStats*)c1->GetPrimitive("stats");
  // stats_case7_scint->SetName("Quadrant 4");
  // stats_case7_scint->SetTextColor(fColorVec[3]);
  // stats_case7_scint->SetY1NDC(.3); 
  // stats_case7_scint->SetY2NDC(.45);

  // c1->SaveAs("TEST/Scint_CutComparisonQuadrant.pdf");

  


  return 0;
}