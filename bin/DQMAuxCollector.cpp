#include <string>
#include <iostream>
#include "TROOT.h"

#include "DQMServices/Core/interface/AuxCollectorRoot.h"

using std::string;

class DummyAuxCollector : public AuxCollectorRoot
{

 public:
  virtual void process(){}
  DummyAuxCollector(string host, int port_no) : 
    AuxCollectorRoot(host, "AuxCollector", port_no)
  {
    inputAvail_=true;
    run();
  }
  static 
  AuxCollectorRoot *instance(string host, 
			     int port_no = AuxCollectorRoot::defListenPort)
  {
    if(instance_==0)
      instance_ = new DummyAuxCollector(host, port_no);
    return instance_;
  }

 private:
  static AuxCollectorRoot * instance_;
};

AuxCollectorRoot *DummyAuxCollector::instance_=0; 

extern void InitGui(); 
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };
TROOT producer("producer","Simple histogram producer",initfuncs);

// usage: DQMAuxCollector <collector hostname> <port_no>
int main(int argc, char *argv[])
{
  string hostname;
  if(argc >= 2)
    hostname = argv[1];

  // default port #
  int port_no = 9090;
  if(argc >= 3) port_no = atoi(argv[2]);

  try
    {
      DummyAuxCollector::instance(hostname, port_no); 
    }
  catch (...)
    {
      return -1;
    }
  //  gROOT->SetBatch(kTRUE);
  //TApplication app("app",&argc,argv);
  return 0;
}
