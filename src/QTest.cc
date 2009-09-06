#include "DQMServices/Core/interface/QTest.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include "DQMServices/Core/src/QStatisticalTests.h"
#include "DQMServices/Core/src/DQMError.h"
#include "TMath.h"
#include "TH1F.h"
#include "TH1S.h"
#include "TH1D.h"
#include <iostream>
#include <sstream>
#include <math.h>

using namespace std;

const float QCriterion::ERROR_PROB_THRESHOLD = 0.50;
const float QCriterion::WARNING_PROB_THRESHOLD = 0.90;

// initialize values
void
QCriterion::init(void)
{
  errorProb_ = ERROR_PROB_THRESHOLD;
  warningProb_ = WARNING_PROB_THRESHOLD;
  setAlgoName("NO_ALGORITHM");
  status_ = dqm::qstatus::DID_NOT_RUN;
  message_ = "NO_MESSAGE";
  verbose_ = 0; // 0 = silent, 1 = algorithmic failures, 2 = info
}

float QCriterion::runTest(const MonitorElement *me)
{
  raiseDQMError("QCriterion", "virtual runTest method called" );
  return 0.;
}
//===================================================//
//================ QUALITY TESTS ====================//
//==================================================//

//-------------------------------------------------------//
//----------------- Comp2RefEqualH base -----------------//
//-------------------------------------------------------//
// run the test (result: [0, 1] or <0 for failure)
float Comp2RefEqualH::runTest(const MonitorElement*me)
{
  badChannels_.clear();

  if (!me) 
    return -1;
  if (!me->getRootObject() || !me->getRefRootObject()) 
    return -1;
  TH1* h=0; //initialize histogram pointer
  TH1* ref_=0;
  
  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";

  int nbins=0;
  int nbinsref=0;
  //-- TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F)
  { 
    nbins = me->getTH1F()->GetXaxis()->GetNbins(); 
    nbinsref = me->getRefTH1F()->GetXaxis()->GetNbins();
    h  = me->getTH1F(); // access Test histo
    ref_ = me->getRefTH1F(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 
  //-- TH1S
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S)
  { 
    nbins = me->getTH1S()->GetXaxis()->GetNbins(); 
    nbinsref = me->getRefTH1S()->GetXaxis()->GetNbins();
    h  = me->getTH1S(); // access Test histo
    ref_ = me->getRefTH1S(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 
  //-- TH1D
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D)
  { 
    nbins = me->getTH1D()->GetXaxis()->GetNbins(); 
    nbinsref = me->getRefTH1S()->GetXaxis()->GetNbins();
    h  = me->getTH1D(); // access Test histo
    ref_ = me->getRefTH1D(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 
  //-- TH2
  else if (me->kind()==MonitorElement::DQM_KIND_TH2F)
  { 
    nbins = me->getTH2F()->GetXaxis()->GetNbins() *
            me->getTH2F()->GetYaxis()->GetNbins();
    nbinsref = me->getRefTH2F()->GetXaxis()->GetNbins() *
               me->getRefTH2F()->GetYaxis()->GetNbins();
    h = me->getTH2F(); // access Test histo
    ref_ = me->getRefTH2F(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 

  //-- TH2
  else if (me->kind()==MonitorElement::DQM_KIND_TH2S)
  { 
    nbins = me->getTH2S()->GetXaxis()->GetNbins() *
            me->getTH2S()->GetYaxis()->GetNbins();
    nbinsref = me->getRefTH2S()->GetXaxis()->GetNbins() *
               me->getRefTH2S()->GetYaxis()->GetNbins();
    h = me->getTH2S(); // access Test histo
    ref_ = me->getRefTH2S(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 

  //-- TH3
  else if (me->kind()==MonitorElement::DQM_KIND_TH3F)
  { 
    nbins = me->getTH3F()->GetXaxis()->GetNbins() *
            me->getTH3F()->GetYaxis()->GetNbins() *
            me->getTH3F()->GetZaxis()->GetNbins();
    nbinsref = me->getRefTH3F()->GetXaxis()->GetNbins() *
               me->getRefTH3F()->GetYaxis()->GetNbins() *
               me->getRefTH3F()->GetZaxis()->GetNbins();
    h = me->getTH3F(); // access Test histo
    ref_ = me->getRefTH3F(); //access Ref hiso 
    if (nbins != nbinsref) return -1;
  } 

  else
  { 
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefEqualH" 
                << " ME does not contain TH1F/TH1S/TH1D/TH2F/TH2S/TH3F, exiting\n"; 
    return -1;
  } 

  //--  QUALITY TEST itself 
  Int_t first = 0; // 1 //(use underflow bin)
  Int_t last  = nbins+1; //(use overflow bin)
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
  return 1;
}

//-------------------------------------------------------//
//-----------------  Comp2RefChi2    --------------------//
//-------------------------------------------------------//
float Comp2RefChi2::runTest(const MonitorElement *me)
{
  if (!me) 
    return -1;
  if (!me->getRootObject() || !me->getRefRootObject()) 
    return -1;
  TH1* h=0;
  TH1* ref_=0;
 
  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";
  //-- TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F)
  { 
    h = me->getTH1F(); // access Test histo
    ref_ = me->getRefTH1F(); //access Ref histo
  } 
  //-- TH1S
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S)
  { 
    h = me->getTH1S(); // access Test histo
    ref_ = me->getRefTH1S(); //access Ref histo
  } 
  //-- TH1D
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D)
  { 
    h = me->getTH1D(); // access Test histo
    ref_ = me->getRefTH1D(); //access Ref histo
  } 
  //-- TProfile
  else if (me->kind()==MonitorElement::DQM_KIND_TPROFILE)
  {
    h = me->getTProfile(); // access Test histo
    ref_ = me->getRefTProfile(); //access Ref histo
  } 
  else
  { 
    if (verbose_>0) 
      std::cout << "QTest::Comp2RefChi2"
                << " ME does not contain TH1F/TH1S/TH1D/TProfile, exiting\n"; 
    return -1;
  } 

   //-- isInvalid ? - Check consistency in number of channels
  Int_t ncx1  = h->GetXaxis()->GetNbins(); 
  Int_t ncx2  = ref_->GetXaxis()->GetNbins();
  if ( ncx1 !=  ncx2)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefChi2"
                << " different number of channels! ("
                << ncx1 << ", " << ncx2 << "), exiting\n";
    return -1;
  } 

  //--  QUALITY TEST itself 
  //reset Results
  Ndof_ = 0; chi2_ = -1.; ncx1 = ncx2 = -1;

  Int_t i, i_start, i_end;
  float chi2 = 0.;  int ndof = 0; int constraint = 0;

  i_start = 1;  
  i_end = ncx1;
  //  if (fXaxis.TestBit(TAxis::kAxisRange)) {
  i_start = h->GetXaxis()->GetFirst();
  i_end   = h->GetXaxis()->GetLast();
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
  if (sum1 == 0)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefChi2"
                << " Test Histogram " << h->GetName() 
		<< " is empty, exiting\n";
    return -1;
  }
  if (sum2 == 0)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefChi2"
                << " Ref Histogram " << ref_->GetName() 
                << " is empty, exiting\n";
    return -1;
  }

  Double_t bin1, bin2, err1, err2, temp;
  for (i=i_start; i<=i_end; i++)
  {
    bin1 = h->GetBinContent(i)/sum1;
    bin2 = ref_->GetBinContent(i)/sum2;
    if (bin1 ==0 && bin2==0)
    {
      --ndof; //no data means one less degree of freedom
    } 
    else 
    {
      temp  = bin1-bin2;
      err1=h->GetBinError(i); err2=ref_->GetBinError(i);
      if (err1 == 0 && err2 == 0)
      {
	if (verbose_>0) 
	  std::cout << "QTest:Comp2RefChi2"
	            << " bins with non-zero content and zero error, exiting\n";
	return -1;
      }
      err1*=err1      ; err2*=err2;
      err1/=sum1*sum1 ; err2/=sum2*sum2;
      chi2 +=temp*temp/(err1+err2);
    }
  }
  chi2_ = chi2;  Ndof_ = ndof;
  return TMath::Prob(0.5*chi2, Int_t(0.5*ndof));
}

//-------------------------------------------------------//
//-----------------  Comp2RefKolmogorov    --------------//
//-------------------------------------------------------//

float Comp2RefKolmogorov::runTest(const MonitorElement *me)
{
  const Double_t difprec = 1e-5;
   
  if (!me) 
    return -1;
  if (!me->getRootObject() || !me->getRefRootObject()) 
    return -1;
  TH1* h=0;
  TH1* ref_=0;

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";
  //-- TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F)
  { 
    h = me->getTH1F(); // access Test histo
    ref_ = me->getRefTH1F(); //access Ref histo
  } 
  //-- TH1S
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S)
  { 
    h = me->getTH1S(); // access Test histo
    ref_ = me->getRefTH1S(); //access Ref histo
  } 
  //-- TH1D
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D)
  { 
    h = me->getTH1D(); // access Test histo
    ref_ = me->getRefTH1D(); //access Ref histo
  } 
  //-- TProfile
  else if (me->kind()==MonitorElement::DQM_KIND_TPROFILE)
  {
    h = me->getTProfile(); // access Test histo
    ref_ = me->getRefTProfile(); //access Ref histo
  }
  else
  { 
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov"
                << " ME does not contain TH1F/TH1S/TH1D/TProfile, exiting\n"; 
    return -1;
  } 
   
  //-- isInvalid ? - Check consistency in number of channels
  Int_t ncx1 = h->GetXaxis()->GetNbins(); 
  Int_t ncx2 = ref_->GetXaxis()->GetNbins();
  if ( ncx1 !=  ncx2)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov"
                << " different number of channels! ("
                << ncx1 << ", " << ncx2 << "), exiting\n";
    return -1;
  } 
  //-- isInvalid ? - Check consistency in channel edges
  Double_t diff1 = TMath::Abs(h->GetXaxis()->GetXmin() - ref_->GetXaxis()->GetXmin());
  Double_t diff2 = TMath::Abs(h->GetXaxis()->GetXmax() - ref_->GetXaxis()->GetXmax());
  if (diff1 > difprec || diff2 > difprec)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov"
                << " histograms with different binning, exiting\n";
    return -1;
  }

  //--  QUALITY TEST itself 
  Bool_t afunc1 = kFALSE; Bool_t afunc2 = kFALSE;
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
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov"
                << " Test Histogram: " << h->GetName() 
                << ": integral is zero, exiting\n";
    return -1;
  }
  if (sum2 == 0)
  {
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov"
                << " Ref Histogram: " << ref_->GetName() 
                << ": integral is zero, exiting\n";
    return -1;
  }

  Double_t tsum1 = sum1; Double_t tsum2 = sum2;
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
    {
      if (verbose_>0) 
        std::cout << "QTest:Comp2RefKolmogorov"
                  << " Weighted events and no Sumw2 for "
	          << h->GetName() << "\n";
    }
    else
    {
      esum1 = sum1*sum1/w1;  //number of equivalent entries
    }
  }
  // look at second histogram
  Double_t difsum2 = (ne2-tsum2)/tsum2;
  Double_t esum2   = sum2;
  if (difsum2 > difprec && Int_t(ne2) != ncx1)
  {
    if (ref_->GetSumw2N() == 0)
    {
      if (verbose_>0) 
        std::cout << "QTest:Comp2RefKolmogorov"
                  << " Weighted events and no Sumw2 for "
	          << ref_->GetName() << "\n";
    }
    else
    {
      esum2 = sum2*sum2/w2;  //number of equivalent entries
    }
  }

  Double_t s1 = 1/tsum1; Double_t s2 = 1/tsum2;
  // Find largest difference for Kolmogorov Test
  Double_t dfmax =0, rsum1 = 0, rsum2 = 0;
  // use underflow bin
  Int_t first = 0; // 1
  // use overflow bin
  Int_t last  = ncx1+1; // ncx1
  for ( bin=first; bin<=last; bin++)
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
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov" 
                << " Numerical problems with histogram "
                << h->GetName() << "\n";
  if (TMath::Abs(rsum2-1) > 0.002)
    if (verbose_>0) 
      std::cout << "QTest:Comp2RefKolmogorov" 
                << " Numerical problems with histogram "
                << ref_->GetName() << "\n";

  return TMath::KolmogorovProb(z);
}

