#include "TBmonit.h"
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

#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include "TFile.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TROOT.h"

template <typename T>
TBmonit<T>::TBmonit(const std::string &fConfig_, int fRunNum_)
: fConfig(TBconfig(fConfig_)), fRunNum(fRunNum_), fMaxEvent(-1), fMaxFile(-1)
{
  fIsLive = false;
  fAuxPlotting = false;
  fAuxCut  = false;
  fDraw = false;
  fUtility = TButility();
}

template <typename T>
TBmonit<T>::TBmonit(ObjectCollection* fObj_)
: fObj(fObj_), fConfig(TBconfig("./config_general.yml"))
{
  const YAML::Node fConfig_YAML = fConfig.GetConfig();

  fBaseDir = fConfig_YAML["BaseDirectory"].as<std::string>();
  fMapping = fConfig_YAML["Mapping"].as<std::string>();

  fUtility = TButility(fMapping);

  // fCaseName = fNodePlot["Name"].as<std::string>()

  fObj->GetVariable("RunNumber", &fRunNum);
  fObj->GetVariable("MaxEvent", &fMaxEvent);
  fObj->GetVariable("MaxFile", &fMaxFile);

  fObj->GetVariable("LIVE", &fIsLive);
  fObj->GetVariable("AUX", &fAuxPlotting);
  fObj->GetVariable("AUXcut", &fAuxCut);
  fObj->GetVariable("DRAW", &fDraw);

  fObj->GetVariable("particle", &fParticle);

  gROOT->SetBatch(!fDraw);

  if (fIsLive) {
    fMaxEvent = -1;
    fMaxFile = -1;
  }

  int argc = 0;
  char* argv[] = {};
  fApp = new TApplication("app", &argc, argv);
  if (fIsLive)
    fApp->SetReturnFromRun(true);
}

