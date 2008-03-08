#ifndef DQMSERVICES_CORE_Q_CRITERION_H
# define DQMSERVICES_CORE_Q_CRITERION_H

# include "DQMServices/Core/interface/MonitorElement.h"
# include "TProfile2D.h"
# include "TProfile.h"
# include "TH2F.h"
# include "TH1F.h"
# include <sstream>
# include <string>
# include <map>

#include "DQMServices/Core/interface/DQMStore.h"

//class MonitorElement;
//class Comp2RefChi2;			typedef Comp2RefChi2 Comp2RefChi2ROOT;
//class Comp2RefKolmogorov;		typedef Comp2RefKolmogorov Comp2RefKolmogorovROOT;
//class Comp2RefEqualString;		typedef Comp2RefEqualString Comp2RefEqualStringROOT;
//class Comp2RefEqualInt;			typedef Comp2RefEqualInt Comp2RefEqualIntROOT;
//class Comp2RefEqualFloat;		typedef Comp2RefEqualFloat Comp2RefEqualFloatROOT;
class Comp2RefEqualH1;			typedef Comp2RefEqualH1 Comp2RefEqualH1ROOT;
//class Comp2RefEqualH2;			typedef Comp2RefEqualH2 Comp2RefEqualH2ROOT;
//class Comp2RefEqualH3;			typedef Comp2RefEqualH3 Comp2RefEqualH3ROOT;
class ContentsXRange;			typedef ContentsXRange ContentsXRangeROOT;
//class ContentsYRange;			typedef ContentsYRange ContentsYRangeROOT;
//class DeadChannel;			typedef DeadChannel DeadChannelROOT;
//class NoisyChannel;			typedef NoisyChannel NoisyChannelROOT;
//class ContentsTH2FWithinRange;		typedef ContentsTH2FWithinRange ContentsTH2FWithinRangeROOT;
//class ContentsProfWithinRange;		typedef ContentsProfWithinRange ContentsProfWithinRangeROOT;
//class ContentsProf2DWithinRange;	typedef ContentsProf2DWithinRange ContentsProf2DWithinRangeROOT;
//class MeanWithinExpected;		typedef MeanWithinExpected MeanWithinExpectedROOT;
//class MostProbableLandau;		typedef MostProbableLandau MostProbableLandauROOT;
//class AllContentWithinFixedRange;	typedef AllContentWithinFixedRange RuleAllContentWithinFixedRange;		typedef AllContentWithinFixedRange AllContentWithinFixedRangeROOT;
//class AllContentWithinFloatingRange;	typedef AllContentWithinFloatingRange RuleAllContentWithinFloatingRange;	typedef AllContentWithinFloatingRange AllContentWithinFloatingRangeROOT;
//class FlatOccupancy1d;			typedef FlatOccupancy1d RuleFlatOccupancy1d;					typedef FlatOccupancy1d FlatOccupancy1dROOT;
//class FixedFlatOccupancy1d;		typedef FixedFlatOccupancy1d RuleFixedFlatOccupancy1d;				typedef FixedFlatOccupancy1d FixedFlatOccupancy1dROOT;
//class CSC01;				typedef CSC01 RuleCSC01;							typedef CSC01 CSC01ROOT;
//class AllContentAlongDiagonal;		typedef AllContentAlongDiagonal RuleAllContentAlongDiagonal;			typedef AllContentAlongDiagonal AllContentAlongDiagonalROOT;

/** Base class for quality tests run on Monitoring Elements;

    Currently supporting the following tests:
    - Comparison to reference (Chi2, Kolmogorov)
    - Contents within [Xmin, Xmax]
    - Contents within [Ymin, Ymax]
    - Identical contents
    - Mean value within expected value
    - Check for dead or noisy channels
    - Check that mean, RMS of bins are within allowed range
    (support for 2D histograms, 1D, 2D profiles)  */

class QCriterion
{
  /// (class should be created by DQMStore class)

public:
  /// enable test
  void enable(void)
    { enabled_ = true; }

  /// disable test
  void disable(void)
    { enabled_ = false; }

  /// true if test is enabled
  bool isEnabled(void) const
    { return enabled_; }

  /// true if QCriterion has been modified since last time it ran
  bool wasModified(void) const
    { return wasModified_; }

  /// get test status (see Core/interface/DQMDefinitions.h)
  int getStatus(void) const
    { return status_; }

  /// get message attached to test
  std::string getMessage(void) const
    { return message_; }

  /// get name of quality test
  std::string getName(void) const
    { return qtname_; }

  /// get algorithm name
  std::string algoName(void) const
    { return algoName_; }

  /// set probability limit for test warning (default: 90%)
  void setWarningProb(float prob)
    { if (validProb(prob)) warningProb_ = prob; }

  /// set probability limit for test error (default: 50%)
  void setErrorProb(float prob)
    { if (validProb(prob)) errorProb_ = prob; }

  /// get vector of channels that failed test
  /// (not relevant for all quality tests!)
  virtual std::vector<DQMChannel> getBadChannels(void) const
    { return std::vector<DQMChannel>(); }

protected:
  QCriterion(std::string qtname)
    { qtname_ = qtname; init(); }

  virtual ~QCriterion(void)
    {}

  /// initialize values
  void init(void);
 
  /// set algorithm name
  void setAlgoName(std::string name)
    { algoName_ = name; }

  /// run test (result: [0, 1])
  virtual float runTest(const MonitorElement *me) = 0;