//----------------------------------------------------//
//--------------- ContentsXRange ---------------------//
//----------------------------------------------------//
float ContentsXRange::runTest(const MonitorElement*me)
{
  badChannels_.clear();

  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1;
  TH1* h=0; 

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";
  // -- TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F ) 
  {
    h = me->getTH1F();
  } 
  // -- TH1S
  else if ( me->kind()==MonitorElement::DQM_KIND_TH1S ) 
  {
    h = me->getTH1S();
  } 
  // -- TH1D
  else if ( me->kind()==MonitorElement::DQM_KIND_TH1D ) 
  {
    h = me->getTH1D();
  } 
  else 
  {
    if (verbose_>0) std::cout << "QTest:ContentsXRange"
         << " ME " << me->getFullname() 
         << " does not contain TH1F/TH1S/TH1D, exiting\n"; 
    return -1;
  } 

  if (!rangeInitialized_)
  {
    if ( h->GetXaxis() ) 
      setAllowedXRange(h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax());
    else 
      return -1;
  }
  Int_t ncx = h->GetXaxis()->GetNbins();
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

  if (sum==0) return 1;
  // return fraction of entries within allowed X-range
  return (sum - fail)/sum; 

}

//-----------------------------------------------------//
//--------------- ContentsYRange ---------------------//
//----------------------------------------------------//
float ContentsYRange::runTest(const MonitorElement*me)
{
  badChannels_.clear();

  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1;
  TH1* h=0; 

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";

  if (me->kind()==MonitorElement::DQM_KIND_TH1F) 
  { 
    h = me->getTH1F(); //access Test histo
  } 
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S) 
  { 
    h = me->getTH1S(); //access Test histo
  } 
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D) 
  { 
    h = me->getTH1D(); //access Test histo
  } 
  else 
  {
    if (verbose_>0) 
      std::cout << "QTest:ContentsYRange" 
                << " ME " << me->getFullname() 
                << " does not contain TH1F/TH1S/TH1D, exiting\n"; 
    return -1;
  } 

  if (!rangeInitialized_ || !h->GetXaxis()) return 1; // all bins are accepted if no initialization
  Int_t ncx = h->GetXaxis()->GetNbins();
  // do NOT use underflow bin
  Int_t first = 1;
  // do NOT use overflow bin
  Int_t last  = ncx;
  // bins outside Y-range
  Int_t fail = 0;
  Int_t bin;
  
  if (useEmptyBins_)///Standard test !
  {
    for (bin = first; bin <= last; ++bin)
    {
      Double_t contents = h->GetBinContent(bin);
      bool failure = false;
      failure = (contents < ymin_ || contents > ymax_); // allowed y-range: [ymin_, ymax_]
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
  else ///AS quality test !!!  
  {
    for (bin = first; bin <= last; ++bin)
    {
      Double_t contents = h->GetBinContent(bin);
      bool failure = false;
      if (contents) failure = (contents < ymin_ || contents > ymax_); // allowed y-range: [ymin_, ymax_]
      if (failure) ++fail;
    }
    // return fraction of bins that passed test
    return 1.*(ncx - fail)/ncx;
  } ///end of AS quality tests 
}

//-----------------------------------------------------//
//------------------ DeadChannel ---------------------//
//----------------------------------------------------//
float DeadChannel::runTest(const MonitorElement*me)
{
  badChannels_.clear();
  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1;
  TH1* h1=0;
  TH2* h2=0;//initialize histogram pointers

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";
  //TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F) 
  { 
    h1 = me->getTH1F(); //access Test histo
  } 
  //TH1S
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S) 
  { 
    h1 = me->getTH1S(); //access Test histo
  } 
  //TH1D
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D) 
  { 
    h1 = me->getTH1D(); //access Test histo
  } 
  //-- TH2F
  else if (me->kind()==MonitorElement::DQM_KIND_TH2F)
  { 
    h2  = me->getTH2F(); // access Test histo
  } 
  //-- TH2S
  else if (me->kind()==MonitorElement::DQM_KIND_TH2S)
  { 
    h2  = me->getTH2S(); // access Test histo
  } 
  else 
  {
    if (verbose_>0) 
      std::cout << "QTest:DeadChannel"
         << " ME " << me->getFullname() 
         << " does not contain TH1F/TH1S/TH1D/TH2F/TH1S, exiting\n"; 
    return -1;
  } 

  Int_t fail = 0; // number of failed channels

  //--------- do the quality test for 1D histo ---------------//
  if (h1 != NULL)  
  {
    if (!rangeInitialized_ || !h1->GetXaxis() ) 
      return 1; // all bins are accepted if no initialization
    Int_t ncx = h1->GetXaxis()->GetNbins();
    Int_t first = 1;
    Int_t last  = ncx;
    Int_t bin;

    /// loop over all channels
    for (bin = first; bin <= last; ++bin)
    {
      Double_t contents = h1->GetBinContent(bin);
      bool failure = false;
      failure = contents <= ymin_; // dead channel: equal to or less than ymin_
      if (failure)
      { 
        DQMChannel chan(bin, 0, 0, contents, h1->GetBinError(bin));
        badChannels_.push_back(chan);
        ++fail;
      }
    }
    //return fraction of alive channels
    return 1.*(ncx - fail)/ncx;
  }
  //----------------------------------------------------------//
 
  //--------- do the quality test for 2D -------------------//
  else if (h2 !=NULL )
  {
    int ncx = h2->GetXaxis()->GetNbins(); // get X bins
    int ncy = h2->GetYaxis()->GetNbins(); // get Y bins

    /// loop over all bins 
    for (int cx = 1; cx <= ncx; ++cx)
    {
      for (int cy = 1; cy <= ncy; ++cy)
      {
	Double_t contents = h2->GetBinContent(h2->GetBin(cx, cy));
	bool failure = false;
	failure = contents <= ymin_; // dead channel: equal to or less than ymin_
	if (failure)
	{ 
          DQMChannel chan(cx, cy, 0, contents, h2->GetBinError(h2->GetBin(cx, cy)));
          badChannels_.push_back(chan);
          ++fail;
	}
      }
    }
    //return fraction of alive channels
    return 1.*(ncx*ncy - fail) / (ncx*ncy);
  }
  else 
  {
    if (verbose_>0) 
      std::cout << "QTest:DeadChannel"
                << " TH1/TH2F are NULL, exiting\n"; 
    return -1;
  }
}

