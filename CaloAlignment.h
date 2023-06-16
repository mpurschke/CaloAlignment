// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef HCAL_COSMICS_H
#define HCAL_COSMICS_H

#include <fun4all/SubsysReco.h>

#include <vector>
#include <string>
#include <set>
#include <tuple>

#include <math.h>

#include <fun4all/Fun4AllServer.h>
#include <Event/fileEventiterator.h>


#include <Event/ospEvent.h>
#include <Event/EventTypes.h>
#include <Event/oEvent.h>
#include <Event/A_Event.h>

class Fun4AllHistoManager;
class TH1F;
class TH2F;

class PHCompositeNode;

class CaloAlignment : public SubsysReco
{
 public:

  CaloAlignment(const std::string &name
	       , const std::string &filelist
	       );
    
  ~CaloAlignment() override;

  /** Called during initialization.
      Typically this is where you can book histograms, and e.g.
      register them to Fun4AllServer (so they can be output to file
      using Fun4AllServer::dumpHistos() method).
   */
  int Init(PHCompositeNode *topNode) override;

  /** Called for first event when run number is known.
      Typically this is where you may want to fetch data from
      database, because you know the run number. A place
      to book histograms which have to know the run number.
   */
  int InitRun(PHCompositeNode *topNode) override;

  /** Called for each event.
      This is where you do the real work.
   */
  int process_event(PHCompositeNode *topNode) override;

  /// Clean up internals after each event.
  int ResetEvent(PHCompositeNode *topNode) override;

  /// Called at the end of each run.
  int EndRun(const int runnumber) override;

  /// Called at the end of all processing.
  int End(PHCompositeNode *topNode) override;

  /// Reset
  int Reset(PHCompositeNode * /*topNode*/) override;

  void Print(const std::string &what = "ALL") const override;
  int process();
  int set_min_depth( const unsigned int m) { _min_depth = m; return 0;};

 private:

  int addPackets(Event *, unsigned int &);
  unsigned int getMinDepth() const;

  //  int Analysis(std::vector<Packet *> &);
  int Analysis( Event *E);
  int fillHist(std::vector<Packet *> &);

  Fun4AllServer *se;

  int current_evtnr;

  unsigned int _min_depth;

  //  std::string _combined_filename;
  std::string _file_list;

  std::vector<Eventiterator *> _event_itr_list;


  struct SortByEvtNr
  {
    bool operator ()(Packet *lhs, Packet *rhs) const
    {
      /* std::cout << " lhs " << lhs << " rhs " << rhs << std::endl; */
      /* lhs->identify(); */
      /* rhs->identify(); */
      return lhs->iValue(0,"EVTNR") < rhs->iValue(0,"EVTNR");
    }
  };

  std::map<int, std::set<Packet *, SortByEvtNr> > packet_pool;
  PHDWORD workmem[4*1024*1024];

  Fun4AllHistoManager* hm;
  TH1F * h1;
  TH2F * h2_packet_vs_event;
  oEvent *oph;
  ospEvent *osp;
  int _TheRunNumber;
};

#endif // HCAL_COSMICS_H
