#ifndef LASHClient_h
#define LASHClient_h

#include <string>
#include <pthread.h>
#include <lash/lash.h>


/** This class wraps up some functions for initialising and polling
    the LASH daemon. */
class LASHClient {
 public:
  
  enum Event {
    Save,
    Restore,
    Quit,
    NoEvent
  };
  
  LASHClient(int* argc, char*** argv);
  
  void setalsaid(int id);
  void setjackname(const char* name);
  Event checkevents(std::string& filename);
  void confirmevent(Event event);
  
 private:
  
  lash_client_t* client;
};


#endif

