/*
  ZynAddSubFX - a software synthesizer
 
  JACKaudiooutput.C - Audio output for JACK
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License 
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <stdlib.h>
#include "JACKaudiooutput.h"

Master *jackmaster;
jack_client_t *jackclient;
jack_port_t *outport_left,*outport_right;

int jackprocess(jack_nframes_t nframes,void *arg);
int jacksrate(jack_nframes_t nframes,void *arg);
void jackshutdown(void *arg);

bool JACKaudiooutputinit(Master *master_){
    jackmaster=master_;
    jackclient=0;
    char tmpstr[100];

    for (int i=0;i<15;i++){
	if (i!=0) snprintf(tmpstr,100,"ZynAddSubFX_%d",i);
	    else snprintf(tmpstr,100,"ZynAddSubFX");
	jackclient=jack_client_new(tmpstr);
	if (jackclient!=0) break;
    };

    if (jackclient==0) {
	fprintf(stderr,"\nERROR: Cannot make a jack client (possible reasons: JACK server is not running or jackd is launched by root and zynaddsubfx by another user.).\n");
	return(false);
    };

    fprintf(stderr,"Internal SampleRate   = %d\nJack Output SampleRate= %d\n",SAMPLE_RATE,jack_get_sample_rate(jackclient));
    if ((unsigned int)jack_get_sample_rate(jackclient)!=(unsigned int) SAMPLE_RATE) 
	fprintf(stderr,"It is recomanded that the both samplerates to be equal.\n");
    
    jack_set_process_callback(jackclient,jackprocess,0);    
    jack_set_sample_rate_callback(jackclient,jacksrate,0);    
    jack_on_shutdown(jackclient,jackshutdown,0);    
    
    outport_left=jack_port_register(jackclient,"out_1",
	JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);    
    outport_right=jack_port_register(jackclient,"out_2",
	JACK_DEFAULT_AUDIO_TYPE,JackPortIsOutput|JackPortIsTerminal,0);    

    if (jack_activate(jackclient)){
	fprintf(stderr,"Cannot activate jack client\n");
	return(false);
    };
    
    /*
    jack_connect(jackclient,jack_port_name(outport_left),"alsa_pcm:out_1");
    jack_connect(jackclient,jack_port_name(outport_right),"alsa_pcm:out_2");
     */
     return(true);
};

int jackprocess(jack_nframes_t nframes,void *arg){
    jack_default_audio_sample_t *outl=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_left, nframes);
    jack_default_audio_sample_t *outr=(jack_default_audio_sample_t *) jack_port_get_buffer (outport_right, nframes);

    pthread_mutex_lock(&jackmaster->mutex);
    jackmaster->GetAudioOutSamples(nframes,jack_get_sample_rate(jackclient),outl,outr);
    pthread_mutex_unlock(&jackmaster->mutex);
    
    return(0);
};

void JACKfinish(){
    jack_client_close(jackclient);
};

int jacksrate(jack_nframes_t nframes,void *arg){
    
    return(0);
};

void jackshutdown(void *arg){
};



