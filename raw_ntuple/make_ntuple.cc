#include "TFile.h"
#include "TBconfig.h"
#include "TBobject.h"
#include "TBmid.h"
#include "TBevt.h"
#include "TBread.h"
#include "TButility.h"
#include "TBaux.h"

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <chrono>

#include <sys/types.h>

#include "TFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TROOT.h"


class DetIDContainter {

public:
    DetIDContainter() {}
  ~DetIDContainter() {}

  const std::vector<int> GetUniqueMID() const { 
    
    std::vector<int> tReturnVec;
    std::map<int, int> tMap;
    for (int i = 0; i < fCidCollection.size(); i++) {
      if (tMap.find(fCidCollection.at(i).mid()) == tMap.end()) {
        tMap.insert(std::make_pair(fCidCollection.at(i).mid(), 1));
        tReturnVec.push_back(fCidCollection.at(i).mid());
      }
    }

    return tReturnVec;
  }

  void AddCid(TBcid tCid) {
    fCidCollection.push_back(tCid);
  }

private:
  std::vector<TBcid> fCidCollection;

};

int GetDRS(std::vector<int> tDRS, int tChannel) {

  return tDRS.at((int)((tChannel - 1) / 8));
}

int main(int argc, char* argv[]) {

  ANSI_CODE ANSI = ANSI_CODE();

  gInterpreter->GenerateDictionary("vector<short>","vector");

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  int fRunNum;
  fObj->GetVariable("RunNumber", &fRunNum);

  TBconfig* fConfig = new TBconfig("./config_general.yml");
  const YAML::Node fConfig_YAML = fConfig->GetConfig();
  std::string fMapping = fConfig_YAML["Mapping"].as<std::string>();

  TButility* fUtility = new TButility(fMapping);

  DetIDContainter* fDetIDContainter = new DetIDContainter();

  std::vector<std::vector< std::vector<short> >> fWaveC;
  std::vector<std::vector< std::vector<short> >> fWaveS;

  std::vector<std::vector<int>> fDRS_C;
  std::vector<std::vector<int>> fDRS_S;

  for(int i = 0; i < 9; i++) {
    std::vector<std::vector<short>> tTEMPVEC = {std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}};
    fWaveC.push_back(tTEMPVEC);

    std::vector<int> tTEMPVEC_DRS = {0, 0, 0, 0};
    fDRS_C.push_back(tTEMPVEC_DRS);
  }

  for(int i = 0; i < 9; i++) {
    std::vector<std::vector<short>> tTEMPVEC = {std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}};
    fWaveS.push_back(tTEMPVEC);

    std::vector<int> tTEMPVEC_DRS = {0, 0, 0, 0};
    fDRS_S.push_back(tTEMPVEC_DRS);
  }

  std::vector<std::vector<TBcid>> fCidVec_C = {};
  std::vector<std::vector<TBcid>> fCidVec_S = {};

  for (int iModule = 0; iModule < 9; iModule++) {
    std::vector<TBcid> tTEMPVEC_C = {};
    fCidVec_C.push_back(tTEMPVEC_C);
    std::vector<TBcid> tTEMPVEC_S = {};
    fCidVec_S.push_back(tTEMPVEC_S);

    for (int iTower = 0; iTower < 4; iTower++) {
    
      std::string tName_C = (std::string)Form("M%d-T%d-C", iModule + 1, iTower + 1);
      fCidVec_C[iModule].push_back(fUtility->GetCID(tName_C));
    
      std::string tName_S = (std::string)Form("M%d-T%d-S", iModule + 1, iTower + 1);
      fCidVec_S[iModule].push_back(fUtility->GetCID(tName_S));

      fDetIDContainter->AddCid(fUtility->GetCID(tName_C));
      fDetIDContainter->AddCid(fUtility->GetCID(tName_S));
    }
  }

  std::vector<std::vector<short>> fWaveAUX = {std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}};
  std::vector<int> fDRS_AUX = {0, 0, 0, 0, 0};
  std::vector<float> fDWCPOS = {0., 0., 0., 0.};

  fDetIDContainter->AddCid(fUtility->GetCID("CC1"));
  fDetIDContainter->AddCid(fUtility->GetCID("CC2"));
  fDetIDContainter->AddCid(fUtility->GetCID("PS"));
  fDetIDContainter->AddCid(fUtility->GetCID("MC"));
  fDetIDContainter->AddCid(fUtility->GetCID("TC"));

  fDetIDContainter->AddCid(fUtility->GetCID("DWC1R"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC1L"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC1U"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC1D"));

  fDetIDContainter->AddCid(fUtility->GetCID("DWC2R"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC2L"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC2U"));
  fDetIDContainter->AddCid(fUtility->GetCID("DWC2D"));

  std::vector<std::vector<short>> fWaveLC = {
    std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, 
    std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, 
    std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}, std::vector<short>{}};
  std::vector<int> fDRS_LC = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<int> fLC_ID = {2, 4, 8, 10, 3, 5, 7, 9, 11, 12, 13, 19, 14, 15, 16, 20}; 

  for (int i = 0; i < fLC_ID.size(); i++) {
    std::string aNameLC = (std::string)Form("LC%d", fLC_ID.at(i));
    fDetIDContainter->AddCid(fUtility->GetCID(aNameLC));
  }

  TBaux fAux = TBaux(fConfig->GetConfig()["AUX"], fRunNum, false, false, false, *fUtility);
  fAux.SetMethod("PeakADC");
  fAux.SetRange(fConfig->GetConfig()["ModuleConfig"]);


  TTree* fTree = new TTree("data", "data");

  for (int iModule = 0; iModule < 9; iModule++) {
    for (int iTower = 0; iTower < 4; iTower++) {
      fTree->Branch((TString)Form("WAVE_M%d_T%d_C", iModule + 1, iTower + 1), &(fWaveC[iModule][iTower]));
      fTree->Branch((TString)Form("WAVE_M%d_T%d_S", iModule + 1, iTower + 1), &(fWaveS[iModule][iTower]));

      fTree->Branch((TString)Form("DRS_M%d_T%d_C", iModule + 1, iTower + 1), &(fDRS_C[iModule][iTower]));
      fTree->Branch((TString)Form("DRS_M%d_T%d_S", iModule + 1, iTower + 1), &(fDRS_S[iModule][iTower]));
    }
  }

  for (int i = 0; i < fLC_ID.size(); i++) {
    fTree->Branch((TString)Form("WAVE_LC%d", fLC_ID.at(i)), &(fWaveLC.at(i)));
    fTree->Branch((TString)Form("DRS_LC%d", fLC_ID.at(i)), &(fDRS_LC.at(i)));
  }
  
  fTree->Branch((TString)Form("WAVE_CC1"), &(fWaveAUX[0]));
  fTree->Branch((TString)Form("WAVE_CC2"), &(fWaveAUX[1]));
  fTree->Branch((TString)Form("WAVE_PC"), &(fWaveAUX[2]));
  fTree->Branch((TString)Form("WAVE_MC"), &(fWaveAUX[3]));
  fTree->Branch((TString)Form("WAVE_TC"), &(fWaveAUX[4]));

  fTree->Branch((TString)Form("DRS_CC1"), &(fDRS_AUX[0]));
  fTree->Branch((TString)Form("DRS_CC2"), &(fDRS_AUX[1]));
  fTree->Branch((TString)Form("DRS_PC"), &(fDRS_AUX[2]));
  fTree->Branch((TString)Form("DRS_MC"), &(fDRS_AUX[3]));
  fTree->Branch((TString)Form("DRS_TC"), &(fDRS_AUX[4]));

  fTree->Branch((TString)Form("DWC1X"), &(fDWCPOS[0]));
  fTree->Branch((TString)Form("DWC1Y"), &(fDWCPOS[1]));
  fTree->Branch((TString)Form("DWC2X"), &(fDWCPOS[2]));
  fTree->Branch((TString)Form("DWC2Y"), &(fDWCPOS[3]));
  
  TBread<TBwaveform> readerWave =
    TBread<TBwaveform>(
      fRunNum,
      -1,
      -1,
      false,
      "/pnfs/knu.ac.kr/data/cms/store/user/sungwon/2025_DRC_TB_Data",
      fDetIDContainter->GetUniqueMID()
    );

  int fMaxEvent = readerWave.GetMaxEvent();
  fMaxEvent = 1000;

  auto time_begin = std::chrono::system_clock::now();
  for (int i = 0; i < fMaxEvent; i++) { 
    
    if (i > 0 && i % 10 == 0) {

      std::chrono::duration time_taken = std::chrono::system_clock::now() - time_begin; // delete
      float percent_done = 1. * (float)(i) / (float)(fMaxEvent);
      std::chrono::duration time_left = time_taken * (1 / percent_done - 1);
      std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
      std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
      std::cout << "\r\033[F" //+ ANSI.HIGHLIGHTED_GREEN + ANSI.BLACK
                << " " << i << " / " << fMaxEvent << " events  " << minutes_left.count() << ":";
      printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);

      std::cout << ANSI.END << std::endl;
    }

    TBevt<TBwaveform> anEvent = readerWave.GetAnEvent();
    
    for (int iModule = 0; iModule < 9; iModule++) {
      for (int iTower = 0; iTower < 4; iTower++) {
        fWaveC.at(iModule).at(iTower) = anEvent.GetData(fCidVec_C.at(iModule).at(iTower)).waveform();
        fWaveS.at(iModule).at(iTower) = anEvent.GetData(fCidVec_S.at(iModule).at(iTower)).waveform();
        fDRS_C.at(iModule).at(iTower) = GetDRS(anEvent.Mid(fCidVec_C.at(iModule).at(iTower).mid()).GetDrsStop(), fCidVec_C.at(iModule).at(iTower).channel());
        fDRS_S.at(iModule).at(iTower) = GetDRS(anEvent.Mid(fCidVec_C.at(iModule).at(iTower).mid()).GetDrsStop(), fCidVec_S.at(iModule).at(iTower).channel());
      }
    }

    for (int iLC = 0; iLC < fLC_ID.size(); iLC++) {
      fWaveLC.at(iLC) = anEvent.GetData(fUtility->GetCID(Form("LC%d", fLC_ID.at(iLC)))).waveform();
      fDRS_LC.at(iLC) = GetDRS(anEvent.Mid(fUtility->GetCID(Form("LC%d", fLC_ID.at(iLC))).mid()).GetDrsStop(), fUtility->GetCID(Form("LC%d", fLC_ID.at(iLC))).channel());
    }
    
    fWaveAUX.at(0) = anEvent.GetData(fUtility->GetCID("CC1")).waveform();
    fWaveAUX.at(1) = anEvent.GetData(fUtility->GetCID("CC2")).waveform();
    fWaveAUX.at(2) = anEvent.GetData(fUtility->GetCID("PS")).waveform();
    fWaveAUX.at(3) = anEvent.GetData(fUtility->GetCID("MC")).waveform();
    fWaveAUX.at(4) = anEvent.GetData(fUtility->GetCID("TC")).waveform();

    fDRS_AUX.at(0) = GetDRS(anEvent.Mid(fUtility->GetCID("CC1").mid()).GetDrsStop(), fUtility->GetCID("CC1").channel());
    fDRS_AUX.at(1) = GetDRS(anEvent.Mid(fUtility->GetCID("CC2").mid()).GetDrsStop(), fUtility->GetCID("CC2").channel());
    fDRS_AUX.at(2) = GetDRS(anEvent.Mid(fUtility->GetCID("PS").mid()).GetDrsStop(), fUtility->GetCID("PS").channel());
    fDRS_AUX.at(3) = GetDRS(anEvent.Mid(fUtility->GetCID("MC").mid()).GetDrsStop(), fUtility->GetCID("MC").channel());
    fDRS_AUX.at(4) = GetDRS(anEvent.Mid(fUtility->GetCID("TC").mid()).GetDrsStop(), fUtility->GetCID("TC").channel());

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

    fTree->Fill();


  }


  TFile* fFile = new TFile(Form("./RAW/Run%d.root", fRunNum), "RECREATE");
  fFile->cd();
  fTree->Write();
  fFile->Close();



  return 1;
}



