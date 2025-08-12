
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

#include "TH2D.h"

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

int main(int argc, char* argv[]) {

  ObjectCollection* fObj = new ObjectCollection(argc, argv);
  if (fObj->Help())
    return 1;

  ANSI_CODE ANSI = ANSI_CODE();

  TBconfig* fConfig = new TBconfig("./config_general.yml");
  const YAML::Node fConfig_YAML = fConfig->GetConfig();

  std::string fBaseDir = fConfig_YAML["BaseDirectory"].as<std::string>();
  std::string fMapping = fConfig_YAML["Mapping"].as<std::string>();

  TButility* fUtility = new TButility(fMapping);

  int fRunNum = 0;
  fObj->GetVariable("RunNumber", &fRunNum);

  std::vector<std::string> fModuleList = {};
  fObj->GetVector("module", &fModuleList);

  std::vector<TBcid> fCIDtoPlot = {};
  fCIDtoPlot.push_back(fUtility->GetCID(fModuleList.at(0)));

  TH1D* fWaveAvg = new TH1D((TString)Form("%s", fModuleList.at(0).c_str()), "WaveAvg", 1000, 0, 1000);

  TBread<TBwaveform> readerWave = TBread<TBwaveform>(fRunNum, -1, -1, false, fBaseDir, std::vector<int>{fCIDtoPlot.at(0).mid()});
  int fMaxEvent = readerWave.GetMaxEvent();

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

    TBevt<TBwaveform> anEvent = readerWave.GetAnEvent();
    auto tWaveform = anEvent.GetData(fCIDtoPlot.at(0)).waveform();

    for (int i = 1; i <= 1000; i++)
      fWaveAvg->Fill(i, tWaveform.at(i));

  }

  fWaveAvg->Scale(1. / fMaxEvent);
  for (int i = 1; i <= 1000; i++)
    fWaveAvg->SetBinError(i, 0.);

  TFile* fFile = new TFile(Form("./AVG/%s_Run%d.root", fModuleList.at(0).c_str(), fRunNum), "RECREATE");
  fFile->cd();
  fWaveAvg->Write();
  fFile->Close();

  return 0;
}