//____________________________________________________________________________..
//
// This is a template for a Fun4All SubsysReco module with all methods from the
// $OFFLINE_MAIN/include/fun4all/SubsysReco.h baseclass
// You do not have to implement all of them, you can just remove unused methods
// here and in CaloAlignment.h.
//

#include <iostream>
#include <iomanip>

#include "CaloAlignment.h"

#include <fstream>

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>

#include <fun4all/Fun4AllServer.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/PHNode.h>                               // for PHNode
#include <phool/PHNodeIterator.h>
#include <phool/PHObject.h>                             // for PHObject
#include <phool/getClass.h>

#include <TH1.h>
#include <TH2.h>


#include <fun4all/Fun4AllHistoManager.h>

#include <Event/Event.h>
#include <Event/EventTypes.h>
#include <Event/packet.h>

using namespace std;

#define coutfl std::cout << __FILE__<< "  " << __LINE__ << " "
#define cerrfl std::cerr << __FILE__<< "  " << __LINE__ << " "


//____________________________________________________________________________..
CaloAlignment::CaloAlignment(const std::string &name,  const std::string &filelist)
  : SubsysReco(name)
{

  hm = new Fun4AllHistoManager(name);
  Fun4AllServer *se = Fun4AllServer::instance();
  se->registerHistoManager(hm);

  h1 =   new TH1F ( "h1","Packets per event", 70, -0.5, 69.5);  
  h1->GetXaxis()->SetTitle("Number of packets");
  //h1->GetXaxis()->SetTitleOffset(1.7);

  h1->GetYaxis()->SetTitle("Count");
  //h1->GetYaxis()->SetTitleOffset(1.7);

  h2_packet_vs_event = new TH2F ( "h2_packet_vs_event","Packets found vs event nr", 200, 0, 20000, 20, -0.5, 19.5);  
  h2_packet_vs_event->GetXaxis()->SetTitle("Event Nr");
  h2_packet_vs_event->GetYaxis()->SetTitle("Packets found");

  //  _combined_filename = combined;
  _file_list = filelist;

  //  std::cout << "CaloAlignment::CaloAlignment(const std::string &name) Calling ctor" << std::endl;

  se = Fun4AllServer::instance();

  _min_depth=10;

  oph = new oEvent(workmem,4*1024*1024, 1,1,1);
  _TheRunNumber = 0;
}

//____________________________________________________________________________..
CaloAlignment::~CaloAlignment()
{
  std::cout << "CaloAlignment::~CaloAlignment() Calling dtor" << std::endl;
}

