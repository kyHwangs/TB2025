#include "TBread.h"
#include "TButility.h"

#include <chrono>
#include <numeric>
#include <vector>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "TCanvas.h"
#include "TH1.h"

int main(int argc, char *argv[])
{
  // setup for prompt analysis
  // it could be like
  // int fRunNum = std::stoi(argv[1]);
  int fRunNum = 6034;
  int fMaxEvent = -1;
  int fMaxFile = -1;

  ANSI_CODE ANSI = ANSI_CODE();

  // initialize the utility class
  TButility util = TButility();
  util.LoadMapping("../mapping/mapping_TB2024_v1.root");

  // 1 17 M11-T1-S 1 0 0 0
  // 1 26 M11-T2-C 1 1 0 0
  // 3 4 M1-T4-C 1 1 0 0

  // prepare CIDs that we want to use
  TBcid cid_M11_T1_S = util.GetCID("M11-T1-S");
  TBcid cid_M1_T4_C = util.GetCID("M1-T4-C");
  TBcid cid_M11_T2_C = util.GetCID("M11-T2-C");

  // prepare the histograms wa want to draw
  TH1F* hist_M11_T1_S = new TH1F("M11_T1_S", ";intADC;nEvents", 4400, -4000, 40000);
  TH1F* hist_M1_T4_C  = new TH1F("M1_T4_C",  ";intADC;nEvents", 4400, -4000, 40000);
  TH1F* hist_M11_T2_C = new TH1F("M11_T2_C", ";intADC;nEvents", 4400, -4000, 40000);

  // adding all TBcid to vector: used in @MID-to-load for TBread<T>
  std::vector<TBcid> aCID = {TBcid(1, 17), TBcid(3,4), TBcid(1, 26)};
  // or below is possible
  // std::vector<std::string> aName = {"M11-T1-S", "M1-T4-C", "M11-T2-C"};
  // std::vector<TBcid> aCID;
  // for (int i = 0; i < aName.size(); i++)
  //   aCID.push_back(util.GetCID(aName.at(i)));

  // initializing the file reader
  // TBread<T>(RunNumber, MaximumEvents, MaximumFile, isLive, path-to-data, MID-to-load)
  // @RunNumber: which run we want to use
  // @MaximumEvents, @MaximumFile: set to -1 for prompt analysis
  // @path-to-data: path to data location
  // @MID-to-load: which MID data we want to use, is should be set to reduce time
  TBread<TBwaveform> readerWave = TBread<TBwaveform>(fRunNum, fMaxEvent, fMaxFile, false, "../sample_data", util.GetUniqueMID(aCID));

  // Set Maximum event
  if (fMaxEvent == -1)
    fMaxEvent = readerWave.GetMaxEvent();

  if (fMaxEvent > readerWave.GetMaxEvent())
    fMaxEvent = readerWave.GetMaxEvent();

  std::chrono::time_point time_begin = std::chrono::system_clock::now();
  for (int i = 0; i < fMaxEvent; i++) {
    if (i > 0 && i % 10 == 0) {

      std::chrono::duration time_taken = std::chrono::system_clock::now() - time_begin; // delete
      float percent_done = 1. * (float)i / (float)fMaxEvent;
      std::chrono::duration time_left = time_taken * (1 / percent_done - 1);
      std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
      std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
      std::cout << "\r\033[F" + ANSI.BOLD
                << " " << i << " / " << fMaxEvent << " events  " << minutes_left.count() << ":";
      printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);

      std::cout << ANSI.END << std::endl;
    }

    // Get waveform of certain channel we want to use
    TBevt<TBwaveform> aEvent = readerWave.GetAnEvent();
    TBwaveform wave_M11_T1_S = aEvent.GetData(cid_M11_T1_S);
    TBwaveform wave_M1_T4_C  = aEvent.GetData(cid_M1_T4_C);
    TBwaveform wave_M11_T2_C = aEvent.GetData(cid_M11_T2_C);

    // Get pedestal corrected waveform
    // TBwaveform::pedcorrectedWaveform(float ped): when using external (e.g. run pedestal)
    // TBwaveform::pedcorrectedWaveform(): when using event-by-event pedestal (ped = average of first 100 bin except 0th bin)
    std::vector<float> pedCorr_wave_M11_T1_S = wave_M11_T1_S.pedcorrectedWaveform();
    std::vector<float> pedCorr_wave_M1_T4_C  = wave_M1_T4_C.pedcorrectedWaveform();
    std::vector<float> pedCorr_wave_M11_T2_C = wave_M11_T2_C.pedcorrectedWaveform();

    // integration range: should be set channel by channel with average time structure
    int first = 130;
    int last = 250;

    // filling the histogram with integrated ADC
    hist_M11_T1_S->Fill(std::accumulate(pedCorr_wave_M11_T1_S.begin() + first, pedCorr_wave_M11_T1_S.begin() + last, 0.));
    hist_M1_T4_C->Fill(std::accumulate(pedCorr_wave_M1_T4_C.begin() + first, pedCorr_wave_M1_T4_C.begin() + last, 0.));
    hist_M11_T2_C->Fill(std::accumulate(pedCorr_wave_M11_T2_C.begin() + first, pedCorr_wave_M11_T2_C.begin() + last, 0.));
  }


  TCanvas* c = new TCanvas("", "");

  c->cd();
  hist_M11_T1_S->Draw("Hist");
  c->SaveAs((TString)hist_M11_T1_S->GetName() + ".pdf");

  c->cd();
  hist_M1_T4_C->Draw("Hist");
  c->SaveAs((TString)hist_M1_T4_C->GetName() + ".pdf");

  c->cd();
  hist_M11_T2_C->Draw("Hist");
  c->SaveAs((TString)hist_M11_T2_C->GetName() + ".pdf");

  return 0;
}
