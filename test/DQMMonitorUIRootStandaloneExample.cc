// -*- C++ -*-
//
// Package:    DQMServices/CoreROOT
// Class:      DQMMonitorUIRootStandaloneExample
// 
/**\class DQMMonitorUIRootStandaloneExample

Description: Simple example that fills monitoring elements and 
             compares them to reference

Implementation:
<Notes on implementation>
*/
//
//
//


// system include files
#include <memory>
#include <iostream>

// user include files
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ServiceRegistry/interface/Service.h"

#include "DQMServices/Core/interface/QTestStatus.h"
#include "DQMServices/Core/interface/DQMDefinitions.h"
#include "DQMServices/Core/interface/MonitorUIRoot.h"
#include "DQMServices/Core/interface/QCriterionRoot.h"

#include <TRandom.h>

#include <iostream>
#include <string>
#include <vector>

using std::cout; using std::endl;
using std::string; using std::vector;

const int sample_int_value = 5;
const float XMIN = -3; 
const float XMAX = 3;
//
// class declaration
//

class DQMMonitorUIRootStandaloneExample : public edm::EDAnalyzer {
public:
  explicit DQMMonitorUIRootStandaloneExample( const edm::ParameterSet& );
  ~DQMMonitorUIRootStandaloneExample();
  
  virtual void analyze( const edm::Event&, const edm::EventSetup& );
  
  virtual void endJob(void);

private:
  // ----------member data ---------------------------
  
  // the test objects
  MonitorElement * h1;
  MonitorElement * int1;
  // the reference objects
  MonitorElement * href;
  MonitorElement * intRef1;
  // event counter
  int counter;
  // back-end interface
  DaqMonitorBEInterface * dbe;
  // Monitor UI
  MonitorUserInterface * mui;
  // quality tests
  MEComp2RefChi2ROOT * chi2_test; // chi2 test
  MEComp2RefKolmogorovROOT * ks_test; // Kolmogorov test
  MEContentsXRangeROOT * xrange_test; // contents within x-range test
  MEContentsYRangeROOT * yrange_test;  // contents within y-range test
  MEDeadChannelROOT * deadChan_test;  // check against dead channels
  MENoisyChannelROOT * noisyChan_test;  // check against noisy channels
  MEComp2RefEqualH1ROOT * equalH1_test; // equality test for histograms
  MEComp2RefEqualIntROOT * equalInt_test; // equality test for integers
  MEMeanWithinExpectedROOT * meanNear_test; // mean-within-expected test
  // names for all quality tests
  vector<string> testNames;

  // create the monitoring structure
  void createMonitorElements(void);
  // create the quality tests
  void createQualityTests(void);
  // tune cuts for quality tests
  void tuneCuts(void);

  // use <ref> as the reference for the quality tests
  void setReference(MonitorElement * ref);
  // run quality tests;
  // (see Core/interface/QTestStatus.h)
  void runTests(void);
  // show channels that failed test
  void showBadChannels(QReport *qr);

  // gaussian parameters for generated distribution
  float mean_; float sigma_;
};

// create the monitoring structure
void DQMMonitorUIRootStandaloneExample::createMonitorElements(void)
{
  // set # of bins, range for histogram(s)
  const int NBINS = 50;
  h1 = dbe->book1D("histo_1", "Example 1D histogram.", NBINS, XMIN, XMAX );
  href = dbe->book1D("href", "Reference histogram", NBINS, XMIN, XMAX );
  
  int1 = dbe->bookInt("int1");
  intRef1 = dbe->bookInt("int1Ref");
  
  mean_ = (XMIN + XMAX)/2.0;
  sigma_ = (XMAX - XMIN)/6.0;
  
  // fill in reference histogram with random data
  for(unsigned i = 0; i != 10000; ++i)
    href->Fill(gRandom->Gaus(mean_, sigma_));
}