//-----------------------------------------------------//
//----------------  NoisyChannel ---------------------//
//----------------------------------------------------//
// run the test (result: fraction of channels not appearing noisy or "hot")
// [0, 1] or <0 for failure
float NoisyChannel::runTest(const MonitorElement *me)
{
  badChannels_.clear();
  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1; 
  TH1* h=0;//initialize histogram pointer

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";

  int nbins=0;
  //-- TH1F
  if (me->kind()==MonitorElement::DQM_KIND_TH1F)
  { 
    nbins = me->getTH1F()->GetXaxis()->GetNbins(); 
    h  = me->getTH1F(); // access Test histo
  } 
  //-- TH1S
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S)
  { 
    nbins = me->getTH1S()->GetXaxis()->GetNbins(); 
    h  = me->getTH1S(); // access Test histo
  } 
  //-- TH1D
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D)
  { 
    nbins = me->getTH1D()->GetXaxis()->GetNbins(); 
    h  = me->getTH1D(); // access Test histo
  } 
  //-- TH2
  else if (me->kind()==MonitorElement::DQM_KIND_TH2F)
  { 
    nbins = me->getTH2F()->GetXaxis()->GetNbins() *
            me->getTH2F()->GetYaxis()->GetNbins();
    h  = me->getTH2F(); // access Test histo
  } 
  //-- TH2
  else if (me->kind()==MonitorElement::DQM_KIND_TH2S)
  { 
    nbins = me->getTH2S()->GetXaxis()->GetNbins() *
            me->getTH2S()->GetYaxis()->GetNbins();
    h  = me->getTH2S(); // access Test histo
  } 
  else 
  {  
    if (verbose_>0) 
      std::cout << "QTest:NoisyChannel"
        << " ME " << me->getFullname() 
        << " does not contain TH1F/TH1S/TH1D or TH2F/TH2S, exiting\n"; 
    return -1;
  }

  //--  QUALITY TEST itself 

  if ( !rangeInitialized_ || !h->GetXaxis() ) 
    return 1; // all channels are accepted if tolerance has not been set

  // do NOT use underflow bin
  Int_t first = 1;
  // do NOT use overflow bin
  Int_t last  = nbins;
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
  return 1.*(nbins - fail)/nbins;
}

