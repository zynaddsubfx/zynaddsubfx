/*
    JackEngine.cpp

    Copyright 2009, Alan Calvert

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

//#include <jack/midiport.h>
#include <fcntl.h>
#include <sys/stat.h>


//#include "../Misc/Config.h"
#include "../Misc/Master.h"
#include "JackEngine.h"

using namespace std;

JackEngine::JackEngine(OutMgr *out)
    :AudioOut(out), jackClient(NULL)
{
    audio.jackSamplerate = 0;
    audio.jackNframes = 0;
    for (int i = 0; i < 2; ++i)
    {
        audio.ports[i] = NULL;
        audio.portBuffs[i] = NULL;
    }
}

bool JackEngine::connectServer(string server)
{
    cout << "Bla" << endl;
    //temporary (move to a higher level configuation)
    bool autostart_jack = true;


    if (NULL == jackClient) // ie, not already connected
    {
        string clientname = "zynaddsubfx";
        jack_status_t jackstatus;
        bool use_server_name = server.size() && server.compare("default") != 0;
        jack_options_t jopts = (jack_options_t)
            (((use_server_name) ? JackServerName : JackNullOption)
            | ((autostart_jack) ? JackNullOption : JackNoStartServer));
        if (use_server_name)
            jackClient = jack_client_open(clientname.c_str(), jopts, &jackstatus,
                    server.c_str());
        else
            jackClient = jack_client_open(clientname.c_str(), jopts, &jackstatus);
        if (NULL != jackClient)
            return true;
        else
            cerr << "Error, failed to open jack client on server: " << server
                 << " status " << jackstatus << endl;
        return false;
    }
    return true;
}

bool JackEngine::Start()
{
    if(enabled())
        return true;
    enabled = true;
    if(!connectServer(""))
        return false;
    openAudio();
    if (NULL != jackClient)
    {
        int chk;
        jack_set_error_function(_errorCallback);
        jack_set_info_function(_infoCallback);
        if ((chk = jack_set_xrun_callback(jackClient, _xrunCallback, this)))
            cerr << "Error setting jack xrun callback" << endl;
        if (jack_set_process_callback(jackClient, _processCallback, this))
        {
            cerr << "Error, JackEngine failed to set process callback" << endl;
            goto bail_out;
        }
        if (jack_activate(jackClient))
        {
            cerr << "Error, failed to activate jack client" << endl;;
            goto bail_out;
        }

        return true;
    }
    else
        cerr << "Error, NULL jackClient through Start()" << endl;
bail_out:
    Stop();
    return false;
}

void JackEngine::Stop()
{
    enabled = false;
    if (jackClient)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (NULL != audio.ports[i])
                jack_port_unregister(jackClient, audio.ports[i]);
            audio.ports[i] = NULL;
        }
        jack_client_close(jackClient);
        jackClient = NULL;
    }
}

bool JackEngine::openAudio()
{
    const char *portnames[] = { "left", "right" };
    for (int port = 0; port < 2; ++port)
    {
        audio.ports[port] = jack_port_register(jackClient, portnames[port],
                                              JACK_DEFAULT_AUDIO_TYPE,
                                              JackPortIsOutput, 0);
    }
    if (NULL != audio.ports[0] && NULL != audio.ports[1])
    {
        audio.jackSamplerate = jack_get_sample_rate(jackClient);
        audio.jackNframes = jack_get_buffer_size(jackClient);
        return true;
    }
    else
        cerr << "Error, failed to register jack audio ports" << endl;
    Stop();
    return false;
}

int JackEngine::clientId()
{
    if (NULL != jackClient)
        return jack_client_thread_id(jackClient);
    else
        return -1;
}

string JackEngine::clientName()
{
    if (NULL != jackClient)
        return string(jack_get_client_name(jackClient));
    else
        cerr << "Error, clientName() with null jackClient" << endl;
    return string("Oh, yoshimi :-(");
}

int JackEngine::_processCallback(jack_nframes_t nframes, void *arg)
{
    return static_cast<JackEngine*>(arg)->processCallback(nframes);
}

int JackEngine::processCallback(jack_nframes_t nframes)
{
    bool okaudio = true;

    if (NULL != audio.ports[0] && NULL != audio.ports[1])
        okaudio = processAudio(nframes);
    return okaudio ? 0 : -1;
}

bool JackEngine::processAudio(jack_nframes_t nframes)
{
    //cout << "I got called with: " << nframes << endl;
    for (int port = 0; port < 2; ++port)
    {
        audio.portBuffs[port] =
            (jsample_t*)jack_port_get_buffer(audio.ports[port], nframes);
        if (NULL == audio.portBuffs[port])
        {
            cerr << "Error, failed to get jack audio port buffer: "
                << port << endl;
            return false;
        }
    }
    
    Stereo<Sample> smp = getNext(nframes);
    //cout << "smp size of: " << smp.l().size() << endl;
    memcpy(audio.portBuffs[0], smp.l().c_buf(), smp.l().size()*sizeof(REALTYPE));
    memcpy(audio.portBuffs[1], smp.r().c_buf(), smp.r().size()*sizeof(REALTYPE));
    return true;

}

int JackEngine::_xrunCallback(void *arg)
{
    cerr << "Jack reports xrun" << endl;
    return 0;
}

void JackEngine::_errorCallback(const char *msg)
{
    cerr << "Jack reports error: " << msg << endl;
}

void JackEngine::_infoCallback(const char *msg)
{
    cerr << "Jack info message: " << msg << endl;
}
