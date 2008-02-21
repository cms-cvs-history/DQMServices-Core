#include "DQMServices/Core/interface/QTest.h"
#include "DQMServices/Core/src/QStatisticalTests.h"
#include "TMath.h"
#include "TH1F.h"
#include <iostream>
#include <sstream>
#include <math.h>

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

// make sure algorithm can run (false: should not run)
bool
QCriterion::check(const MonitorElement *me)
{
  // reset result
  prob_ = -1;

  if (!isEnabled())
  {
    setDisabled();
    return false;
  }

  if (!me)
  {
    setInvalid();
    return false;
  }

  if (notEnoughStats(me))
  {
    setNotEnoughStats();
    return false;
  }

  // if here, we can run the test
  return true;
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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// true if test cannot run
bool
Comp2RefChi2::isInvalid(const TH1F *h)
{
  if (hasNullReference())
    return true;
  if (!h)
    return true;

  //check dimensions
  if (h->GetDimension() != 1 || ref_->GetDimension() != 1)
  {
    std::cerr << " Comp2RefChi2 error: Histograms must be 1-D\n " << std::endl;
    return true;
  }

  TAxis *axis1 = h->GetXaxis();
  TAxis *axis2 = ref_->GetXaxis();
  nbins1 = axis1->GetNbins();
  nbins2 = axis2->GetNbins();

  //check number of channels
  if (nbins1 != nbins2)
  {
    std::cerr << " Comp2RefChi2 error: different number of channels! (" << nbins1
	      << ", " << nbins2 << ") " << std::endl;
    return true;
  }

  // if here, everything is good
  return false;
}

float
Comp2RefChi2::runTest(const TH1F *h)
{
  resetResults();

  if (isInvalid(h))
    return -1;

  // copy & past from TH1::Chi2TestX, till we have public method
  // for all of chi2, NDF and probability variables...

  Int_t i, i_start, i_end;
  float chi2 = 0;
  int ndof = 0;
  int constraint = 0;

  TAxis *axis1 = h->GetXaxis();
  //see options
  i_start = 1;
  i_end = nbins1;
  //  if (fXaxis.TestBit(TAxis::kAxisRange)) {
  i_start = axis1->GetFirst();
  i_end   = axis1->GetLast();
  //  }
  ndof = i_end-i_start+1-constraint;

  //Compute the normalisation factor
  Double_t sum1=0, sum2=0;
  for (i=i_start; i<=i_end; i++)
  {
    sum1 += h->GetBinContent(i);
    sum2 += ref_->GetBinContent(i);
  }

  //check that the histograms are not empty
  if (sum1 == 0 || sum2 == 0)
  {
    std::cerr << " Comp2RefChi2 error: one of the histograms is empty" << std::endl;
    return -1;
  }

  Double_t bin1, bin2, err1, err2, temp;
  for (i=i_start; i<=i_end; i++){
    bin1 = h->GetBinContent(i)/sum1;
    bin2 = ref_->GetBinContent(i)/sum2;
    if (bin1 ==0 && bin2==0){
      --ndof; //no data means one less degree of freedom
    } else {

      temp  = bin1-bin2;
      err1=h->GetBinError(i);
      err2=ref_->GetBinError(i);
      if (err1 == 0 && err2 == 0)
      {
	std::cerr << " Comp2RefChi2 error: bins with non-zero content and zero error"
		  << std::endl;
	return -1;
      }
      err1*=err1;
      err2*=err2;
      err1/=sum1*sum1;
      err2/=sum2*sum2;
      chi2 +=temp*temp/(err1+err2);
    }
  }

  chi2_ = chi2;
  Ndof_ = ndof;
  return TMath::Prob(0.5*chi2, Int_t(0.5*ndof));
}

void
Comp2RefChi2::resetResults(void)
{
  Ndof_ = 0; chi2_ = -1; nbins1 = nbins2 = -1;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Note: runTest was supposed to be a method of the (template) class Comp2RefEqual;
// This caused problems with the ROOT histograms, because operator== is not defined
// for TH1F, TH2F, TH3F, etc; So, implementing everything here for now
// christos, 01-FEB-06

// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualString::runTest(const std::string *t)
{
  if (isInvalid(t))
    return -1;

  if (*t == *ref_)
    return 1;
  else
    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualInt::runTest(const int *t)
{
  if (isInvalid(t))
    return -1;

  if (*t == *ref_)
    return 1;
  else
    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualFloat::runTest(const double *t)
{
  if (isInvalid(t))
    return -1;

  if (*t == *ref_)
    return 1;
  else
    return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// true if test cannot run
bool
Comp2RefEqualH1::isInvalid(const TH1F *h)
{
  if (Comp2RefEqualT<TH1F>::isInvalid(h))
    return true;

  // Check consistency of dimensions
  if (h->GetDimension() != 1 || ref_->GetDimension() != 1)
  {
    std::cerr << " Comp2RefEqualH1 error: Histograms must be 1-D\n";
    return true;
  }

  TAxis *axis1 = h->GetXaxis();
  TAxis *axis2 = ref_->GetXaxis();
  ncx1   = axis1->GetNbins();
  ncx2   = axis2->GetNbins();
  // Check consistency in number of channels
  if (ncx1 != ncx2)
  {
    std::cerr << " Comp2RefEqualH1 error: different number of channels! ("
	      << ncx1 << ", " << ncx2 << ") " << std::endl;
    return true;
  }

  // if here, everything is good
  return false;
}

// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualH1::runTest(const TH1F *h)
{
  badChannels_.clear();
  if (isInvalid(h))
    return -1;

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

  if (failure)
    return 0;
  else
    return 1;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// true if test cannot run
bool
Comp2RefEqualH2::isInvalid(const TH2F *h)
{
  if (Comp2RefEqualT<TH2F>::isInvalid(h))
    return true;

  // Check consistency of dimensions
  if (h->GetDimension() != 2 || ref_->GetDimension() != 2)
  {
    std::cerr << " Comp2RefEqualH2 error: Histograms must be 2-D\n";
    return true;
  }

  TAxis *xaxis1 = h->GetXaxis();
  TAxis *xaxis2 = ref_->GetXaxis();
  TAxis *yaxis1 = h->GetYaxis();
  TAxis *yaxis2 = ref_->GetYaxis();
  ncx1   = xaxis1->GetNbins();
  ncx2   = xaxis2->GetNbins();
  ncy1   = yaxis1->GetNbins();
  ncy2   = yaxis2->GetNbins();

  // Check consistency in number of X-channels
  if (ncx1 != ncx2)
  {
    std::cerr << " Comp2RefEqualH2 error: different number of X-channels! ("
	      << ncx1 << ", " << ncx2 << ") " << std::endl;
    return true;
  }

  // Check consistency in number of Y-channels
  if (ncy1 != ncy2)
  {
    std::cerr << " Comp2RefEqualH2 error: different number of Y-channels! ("
	      << ncy1 << ", " << ncy2 << ") " << std::endl;
    return true;
  }

  // if here, everything is good
  return false;
}

// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualH2::runTest(const TH2F *h)
{
  badChannels_.clear();
  if (isInvalid(h))
    return -1;

  // use underflow bin
  Int_t firstX = 0; // 1
  // use overflow bin
  Int_t lastX  = ncx1+1; // ncx1
  // use underflow bin
  Int_t firstY = 0; // 1
  // use overflow bin
  Int_t lastY  = ncy1+1; // ncy1

  bool failure = false;
  Int_t binx, biny;
  for (biny = firstY; biny <= lastY; ++biny)
  {
    for (binx = firstX; binx <= lastX; ++binx)
    {
      float contents = h->GetBinContent(binx, biny);
      if (ref_->GetBinContent(binx,biny) != contents)
      {
	failure = true;
	DQMChannel chan(binx, biny, 0, contents, h->GetBinError(binx, biny));
	badChannels_.push_back(chan);
      }
    }
  }

  if (failure)
    return 0;
  else
    return 1;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// true if test cannot run
bool
Comp2RefEqualH3::isInvalid(const TH3F *h)
{
  if (Comp2RefEqualT<TH3F>::isInvalid(h))
    return true;

  // Check consistency of dimensions
  if (h->GetDimension() != 3 || ref_->GetDimension() != 3)
  {
    std::cerr << " Comp2RefEqualH3 error: Histograms must be 3-D\n";
    return true;
  }

  TAxis *xaxis1 = h->GetXaxis();
  TAxis *xaxis2 = ref_->GetXaxis();
  TAxis *yaxis1 = h->GetYaxis();
  TAxis *yaxis2 = ref_->GetYaxis();
  TAxis *zaxis1 = h->GetZaxis();
  TAxis *zaxis2 = ref_->GetZaxis();
  ncx1   = xaxis1->GetNbins();
  ncx2   = xaxis2->GetNbins();
  ncy1   = yaxis1->GetNbins();
  ncy2   = yaxis2->GetNbins();
  ncz1   = zaxis1->GetNbins();
  ncz2   = zaxis2->GetNbins();

  // Check consistency of dimensions
  if (h->GetDimension() != 3 || ref_->GetDimension() != 3)
  {
    std::cerr << " Comp2RefEqualH3 error: Histograms must be 3-D\n";
    return false;
  }

  // Check consistency in number of X-channels
  if (ncx1 != ncx2)
  {
    std::cerr << " Comp2RefEqualH3 error: different number of X-channels! ("
	      << ncx1 << ", " << ncx2 << ") " << std::endl;
    return false;
  }

  // Check consistency in number of Y-channels
  if (ncy1 != ncy2)
  {
    std::cerr << " Comp2RefEqualH3 error: different number of Y-channels! ("
	      << ncy1 << ", " << ncy2 << ") " << std::endl;
    return false;
  }

  // Check consistency in number of Z-channels
  if (ncz1 != ncz2)
  {
    std::cerr << " Comp2RefEqualH3 error: different number of Z-channels! ("
	      << ncz1 << ", " << ncz2 << ") " << std::endl;
    return false;
  }

  // if here, everything is good
  return false;
}

// run the test (result: [0, 1] or <0 for failure)
float
Comp2RefEqualH3::runTest(const TH3F *h)
{
  badChannels_.clear();
  if (isInvalid(h))
    return -1;

  // use underflow bin
  Int_t firstX = 0; // 1
  // use overflow bin
  Int_t lastX  = ncx1+1; // ncx1
  // use underflow bin
  Int_t firstY = 0; // 1
  // use overflow bin
  Int_t lastY  = ncy1+1; // ncy1
  // use underflow bin
  Int_t firstZ = 0; // 1
  // use overflow bin
  Int_t lastZ  = ncz1+1; // ncz1

  bool failure = false;
  Int_t binx, biny, binz;
  for (binz = firstZ; binz <= lastZ; ++binz)
  {
    for (biny = firstY; biny <= lastY; ++biny)
    {
      for (binx = firstX; binx <= lastX; ++binx)
      {
	float contents = h->GetBinContent(binx, biny, binz);
	if (ref_->GetBinContent(binx,biny, binz) != contents)
	{
	  failure = true;
	  DQMChannel chan(binx, biny, binz, contents,
			  h->GetBinError(binx, biny, binz));
	  badChannels_.push_back(chan);
	}
      }
    }
  }

  if (failure)
    return 0;
  else
    return 1;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// true if test cannot run
bool
Comp2RefKolmogorov::isInvalid(const TH1F *h)
{
  if (hasNullReference())
    return true;
  if (!h)
    return true;

  // Check consistency of dimensions
  if (h->GetDimension() != 1 || ref_->GetDimension() != 1)
  {
    std::cerr << " KolmogorovTest error: Histograms must be 1-D\n";
    return true;
  }

  TAxis *axis1 = h->GetXaxis();
  TAxis *axis2 = ref_->GetXaxis();
  ncx1   = axis1->GetNbins();
  ncx2   = axis2->GetNbins();

  // Check consistency in number of channels
  if (ncx1 != ncx2)
  {
    std::cerr << " KolmogorovTest error: different number of channels! (" << ncx1
	      << ", " << ncx2 << ") " << std::endl;
    return true;
  }

  // Check consistency in channel edges
  Double_t diff1 = TMath::Abs(axis1->GetXmin() - axis2->GetXmin());
  Double_t diff2 = TMath::Abs(axis1->GetXmax() - axis2->GetXmax());
  if (diff1 > difprec || diff2 > difprec)
  {
    std::cerr << " KolmogorovTest error: histograms with different binning";
    return true;
  }

  // if here, everything is good
  return false;

}

const Double_t Comp2RefKolmogorov::difprec = 1e-5;

float
Comp2RefKolmogorov::runTest(const TH1F *h)
{
  if (isInvalid(h))
    return -1;

  // copy & past from TH1::KolmogorovTest, till we have public method
  // for all of chi2, NDF and probability variables...
  Bool_t afunc1 = kFALSE;
  Bool_t afunc2 = kFALSE;
  Double_t sum1 = 0, sum2 = 0;
  Double_t ew1, ew2, w1 = 0, w2 = 0;
  Int_t bin;
  for (bin=1;bin<=ncx1;bin++)
  {
    sum1 += h->GetBinContent(bin);
    sum2 += ref_->GetBinContent(bin);
    ew1   = h->GetBinError(bin);
    ew2   = ref_->GetBinError(bin);
    w1   += ew1*ew1;
    w2   += ew2*ew2;
  }
  if (sum1 == 0)
  {
    std::cerr << "KolmogorovTest error: Histogram " << h->GetName()
	      << " integral is zero\n";
    return -1;
  }
  if (sum2 == 0)
  {
    std::cerr << "KolmogorovTest error: Histogram " << ref_->GetName()
	      << " integral is zero\n";
    return -1;
  }

  Double_t tsum1 = sum1;
  Double_t tsum2 = sum2;
  tsum1 += h->GetBinContent(0);
  tsum2 += ref_->GetBinContent(0);
  tsum1 += h->GetBinContent(ncx1+1);
  tsum2 += ref_->GetBinContent(ncx1+1);

  // Check if histograms are weighted.
  // If number of entries = number of channels, probably histograms were
  // not filled via Fill(), but via SetBinContent()
  Double_t ne1 = h->GetEntries();
  Double_t ne2 = ref_->GetEntries();
  // look at first histogram
  Double_t difsum1 = (ne1-tsum1)/tsum1;
  Double_t esum1 = sum1;
  if (difsum1 > difprec && Int_t(ne1) != ncx1)
  {
    if (h->GetSumw2N() == 0)
      std::cout << " KolmogorovTest warning: Weighted events and no Sumw2 for "
		<< h->GetName() << std::endl;
    else
      esum1 = sum1*sum1/w1;  //number of equivalent entries
  }
  // look at second histogram
  Double_t difsum2 = (ne2-tsum2)/tsum2;
  Double_t esum2   = sum2;
  if (difsum2 > difprec && Int_t(ne2) != ncx1)
  {
    if (ref_->GetSumw2N() == 0)
      std::cout << " KolmogorovTest warning: Weighted events and no Sumw2 for "
		<< ref_->GetName() << std::endl;
    else
      esum2 = sum2*sum2/w2;  //number of equivalent entries
  }

  Double_t s1 = 1/tsum1;
  Double_t s2 = 1/tsum2;

  // Find largest difference for Kolmogorov Test
  Double_t dfmax =0, rsum1 = 0, rsum2 = 0;

  // use underflow bin
  Int_t first = 0; // 1
  // use overflow bin
  Int_t last  = ncx1+1; // ncx1
  for (bin=first;bin<=last;bin++)
  {
    rsum1 += s1*h->GetBinContent(bin);
    rsum2 += s2*ref_->GetBinContent(bin);
    dfmax = TMath::Max(dfmax,TMath::Abs(rsum1-rsum2));
  }

  // Get Kolmogorov probability
  Double_t z = 0;
  if (afunc1)      z = dfmax*TMath::Sqrt(esum2);
  else if (afunc2) z = dfmax*TMath::Sqrt(esum1);
  else             z = dfmax*TMath::Sqrt(esum1*esum2/(esum1+esum2));

  // This numerical error condition should never occur:
  if (TMath::Abs(rsum1-1) > 0.002)
    std::cout << " KolmogorovTest warning: Numerical problems with histogram "
	      << h->GetName() << std::endl;
  if (TMath::Abs(rsum2-1) > 0.002)
    std::cout << " KolmogorovTest warning: Numerical problems with histogram "
	      << ref_->GetName() << std::endl;

  return TMath::KolmogorovProb(z);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of entries [*not* bins!] within X-range)
// [0, 1] or <0 for failure
float
ContentsXRange::runTest(const TH1F *h)
{
  if (!h)
    return -1;

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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of bins [*not* entries!] that passed test)
// [0, 1] or <0 for failure
float
ContentsYRange::runTest(const TH1F *h)
{
  badChannels_.clear();

  if (!h)
    return -1;

  TAxis * axis = h->GetXaxis();
  if (!rangeInitialized_ || !axis)
    return 1; // all bins are accepted if no initialization

  Int_t ncx = axis->GetNbins();
  // do NOT use underflow bin
  Int_t first = 1;
  // do NOT use overflow bin
  Int_t last  = ncx;
  // bins outside Y-range
  Int_t fail = 0;
  Int_t bin;
  for (bin = first; bin <= last; ++bin)
  {
    Double_t contents = h->GetBinContent(bin);
    bool failure = false;
    if (deadChanAlgo_)
      // dead channel: equal to or less than ymin_
      failure = contents <= ymin_;
    else
      // allowed y-range: [ymin_, ymax_]
      failure = (contents < ymin_ || contents > ymax_);

    if (failure)
    {
      DQMChannel chan(bin, 0, 0, contents, h->GetBinError(bin));
      badChannels_.push_back(chan);
      ++fail;
    }
  }

  // return fraction of bins that passed test
  return 1.*(ncx - fail)/ncx;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of channels not appearing noisy or "hot")
// [0, 1] or <0 for failure
float
NoisyChannel::runTest(const TH1F *h)
{
  badChannels_.clear();
  if (!h)
    return -1;

  TAxis *axis = h->GetXaxis();
  if (!rangeInitialized_ || !axis)
    return 1; // all channels are accepted if tolerance has not been set

  Int_t ncx = axis->GetNbins();
  // do NOT use underflow bin
  Int_t first = 1;
  // do NOT use overflow bin
  Int_t last  = ncx;
  // bins outside Y-range
  Int_t fail = 0;
  Int_t bin;
  for (bin = first; bin <= last; ++bin)
  {
    Double_t contents = h->GetBinContent(bin);
    Double_t average = getAverage(bin, h);
    bool failure = false;
    if (average != 0)
      failure = (((contents-average)/TMath::Abs(average)) > tolerance_);

    if (failure)
    {
      ++fail;
      DQMChannel chan(bin, 0, 0, contents, h->GetBinError(bin));
      badChannels_.push_back(chan);
    }
  }

  // return fraction of bins that passed test
  return 1.*(ncx - fail)/ncx;
}

// get average for bin under consideration
// (see description of method setNumNeighbors)
Double_t
NoisyChannel::getAverage(int bin, const TH1F *h) const
{
  // do NOT use underflow bin
  Int_t first = 1;
  // do NOT use overflow bin
  Int_t ncx  = h->GetXaxis()->GetNbins();

  Double_t sum = 0; int bin_low, bin_hi;
  for (unsigned i = 1; i <= numNeighbors_; ++i)
  {
    // use symmetric-to-bin bins to calculate average
    bin_low = bin-i;
    bin_hi = bin+i;
    // check if need to consider bins on other side of spectrum
    // (ie. if bins below 1 or above ncx)

    while (bin_low < first) // shift bin by +ncx
      bin_low = ncx + bin_low;
    while (bin_hi > ncx) // shift bin by -ncx
      bin_hi = bin_hi - ncx;

    sum += h->GetBinContent(bin_low) + h->GetBinContent(bin_hi);
  }

  // average is sum over the # of bins used
  return sum/(numNeighbors_ * 2);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of channels that passed test);
// [0, 1] or <0 for failure
// implementation: Giuseppe Della Ricca
float
ContentsTH2FWithinRange::runTest(const TH2F *h)
{
  badChannels_.clear();

  if (!h)
    return -1;
  if (isInvalid())
    return -1;

  TAxis*xaxis = h->GetXaxis();
  if (!xaxis)
    return -1;

  TAxis*yaxis = h->GetYaxis();
  if (!yaxis)
    return -1;

  int ncx = xaxis->GetNbins();
  int ncy = yaxis->GetNbins();
  int fail = 0;
  int nsum = 0;
  float sum = 0.0;
  float average = 0.0;

  if (checkMeanTolerance_)
  { // calculate average value of all bin contents

    for (int cx = 1; cx <= ncx; ++cx)
    {
      for (int cy = 1; cy <= ncy; ++cy)
      {
	sum += h->GetBinContent(h->GetBin(cx, cy));
	++nsum;
      }
    }

    if (nsum > 0)
      average = sum/nsum;

  } // calculate average value of all bin contents

  for (int cx = 1; cx <= ncx; ++cx)
  {
    for (int cy = 1; cy <= ncy; ++cy)
    {
      bool failMean = false;
      bool failRMS = false;
      bool failMeanTolerance = false;

      if (checkMean_)
      {
        float mean = h->GetBinContent(h->GetBin(cx, cy));
        failMean = (mean < minMean_ || mean > maxMean_);
      }

      if (checkRMS_)
      {
        float rms = h->GetBinError(h->GetBin(cx, cy));
        failRMS = (rms < minRMS_ || rms > maxRMS_);
      }

      if (checkMeanTolerance_)
      {
        float mean = h->GetBinContent(h->GetBin(cx, cy));
        failMeanTolerance = (TMath::Abs(mean - average) >
			     toleranceMean_ * TMath::Abs(average));
      }

      if (failMean || failRMS || failMeanTolerance)
      {
        DQMChannel chan(cx, cy, 0, h->GetBinContent(h->GetBin(cx, cy)),
			h->GetBinError(h->GetBin(cx, cy)));
        badChannels_.push_back(chan);
        ++fail;
      }
    }
  }

  return 1.*(ncx*ncy - fail)/(ncx*ncy);
}

// check that allowed range is logical
void
ContentsTH2FWithinRange::checkRange(const float xmin, const float xmax)
{
  if (xmin < xmax)
    validMethod_ = true;
  else
  {
    std::cerr << " *** Error! Illogical range: (" << xmin << ", " << xmax
	      << ") in algorithm " << getAlgoName() << std::endl;
    validMethod_ = false;
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of channels that passed test);
// [0, 1] or <0 for failure
// implementation: Giuseppe Della Ricca
float
ContentsProfWithinRange::runTest(const TProfile *h)
{
  badChannels_.clear();

  if (!h)
    return -1;
  if (isInvalid())
    return -1;

  TAxis*xaxis = h->GetXaxis();
  if (!xaxis)
    return -1;

  int ncx = xaxis->GetNbins();
  int fail = 0;
  int nsum = 0;
  float sum = 0.0;
  float average = 0.0;

  // calculate average value of all bin contents
  if (checkMeanTolerance_)
  {

    for (int cx = 1; cx <= ncx; ++cx)
    {
      if (h->GetBinEntries(h->GetBin(cx)) >= minEntries_/(ncx))
      {
	sum += h->GetBinContent(h->GetBin(cx));
	++nsum;
      }
    }

    if (nsum > 0)
      average = sum/nsum;
  }

  for (int cx = 1; cx <= ncx; ++cx)
  {
    bool failMean = false;
    bool failRMS = false;
    bool failMeanTolerance = false;

    if (h->GetBinEntries(h->GetBin(cx)) >= minEntries_/(ncx))
    {
      if (checkMean_)
      {
	float mean = h->GetBinContent(h->GetBin(cx));
	failMean = (mean < minMean_ || mean > maxMean_);
      }

      if (checkRMS_)
      {
	float rms = h->GetBinError(h->GetBin(cx));
	failRMS = (rms < minRMS_ || rms > maxRMS_);
      }

      if (checkMeanTolerance_)
      {
	float mean = h->GetBinContent(h->GetBin(cx));
	failMeanTolerance = (TMath::Abs(mean - average) >
			     toleranceMean_ * TMath::Abs(average));
      }
    }

    if (failMean || failRMS || failMeanTolerance)
    {
      DQMChannel chan(cx, int(h->GetBinEntries(h->GetBin(cx))),
		      0, h->GetBinContent(h->GetBin(cx)),
		      h->GetBinError(h->GetBin(cx)));
      badChannels_.push_back(chan);
      ++fail;
    }
  }

  return 1.*(ncx - fail)/(ncx);
}

// check that allowed range is logical
void
ContentsProfWithinRange::checkRange(const float xmin, const float xmax)
{
  if (xmin < xmax)
    validMethod_ = true;
  else
  {
    std::cerr << " *** Error! Illogical range: (" << xmin << ", " << xmax
	      << ") in algorithm " << getAlgoName() << std::endl;
    validMethod_ = false;
  }
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// run the test (result: fraction of channels that passed test);
// [0, 1] or <0 for failure
// implementation: Giuseppe Della Ricca
float
ContentsProf2DWithinRange::runTest(const TProfile2D *h)
{
  badChannels_.clear();

  if (!h)
    return -1;
  if (isInvalid())
    return -1;

  TAxis *xaxis = h->GetXaxis();
  if (!xaxis)
    return -1;

  TAxis *yaxis = h->GetYaxis();
  if (!yaxis)
    return -1;

  int ncx = xaxis->GetNbins();
  int ncy = yaxis->GetNbins();
  int fail = 0;
  int nsum = 0;
  float sum = 0.0;
  float average = 0.0;

  // calculate average value of all bin contents
  if (checkMeanTolerance_)
  {
    for (int cx = 1; cx <= ncx; ++cx)
    {
      for (int cy = 1; cy <= ncy; ++cy)
      {
	if (h->GetBinEntries(h->GetBin(cx, cy)) >= minEntries_/(ncx*ncy))
	{
	  sum += h->GetBinContent(h->GetBin(cx, cy));
	  ++nsum;
	}
      }
    }

    if (nsum > 0)
      average = sum/nsum;
  }

  for (int cx = 1; cx <= ncx; ++cx)
  {
    for (int cy = 1; cy <= ncy; ++cy)
    {
      bool failMean = false;
      bool failRMS = false;
      bool failMeanTolerance = false;

      if (h->GetBinEntries(h->GetBin(cx, cy)) >= minEntries_/(ncx*ncy))
      {
	if (checkMean_)
	{
	  float mean = h->GetBinContent(h->GetBin(cx, cy));
	  failMean = (mean < minMean_ || mean > maxMean_);
	}

	if (checkRMS_)
	{
	  float rms = h->GetBinError(h->GetBin(cx, cy));
	  failRMS = (rms < minRMS_ || rms > maxRMS_);
	}

	if (checkMeanTolerance_)
	{
	  float mean = h->GetBinContent(h->GetBin(cx, cy));
	  failMeanTolerance = (TMath::Abs(mean - average)
			       > toleranceMean_ * TMath::Abs(average));
	}
      }

      if (failMean || failRMS || failMeanTolerance)
      {
	DQMChannel chan(cx, cy, int(h->GetBinEntries(h->GetBin(cx, cy))),
			h->GetBinContent(h->GetBin(cx, cy)),
			h->GetBinError(h->GetBin(cx, cy)));
	badChannels_.push_back(chan);
	++fail;
      }
    }
  }

  return 1.*(ncx*ncy - fail)/(ncx*ncy);
}

// check that allowed range is logical
void
ContentsProf2DWithinRange::checkRange(const float xmin, const float xmax)
{
  if (xmin < xmax)
    validMethod_ = true;
  else
  {
    std::cerr << " *** Error! Illogical range: (" << xmin << ", " << xmax
	      << ") in algorithm " << getAlgoName() << std::endl;
    validMethod_ = false;
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
/* run the test;
   (a) if useRange is called: 1 if mean within allowed range, 0 otherwise

   (b) is useRMS or useSigma is called: result is the probability
   Prob(chi^2, ndof=1) that the mean of histogram will be deviated by more than
   +/- delta from <expected_mean>, where delta = mean - <expected_mean>, and
   chi^2 = (delta/sigma)^2. sigma is the RMS of the histogram ("useRMS") or
   <expected_sigma> ("useSigma")
   e.g. for delta = 1, Prob = 31.7%
   for delta = 2, Prob = 4.55%

   (returns result in [0, 1] or <0 for failure) */
float
MeanWithinExpected::runTest(const TH1F *h)
{
  if (!h)
    return -1;
  if (isInvalid())
    return -1;

  if (useRange_)
    return doRangeTest(h);

  if (useSigma_)
    return doGaussTest(h, sigma_);

  if (useRMS_)
    return doGaussTest(h, h->GetRMS());

  // we should never reach this point;
  return -99;
}

// test assuming mean value is quantity with gaussian errors
float
MeanWithinExpected::doGaussTest(const TH1F *h, float sigma)
{
  float chi = (h->GetMean() - expMean_)/sigma;
  return TMath::Prob(chi*chi, 1);
}

// test for useRange_ = true case
float
MeanWithinExpected::doRangeTest(const TH1F *h)
{
  float mean = h->GetMean();
  if (mean <= xmax_ && mean >= xmin_)
    return 1;
  else
    return 0;
}

// check that exp_sigma_ is non-zero
void
MeanWithinExpected::checkSigma(void)
{
  if (sigma_ != 0)
    validMethod_ = true;
  else
  {
    std::cerr << " *** Error! Expected sigma = " << sigma_ << " in algorithm "
	      << getAlgoName() << std::endl;
    validMethod_ = false;
  }
}

// check that allowed range is logical
void
MeanWithinExpected::checkRange(void)
{
  if (xmin_ < xmax_)
    validMethod_ = true;
  else
  {
    std::cerr << " *** Error! Illogical range: (" << xmin_ << ", " << xmax_
	      << ") in algorithm " << getAlgoName() << std::endl;
    validMethod_ = false;
  }
}

// true if test cannot run
bool
MeanWithinExpected::isInvalid(void)
{
  // if useRange_ = true, test does not need a "expected mean value"
  if (useRange_)
    return !validMethod_; // set by checkRange()

  // otherwise (useSigma_ or useRMS_ case), we also need to check
  // if "expected mean value" has been set
  return !validMethod_  // set by useRMS() or checkSigma()
    || !validExpMean_; // set by setExpectedMean()

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Author : Samvel Khalatian (samvel at fnal dot gov)
// Created: 04/26/07

/**
 * @brief
 *   Rational approximation for erfc(rdX) (Abramowitz & Stegun, Sec. 7.1.26)
 *   Fifth order approximation. | error| <= 1.5e-7 for all rdX
 *
 * @param rdX
 *   Significance value
 *
 * @return
 *   Probability
 */
double
edm::qtests::fits::erfc(const double &rdX)
{
  const double dP  = 0.3275911;
  const double dA1 = 0.254829592;
  const double dA2 = -0.284496736;
  const double dA3 = 1.421413741;
  const double dA4 = -1.453152027;
  const double dA5 = 1.061405429;
  const double dT  = 1.0 / (1.0 + dP * rdX);
  return (dA1 + (dA2 + (dA3 + (dA4 + dA5 * dT) * dT) * dT) * dT) * exp(-rdX * rdX);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// --[ Fit: Base - MostProbableBase ]--------------------------------------
MostProbableBase::MostProbableBase(const std::string &name)
  : SimpleTest<TH1F>(name),
    dMostProbable_(0),
    dSigma_(0),
    dXMin_(0),
    dXMax_(0)
{}

/**
 * @brief
 *   Run QTest
 *
 * @param poPLOT
 *   See Interface
 *
 * @return
 *   See Interface
 */
float
MostProbableBase::runTest(const TH1F *const poPLOT)
{
  float dResult = -1;

  if (poPLOT && !isInvalid())
  {
    // It is childrens responsibility to implement and actually fit Histogram.
    // Constness should be removed since TH1::Fit(...) method is declared as
    // non const.
    if (TH1F *const poPlot = const_cast<TH1F *const>(poPLOT))
      dResult = fit(poPlot);
  }

  return dResult;
}

/**
 * @brief
 *   Check if given QTest is Invalid
 *
 * @return
 *   See Interface
 */
bool
MostProbableBase::isInvalid(void)
{
  return !(dMostProbable_ > dXMin_
	   && dMostProbable_ < dXMax_
	   && dSigma_ > 0);
}

/**
 * @brief
 *   Function will compare two MostProbable values and return value representing
 *   percent of match. For that two tests are used:
 *     1. Distance between MostProbables should be less than 2 Sigmas of
 *        estimated distribution, e.g.:
 *
 *          | Mn - Me |
 *          ----------- < Alpha
 *            Sigma_e
 *
 *        where:
 *          Mn       Most Probable new value gotten from Fit or any other
 *                   algorithm
 *          Me       Estimated value of Most Probable
 *          Sigma_e  Sigma of Estimated distribution (Landau, Gauss, any other)
 *          Alpha    Number of sigmas (at the moment value of 2 is used... but
 *                   check the code below to make sure number is not outdated)

 *     2. Significance that is calculated with formula:
 *
 *                | Mn - Me |
 *          -------------------------
 *          Sigma_e ^ 2 + Sigma_n ^ 2
 *
 *        where:
 *          Mn       Most Probable new value gotten from Fit or any other
 *                   algorithm
 *          Me       Estimated value of Most Probable
 *          Sigma_e  Sigma of Estimated distribution (Landau, Gauss, any other)
 *          Sigma_n  Sigma new value gotten from Fit or any other algorithm
 *
 *   Once both values are calculated Distance between MostProbables is checked
 *   for Alpha. -1 is returned if given value is greater than Alpha, otherwise
 *   Significance is transformed into probability [0,1] and the number is
 *   returned.
 *
 * @param rdMP_FIT
 * @param rdSIGMA_FIT
 *   See Interface
 *
 * @return
 *   See Interface
 */
double
MostProbableBase::compareMostProbables(const double &rdMP_FIT, const double &rdSIGMA_FIT) const
{
  double dDeltaMP = rdMP_FIT - dMostProbable_;
  if (dDeltaMP < 0)
    dDeltaMP = -dDeltaMP;

  return (dDeltaMP / dSigma_ < 2 /* Check Deviation */
	  ? edm::qtests::fits::erfc((dDeltaMP / sqrt(rdSIGMA_FIT * rdSIGMA_FIT + dSigma_ * dSigma_))) /* Calculate Significance */ / sqrt(2.0)
	  : 0);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// --[ Fit: Landau ]-----------------------------------------------------------
MostProbableLandau::MostProbableLandau(const std::string &name)
  : MostProbableBase(name),
    dNormalization_(0)
{
  setMinimumEntries(50);
  setAlgoName(getAlgoName());
}

/**
 * @brief
 *   Perform Landau Fit
 *
 * @param poPlot
 *   See Interface
 *
 * @return
 *   See Interface
 */
float
MostProbableLandau::fit(TH1F *const poPlot)
{
  double dResult = -1;

  // Create Fit Function
  TF1 *poFFit = new TF1("Landau", "landau", getXMin(), getXMax());

  // Fit Parameters
  //   [0]  Normalisation coefficient.
  //   [1]  Most probable value.
  //   [2]  Lambda in most books (the width of the distribution)
  poFFit->SetParameters(dNormalization_, getMostProbable(), getSigma());

  // Fit
  if (!poPlot->Fit(poFFit, "RB0"))
  {
    // Obtain Fit Parameters: We are interested in Most Probable and Sigma
    // so far.
    const double dMP_FIT    = poFFit->GetParameter(1);
    const double dSIGMA_FIT = poFFit->GetParameter(2);

    // Compare gotten values with expected ones.
    dResult = compareMostProbables(dMP_FIT, dSIGMA_FIT);
  }

  return dResult;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
float
AllContentWithinFixedRange::runTest(const TH1F *histogram)
{
  //double x, y, z; 
  set_x_min( 6.0 );
  set_x_max( 9.0 );
  set_epsilon_max( 0.1 );
  set_S_fail( 5.0 );
  set_S_pass( 5.0 );

  /*--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH1F* histogram,      : histogram to be compared with Rule                |
    |double x_min,         : x range (low). Note low edge <= bin < high edge   |
    |double x_max,         : x range (high). Note low edge <= bin < high edge  |
    |double epsilon_max,   : maximum allowed failure rate fraction             |
    |double S_fail,        : required Statistical Significance to fail rule    |
    |double S_pass,        : required Significance to pass rule                |
    |double* epsilon_obs   : uninitialised observed failure rate fraction      |
    |double* S_fail_obs    : uninitialised Significance of failure             |
    |double* S_pass_obs    : uninitialised Significance of Success             |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double* epsilon_obs   : the observed failure rate frac. from the histogram|
    |double* S_fail_obs    : the observed Significance of failure              |
    |double* S_pass_obs    : the observed Significance of Success              |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 08.July.2005                                              |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    |   11.July.2005 - moved the part which calculates the statistical         |
    |                  significance of the result into a separate function     |
    +--------------------------------------------------------------------------*/
  epsilon_obs = 0.0;
  S_fail_obs = 0.0;
  S_pass_obs = 0.0;

  //-----------Perform Quality Checks on Input-------------
  if (!histogram)  {result = 5; return 0.0;}//exit if histo does not exist
  TAxis *xAxis = histogram -> GetXaxis();   //retrieve x-axis information
  if (x_min < xAxis -> GetXmin() || xAxis -> GetXmax() < x_max)
  {result = 5; return 0.0;}//exit if x range not in hist range
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}//exit if epsilon_max not in (0,1)
  if (S_fail < 0)  {result = 5; return 0.0;}//exit if Significance < 0
  if (S_pass < 0)  {result = 5; return 0.0;}//exit if Significance < 0
  S_fail_obs = 0.0; S_pass_obs = 0.0;       //initialise Sig return values
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1){result = 4; return 0.0;}//exit if histo has 0 entries

  //-----------Find number of successes and failures-------------
  int low_bin, high_bin;                   //convert x range to bin range
  if (x_min != x_max)                      //Note: x in [low_bin, high_bin)
  {                                      //Or:   x in [0,high_bin) &&
    //           [low_bin, max_bin]
    low_bin  = (int)(histogram -> GetNbinsX() /
		     (xAxis -> GetXmax() - xAxis -> GetXmin()) *
		     (x_min - xAxis -> GetXmin())) + 1;
    high_bin = (int)(histogram -> GetNbinsX() /
		     (xAxis -> GetXmax() - xAxis -> GetXmin()) *
		     (x_max - xAxis -> GetXmin())) + 1;
  }
  else                                     //convert x point to particular bin
  {
    low_bin = high_bin = (int)(histogram -> GetNbinsX() /
			       (xAxis -> GetXmax() - xAxis -> GetXmin()) *
			       (x_min - xAxis -> GetXmin())) + 1;
  }
  int Nsuccesses = 0;
  if (low_bin <= high_bin)                  //count number of entries
    for (int i = low_bin; i <= high_bin; i++) //in bin range
      Nsuccesses += (int) histogram -> GetBinContent(i);
  else                                     //include wrap-around case
  {
    for (int i = 0; i <= high_bin; i++)
      Nsuccesses += (int) histogram -> GetBinContent(i);
    for (int i = low_bin; i <= histogram -> GetNbinsX(); i++)
      Nsuccesses += (int) histogram -> GetBinContent(i);
  }
  int Nfailures       = Nentries - Nsuccesses;
  double Nepsilon_max = (double)Nentries * epsilon_max;
  epsilon_obs         = (double)Nfailures / (double)Nentries;

  //-----------Calculate Statistical Significance-------------
  BinLogLikelihoodRatio(Nentries,Nfailures,epsilon_max,&S_fail_obs,&S_pass_obs);
  if (Nfailures > Nepsilon_max)
  {
    if (S_fail_obs > S_fail)
    {result = 1; return 0.0;}           //exit if statistically fails rule
    else
    {result = 3; return 0.0;}           //exit if non-stat significant result
  }
  else
  {
    if (S_pass_obs > S_pass)
    {result = 0; return 1.0;}           //exit if statistically passes rule
    else
    {result = 2; return 0.0;}           //exit if non-stat significant result
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
float
AllContentWithinFloatingRange::runTest(const TH1F *histogram)
{
  //double x, y, z; 
  set_Nrange( 1 );
  set_epsilon_max( 0.1 );
  set_S_fail( 5.0 );
  set_S_pass( 5.0 );

  /*--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH1F* histogram,      : histogram to be compared with Rule                |
    |int     Nrange,       : number of contiguous bins holding entries         |
    |double  epsilon_max,  : maximum allowed failure rate fraction             |
    |double  S_fail,       : required Statistical Significance to fail rule    |
    |double  S_pass,       : required Significance to pass rule                |
    |double* epsilon_obs   : uninitialised observed failure rate fraction      |
    |double* S_fail_obs    : uninitialised Significance of failure             |
    |double* S_pass_obs    : uninitialised Significance of Success             |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double* epsilon_obs   : the observed failure rate frac. from the histogram|
    |double* S_fail_obs    : the observed Significance of failure              |
    |double* S_pass_obs    : the observed Significance of Success              |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 07.Jan.2006                                               |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    +--------------------------------------------------------------------------*/
  epsilon_obs = 0.0;
  S_fail_obs = 0.0;
  S_pass_obs = 0.0;

  //-----------Perform Quality Checks on Input-------------
  if (!histogram)    {result = 5; return 0.0;}//exit if histo does not exist
  int Nbins = histogram -> GetNbinsX();
  if (Nrange > Nbins){result = 5; return 0.0;}//exit if Nrange > # bins in histo
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}//exit if epsilon_max not in (0,1)
  if (S_fail < 0)    {result = 5; return 0.0;}//exit if Significance < 0
  if (S_pass < 0)    {result = 5; return 0.0;}//exit if Significance < 0
  S_fail_obs = 0.0; S_pass_obs = 0.0;         //initialise Sig return values
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1)  {result = 4; return 0.0;}//exit if histo has 0 entries

  //-----------Find number of successes and failures-------------
  int Nsuccesses = 0, EntriesInCurrentRange = 0;
  for (int i = 1; i <= Nrange; i++)  //initialise Nsuccesses
  {                                 //histos start with bin index 1 (not 0)
    Nsuccesses += (int) histogram -> GetBinContent(i);
  }
  EntriesInCurrentRange = Nsuccesses;
  for (int i = Nrange + 1; i <= Nbins; i++) //optimise floating bin range
  { //slide range by adding new high side bin & subtracting old low side bin
    EntriesInCurrentRange +=
      (int) (histogram -> GetBinContent(i) -
	     histogram -> GetBinContent(i - Nrange));
    if (EntriesInCurrentRange > Nsuccesses)
      Nsuccesses = EntriesInCurrentRange;
  }
  for (int i = 1; i < Nrange; i++) //include possiblity of wrap-around
  { //slide range by adding new low side bin & subtracting old high side bin
    EntriesInCurrentRange +=
      (int) (histogram -> GetBinContent(i) -
	     histogram -> GetBinContent(Nbins - (Nrange - i)));
    if (EntriesInCurrentRange > Nsuccesses)
      Nsuccesses = EntriesInCurrentRange;
  }
  int Nfailures       = Nentries - Nsuccesses;
  double Nepsilon_max = (double)Nentries * epsilon_max;
  epsilon_obs        = (double)Nfailures / (double)Nentries;

  //-----------Calculate Statistical Significance-------------
  BinLogLikelihoodRatio(Nentries,Nfailures,epsilon_max,&S_fail_obs,&S_pass_obs);
  if (Nfailures > Nepsilon_max)
  {
    if (S_fail_obs > S_fail)
    {result = 1; return 0.0;}        //exit if statistically fails rule
    else
    {result = 3; return 0.0;}        //exit if non-stat significant result
  }
  else
  {
    if (S_pass_obs > S_pass)
    {result = 0; return 1.0;}        //exit if statistically passes rule
    else
    {result = 2; return 0.0;}        //exit if non-stat significant result
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
float
CSC01::runTest(const TH1F *histogram)
{
  set_epsilon_max( 0.1 );
  set_S_fail( 5.0 );
  set_S_pass( 5.0 );

  /*--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH1F* histogram,      : histogram to be compared with Rule                |
    |double  epsilon_max,  : maximum allowed failure rate fraction             |
    |double  S_fail,       : required Statistical Significance to fail rule    |
    |double  S_pass,       : required Significance to pass rule                |
    |double* epsilon_obs   : uninitialised observed failure rate fraction      |
    |double* S_fail_obs    : uninitialised Significance of failure             |
    |double* S_pass_obs    : uninitialised Significance of Success             |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double* epsilon_obs   : the observed failure rate frac. from the histogram|
    |double* S_fail_obs    : the observed Significance of failure              |
    |double* S_pass_obs    : the observed Significance of Success              |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 07.Jan.2006                                               |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    +--------------------------------------------------------------------------*/
  epsilon_obs = 0.0;
  S_fail_obs = 0.0;
  S_pass_obs = 0.0;

  //-----------Perform Quality Checks on Input-------------
  if (!histogram)  {result = 5; return 0.0;}   //exit if histo does not exist
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}   //exit if epsilon_max not in (0,1)
  if (S_fail < 0)  {result = 5; return 0.0;}   //exit if Significance < 0
  if (S_pass < 0)  {result = 5; return 0.0;}   //exit if Significance < 0
  S_fail_obs = 0.0; S_pass_obs = 0.0;          //initialise Sig return values
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1){result = 4; return 0.0;}   //exit if histo has 0 entries

  //-----------Find number of successes and failures-------------
  int Nsuccesses = 0, MaxSuccesses = 0;
  int Nbins = histogram -> GetNbinsX();
  for (int i = 1; i <= Nbins-2; i++)  //initialise Nsuccesses
  { //histos start with bin index 1 (not 0)
    //proper CFEB synchronisation is represented by having either
    //  all entries in one bin or
    //  all entries in two bins which are separated by one bin
    Nsuccesses  = (int) histogram -> GetBinContent(i)
	          + (int) histogram -> GetBinContent(i+2);
    //since we do not know which bin(s) represent the CFEB
    //synchronisation assume that the bin(s) with the maximum entries
    //is(are) the correct bin(s)
    if (Nsuccesses > MaxSuccesses) MaxSuccesses = Nsuccesses;
  }
  //Histogram wrap-around case #1
  Nsuccesses  = (int) histogram -> GetBinContent(Nbins-1)
		+ (int) histogram -> GetBinContent(1);
  if (Nsuccesses > MaxSuccesses) MaxSuccesses = Nsuccesses;
  //Histogram wrap-around case #2
  Nsuccesses  = (int) histogram -> GetBinContent(Nbins)
		+ (int) histogram -> GetBinContent(2);
  if (Nsuccesses > MaxSuccesses) MaxSuccesses = Nsuccesses;

  int Nfailures       = Nentries - MaxSuccesses;
  double Nepsilon_max = (double)Nentries * epsilon_max;
  epsilon_obs        = (double)Nfailures / (double)Nentries;

  //-----------Calculate Statistical Significance-------------
  BinLogLikelihoodRatio(Nentries,Nfailures,epsilon_max,&S_fail_obs,&S_pass_obs);
  if (Nfailures > Nepsilon_max)
  {
    if (S_fail_obs > S_fail)
    {result = 1; return 0.0;}           //exit if statistically fails rule
    else
    {result = 3; return 0.0;}           //exit if non-stat significant result
  }
  else
  {
    if (S_pass_obs > S_pass)
    {result = 0; return 1.0;}           //exit if statistically passes rule
    else
    {result = 2; return 0.0;}           //exit if non-stat significant result
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
#if 0
float
FlatOccupancy1d::runTest(const TH1F *histogram)
{
  /*--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH1F* histogram,      : histogram to be compared with Rule                |
    |int* mask             : bit mask which excludes bins from consideration   |
    |double epsilon_min,   : minimum tolerance (fraction of line)              |
    |double epsilon_max,   : maximum tolerance (fraction of line)              |
    |double S_fail,        : required Statistical Significance to fail rule    |
    |double S_pass,        : required Significance to pass rule                |
    |double[2][] FailedBins: uninit. vector of bins out of tolerance           |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double[2][] FailedBins: the obs. vector of bins out of tolerance          |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 07.Jan.2006                                               |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    +--------------------------------------------------------------------------*/
  double *S_fail_obs;
  double *S_pass_obs;
  double dummy1, dummy2;
  S_fail_obs = &dummy1;
  S_pass_obs = &dummy2;
  *S_fail_obs = 0.0;
  *S_pass_obs = 0.0;
  Nbins = histogram -> GetNbinsX();

  //-----------Perform Quality Checks on Input-------------
  if (!histogram)  {result = 5; return 0.0;}//exit if histo does not exist
  if (epsilon_min <= 0.0 || epsilon_min >= 1.0)
  {result = 5; return 0.0;}//exit if epsilon_min not in (0,1)
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}//exit if epsilon_max not in (0,1)
  if (epsilon_max < epsilon_min)
  {result = 5; return 0.0;}//exit if max < min
  if (S_fail < 0) {result = 5; return 0.0;}//exit if Significance < 0
  if (S_pass < 0) {result = 5; return 0.0;}//exit if Significance < 0
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1){result = 4; return 0.0;}//exit if histo has 0 entries

  //-----------Find best value for occupancy b----------------
  double b = 0.0;
  int NusedBins = 0;
  for (int i = 1; i <= Nbins; i++)          //loop over all bins
  {
    if (ExclusionMask[i-1] != 1)          //do not check if bin excluded (=1)
    {
      b += histogram -> GetBinContent(i);
      NusedBins += 1;                  //keep track of # checked bins
    }
  }
  b *= 1.0 / (double) NusedBins;           //average for poisson stats

  //-----------Calculate Statistical Significance-------------
  double S_pass_obs_min = 0.0, S_fail_obs_max = 0.0;
  // allocate Nbins of memory for FailedBins
  for (int i = 0; i <= 1; i++) FailedBins[i] = new double [Nbins];
  // remember to delete[] FailedBins[0] and delete[] FailedBins[1]
  for (int i = 1; i <= Nbins; i++)         //loop (again) over all bins
  {
    FailedBins[0][i-1] = 0.0;            //initialise obs fraction
    FailedBins[1][i-1] = 0.0;            //initialise obs significance
    if (ExclusionMask[i-1] != 1)          //do not check if bin excluded (=1)
    {
      //determine significance for bin to fail or pass, given occupancy
      //hypothesis b with tolerance epsilon_min < b < epsilon_max
      PoissionLogLikelihoodRatio(histogram->GetBinContent(i),
				 b,
				 epsilon_min, epsilon_max,
				 S_fail_obs, S_pass_obs);
      //set S_fail_obs to maximum over all non-excluded bins
      //set S_pass_obs to non-zero minimum over all non-excluded bins
      if (S_fail_obs_max == 0.0 && *S_pass_obs > 0.0)
	S_pass_obs_min = *S_pass_obs;  //init to first non-zero value
      if (*S_fail_obs > S_fail_obs_max) S_fail_obs_max = *S_fail_obs;
      if (*S_pass_obs < S_pass_obs_min) S_pass_obs_min = *S_pass_obs;
      //set FailedBins[0][] to fraction away from fitted line b
      //set to zero if bin is within tolerance (via initialisation)
      if (*S_fail_obs > 0) FailedBins[0][i-1] =
			     histogram->GetBinContent(i)/b - 1.0;
      //set FailedBins[1][] to observed significance of failure
      //set to zero if bin is within tolerance (via initialisation)
      if (*S_fail_obs > 0) FailedBins[1][i-1] = *S_fail_obs;
    }
  }
  *S_fail_obs = S_fail_obs_max;
  *S_pass_obs = S_pass_obs_min;
  if (*S_fail_obs > 0.0)
  {
    if (*S_fail_obs > S_fail)
    {result = 1; return 0.0;}           //exit if statistically fails rule
    else
    {result = 3; return 0.0;}           //exit if non-stat significant result
  }
  else
  {
    if (*S_pass_obs > S_pass)
    {result = 0; return 1.0;}           //exit if statistically passes rule
    else
    {result = 2; return 0.0;}           //exit if non-stat significant result
  }
}
#endif

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
float
FixedFlatOccupancy1d::runTest(const TH1F *histogram)
{
  set_Occupancy( 1.0 );
  double mask[10] = {1,0,0,0,1,1,1,1,1,1};
  set_ExclusionMask( mask );
  set_epsilon_min( 0.099 );
  set_epsilon_max( 0.101 );
  set_S_fail( 5.0 );
  set_S_pass( 5.0 ); 

  /*--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH1F* histogram,      : histogram to be compared with Rule                |
    |int* mask             : bit mask which excludes bins from consideration   |
    |double epsilon_min,   : minimum tolerance (fraction of line)              |
    |double epsilon_max,   : maximum tolerance (fraction of line)              |
    |double S_fail,        : required Statistical Significance to fail rule    |
    |double S_pass,        : required Significance to pass rule                |
    |double[2][] FailedBins: uninit. vector of bins out of tolerance           |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double[2][] FailedBins: the obs. vector of bins out of tolerance          |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 07.Jan.2006                                               |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    +--------------------------------------------------------------------------*/
  double *S_fail_obs;
  double *S_pass_obs;
  double dummy1, dummy2;
  S_fail_obs = &dummy1;
  S_pass_obs = &dummy2;
  *S_fail_obs = 0.0;
  *S_pass_obs = 0.0;
  Nbins = histogram -> GetNbinsX();

  //-----------Perform Quality Checks on Input-------------
  if (!histogram)  {result = 5; return 0.0;}   //exit if histo does not exist
  if (epsilon_min <= 0.0 || epsilon_min >= 1.0)
  {result = 5; return 0.0;}   //exit if epsilon_min not in (0,1)
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}   //exit if epsilon_max not in (0,1)
  if (epsilon_max < epsilon_min)
  {result = 5; return 0.0;}   //exit if max < min
  if (S_fail < 0)  {result = 5; return 0.0;}   //exit if Significance < 0
  if (S_pass < 0)  {result = 5; return 0.0;}   //exit if Significance < 0
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1){result = 4; return 0.0;}    //exit if histo has 0 entries

  //-----------Calculate Statistical Significance-------------
  double S_pass_obs_min = 0.0, S_fail_obs_max = 0.0;
  // allocate Nbins of memory for FailedBins
  for (int i = 0; i <= 1; i++) FailedBins[i] = new double [Nbins];
  // remember to delete[] FailedBins[0] and delete[] FailedBins[1];
  for (int i = 1; i <= Nbins; i++)         //loop over all bins
  {
    FailedBins[0][i-1] = 0.0;            //initialise obs fraction
    FailedBins[1][i-1] = 0.0;            //initialise obs significance
    if (ExclusionMask[i-1] != 1)          //do not check if bin excluded
    {
      //determine significance for bin to fail or pass, given occupancy
      //hypothesis b with tolerance epsilon_min < b < epsilon_max
      PoissionLogLikelihoodRatio(histogram->GetBinContent(i),
				 b,
				 epsilon_min, epsilon_max,
				 S_fail_obs, S_pass_obs);
      //set S_fail_obs to maximum over all non-excluded bins
      //set S_pass_obs to non-zero minimum over all non-excluded bins
      if (S_fail_obs_max == 0.0 && *S_pass_obs > 0.0)
	S_pass_obs_min = *S_pass_obs;  //init to first non-zero value
      if (*S_fail_obs > S_fail_obs_max) S_fail_obs_max = *S_fail_obs;
      if (*S_pass_obs < S_pass_obs_min) S_pass_obs_min = *S_pass_obs;
      //set FailedBins[0][] to fraction away from fitted line b
      //set to zero if bin is within tolerance (via initialisation)
      if (*S_fail_obs > 0) FailedBins[0][i-1] =
			     histogram->GetBinContent(i)/b - 1.0;
      //set FailedBins[1][] to observed significance of failure
      //set to zero if bin is within tolerance (via initialisation)
      if (*S_fail_obs > 0) FailedBins[1][i-1] = *S_fail_obs;
    }
  }
  *S_fail_obs = S_fail_obs_max;
  *S_pass_obs = S_pass_obs_min;
  if (*S_fail_obs > 0.0)
  {
    if (*S_fail_obs > S_fail)
    {result = 1; return 0.0;}            //exit if statistically fails rule
    else
    {result = 3; return 0.0;}            //exit if non-stat significant result
  }
  else
  {
    if (*S_pass_obs > S_pass)
    {result = 0; return 1.0;}            //exit if statistically passes rule
    else
    {result = 2; return 0.0;}            //exit if non-stat significant result
  }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
#if 0
float
AllContentAlongDiagonal::runTest(const TH2F *histogram)
{
  /*
    +--------------------------------------------------------------------------+
    |                 Input to this function                                   |
    +--------------------------------------------------------------------------+
    |TH2* histogram,       : histogram to be compared with Rule                |
    |double epsilon_max,   : maximum allowed failure rate fraction             |
    |double S_fail,        : required Significance to fail rule                |
    |double S_pass,        : required Significance to pass rule                |
    |double* epsilon_obs   : uninitialised actual failure rate fraction        |
    |double* S_fail_obs    : uninitialised Statistical Significance of failure |
    |double* S_pass_obs    : uninitialised Significance of Success             |
    +--------------------------------------------------------------------------+
    |                 Result values for this function                          |
    +--------------------------------------------------------------------------+
    |int result            : "0" = "Passed Rule & is statistically significant"|
    |                        "1" = "Failed Rule & is statistically significant"|
    |                        "2" = "Passed Rule & not stat. significant"       |
    |                        "3" = "Failed Rule & not stat. significant"       |
    |                        "4" = "zero histo entries, can not evaluate Rule" |
    |                        "5" = "Input invalid,      can not evaluate Rule" |
    |double* epsilon_obs   : the observed failure rate frac. from the histogram|
    |double* S_fail_obs    : the observed Significance of failure              |
    |double* S_pass_obs    : the observed Significance of Success              |
    +--------------------------------------------------------------------------+
    | Author: Richard Cavanaugh, University of Florida                         |
    | email:  Richard.Cavanaugh@cern.ch                                        |
    | Creation Date: 11.July.2005                                              |
    | Last Modified: 16.Jan.2006                                               |
    | Comments:                                                                |
    +--------------------------------------------------------------------------+
  */
  //-----------Perform Quality Checks on Input-------------
  if (!histogram)  {result = 5; return 0.0;}//exit if histo does not exist
  if (histogram -> GetNbinsX() != histogram -> GetNbinsY())
  {result = 5; return 0.0;}//exit if histogram not square
  if (epsilon_max <= 0.0 || epsilon_max >= 1.0)
  {result = 5; return 0.0;}//exit if epsilon_max not in (0,1)
  if (S_fail < 0)  {result = 5; return 0.0;}//exit if Significance < 0
  if (S_pass < 0)  {result = 5; return 0.0;}//exit if Significance < 0
  S_fail_obs = 0.0; S_pass_obs = 0.0;       //initialise Sig return values
  int Nentries = (int) histogram -> GetEntries();
  if (Nentries < 1){result = 4; return 0.0;}//exit if histo has 0 entries

  //-----------Find number of successes and failures-------------
  int Nsuccesses = 0;
  for (int i = 0; i <= histogram -> GetNbinsX() + 1; i++)//count the number of
  {                                       //entries contained along diag.
    Nsuccesses += (int) histogram -> GetBinContent(i,i);
  }
  int Nfailures       = Nentries - Nsuccesses;
  double Nepsilon_max = (double)Nentries * epsilon_max;
  epsilon_obs         = (double)Nfailures / (double)Nentries;

  //-----------Calculate Statistical Significance-------------
  BinLogLikelihoodRatio(Nentries,Nfailures,epsilon_max,&S_fail_obs,&S_pass_obs);
  if (Nfailures > Nepsilon_max)
  {
    if (S_fail_obs > S_fail)
    {result = 1; return 0.0;}           //exit if statistically fails rule
    else
    {result = 3; return 0.0;}           //exit if non-stat significant result
  }
  else
  {
    if (S_pass_obs > S_pass)
    {result = 0; return 1.0;}           //exit if statistically passes rule
    else
    {result = 2; return 0.0;}           //exit if non-stat significant result
  }
}
#endif