// get average for bin under consideration
// (see description of method setNumNeighbors)
Double_t NoisyChannel::getAverage(int bin, const TH1 *h) const
{
  /// do NOT use underflow bin
  Int_t first = 1;
  /// do NOT use overflow bin
  Int_t ncx  = h->GetXaxis()->GetNbins();
  Double_t sum = 0; int bin_low, bin_hi;
  for (unsigned i = 1; i <= numNeighbors_; ++i)
  {
    /// use symmetric-to-bin bins to calculate average
    bin_low = bin-i;  bin_hi = bin+i;
    /// check if need to consider bins on other side of spectrum
    /// (ie. if bins below 1 or above ncx)
    while (bin_low < first) // shift bin by +ncx
      bin_low = ncx + bin_low;
    while (bin_hi > ncx) // shift bin by -ncx
      bin_hi = bin_hi - ncx;
      sum += h->GetBinContent(bin_low) + h->GetBinContent(bin_hi);
  }
  /// average is sum over the # of bins used
  return sum/(numNeighbors_ * 2);
}


//-----------------------------------------------------------//
//----------------  ContentsWithinExpected ---------------------//
//-----------------------------------------------------------//
// run the test (result: fraction of channels that passed test);
// [0, 1] or <0 for failure
float ContentsWithinExpected::runTest(const MonitorElement*me)
{
  badChannels_.clear();
  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1;
  TH1* h=0; //initialize histogram pointer

  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";

  int ncx;
  int ncy;

  if (useEmptyBins_)
  {
    //-- TH2
    if (me->kind()==MonitorElement::DQM_KIND_TH2F)
    {
      ncx = me->getTH2F()->GetXaxis()->GetNbins();
      ncy = me->getTH2F()->GetYaxis()->GetNbins(); 
      h  = me->getTH2F(); // access Test histo
    }
    //-- TH2S
    else if (me->kind()==MonitorElement::DQM_KIND_TH2S)
    {
      ncx = me->getTH2S()->GetXaxis()->GetNbins();
      ncy = me->getTH2S()->GetYaxis()->GetNbins(); 
      h  = me->getTH2S(); // access Test histo
    }
    //-- TProfile
    else if (me->kind()==MonitorElement::DQM_KIND_TPROFILE)
    {
      ncx = me->getTProfile()->GetXaxis()->GetNbins();
      ncy = 1;
      h  = me->getTProfile(); // access Test histo
    }
    //-- TProfile2D
    else if (me->kind()==MonitorElement::DQM_KIND_TPROFILE2D)
    {
      ncx = me->getTProfile2D()->GetXaxis()->GetNbins();
      ncy = me->getTProfile2D()->GetYaxis()->GetNbins();
      h  = me->getTProfile2D(); // access Test histo
    }
    else
    {
      if (verbose_>0) 
        std::cout << "QTest:ContentsWithinExpected" 
	 << " ME does not contain TH2F/TH2S/TPROFILE/TPROFILE2D, exiting\n"; 
      return -1;
    } 

    int nsum = 0;
    float sum = 0.0;
    float average = 0.0;

    if (checkMeanTolerance_)
    { // calculate average value of all bin contents

      for (int cx = 1; cx <= ncx; ++cx)
      {
	for (int cy = 1; cy <= ncy; ++cy)
	{
	  if (me->kind() == MonitorElement::DQM_KIND_TH2F)
	  {
	    sum += h->GetBinContent(h->GetBin(cx, cy));
	    ++nsum;
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TH2S)
	  {
	    sum += h->GetBinContent(h->GetBin(cx, cy));
	    ++nsum;
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TPROFILE)
	  {
	    if (me->getTProfile()->GetBinEntries(h->GetBin(cx)) >= minEntries_/(ncx))
	    {
	      sum += h->GetBinContent(h->GetBin(cx));
	      ++nsum;
	    }
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TPROFILE2D)
	  {
	    if (me->getTProfile2D()->GetBinEntries(h->GetBin(cx, cy)) >= minEntries_/(ncx*ncy))
	    {
	      sum += h->GetBinContent(h->GetBin(cx, cy));
	      ++nsum;
	    }
	  }
	}
      }

      if (nsum > 0) 
	average = sum/nsum;

    } // calculate average value of all bin contents

    int fail = 0;

    for (int cx = 1; cx <= ncx; ++cx)
    {
      for (int cy = 1; cy <= ncy; ++cy)
      {
	bool failMean = false;
	bool failRMS = false;
	bool failMeanTolerance = false;

	if (me->kind() == MonitorElement::DQM_KIND_TPROFILE && 
            me->getTProfile()->GetBinEntries(h->GetBin(cx)) < minEntries_/(ncx))
          continue;

	if (me->kind() == MonitorElement::DQM_KIND_TPROFILE2D && 
            me->getTProfile2D()->GetBinEntries(h->GetBin(cx, cy)) < minEntries_/(ncx*ncy)) 
	  continue;

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
          failMeanTolerance = (TMath::Abs(mean - average) > toleranceMean_*TMath::Abs(average));
	}

	if (failMean || failRMS || failMeanTolerance)
	{

	  if (me->kind() == MonitorElement::DQM_KIND_TH2F) 
	  {
            DQMChannel chan(cx, cy, 0,
			    h->GetBinContent(h->GetBin(cx, cy)),
			    h->GetBinError(h->GetBin(cx, cy)));
            badChannels_.push_back(chan);
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TH2S) 
	  {
            DQMChannel chan(cx, cy, 0,
			    h->GetBinContent(h->GetBin(cx, cy)),
			    h->GetBinError(h->GetBin(cx, cy)));
            badChannels_.push_back(chan);
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TPROFILE) 
	  {
	    DQMChannel chan(cx, cy, int(me->getTProfile()->GetBinEntries(h->GetBin(cx))),
			    0,
			    h->GetBinError(h->GetBin(cx)));
            badChannels_.push_back(chan);
	  }
	  else if (me->kind() == MonitorElement::DQM_KIND_TPROFILE2D) 
	  {
	    DQMChannel chan(cx, cy, int(me->getTProfile2D()->GetBinEntries(h->GetBin(cx, cy))),
			    h->GetBinContent(h->GetBin(cx, cy)),
			    h->GetBinError(h->GetBin(cx, cy)));
            badChannels_.push_back(chan);
	  }
          ++fail;
	}
      }
    }
    return 1.*(ncx*ncy - fail)/(ncx*ncy);
  } /// end of normal Test

  else     /// AS quality test !!!  
  {
    if (me->kind()==MonitorElement::DQM_KIND_TH2F)
    {
      ncx = me->getTH2F()->GetXaxis()->GetNbins();
      ncy = me->getTH2F()->GetYaxis()->GetNbins();
      h  = me->getTH2F(); // access Test histo
    }
    else if (me->kind()==MonitorElement::DQM_KIND_TH2S)
    {
      ncx = me->getTH2S()->GetXaxis()->GetNbins();
      ncy = me->getTH2S()->GetYaxis()->GetNbins();
      h  = me->getTH2S(); // access Test histo
    }
    else 
    {
      if (verbose_>0) 
        std::cout << "QTest:ContentsWithinExpected AS" 
                  << " ME does not contain TH2F/TH2S, exiting\n"; 
      return -1;
    } 

    // if (!rangeInitialized_) return 0; // all accepted if no initialization
    int fail = 0;
    for (int cx = 1; cx <= ncx; ++cx)
    {
      for (int cy = 1; cy <= ncy; ++cy)
      {
	bool failure = false;
	float Content = h->GetBinContent(h->GetBin(cx, cy));
	if (Content) 
	  failure = (Content <  minMean_ || Content >  maxMean_);
	if (failure) 
	  ++fail;
      }
    }
    return 1.*(ncx*ncy-fail)/(ncx*ncy);
  } /// end of AS quality test 

}
/// set expected value for mean
void ContentsWithinExpected::setMeanRange(float xmin, float xmax)
{
  if (xmax < xmin)
    if (verbose_>0) 
      std::cout << "QTest:ContentsWitinExpected" 
                << " Illogical range: (" << xmin << ", " << xmax << "\n";
  minMean_ = xmin;
  maxMean_ = xmax;
  checkMean_ = true;
}