  float runTest(const MonitorElement *me, QReport &qr, DQMNet::QValue &qv)
    {

      //std::cout << "!!! ATTENTION:QCriterion - runTest 1 !!!" << std::endl;

      assert(qr.qcriterion_ == this);
      assert(qv.qtname == qtname_);


      //this runTest goes to runTest in QCriterionBase !
      float prob = runTest(me);

      //std::cout << "!!! ATTENTION:QCriterion - runTest 2 !!!" << std::endl;

      qv.code = status_ ;
      qv.message = message_;
      qv.qtname = qtname_ ;
      qv.algorithm = algoName_;
      qr.badChannels_ = getBadChannels();

      return prob;
    }

  /// call method when something in the algorithm changes
  void update(void)
    { wasModified_ = true; }

  /// make sure algorithm can run (false: should not run)
  bool check(const MonitorElement *me);

  /// true if probability value is valid
  bool validProb(float prob) const
    { return prob >= 0 && prob <= 1; }

  /// set status & message for disabled tests
  void setDisabled(void);

  /// set status & message for invalid tests
  void setInvalid(void);

  /// set status & message for tests w/o enough statistics
  void setNotEnoughStats(void);

  /// set status & message for succesfull tests
  void setOk(void);

  /// set status & message for tests w/ warnings
  void setWarning(void);

  /// set status & message for tests w/ errors
  void setError(void);

  /// set message after test has run
  virtual void setMessage(void) = 0;

  bool enabled_;  /// if true will run test
  int status_;  /// quality test status (see Core/interface/QTestStatus.h)
  std::string message_;  /// message attached to test
  std::string qtname_;  /// name of quality test
  bool wasModified_;  /// flag for indicating algorithm modifications since last time it ran
  std::string algoName_;  /// name of algorithm
  float warningProb_, errorProb_;  /// probability limits for warnings, errors
  /// test result [0, 1] ;
  /// (a) for comparison to reference :
  /// probability that histogram is consistent w/ reference
  /// (b) for "contents within range":
  /// fraction of entries that fall within allowed range
  float prob_;

private:
  /// default "probability" values for setting warnings & errors when running tests
  static const float WARNING_PROB_THRESHOLD;
  static const float ERROR_PROB_THRESHOLD;

  /// for creating and deleting class instances
  friend class DQMStore;
  /// for running the test
  friend class MonitorElement;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class QCriterionBase : public QCriterion
{
public:
  QCriterionBase(const std::string &name)
    : QCriterion(name)
    {}


protected:

  //define runMyTest function 
 virtual float runMyTest(const MonitorElement *me) = 0;
 
 /// run the test on MonitorElement <me> (result: [0, 1] or <0 for failure)
 virtual float runTest(const MonitorElement *me)
 {
      if (! check(me)) return -1;
     
      prob_ = runMyTest(me);

      setStatusMessage();
      return prob_;
 }

  /// set status & message after test has run
  void setStatusMessage(void)
    {
      if (! validProb(prob_)) setInvalid();
      else if (prob_ < errorProb_) setError();
      else if (prob_ < warningProb_) setWarning();
      else setOk();
    }

};

/// Class T must be one of the usual histogram/profile objects: THX
/// for method notEnoughStats to be used...

class SimpleTest : public QCriterionBase
{
public:
  SimpleTest(const std::string &name, bool keepBadChannels = false)
    : QCriterionBase(name),
      minEntries_ (0),
      keepBadChannels_ (keepBadChannels)
    {}

  /// set minimum # of entries needed
  void setMinimumEntries(unsigned n)
    { minEntries_ = n; this->update(); }

  /// get vector of channels that failed test (not always relevant!)
  virtual std::vector<DQMChannel> getBadChannels(void) const
    { return keepBadChannels_ ? badChannels_ : QCriterionBase::getBadChannels(); }

protected:
  virtual void setMessage(void)
    {
      std::ostringstream message;
      message << " Test " << this->qtname_ << " (" << this->algoName_
	      << "): prob = " << this->prob_;
      this->message_ = message.str();
    }

  unsigned minEntries_;  //< minimum # of entries needed
  std::vector<DQMChannel> badChannels_;
  bool keepBadChannels_;
};

//===============================================================//
//========= Classes for particular QUALITY TESTS ================//
//===============================================================//


//==================== ContentsXRange =========================//
//== Check that histogram contents are between [Xmin, Xmax] ==//

class ContentsXRange : public SimpleTest
{
public:
  ContentsXRange(const std::string &name) : SimpleTest(name)
  {
      rangeInitialized_ = false;
      setAlgoName(getAlgoName());
  }

  /// set allowed range in X-axis (default values: histogram's X-range)
  virtual void setAllowedXRange(float xmin, float xmax)
    {
      xmin_ = xmin;
      xmax_ = xmax;
      rangeInitialized_ = true;
    }

  static std::string getAlgoName(void)
  { return "ContentsXRange"; }

public:
  using SimpleTest::runTest;
  virtual float runMyTest(const MonitorElement *me) ;

protected: 
  virtual void setMessage(void)
    {
    std::ostringstream message;
    message << " Test " << qtname_ << " (" << algoName_
            << "): Entry fraction within X range = " << prob_;
    message_ = message.str();
    }

  /// allowed range in X-axis
  float xmin_;
  float xmax_;
  /// init-flag for xmin_, xmax_
  bool rangeInitialized_;
};



//===================== Comp2RefEqualH1 ===================//
//== Algorithm for comparing equality of 1D histograms ==//
class Comp2RefEqualH1 : public SimpleTest
{
public:
  Comp2RefEqualH1(const std::string &name) : SimpleTest(name)
  { setAlgoName( getAlgoName() ); }

  static std::string getAlgoName(void)
  { return "Comp2RefEqualH1"; }

public:
  using SimpleTest::runTest;
  virtual float runMyTest(const MonitorElement*me);

protected:
  /// # of bins for test & reference histograms
  Int_t ncx1;
  Int_t ncx2;
};


#endif
