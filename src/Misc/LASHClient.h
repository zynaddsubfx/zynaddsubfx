#ifndef LASHClient_h
#define LASHClient_h

#include <string>
#include <pthread.h>
#include <lash/lash.h>


/** This class wraps up some functions for initialising and polling
 *  the LASH daemon.
 *  \todo fix indentation nonconformism
 *  \todo see why there is no destructor*/
class LASHClient {
 public:
  /**Enum to represent the LASH events that are currently handled*/
  enum Event {
    Save,
    Restore,
    Quit,
    NoEvent
  };

  /** Constructor
   *  @param argc number of arguments
   *  @param argv the text arguments*/
  LASHClient(int* argc, char*** argv);
  
  /**set the ALSA id
   * @param id new ALSA id*/
  void setalsaid(int id);
  /**Set the JACK name
   * @param name the new name*/
  void setjackname(const char* name);
  Event checkevents(std::string& filename);
  void confirmevent(Event event);
  
 private:
  
  lash_client_t* client;
};


#endif

