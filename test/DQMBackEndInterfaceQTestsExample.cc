// -*- C++ -*-
//
// Package:    DQMServices/CoreROOT
// Class:      DQMBackEndInterfaceQTestsExample
// 
/**\class DQMBackEndInterfaceQTestsExample

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

#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "DQMServices/Core/interface/QTestStatus.h"
#include "DQMServices/Core/interface/QCriterionRoot.h"

#include <TRandom.h>

#include <iostream>
#include <string>

using std::cout; using std::endl;
using std::string; using std::vector;

const int sample_int_value = 5;
//
// class declaration
//

class DQMBackEndInterfaceQTestsExample : public edm::EDAnalyzer {
public:
  explicit DQMBackEndInterfaceQTestsExample( const edm::ParameterSet& );
  ~DQMBackEndInterfaceQTestsExample();
  
  virtual void analyze( const edm::Event&, const edm::EventSetup& );
  
  virtual void endJob(void);

private:
  // ----------member data ---------------------------
  
  // the test objects
  MonitorElement * h1;
  MonitorElement * int1;
  MonitorElement *poMPLandauH1_;
  // the reference objects
  MonitorElement * href;
  MonitorElement * intRef1;
  // more test objects
  MonitorElement * testh2f;
  MonitorElement * testprof;
  MonitorElement * testprof2d;
  // event counter
  int counter;
  // back-end interface
  DaqMonitorBEInterface * dbe;
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
  MEMostProbableLandauROOT *poMPLandau_test_;
  // contents within z-range tests
  MEContentsTH2FWithinRangeROOT * zrangeh2f_test; 
  MEContentsProfWithinRangeROOT * zrangeprof_test; 
  MEContentsProf2DWithinRangeROOT * zrangeprof2d_test;

  // use <ref> as the reference for the quality tests
  void setReference(MonitorElement * ref);
  // run quality tests; expected_status: test status that is expected
  // (see Core/interface/QTestStatus.h)
  // test_type: info message on what kind of tests are run
  void runTests(int expected_status, string test_type);
  // called by runTests; return status
  int checkTest(QCriterion *qc);
  // show channels that failed test
  void showBadChannels(QCriterion *qc);

  // gaussian parameters for generated distribution
  float mean_; float sigma_;
  float dLandauMP_;
  float dLandauSigma_;
};

// constructors and destructor
DQMBackEndInterfaceQTestsExample::DQMBackEndInterfaceQTestsExample(const edm::ParameterSet& iConfig ) : counter(0)
{
  // get hold of back-end interface
  dbe = edm::Service<DaqMonitorBEInterface>().operator->();

  // set # of bins, range for histogram(s)
  const int NBINS = 50; const float XMIN = -3; const float XMAX = 3;
  h1 = dbe->book1D("histo_1", "Example 1D histogram.", NBINS, XMIN, XMAX );
  href = dbe->book1D("href", "Reference histogram", NBINS, XMIN, XMAX );
  poMPLandauH1_ = dbe->book1D( "landau_hist", "Example Landau Histogram",
                               NBINS, XMIN, XMAX);

  int1 = dbe->bookInt("int1");
  intRef1 = dbe->bookInt("int1Ref");


  testh2f = dbe->book2D("testh2f", "testh2f histo", NBINS/10, XMIN, XMAX, 
			NBINS/10, XMIN, XMAX);
  // profile histogram
  testprof = dbe->bookProfile("testprof", "testprof histo", 
			      NBINS/10, XMIN, XMAX, NBINS/10, XMIN, XMAX);
  // 2D profile histogram
  testprof2d = dbe->bookProfile2D("testprof2d", "testprof2d histo", 
				  NBINS/10, XMIN, XMAX, NBINS/10, XMIN, XMAX,
				  NBINS/10, XMIN, XMAX);
  
  mean_ = (XMIN + XMAX)/2.0;
  sigma_ = (XMAX - XMIN)/6.0;

  dLandauMP_    = ( XMIN + XMAX) / 4.0;
  dLandauSigma_ = ( XMAX - XMIN) / 6.0;

  // fill in reference histogram with random data
  for(unsigned i = 0; i != 10000; ++i)
    href->Fill(gRandom->Gaus(mean_, sigma_));
  
  // instantiate the quality tests
  chi2_test = new MEComp2RefChi2ROOT("my_chi2");
  ks_test = new MEComp2RefKolmogorovROOT("my_kolm");
  xrange_test = new MEContentsXRangeROOT("my_xrange");
  yrange_test = new MEContentsYRangeROOT("my_yrange");
  deadChan_test = new MEDeadChannelROOT("deadChan");
  noisyChan_test = new MENoisyChannelROOT("noisyChan");
  equalH1_test = new MEComp2RefEqualH1ROOT("my_histo_equal");
  equalInt_test = new MEComp2RefEqualIntROOT("my_int_equal");
  meanNear_test = new MEMeanWithinExpectedROOT("meanNear");
  zrangeh2f_test = new MEContentsTH2FWithinRangeROOT("zrangeh2f");
  zrangeprof_test = new MEContentsProfWithinRangeROOT("zrangeprof");
  zrangeprof2d_test = new MEContentsProf2DWithinRangeROOT("zrangeprof2d");
  poMPLandau_test_ = new MEMostProbableLandauROOT( "mplandau");
  
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
  // Setup MostProbableLandau
  poMPLandau_test_->setXMin( 0.1 * XMIN);
  poMPLandau_test_->setXMax( 0.9 * XMAX);
  poMPLandau_test_->setMostProbable( dLandauMP_);
  poMPLandau_test_->setSigma( dLandauSigma_);

  // fill in test integer
  int1->Fill(sample_int_value);
}

// use <ref> as the reference for the quality tests
void DQMBackEndInterfaceQTestsExample::setReference(MonitorElement * ref)
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


DQMBackEndInterfaceQTestsExample::~DQMBackEndInterfaceQTestsExample()
{
  // do anything here that needs to be done at destruction time
  // (e.g. close files, deallocate resources etc.)
  if(chi2_test)delete chi2_test;
  if(ks_test)delete ks_test;
  if(xrange_test)delete xrange_test;
  if(yrange_test)delete yrange_test;
  if(deadChan_test)delete deadChan_test;
  if(noisyChan_test)delete noisyChan_test;
  if(equalH1_test) delete equalH1_test;
  if(equalInt_test) delete equalInt_test;
  if(meanNear_test)delete meanNear_test;
  if(zrangeh2f_test) delete zrangeh2f_test;
  if(zrangeprof_test) delete zrangeprof_test;
  if(zrangeprof2d_test) delete zrangeprof2d_test;
  if( poMPLandau_test_) delete poMPLandau_test_;

}

void DQMBackEndInterfaceQTestsExample::endJob(void)
{
  // test # 1:  disable tests
  chi2_test->disable();
  ks_test->disable();
  xrange_test->disable();
  yrange_test->disable();
  deadChan_test->disable();
  noisyChan_test->disable();
  equalH1_test->disable();
  equalInt_test->disable();
  meanNear_test->disable();
  zrangeh2f_test->disable();
  zrangeprof_test->disable();
  zrangeprof2d_test->disable();
  poMPLandau_test_->disable();

  // attempt to run tests after they have been disabled
  runTests(dqm::qstatus::DISABLED, "disabled tests");

  // test #2: enable tests, run w/o reference 
  // (affects only chi2_test, ks_test and equality tests)
  chi2_test->enable();
  ks_test->enable();
  xrange_test->enable();
  yrange_test->enable();
  deadChan_test->enable();
  noisyChan_test->enable();
  equalH1_test->enable();
  equalInt_test->enable();
  // test will not run because mean value has not been set
  meanNear_test->enable(); 
  // test will not run because allowed range for mean & RMS has not been set
  zrangeh2f_test->enable();
  zrangeprof_test->enable();
  zrangeprof2d_test->enable();
  poMPLandau_test_->enable();
  setReference(0);
  
  // attempt to run tests w/o a reference histogram
  runTests(dqm::qstatus::INVALID, "tests w/o reference");

  // test #3: set reference, but assume low statistics
  setReference(href);
  // set expected mean value
  meanNear_test->setExpectedMean(mean_);
  // set allowed range for mean & RMS
  zrangeh2f_test->setMeanRange(0., 3.);
  zrangeh2f_test->setRMSRange(0., 2.);
  zrangeh2f_test->setMeanTolerance(2.);
  zrangeprof_test->setMeanRange(0., 1.);
  zrangeprof_test->setRMSRange(0., 1.);
  zrangeprof_test->setMeanTolerance(2.);
  zrangeprof2d_test->setMeanRange(0., 3.);
  zrangeprof2d_test->setRMSRange(0., 1.);
  zrangeprof2d_test->setMeanTolerance(2.);

  chi2_test->setMinimumEntries(10000);
  ks_test->setMinimumEntries(10000);
  xrange_test->setMinimumEntries(10000);
  yrange_test->setMinimumEntries(10000);
  deadChan_test->setMinimumEntries(10000);
  noisyChan_test->setMinimumEntries(10000);
  equalH1_test->setMinimumEntries(10000);
  equalInt_test->setMinimumEntries(10000);
  meanNear_test->setMinimumEntries(10000);
  zrangeh2f_test->setMinimumEntries(10000);
  zrangeprof_test->setMinimumEntries(10000);
  zrangeprof2d_test->setMinimumEntries(10000);
  poMPLandau_test_->setMinimumEntries( 10000);

  // attempt to run tests after specifying a minimum # of entries that is too high
  runTests(dqm::qstatus::INSUF_STAT, "tests w/ low statistics");

  // test #4: this should be the normal test
  chi2_test->setMinimumEntries(0);
  ks_test->setMinimumEntries(0);
  xrange_test->setMinimumEntries(0);
  yrange_test->setMinimumEntries(0);
  deadChan_test->setMinimumEntries(0);
  noisyChan_test->setMinimumEntries(0);
  equalH1_test->setMinimumEntries(0);
  equalInt_test->setMinimumEntries(0);
  meanNear_test->setMinimumEntries(0);
  zrangeh2f_test->setMinimumEntries(0);
  zrangeprof_test->setMinimumEntries(0);
  zrangeprof2d_test->setMinimumEntries(0);
  poMPLandau_test_->setMinimumEntries( 0);
  // run tests normally
  runTests(0, "regular tests");
}

// run quality tests; expected_status: test status that is expected
// (see Core/interface/QTestStatus.h)
// test_type: info message on what kind of tests are run
void DQMBackEndInterfaceQTestsExample::runTests(int expected_status, 
					    string test_type)
{
  cout << " ========================================================== " << endl;
  cout << " Results of attempt to run " << test_type << endl;
  
  QCriterionRoot<TH1F> * qcr = (QCriterionRoot<TH1F> *) chi2_test;
  qcr->runTest(h1);
  checkTest(chi2_test);

  qcr = (QCriterionRoot<TH1F> *) ks_test;
  qcr->runTest(h1);
  checkTest(ks_test);

  qcr = (QCriterionRoot<TH1F> *) xrange_test;
  qcr->runTest(h1);
  checkTest(xrange_test);

  qcr = (QCriterionRoot<TH1F> *) yrange_test;
  qcr->runTest(h1);
  checkTest(yrange_test);
  showBadChannels(qcr);

  qcr = (QCriterionRoot<TH1F> *) deadChan_test;
  qcr->runTest(h1);
  checkTest(deadChan_test);
  showBadChannels(qcr);

  qcr = (QCriterionRoot<TH1F> *) noisyChan_test;
  qcr->runTest(h1);
  checkTest(noisyChan_test);
  showBadChannels(qcr);

  qcr = (QCriterionRoot<TH1F> *) meanNear_test;
  qcr->runTest(h1);
  checkTest(meanNear_test);

  qcr = (QCriterionRoot<TH1F> *) poMPLandau_test_;
  qcr->runTest( poMPLandauH1_);
  checkTest( poMPLandau_test_);

  qcr = (QCriterionRoot<TH1F> *) equalH1_test;
  qcr->runTest(h1);
  checkTest(equalH1_test);
  showBadChannels(qcr);

  QCriterionRoot<int> * qcr_int = (QCriterionRoot<int> *) equalInt_test;
  qcr_int->runTest(int1);
  checkTest(equalInt_test);

  QCriterionRoot<TH2F> * qcr2 = (QCriterionRoot<TH2F> *) zrangeh2f_test;
  qcr2->runTest(testh2f);
  checkTest(zrangeh2f_test);
  showBadChannels(qcr2);

  QCriterionRoot<TProfile> * qcr_p 
    = (QCriterionRoot<TProfile> *) zrangeprof_test;
  qcr_p->runTest(testprof);
  checkTest(zrangeprof_test);
  showBadChannels(qcr_p);

  QCriterionRoot<TProfile2D> * qcr_p2 
    = (QCriterionRoot<TProfile2D> *) zrangeprof2d_test;
  qcr_p2->runTest(testprof2d);
  checkTest(zrangeprof2d_test);
  showBadChannels(qcr_p2);

  int status = 0;
  status = chi2_test->getStatus();
  if(expected_status)
    assert(status == expected_status);
  status = ks_test->getStatus();
  if(expected_status)
    assert(status == expected_status);  

  status = xrange_test->getStatus();
  // there is no "INVALID" result when running "contents within x-range" test
  if(expected_status && expected_status != dqm::qstatus::INVALID)
    assert(status == expected_status);  

  status = yrange_test->getStatus();
  // there is no "INVALID" result when running "contents within y-range" test
  if(expected_status && expected_status != dqm::qstatus::INVALID)
    assert(status == expected_status);  

  status = deadChan_test->getStatus();
  // there is no "INVALID" result when running "dead channel" test
  if(expected_status && expected_status != dqm::qstatus::INVALID)
    assert(status == expected_status);  

  status = noisyChan_test->getStatus();
  // there is no "INVALID" result when running "noisy channel" test
  if(expected_status && expected_status != dqm::qstatus::INVALID)
    assert(status == expected_status);  

  status = meanNear_test->getStatus();
  if(expected_status)
    assert(status == expected_status);  

  status = poMPLandau_test_->getStatus();
  if( expected_status) {
    assert( status == expected_status);
  }

  status = equalH1_test->getStatus();
  if(expected_status)
    assert(status == expected_status);  

  status = equalInt_test->getStatus();
  // there is no "INSUF_STAT" result when running "int equality" tests
  if(expected_status && expected_status != dqm::qstatus::INSUF_STAT)
    assert(status == expected_status);  

  status = zrangeh2f_test->getStatus();
   if(expected_status)
    assert(status == expected_status);

  status = zrangeprof_test->getStatus();
   if(expected_status)
    assert(status == expected_status);

  status = zrangeprof2d_test->getStatus();
   if(expected_status)
    assert(status == expected_status); 

}

// called by runTests; return status
int DQMBackEndInterfaceQTestsExample::checkTest(QCriterion *qc)
{
  if(!qc)return -1;
  
  int status = qc->getStatus();
  cout << " Test name: " << qc->getName() << " (Algorithm: " 
	    << qc->getAlgoName() << "), Result:"; 
  
  switch(status)
    {
    case dqm::qstatus::STATUS_OK: 
      cout << " Status ok " << endl;
      break;
    case dqm::qstatus::WARNING: 
      cout << " Warning " << endl;
      break;
    case dqm::qstatus::ERROR : 
      cout << " Error " << endl;
      break;
    case dqm::qstatus::DISABLED : 
      cout << " Disabled " << endl;
      break;
    case dqm::qstatus::INVALID: 
      cout << " Invalid " << endl;
      break;
    case dqm::qstatus::INSUF_STAT: 
      cout << " Not enough statistics " << endl;
      break;

    default:
      cout << " Unknown (status = " << status << ") " << endl;
    }
  
  string message = qc->getMessage();
  cout << " Message:" << message << endl;
  
  return status;
}

//
// member functions
//

// ------------ method called to produce the data  ------------
void DQMBackEndInterfaceQTestsExample::analyze(const edm::Event& iEvent, 
					 const edm::EventSetup& iSetup )
{
  // fill in test histogram with random data
  h1->Fill(gRandom->Gaus(mean_, sigma_)); 

  testh2f->Fill(gRandom->Gaus(mean_, sigma_), 
		gRandom->Gaus(mean_, sigma_));

  testprof->Fill(gRandom->Gaus(mean_, sigma_), 
		 TMath::Abs(gRandom->Gaus(mean_, sigma_)));

  testprof2d->Fill(gRandom->Gaus(mean_, sigma_), 
		   gRandom->Gaus(mean_, sigma_), 
		   TMath::Abs(gRandom->Gaus(mean_, sigma_)));

  poMPLandauH1_->Fill( gRandom->Landau( dLandauMP_, dLandauSigma_));

  ++counter;
}

// show channels that failed test
void DQMBackEndInterfaceQTestsExample::showBadChannels(QCriterion *qc)
{
  vector<dqm::me_util::Channel> badChannels = qc->getBadChannels();
  if(!badChannels.empty())
    cout << " Channels that failed test " << qc->getAlgoName() << ":\n";
  
  vector<dqm::me_util::Channel>::iterator it = badChannels.begin();
  while(it != badChannels.end())
    {
      cout << " Channel ("
           << it->getBinX() << ","
           << it->getBinY() << ","
           << it->getBinZ()
	   << ") Contents: " << it->getContents() << " +- " 
	   << it->getRMS() << endl;

      ++it;
    }
    
}


// define this as a plug-in
DEFINE_FWK_MODULE(DQMBackEndInterfaceQTestsExample);
