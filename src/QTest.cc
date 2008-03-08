#include "DQMServices/Core/interface/QTest.h"
#include "DQMServices/Core/src/QStatisticalTests.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "TMath.h"
#include "TH1F.h"
#include <iostream>
#include <sstream>
#include <math.h>

//#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DQMServices/Core/interface/DQMStore.h"

const float QCriterion::ERROR_PROB_THRESHOLD = 0.50;
const float QCriterion::WARNING_PROB_THRESHOLD = 0.90;

// initialize values
void
QCriterion::init(void)
{
  enabled_ = wasModified_ = true;
  errorProb_ = ERROR_PROB_THRESHOLD;
  warningProb_ = WARNING_PROB_THRESHOLD;
  setAlgoName("NO_ALGORITHM");
  status_ = dqm::qstatus::DID_NOT_RUN;
  message_ = "NO_MESSAGE";
}

// set status & message for disabled tests
void
QCriterion::setDisabled(void)
{
  status_ = dqm::qstatus::DISABLED;
  std::ostringstream message;
  message << " Test " << qtname_ << " (" << algoName()
	  << ") has been disabled ";
  message_ = message.str();
}

// set status & message for invalid tests
void
QCriterion::setInvalid(void)
{
  status_ = dqm::qstatus::INVALID;
  std::ostringstream message;
  message << " Test " << qtname_ << " (" << algoName()
	  << ") cannot run due to problems ";
  message_ = message.str();
}

// set status & message for tests w/o enough statistics
void
QCriterion::setNotEnoughStats(void)
{
  status_ = dqm::qstatus::INSUF_STAT;
  std::ostringstream message;
  message << " Test " << qtname_ << " (" << algoName()
	  << ") cannot run (insufficient statistics) ";
  message_ = message.str();
}

// set status & message for succesfull tests
void
QCriterion::setOk(void)
{
  status_ = dqm::qstatus::STATUS_OK;
  setMessage();
}

// set status & message for tests w/ warnings
void
QCriterion::setWarning(void)
{
  status_ = dqm::qstatus::WARNING;
  setMessage();
}

// set status & message for tests w/ errors
void
QCriterion::setError(void)
{
  status_ = dqm::qstatus::ERROR;
  setMessage();
}



//===================================================//
//================ QUALITY TESTS ====================//
//==================================================//

//------------------------------//
//----- ContentsXRange --------//
//------------------------------//
float ContentsXRange::runMyTest(const MonitorElement*me)
{
  if (!me) return -1;

  //obtain ROOTobject from Monitor Element ! 
  TH1F *h = me->getTH1F();
  if (!h) return -1;
 
  TAxis *axis = h->GetXaxis();
  if (!rangeInitialized_)
  {
    if (axis)
      setAllowedXRange(axis->GetXmin(), axis->GetXmax());
    else
      return -1;
  }
  Int_t ncx = axis->GetNbins();
  // use underflow bin
  Int_t first = 0; // 1
  // use overflow bin
  Int_t last  = ncx+1; // ncx
  // all entries
  Double_t sum = 0;
  // entries outside X-range
  Double_t fail = 0;
  Int_t bin;
  for (bin = first; bin <= last; ++bin)
  {
    Double_t contents = h->GetBinContent(bin);
    float x = h->GetBinCenter(bin);
    sum += contents;
    if (x < xmin_ || x > xmax_)fail += contents;
  }

  // return fraction of entries within allowed X-range
  return (sum - fail)/sum;

}

//-------------------------------------------------------//
//----------------- Comp2RefEqualH1 ---------------------//
//-------------------------------------------------------//
// run the test (result: [0, 1] or <0 for failure)
float Comp2RefEqualH1::runMyTest(const MonitorElement*me)
{
  if (me->kind()==MonitorElement::DQM_KIND_TH1F) {
     TH1F *h = me->getTH1F();
     TH1F *ref_ = me->getRefTH1F();
     
  
     std::cout<< "!!!!!!!!!!!!! MEAN = " << ref_->GetMean() << std::endl;
   

  //-- Check consistency in number of channels
  ncx1   = h->GetXaxis()->GetNbins();
  ncx2   = ref_->GetXaxis()->GetNbins();
  if (ncx1 != ncx2){
    std::cerr << " Comp2RefEqualH1 error: different number of channels! ("
	      << ncx1 << ", " << ncx2 << ") " << std::endl;
    return -1;
  }


  //--  QUALITY TEST itself 
 
  // use underflow bin
  Int_t first = 0; // 1
  // use overflow bin
  Int_t last  = ncx1+1; // ncx1

  bool failure = false;
  for (Int_t bin=first;bin<=last;bin++)
  {
    float contents = h->GetBinContent(bin);
    if (contents != ref_->GetBinContent(bin))
    {
      failure = true;
      DQMChannel chan(bin, 0, 0, contents, h->GetBinError(bin));
      badChannels_.push_back(chan);
    }
  }

  if (failure) return 0;
  else         return 1;
  }
}