template <typename T>
void TBmonit<T>::GetFormattedRamInfo() {

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

template <typename T>
void TBmonit<T>::Loop() {
  if (fIsLive) LoopLive();
  else         LoopAfterRun();
}

template <typename T>
void TBmonit<T>::LoopLive() {

  ANSI_CODE ANSI = ANSI_CODE();

  TBplotengine fPlotter = TBplotengine(fConfig.GetConfig()["ModuleConfig"], fRunNum, fIsLive, fDraw, fUtility);
  TBaux fAux = TBaux(fConfig.GetConfig()["AUX"], fRunNum, fAuxPlotting, fIsLive, fDraw, fUtility);

  std::string aCase;
  fObj->GetVariable("type", &aCase); //'single', 'heatmap'
  if (aCase == "null") {
    // !thow exception
  } else {
    fPlotter.SetCase(aCase);
  }

  std::string aMethod;
  fObj->GetVariable("method", &aMethod); //'IntADC', 'PeakADC', 'Avg', 'Overlay'
  if (aMethod == "null") {
    // !throw exception
  } else {
    fPlotter.SetMethod(aMethod);
    fAux.SetMethod("PeakADC");
  }

  std::vector<std::string> aModules = {};
  fObj->GetVector("module", &aModules);
  // std::cout << "std::vector<std::string> aModules " << aModules.size() << " " << aModules.at(0) << std::endl;

  if (aModules.size() == 1 && (aCase == "heatmap" || aCase == "module")) {
    fPlotter.SetModule(aModules.at(0));
  } else if (aCase == "full") {

  } else if (aCase == "single") {

    // std::vector<TBcid> aCID;
    // for (int i = 0; i < aModules.size(); i++)
    //   aCID.push_back(fUtility.GetCID(aModules.at(i)));

    fPlotter.SetCID(aModules);
  } else {
    // !throw exception
  }

  fPlotter.init();
  if (fAuxPlotting) {
    fAux.init();
    fAux.SetRange(fConfig.GetConfig()["ModuleConfig"]);
  }

  if (fParticle != "null")
    fAux.SetParticle(fParticle);

  fPlotter.SetApp(fApp);
  if (fAuxPlotting)
    fAux.SetApp(fApp);

  // for (int idx = 0; idx < 10; idx++) {

  std::vector<int> tUniqueMID = {};
  if (fAuxPlotting) tUniqueMID = fUtility.GetUniqueMID(fPlotter.GetUniqueMID(), fAux.GetUniqueMID());
  else              tUniqueMID = fPlotter.GetUniqueMID();

  if (fAuxCut) {
    fPlotter.SetAUXCut(true);
    fAux.SetAUXCut(true);
  }

  TBread<TBwaveform> readerWave =
    TBread<TBwaveform>(
      fRunNum,
      fMaxEvent,
      fMaxFile,
      fIsLive,
      fBaseDir,
      tUniqueMID
    );

    while(1) {
      readerWave.CheckNextFileExistence();

      int iLiveCurrentEvent = readerWave.GetLiveCurrentEvent();
      int iCurrentEvent = readerWave.GetCurrentEvent();
      int iMaxEvent = readerWave.GetLiveMaxEvent();

      std::chrono::time_point time_begin = std::chrono::system_clock::now();

      for (int i = iCurrentEvent; i < iMaxEvent; i++) {

        if (i > iCurrentEvent && i % 10 == 0) {

          std::chrono::duration time_taken = std::chrono::system_clock::now() - time_begin; // delete
          float percent_done = 1. * (float)(i - iCurrentEvent) / (float)(iMaxEvent - iCurrentEvent);
          std::chrono::duration time_left = time_taken * (1 / percent_done - 1);
          std::chrono::minutes minutes_left = std::chrono::duration_cast<std::chrono::minutes>(time_left);
          std::chrono::seconds seconds_left = std::chrono::duration_cast<std::chrono::seconds>(time_left - minutes_left);
          std::cout << "\r\033[F" //+ ANSI.HIGHLIGHTED_GREEN + ANSI.BLACK
                    << " " << i << " / " << iMaxEvent << " events  " << minutes_left.count() << ":";
          printf("%02d left (%.1f %%) | ", int(seconds_left.count()), percent_done * 100);
          GetFormattedRamInfo();

          std::cout << ANSI.END << std::endl;
        }

        TBevt<TBwaveform> anEvent = readerWave.GetAnEvent();

        if (fAuxCut)
          if (!fAux.IsPassing(anEvent))
            continue;

        fPlotter.Fill(anEvent);

        if (fAuxPlotting)
          fAux.Fill(anEvent);

      }
      fPlotter.Update();
      if (fAuxPlotting)
        fAux.Update();
    }
}


template <typename T>
void TBmonit<T>::LoopAfterRun() {

  ANSI_CODE ANSI = ANSI_CODE();

  TBplotengine fPlotter = TBplotengine(fConfig.GetConfig()["ModuleConfig"], fRunNum, fIsLive, fDraw, fUtility);
  TBaux fAux = TBaux(fConfig.GetConfig()["AUX"], fRunNum, fAuxPlotting, fIsLive, fDraw, fUtility);

  if (fAuxCut) {
    fPlotter.SetAUXCut(true);
    fAux.SetAUXCut(true);
  }

  std::string aCase;
  fObj->GetVariable("type", &aCase); //'single', 'heatmap', 'module'
  if (aCase == "null") {
    // !thow exception
  } else {
    fPlotter.SetCase(aCase);
  }

  std::string aMethod;
  fObj->GetVariable("method", &aMethod); //'IntADC', 'PeakADC', 'Avg', 'Overlay'
  if (aMethod == "null") {
    // !throw exception
  } else {
    fPlotter.SetMethod(aMethod);
    fAux.SetMethod("PeakADC");
  }

  std::vector<std::string> aModules = {};
  fObj->GetVector("module", &aModules);

  // std::cout << "std::vector<std::string> aModules " << aModules.size() << " " << aModules.at(0) << std::endl;
  if (aModules.size() == 1 && (aCase == "heatmap" || aCase == "module")) {
    fPlotter.SetModule(aModules.at(0));
  } else if (aCase == "single") {
    // std::vector<TBcid> aCID;
    // for (int i = 0; i < aModules.size(); i++)
    //   aCID.push_back(fUtility.GetCID(aModules.at(i)));

    fPlotter.SetCID(aModules);
  } else {
    // !throw exception
  }

  fPlotter.init();
  if (fAuxPlotting) {
    fAux.init();
    fAux.SetRange(fConfig.GetConfig()["ModuleConfig"]);
    fPlotter.SetAUX();
  }

  if (fParticle != "null")
    fAux.SetParticle(fParticle);

  fPlotter.SetApp(fApp);
  if (fAuxPlotting)
    fAux.SetApp(fApp);

  std::vector<int> tUniqueMID = {};
  if (fAuxPlotting) tUniqueMID = fUtility.GetUniqueMID(fPlotter.GetUniqueMID(), fAux.GetUniqueMID());
  else              tUniqueMID = fPlotter.GetUniqueMID();

  TBread<TBwaveform> readerWave =
    TBread<TBwaveform>(
      fRunNum,
      fMaxEvent,
      fMaxFile,
      fIsLive,
      fBaseDir,
      tUniqueMID
    );


  if (fMaxEvent == -1)
    fMaxEvent = readerWave.GetMaxEvent();

  if (fMaxEvent > readerWave.GetMaxEvent())
    fMaxEvent = readerWave.GetMaxEvent();

  std::chrono::time_point time_begin = std::chrono::system_clock::now();
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
      GetFormattedRamInfo();

      std::cout << ANSI.END << std::endl;
    }

    TBevt<TBwaveform> anEvent = readerWave.GetAnEvent();

    if (fAuxCut)
      if (!fAux.IsPassing(anEvent))
        continue;

    fPlotter.Fill(anEvent);
    if (fAuxPlotting)
      fAux.Fill(anEvent);
  }
  fPlotter.Update();
  if (fAuxPlotting)
    fAux.Update();
}


template class TBmonit<TBwaveform>;
template class TBmonit<TBfastmode>;
