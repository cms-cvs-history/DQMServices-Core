#include "DQMServices/Core/interface/MonitorElement.h"
#include "DQMServices/Core/interface/MonitorUIRoot.h"

#include <iostream>
#include <math.h>

#include <TApplication.h>
#include <TBrowser.h>
#include <TRandom.h> // this is just the random number generator

#include <SealBase/Callback.h>


using std::cout; using std::endl;
using std::string; using std::vector;

class DQMLocalGUI
{
public:
  // ---------------------------------------------------------------------
  // ctor
  DQMLocalGUI(string hostname, int port_no, string cfuname)
  {
    cout<< " GUI " << cfuname << " begins requesting monitoring from host " 
	<< hostname << endl;
  
    int reconnect_delay_secs = 5;
    // start user interface instance
    mui = new MonitorUIRoot(hostname, port_no, cfuname, 
			    reconnect_delay_secs);
    bei = mui->getBEInterface();		    
  }

  // ---------------------------------------------------------------------
  // dtor
  ~DQMLocalGUI(void)
  {
    if(mui)delete mui;
  }

  // ---------------------------------------------------------------------
  // save monitoring structure every N monitoring cycles
  void save()
  { 
        // save monitoring structure in root-file
        bei->save("test.root");
  }

  // ---------------------------------------------------------------------
  // receive monitoring data (+send subscription requests)
  // return success flag
  bool receiveMonitoring(TCanvas* canvas)
  {
    
    static int iupdate=0;
    // this is the "main" loop where we receive monitoring
    bool ret = mui->update();
    iupdate++;
    
    // subscribe to new monitorable
    // mui->subscribe("Collector",true);
    // subscribe to new monitorable matching pattern "*/C1/C2/*"
    mui->subscribeNew("*");    
    
    bei->showDirStructure();
    
    int icanv=1;
    
    vector<MonitorElement*> contents = bei->getAllContents("*");
    for(vector<MonitorElement *>::iterator it = contents.begin(); it != contents.end(); ++it)
    {
        
        string fullname =  (*it)->getFullname();
        cout << iupdate << " Monitoring Element = " << fullname << endl;
        if ((*it)->getPathname() != ".") {
        MonitorElementT<TNamed>* ob = dynamic_cast<MonitorElementT<TNamed>*>(*it);
	canvas->cd(icanv);
//        if (ob && (ob->operator->())->IsA()->InheritsFrom("TH1F")) {
        if (ob) {
	  cout << " Drawing " << (*it)->getFullname() << endl;
          ob->operator->()->Draw();
          icanv++; if (icanv>6) icanv=1;
          gPad->Update();
        }
	}
    }
    
    if (iupdate%200==0) bei->save("test.root");
    
    return ret;
  }


private:
  MonitorUserInterface * mui;
  DaqMonitorBEInterface * bei;
  
};


// usage: DQMLocalGUI <name> <host name> <port_no>
// <name>: name of client; default: User0
// <host name>: name of collector; 
//                examples: localhost (default), lxcmse2.cern.ch, etc.
// <port_no>: port number for connection with collector
int main(int argc, char** argv)
{
  TApplication app("app",&argc,argv);
  // default client name
  string cfuname = "DQMLocalGUI";
  // default collector host name
  string hostname = "localhost";
  // default port #
  int port_no = 9090;

  if(argc >= 2) cfuname = argv[1];
  if(argc >= 3) hostname = argv[2];
  if(argc >= 4) port_no = atoi(argv[3]);

  DQMLocalGUI * gui = new DQMLocalGUI(hostname, port_no, cfuname);
  
  TCanvas* canvas = new TCanvas("DQMLocalGUI", "DQM Local GUI", 600, 700);
  canvas->Divide(2, 3);

  bool stay_in_loop = true;
  
  while( stay_in_loop )
    {
      // receive monitoring data (+send subscription requests)
      // return success flag
      stay_in_loop = gui->receiveMonitoring(canvas);

    }
  
  return 0;
}