// create the quality tests
void DQMMonitorUIRootStandaloneExample::createQualityTests(void)
{
  testNames.push_back("my_chi2");
  testNames.push_back("my_kolm");
  testNames.push_back("my_xrange");
  testNames.push_back("my_yrange");
  testNames.push_back("deadChan");
  testNames.push_back("noisyChan");
  testNames.push_back("my_histo_equal");
  testNames.push_back("my_int_equal");
  testNames.push_back("meanNear");

  dqm::me_util::cvIt it = testNames.begin();

  // create the quality tests
  chi2_test = dynamic_cast<MEComp2RefChi2ROOT *> 
    (dbe->createQTest(Comp2RefChi2ROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

 ++it;
 ks_test = dynamic_cast<MEComp2RefKolmogorovROOT *>
    (dbe->createQTest(Comp2RefKolmogorovROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

 ++it;
  xrange_test = dynamic_cast<MEContentsXRangeROOT *>
    (dbe->createQTest(ContentsXRangeROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

  ++it;
  yrange_test = dynamic_cast<MEContentsYRangeROOT *>
    (dbe->createQTest(ContentsYRangeROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

 ++it;
  deadChan_test = dynamic_cast<MEDeadChannelROOT *>
    (dbe->createQTest(DeadChannelROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

  ++it;
  noisyChan_test = dynamic_cast<MENoisyChannelROOT *>
    (dbe->createQTest(NoisyChannelROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

  ++it;
  equalH1_test = dynamic_cast<MEComp2RefEqualH1ROOT *>
    (dbe->createQTest(Comp2RefEqualH1ROOT::getAlgoName(), *it) );
  dbe->useQTest("histo_1", *it);

  ++it;
  equalInt_test = dynamic_cast<MEComp2RefEqualIntROOT *>
    (dbe->createQTest(Comp2RefEqualIntROOT::getAlgoName(), *it) );
  dbe->useQTest("int1", *it);

  ++it;
  meanNear_test = dynamic_cast<MEMeanWithinExpectedROOT *>
    (dbe->createQTest(MeanWithinExpectedROOT::getAlgoName(), *it));
  dbe->useQTest("histo_1", *it);

}

// tune cuts for quality tests
void DQMMonitorUIRootStandaloneExample::tuneCuts(void)
{
  // set reference for chi2, ks tests
  setReference(href);
  // set allowed range to [10, 90]% of nominal
  xrange_test->setAllowedXRange(0.1*XMIN, 0.9*XMAX);
  // set allowed range to [0, 15] entries
  yrange_test->setAllowedYRange(0, 40);
  // set threshold for "dead channel" definition (default: 0)
  deadChan_test->setThreshold(0);
  // set tolerance for noisy channel
  noisyChan_test->setTolerance(0.30);
  // set # of neighboring channels for calculating average (default: 1)
  noisyChan_test->setNumNeighbors(2);
  // use RMS of distribution to judge if mean near expected value
  meanNear_test->useRMS();
  // set expected mean value
  meanNear_test->setExpectedMean(mean_);

  // fill in test integer
  int1->Fill(sample_int_value);
  
}


// constructors and destructor
DQMMonitorUIRootStandaloneExample::DQMMonitorUIRootStandaloneExample(const edm::ParameterSet& iConfig ) : counter(0)
{
  // get hold of back-end interface
  dbe = edm::Service<DaqMonitorBEInterface>().operator->();

  // instantiate Monitor UI without connecting to any monitoring server
  // (i.e. "standalone mode")
  mui = new MonitorUIRoot();

  createMonitorElements();
  
  createQualityTests();
 
  tuneCuts();

}

// use <ref> as the reference for the quality tests
void DQMMonitorUIRootStandaloneExample::setReference(MonitorElement * ref)
{
  if(chi2_test)chi2_test->setReference(ref);
  if(ks_test)ks_test->setReference(ref);  

  if(equalH1_test)equalH1_test->setReference(ref);
  // set reference equal to test integer
  intRef1->Fill(sample_int_value);
  if(equalInt_test)
    {
      if(ref)equalInt_test->setReference(intRef1);
      else equalInt_test->setReference(ref);
    }
}


DQMMonitorUIRootStandaloneExample::~DQMMonitorUIRootStandaloneExample()
{
  delete mui;
}

void DQMMonitorUIRootStandaloneExample::endJob(void)
{
  runTests();
}

// run quality tests;
// (see Core/interface/QTestStatus.h)
void DQMMonitorUIRootStandaloneExample::runTests()
{
  DaqMonitorBEInterface * bei = mui->getBEInterface();

  bei->runQTests(); // mui->update() would have the same result

  // determine the "global" status of the system
  int status = bei->getStatus();
  switch(status)
    {
    case dqm::qstatus::ERROR:
      cout << " Error(s)";
      break;
    case dqm::qstatus::WARNING:
      cout << " Warning(s)";
      break;
    case dqm::qstatus::OTHER:
      cout << " Some tests did not run;";
      break; 
    default:
      cout << " No problems";
    }
  cout << " reported after running the quality tests " << endl;
  //
  if(h1)
    {
  
      // get all warnings associated with me
      vector<QReport *> warnings = h1->getQWarnings();
      for(vector<QReport *>::const_iterator it = warnings.begin();
	  it != warnings.end(); ++it)
	cout << " *** Warning for " << h1->getName() << "," 
	     << (*it)->getMessage() << endl;
      // get all errors associated with me
      vector<QReport *> errors = h1->getQErrors();
      for(vector<QReport *>::const_iterator it = errors.begin();
	  it != errors.end(); ++it)
	cout << " *** Error for " << h1->getName() << ","
	     << (*it)->getMessage() << endl;
      
      cout << " ============================================= " << endl;
      cout << "         All messages for quality tests " << endl;
      cout << " ============================================= " << endl;
      for(dqm::me_util::cvIt it = testNames.begin(); 
	  it != testNames.end(); ++it)
	{
	  string test_message;
	  const QReport * qr = h1->getQReport(*it);
	  if(!qr && int1)
	    qr = int1->getQReport(*it);

	  if(qr)
	    cout << qr->getMessage() << endl;
	  else
	    cout << " **** Oops, report " << *it << " does not exist" << endl;
	}

      showBadChannels( (QReport *)h1->getQReport("my_yrange") );
      showBadChannels( (QReport *)h1->getQReport("deadChan") );
      showBadChannels( (QReport *)h1->getQReport("noisyChan") );

      // this test is too verbose...
      //      showBadChannels( (QReport *)h1->getQReport("my_histo_equal") );
      
    }
  

}

//
// member functions
//

// ------------ method called to produce the data  ------------
void DQMMonitorUIRootStandaloneExample::analyze(const edm::Event& iEvent, 
					 const edm::EventSetup& iSetup )
{
  // fill in test histogram with random data
  h1->Fill(gRandom->Gaus(mean_, sigma_)); 
  counter++;
}

// show channels that failed test
void DQMMonitorUIRootStandaloneExample::showBadChannels(QReport *qr)
{
  vector<dqm::me_util::Channel> badChannels = qr->getBadChannels();
  if(!badChannels.empty())
    cout << "\n Channels that failed test " << qr->getQRName() << ":\n";
  
  vector<dqm::me_util::Channel>::iterator it = badChannels.begin();
  while(it != badChannels.end())
    {
      // could use getBinX, getBinY, getBinZ for 2D, 3D histograms
      cout << " Channel #: " << it->getBin() 
	   << " Contents: " << it->getContents()
	   << " +- " << it->getRMS() << endl;	
      
      ++it;
    }
    
}


// define this as a plug-in
DEFINE_FWK_MODULE(DQMMonitorUIRootStandaloneExample);
