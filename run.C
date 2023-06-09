#include <fun4all/Fun4AllServer.h>
#include "CaloAlignment.h"

R__LOAD_LIBRARY($OFFLINE_MAIN/lib/libfun4all.so)
R__LOAD_LIBRARY(/phenix/u/purschke/analysis/f4a/CaloAlignment/install/lib/libCaloAlignment.so)

CaloAlignment * s;
int run()
{
//  .x $HOME/signal.C

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(0);
  s = new CaloAlignment("cosmics", "x.list"); 
  s->Verbosity(1);
  se->registerSubsystem(s);
  // se->Print();
  //  se->run(2);
  return 0;
}

