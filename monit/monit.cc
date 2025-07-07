
#include <iostream>
#include "TBmonit.h"
#include "TBobject.h"

int main(int argc, char* argv[]) {

  ObjectCollection* obj = new ObjectCollection(argc, argv);
  if (obj->Help())
    return 1;

  // obj->Print();

  TBmonit<TBwaveform>* monit = new TBmonit<TBwaveform>(std::move(obj));
  monit->Loop();





  return 1;
}