/// set expected value for mean
void ContentsWithinExpected::setRMSRange(float xmin, float xmax)
{
  if (xmax < xmin)
    if (verbose_>0) 
      std::cout << "QTest:ContentsWitinExpected" 
                << " Illogical range: (" << xmin << ", " << xmax << "\n";
  minRMS_ = xmin;
  maxRMS_ = xmax;
  checkRMS_ = true;
}

//----------------------------------------------------------------//
//--------------------  MeanWithinExpected  ---------------------//
//---------------------------------------------------------------//
// run the test;
//   (a) if useRange is called: 1 if mean within allowed range, 0 otherwise
//   (b) is useRMS or useSigma is called: result is the probability
//   Prob(chi^2, ndof=1) that the mean of histogram will be deviated by more than
//   +/- delta from <expected_mean>, where delta = mean - <expected_mean>, and
//   chi^2 = (delta/sigma)^2. sigma is the RMS of the histogram ("useRMS") or
//   <expected_sigma> ("useSigma")
//   e.g. for delta = 1, Prob = 31.7%
//  for delta = 2, Prob = 4.55%
//   (returns result in [0, 1] or <0 for failure) 
float MeanWithinExpected::runTest(const MonitorElement *me )
{
  if (!me) 
    return -1;
  if (!me->getRootObject()) 
    return -1;
  TH1* h=0;
   
  if (verbose_>1) 
    std::cout << "QTest:" << getAlgoName() << "::runTest called on " 
              << me-> getFullname() << "\n";

  if (me->kind()==MonitorElement::DQM_KIND_TH1F) 
  { 
    h = me->getTH1F(); //access Test histo
  }
  else if (me->kind()==MonitorElement::DQM_KIND_TH1S) 
  { 
    h = me->getTH1S(); //access Test histo
  }
  else if (me->kind()==MonitorElement::DQM_KIND_TH1D) 
  { 
    h = me->getTH1D(); //access Test histo
  }
  else {
    if (verbose_>0) 
      std::cout << "QTest:MeanWithinExpected"
                << " ME " << me->getFullname() 
                << " does not contain TH1F/TH1S/TH1D, exiting\n"; 
    return -1;
  } 
 
  
  if (useRange_) {
    float mean = h->GetMean();
    if (mean <= xmax_ && mean >= xmin_) 
      return 1;
    else
      return 0;
  }
  else if (useSigma_) 
  {
    if (sigma_ != 0.) 
    {
      float chi = (h->GetMean() - expMean_)/sigma_;
      return TMath::Prob(chi*chi, 1);
    }
    else
    {
      if (verbose_>0) 
        std::cout << "QTest:MeanWithinExpected"
                  << " Error, sigma_ is zero, exiting\n";
      return 0;
    }
  }
  else if (useRMS_) 
  {
    if (h->GetRMS() != 0.) 
    {
      float chi = (h->GetMean() - expMean_)/h->GetRMS();
      return TMath::Prob(chi*chi, 1);
    }
    else
    {
      if (verbose_>0) 
        std::cout << "QTest:MeanWithinExpected"
                  << " Error, RMS is zero, exiting\n";
      return 0;
    }
  }
  else 
    if (verbose_>0) 
      std::cout << "QTest:MeanWithinExpected"
                << " Error, neither Range, nor Sigma, nor RMS, exiting\n";
    return -1; 
}

void MeanWithinExpected::useRange(float xmin, float xmax)
{
    useRange_ = true;
    useSigma_ = useRMS_ = false;
    xmin_ = xmin; xmax_ = xmax;
    if (xmin_ > xmax_)  
      if (verbose_>0) 
        std::cout << "QTest:MeanWithinExpected"
                  << " Illogical range: (" << xmin_ << ", " << xmax_ << "\n";
}
void MeanWithinExpected::useSigma(float expectedSigma)
{
  useSigma_ = true;
  useRMS_ = useRange_ = false;
  sigma_ = expectedSigma;
  if (sigma_ == 0) 
    if (verbose_>0) 
      std::cout << "QTest:MeanWithinExpected"
                << " Expected sigma = " << sigma_ << "\n";
}

void MeanWithinExpected::useRMS()
{
  useRMS_ = true;
  useSigma_ = useRange_ = false;
}