//____________________________________________________________________________..
int CaloAlignment::Init(PHCompositeNode *topNode)
{
  // std::cout << "CaloAlignment::Init(PHCompositeNode *topNode) Initializing" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int CaloAlignment::InitRun(PHCompositeNode *topNode)
{
  //  std::cout << "CaloAlignment::InitRun(PHCompositeNode *topNode) Initializing for Run XXX" << std::endl;

  h1->Reset();

  current_evtnr = 0;

  int status;

  ifstream ifs(_file_list);
  if (! ifs.is_open() ) //checking whether the file is open
    {
      cerr << "Could not open list file " << _file_list << endl;
      return Fun4AllReturnCodes::ABORTRUN;
    }

  string line;

  while ( getline ( ifs, line) )
    {
      if ( line.empty() ) continue;  // we ignore empty lines
      unsigned int found= line.find_first_not_of(" \t");  // we take care of lines beginning with #.
      if( found != string::npos)                 // we tolerate whitespace before #
        {
          if( line[found] == '#')         continue;
        }
      if ( Verbosity() >= VERBOSITY_MORE)
	{
	  coutfl  << "opening " << line << endl;
	}
      Eventiterator *E  = new fileEventiterator ( line.c_str(), status);

      if (status) 
	{
	  delete E;
	  coutfl  << " cannot open " << line << std::endl;
	  return Fun4AllReturnCodes::ABORTRUN;
	}
      _event_itr_list.push_back(E);

    }
  
  coutfl  << "init_run succeeded" << endl;

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int CaloAlignment::process_event(PHCompositeNode *topNode)
{
  //coutfl << "CaloAlignment::process_event(PHCompositeNode *topNode) Processing Event" << std::endl;

  // we get one event each from both streams

  std::vector<Event *> event_list; 

  int all_received = 1; // a bit crude logic, but ok

  auto itr = _event_itr_list.begin();

  for ( ; itr != _event_itr_list.end(); ++itr)
    {
      //      (*itr)->identify();

      Event *e = (*itr)->getNextEvent();
      //if (e) e->identify();
      if ( ! e) 
	{
	  all_received =0;  // stream exhausted
	  break;
	}
      _TheRunNumber = e->getRunNumber();
      event_list.push_back(e);
    }

  if ( Verbosity() >= VERBOSITY_MORE)
    {
      coutfl << "all_received = " << all_received << endl;
    }

  // if not all are in, cleanup and stop
  if ( ! all_received) 
    {
      auto eitr = event_list.begin();
      for ( ; eitr != event_list.end(); ++eitr)
	{
	  delete (*eitr);
	}
      event_list.clear();
      return Fun4AllReturnCodes::ABORTRUN;
    }

  // here we have all events from all files
     
  auto eitr = event_list.begin();
  for ( ; eitr != event_list.end(); ++eitr)
    {
      if ( Verbosity() >= VERBOSITY_MORE)
	{
	  // let's see what we got here
	  (*eitr)->identify();
	}

      if ( (*eitr)->getEvtType() <= 7)
	{
	  addPackets  ( (*eitr), _min_depth);
	}
      delete (*eitr);

    }
  event_list.clear();  // we ae done with the events - they are already deleted
  
  if ( Verbosity() >= VERBOSITY_MORE)
    {
      coutfl << "min depth is " << getMinDepth() << endl;
    }

  if ( getMinDepth() >= _min_depth)
    {
      int status = process();
      if (status) return status;
    }

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int CaloAlignment::addPackets(Event *e, unsigned int &min_depth)
{

  // I know we have only 8 packets, so 256 is plenty
  Packet *packetList[256];
  int np;
  int i;


  // get a vector with all Packet * pointers in one fell swoop
  // np says how many we got
  np  = e->getPacketList(packetList, 256);

  // // just some feel-good print
  // if (np &&  Verbosity() >= VERBOSITY_MORE)
  //   {
  //      cout << " packet evt nr " << packetList[0]->iValue(0,"EVTNR") 
  // 	    << " clock  " << hex << packetList[0]->iValue(0,"CLOCK") << dec << endl; 
  //   }

  for ( i = 0; i < np; i++)
    {

      // we need to convert since we have most likely gotten "light" packets
      // Light packets just leave the raw data in the raw data buffer.
      // Good if you just process one event after the other, but since we keep the
      // packets objects around, we make them swallow their raw data into their own
      // memory.
      packetList[i]->convert();

      if ( Verbosity() >= VERBOSITY_MORE)
	{
	  packetList[i]->identify();
	}
      
      // ok, now lets find the place where the packet goes
      int p_id = packetList[i]->getIdentifier(); // saves some typing
      auto itr = packet_pool.find(p_id);
      
      // accept only packets with ok checksums 
      if ( packetList[i]->iValue(0,"EVENCHECKSUMOK") != 0 && packetList[i]->iValue(0,"ODDCHECKSUMOK") != 0)
	{
	  //now the packets go into a std::set, one per packet id. 
	  //If we have never seen this packet id we make the set here
	  if ( itr == packet_pool.end() )
	    {
	      std::set<Packet *, SortByEvtNr> x;
	      packet_pool[p_id] = x;
	    }
	  //packetList[i]->identify();
	  packet_pool[p_id].insert ( packetList[i] );
	  if ( Verbosity() >= VERBOSITY_EVEN_MORE)
	    {
	      coutfl <<  "adding packet " << p_id  << endl;
	    }
	}
      else
	{
	  if ( Verbosity() >= VERBOSITY_MORE)
	    {
	      cout << "**** wrong checksum  ";
	      packetList[i]->identify();
	    }
	}
      //cout << __FILE__ << " " << __LINE__ << " size of " << p_id << " is "  << packet_pool[p_id].size() << endl;

      //  if (  packet_pool[p_id].size() < min_depth ) min_depth = packet_pool[p_id].size();
      
    }


  return 0;
}

//____________________________________________________________________________..
int CaloAlignment::process()
{


  while ( getMinDepth() >= _min_depth ) 
    {

      oph->prepare_next(current_evtnr, _TheRunNumber);
       
       std::vector<Packet *> aligned_packets;

      auto itr = packet_pool.begin();
      for ( ; itr != packet_pool.end(); ++itr)
	{
	  auto p_itr = itr->second.begin();

	  if ( p_itr != itr->second.end())
	    {

	      // if ( Verbosity() >= VERBOSITY_SOME)
	      // 	{
	      // 	  cout << " packet evt nr " << (*p_itr)->iValue(0,"EVTNR") << "  "
	      // 	       << hex << (*p_itr)->iValue(0,"CLOCK") << dec <<"   "; 
	      // 	  (*p_itr)->identify();
	      // 	}

	      if (  (*p_itr)->iValue(0,"EVTNR") ==   current_evtnr )
		{

		  oph->addPacket((*p_itr));

		  if ( Verbosity() >= VERBOSITY_SOME)
		    {
		      cout << " packet " << (*p_itr)->getIdentifier()  
			   <<  " evt nr " << (*p_itr)->iValue(0,"EVTNR") 
			   << "  " << (*p_itr)->iValue(0,"CLOCK")  << " |  " 
			   << "  " << (*p_itr)->iValue(0,"FEMCLOCK")
			   << "  " << (*p_itr)->iValue(1,"FEMCLOCK")
			   << "  " << (*p_itr)->iValue(2,"FEMCLOCK")
			   << endl; 
		    }
		  aligned_packets.push_back(*p_itr);
		  itr->second.erase(p_itr);
		}
	      else if ( (*p_itr)->iValue(0,"EVTNR") <  current_evtnr ) // clean up stuff that's done
		{
		  itr->second.erase(p_itr);
		}
	    }

	}

      Event * E = new A_Event(workmem);
      if ( Verbosity() >= VERBOSITY_MORE)
	{
	  E->identify();
	}

      h1->Fill(aligned_packets.size());

      if ( Verbosity() >= VERBOSITY_MORE)
	{
	  cout << " found " << aligned_packets.size() << " packets for event nr " << setw(5) << current_evtnr << " depth= " << getMinDepth() << endl;
	  auto it = aligned_packets.begin();
	  for ( ; it != aligned_packets.end(); ++it)
	    {
	      coutfl << "evt nr, clock: " << (*it)->iValue(0, "EVTNR") << " " << (*it)->iValue(0, "CLOCK") << endl;
	    }
	  
	}

      h2_packet_vs_event->Fill(current_evtnr, aligned_packets.size() );

      // this is the place where we call the analysis

      int status = Analysis(E);
      delete E;

      auto it = aligned_packets.begin();
      for ( ; it != aligned_packets.end(); ++it)
	{
	  if ( *it) delete (*it);
	}

      current_evtnr++;
      if (status) 
	{
	  cout << "trigger!" << endl;
	  //return status;
	}
    }
  return 0;
}

//____________________________________________________________________________..
int CaloAlignment::Analysis(Event *E)
{

  Packet *packetList[256];
  int np;
  int i;
  
  double baseline_low  = 0.;	


  // get a vector with all Packet * pointers in one fell swoop
  // np says how many we got
  np  = E->getPacketList(packetList, 256);

  for ( i = 0; i < np; i++)
    {
      Packet *p = packetList[i];

      if ( Verbosity() >= VERBOSITY_SOME)
	{
	  packetList[i]->identify();
	}

      for ( int c = 0; c <  p->iValue(0,"CHANNELS"); c++)
	{
	  // calculate the baseline. Bcause we don't know  where a pulse is, 
	  // we calculate  at the low and high sample range  
	  double baseline_high = 0.;	
	  //	  double baseline = 0.;	
	  for ( int s = 0;  s< 3; s++)
	    {
	      baseline_low += p->iValue(s,c);
	    }
	  baseline_low /= 3.;

	  for ( int s = p->iValue(0,"SAMPLES") -3;  s<p->iValue(0,"CHANNELS") ; s++)
	    {
	      baseline_high += p->iValue(s,c);
	    }
	  baseline_high /= 3.;

	  // baseline = baseline_high;
	  // if ( baseline_low < baseline_high) baseline = baseline_low;


	  // cout << "baselines: " << baseline_low <<" " <<  baseline_high << " " <<  baseline << endl;
	  // for ( int s = 0;  s< p->iValue(0,"SAMPLES"); s++)
	  //   {
	  //     double sample = p->iValue(s,c) - baseline; 

	  //   }

	}

    }

  return 0;

}

//____________________________________________________________________________..
int CaloAlignment::ResetEvent(PHCompositeNode *topNode)
{
  //  std::cout << "CaloAlignment::ResetEvent(PHCompositeNode *topNode) Resetting internal structures, prepare for next event" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}


//____________________________________________________________________________..
unsigned int CaloAlignment::getMinDepth() const
{
  // calculates the lowest depth of any of the packet vectors

  unsigned int min_depth = 50; // some impossible large number

  auto itr = packet_pool.begin();
  if ( itr != packet_pool.end() ) 
    {
      //coutfl << "first depth = " << itr->second.size() << endl;
      min_depth = itr->second.size();
    }
  else return 0;

  for ( ; itr != packet_pool.end(); ++itr)
    {
      //      coutfl << "pool depth for packet " << itr->first << "  " << itr->second.size() << endl;
      if (  itr->second.size() < min_depth ) min_depth = itr->second.size();
    }
  //  coutfl << "Returning mid_depth " << min_depth<< endl;
  return min_depth;
} 

//____________________________________________________________________________..
int CaloAlignment::EndRun(const int runnumber)
{
  //  std::cout << "CaloAlignment::EndRun(const int runnumber) Ending Run for Run " << runnumber << std::endl;


  auto itr = _event_itr_list.begin();
  for ( ; itr != _event_itr_list.end(); ++itr)
    {
      delete (*itr);
    }
  _event_itr_list.clear();

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int CaloAlignment::End(PHCompositeNode *topNode)
{
  //  std::cout << "CaloAlignment::End(PHCompositeNode *topNode) This is the End..." << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int CaloAlignment::Reset(PHCompositeNode *topNode)
{
 std::cout << "CaloAlignment::Reset(PHCompositeNode *topNode) being Reset" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void CaloAlignment::Print(const std::string &what) const
{

  // here we add a spy probe to give some info about our various containers

  auto itr = packet_pool.begin();

  for ( ; itr != packet_pool.end(); ++itr)
    {
      cout << "pool depth: Packet " << itr->first << setw(4) << "  depth " << itr->second.size();
      auto p_itr = itr->second.begin();
      if ( p_itr != itr->second.end() ) cout << " 1st evt nr  " << (*p_itr)->iValue(0,"EVTNR") << " " <<  (*p_itr)->iValue(0,"CLOCK");
      //if ( p_itr != itr->second.end() ) (*p_itr)->identify();
      cout << endl;

    }

  //std::cout << "CaloAlignment::Print(const std::string &what) const Printing info for " << what << std::endl;
}



int CaloAlignment::fillHist(std::vector<Packet *> & aligned_packets)
{

  auto it = aligned_packets.begin();
  for ( ; it != aligned_packets.end(); ++it)
    {
      Packet *p = *it;  // easier to remember what is what
      if ( p->getIdentifier() < 8000) continue; // only Outer HCal for now
      
      for ( int slice = 0; slice < 4; slice ++) // the 4 slices of a sector
	{
	  int ch = slice *48;
	  
          for  ( int c  = 0; c  < 48; c ++)   // we go through 48 channels
            {
              
	      int ccc = c + ch;

              double baseline = 0.;           // calculate the baseline from the last 3 bins
              for ( int s = p->iValue(0,"SAMPLES")-3;  s <p->iValue(0,"SAMPLES"); s++)
                {
                  baseline += p->iValue(s,ccc);
                }
              baseline /= 3.;

              
              double signal = 0;               // we integrate the other samples              
              for ( int s = 0;  s< p->iValue(0,"SAMPLES") -3; s++)
                {
                  signal += ( p->iValue(s,ccc) - baseline);
                }
              
            }
          
        }
    }
  return 0;
}

