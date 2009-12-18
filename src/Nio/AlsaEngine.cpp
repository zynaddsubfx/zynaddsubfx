/*
    AlsaEngine.cpp

    Copyright 2009, Alan Calvert

    This file is part of ZynAddSubFX, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    ZynAddSubFX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ZynAddSubFX.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cmath>

using namespace std;

#include "../Misc/Util.h"
#include "../Misc/Config.h"
#include "../Misc/Master.h"
#include "AlsaEngine.h"

AlsaEngine::AlsaEngine(OutMgr *out)
    :AudioOut(out)
{
    audio.handle = NULL;
    audio.period_time = 0;
    audio.samplerate = 0;
    audio.buffer_size = SOUND_BUFFER_SIZE;//0;
    audio.period_size = 0;
    audio.alsaId = -1;
    audio.pThread = 0;

//    midi.handle = NULL;
//    midi.alsaId = -1;
//    midi.pThread = 0;

    pthread_mutex_init(&close_m, NULL);
    threadStop = true;
}

AlsaEngine::~AlsaEngine()
{
    Stop();
    pthread_mutex_lock(&close_m);
    pthread_mutex_unlock(&close_m);
    pthread_mutex_destroy(&close_m);
}


//bool AlsaEngine::openMidi()
//{
//    midi.device = config.cfg.midiDevice;
//    if (!midi.device.size())
//        midi.device = "default";
//    string port_name = "input";
//    if (snd_seq_open(&midi.handle, midi.device.c_str(), SND_SEQ_OPEN_INPUT, 0))
//    {
//        cerr << "Error, failed to open alsa midi device: " << midi.device << endl;
//        goto bail_out;
//    }
//    snd_seq_client_info_t *seq_info;
//    snd_seq_client_info_alloca(&seq_info);
//    snd_seq_get_client_info(midi.handle, seq_info);
//    midi.alsaId = snd_seq_client_info_get_client(seq_info);
//    snd_seq_set_client_name(midi.handle, midiClientName().c_str());
//    if (0 > snd_seq_create_simple_port(midi.handle, port_name.c_str(),
//                                       SND_SEQ_PORT_CAP_WRITE
//                                       | SND_SEQ_PORT_CAP_SUBS_WRITE,
//                                       SND_SEQ_PORT_TYPE_SYNTH))
//    {
//        cerr << "Error, failed to acquire alsa midi port" << endl;
//        goto bail_out;
//    }
//    return true;
//
//bail_out:
//    Close();
//    return false;
//}


string AlsaEngine::audioClientName()
{
    string name = "zynaddsubfx";
    if (!config.cfg.nameTag.empty())
        name += ("-" + config.cfg.nameTag);
    return name;
}

//string AlsaEngine::midiClientName()
//{
//    string name = "zynaddsubfx";
//    if (!config.cfg.nameTag.empty())
//        name += ("-" + config.cfg.nameTag);
//    return name;
//}

void *AlsaEngine::_AudioThread(void *arg)
{
    return (static_cast<AlsaEngine*>(arg))->AudioThread();
}


void *AlsaEngine::AudioThread()
{
    RunStuff();
    return NULL;
}


void AlsaEngine::Write(const short *InterleavedSmps,int size)
{
    snd_pcm_uframes_t towrite = size;//getBuffersize();
    snd_pcm_sframes_t wrote = 0;
    const short int *data = InterleavedSmps;
    while (towrite > 0)
    {
        wrote = pcmWrite(audio.handle, &data, towrite);
        if (wrote >= 0)
        {
            if ((snd_pcm_uframes_t)wrote < towrite || wrote == -EAGAIN)
                snd_pcm_wait(audio.handle, 707);
            if (wrote > 0)
            {
                towrite -= wrote;
                data += wrote * 2;
            }
        }
        else // (wrote < 0)
        {
            switch (wrote)
            {
                case -EBADFD:
                    //alsaBad(-EBADFD, "alsa audio unfit for writing");
                    break;
                case -EPIPE:
                    xrunRecover();
                    break;
                case -ESTRPIPE:
                    Recover(wrote);
                    break;
                default:
                    //alsaBad(wrote, "alsa audio, snd_pcm_writei ==> weird state");
                    break;
            }
            wrote = 0;
        }
    }
}


bool AlsaEngine::Recover(int err)
{
    if (err > 0)
        err = -err;
    bool isgood = false;
    switch (err)
    {
        case -EINTR:
            isgood = true; // nuthin to see here
            break;
        case -ESTRPIPE:
           // if (!alsaBad(snd_pcm_prepare(audio.handle),
             //            "Error, AlsaEngine failed to recover from suspend"))
            //    isgood = true;
            break;
        case -EPIPE:
           // if (!alsaBad(snd_pcm_prepare(audio.handle),
           //              "Error, AlsaEngine failed to recover from underrun"))
           //     isgood = true;
            break;
        default:
            break;
    }
    return isgood;
}


bool AlsaEngine::xrunRecover()
{
    bool isgood = false;
    if (audio.handle != NULL)
    {
        //if (!alsaBad(snd_pcm_drop(audio.handle), "pcm drop failed"))
         //   if (!alsaBad(snd_pcm_prepare(audio.handle), "pcm prepare failed"))
                isgood = true;
        ;//config.cfg.verbose
         //   && cout << "Info, xrun recovery " << ((isgood) ? "good" : "not good")
         //           << endl;
    }
    return isgood;
}


bool AlsaEngine::Start()
{
    if(enabled())
        return true;
    if(!OpenStuff())
        return false;

    pthread_attr_t attr;
    threadStop = false;
    enabled = true;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&audio.pThread, &attr, _AudioThread, this);

//    if (NULL != midi.handle)
//    {
//        chk = pthread_attr_init(&attr);
//        chk = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//        chk = pthread_create(&midi.pThread, &attr, _MidiThread, this);
//    }
    return true;

bail_out:
    cerr << "Error - bail out of AlsaEngine::Start()" << endl;
    Stop();
    threadStop = true;
    return false;
}


void AlsaEngine::Stop()
{
    if(!enabled())
        return;
    threadStop = true;
    enabled = false;

    if (NULL != audio.handle && audio.pThread)
        if (pthread_cancel(audio.pThread))
            cerr << "Error, failed to cancel Alsa audio thread" << endl;
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    //if (NULL != midi.handle && midi.pThread)
    //    if (pthread_cancel(midi.pThread))
    //        cerr << "Error, failed to cancel Alsa midi thread" << endl;
}


//void *AlsaEngine::_MidiThread(void *arg)
//{
//    return static_cast<AlsaEngine*>(arg)->MidiThread();
//}


//void *AlsaEngine::MidiThread(void)
//{
//    snd_seq_event_t *event;
//    unsigned char channel;
//    unsigned char note;
//    unsigned char velocity;
//    int ctrltype;
//    int par;
//    int chk;
//    set_realtime();
//    while (!threadStop)
//    {
//        while ((chk = snd_seq_event_input(midi.handle, &event)) > 0)
//        {
//            if (!event)
//                continue;
//            par = event->data.control.param;
//            switch (event->type)
//            {
//                case SND_SEQ_EVENT_NOTEON:
//                     if (event->data.note.note)
//                    {
//                        channel = event->data.note.channel;
//                        note = event->data.note.note;
//                        velocity = event->data.note.velocity;
//                        setMidiNote(channel, note, velocity);
//                    }
//                    break;
//
//                case SND_SEQ_EVENT_NOTEOFF:
//                    channel = event->data.note.channel;
//                    note = event->data.note.note;
//                    setMidiNote(channel, note);
//                    break;
//
//                case SND_SEQ_EVENT_PITCHBEND:
//                    channel = event->data.control.channel;
//                    ctrltype = C_pitchwheel;
//                    par = event->data.control.value;
//                    setMidiController(channel, ctrltype, par);
//                    break;
//
//                case SND_SEQ_EVENT_CONTROLLER:
//                    channel = event->data.control.channel;
//                    ctrltype = event->data.control.param;
//                    par = event->data.control.value;
//                    setMidiController(channel, ctrltype, par);
//                    break;
//
//                case SND_SEQ_EVENT_RESET: // reset to power-on state
//                    channel = event->data.control.channel;
//                    ctrltype = C_resetallcontrollers;
//                    setMidiController(channel, ctrltype, 0);
//                    break;
//
//                case SND_SEQ_EVENT_PORT_SUBSCRIBED: // ports connected
//                    if (config.cfg.verbose)
//                        cout << "Info, alsa midi port connected" << endl;
//                    break;
//
//                case SND_SEQ_EVENT_PORT_UNSUBSCRIBED: // ports disconnected
//                    if (config.cfg.verbose)
//                        cout << "Info, alsa midi port disconnected" << endl;
//                    break;
//
//                case SND_SEQ_EVENT_SYSEX:   // system exclusive
//                case SND_SEQ_EVENT_SENSING: // midi device still there
//                    break;
//
//                default:
//                    if (config.cfg.verbose)
//                        cout << "Info, other non-handled midi event, type: "
//                             << (int)event->type << endl;
//                    break;
//            }
//            snd_seq_free_event(event);
//        }
//        if (chk < 0)
//        {
//            if (config.cfg.verbose)
//                cerr << "Error, ALSA midi input read failed: " << chk << endl;
//            return NULL;
//        }
//    }
//    return NULL;
//}

const short *AlsaEngine::interleave(const Stereo<Sample> smps)const
{
    /**\todo TODO fix repeated allocation*/
    short *shortInterleaved = new short[smps.l().size()*2];//over allocation
    memset(shortInterleaved,0,smps.l().size()*2*sizeof(short));
    int idx = 0;//possible off by one error here
    double scaled;
    for (int frame = 0; frame < smps.l().size(); ++frame)
    {   // with a nod to libsamplerate ...
        scaled = smps.l()[frame] * (8.0 * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
        scaled = smps.r()[frame] * (8.0 * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
    }
    return shortInterleaved;
}

bool AlsaEngine::OpenStuff()
{
  /* Open PCM device for playback. */
    handle=NULL;
  rc = snd_pcm_open(&handle, "hw:0",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    return false;
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  val = SAMPLE_RATE; //44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, NULL);//&dir);

  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, NULL);//&dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    return false;
  }

  /* Set buffer size (in frames). The resulting latency is given by */
  /* latency = periodsize * periods / (rate * bytes_per_frame)     */
  snd_pcm_hw_params_set_buffer_size(handle, params, SOUND_BUFFER_SIZE);

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);

  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  return true;
}

void AlsaEngine::RunStuff()
{
    pthread_mutex_lock(&close_m);
    while (!threadStop()) {
        buffer = interleave(getNext());
        rc = snd_pcm_writei(handle, buffer, SOUND_BUFFER_SIZE);
        delete[] buffer;
        if (rc == -EPIPE) {
            /* EPIPE means underrun */
            cerr << "underrun occurred" << endl;
            snd_pcm_prepare(handle);
        }
        else if (rc < 0)
            cerr << "error from writei: " << snd_strerror(rc) << endl;
        //else if (rc != (int)frames)
        //    cerr << "short write, write " << rc << "frames" << endl;
    }
    pthread_mutex_unlock(&close_m);
}
