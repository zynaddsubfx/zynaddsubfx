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

using namespace std;

#include "../Misc/Util.h"
#include "../Misc/Config.h"
#include "../Misc/Master.h"
#include "AlsaEngine.h"

AlsaEngine::AlsaEngine()
{
    audio.handle = NULL;
    audio.period_time = 0;
    audio.samplerate = 0;
    audio.buffer_size = 0;
    audio.period_size = 0;
    audio.buffer_size = 0;
    audio.alsaId = -1;
    audio.pThread = 0;
    
//    midi.handle = NULL;
//    midi.alsaId = -1;
//    midi.pThread = 0;

    threadStop = true;
    pthread_mutex_init(&outBuf_mutex, NULL);
    pthread_cond_init (&outBuf_cv, NULL);
    manager = sysOut;

}


bool AlsaEngine::openAudio()
{
    audio.device = "default";//config.cfg.audioDevice;
    audio.samplerate = config.cfg.SampleRate;
    audio.period_size = config.cfg.SoundBufferSize;
    audio.period_time =  audio.period_size * 1000000.0f / audio.samplerate;
    alsaBad(snd_config_update_free_global(), "failed to update snd config");
    if (alsaBad(snd_pcm_open(&audio.handle, audio.device.c_str(),
                    SND_PCM_STREAM_PLAYBACK, SND_PCM_NO_AUTO_CHANNELS),
                "failed to open alsa audio device:" + audio.device))
        goto bail_out;
    if (alsaBad(snd_pcm_nonblock(audio.handle, 0), "set blocking failed"))
        goto bail_out;
    if (!prepHwparams())
        goto bail_out;
    if (!prepSwparams())
        goto bail_out;
    //config.cfg.Samplerate = getSamplerate();
    //config.cfg.Buffersize = getBuffersize();
    return true;
bail_out:
    Close();
    return false;
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


void AlsaEngine::Close()
{
    Stop();
    if (audio.handle != NULL)
        alsaBad(snd_pcm_close(audio.handle), "close pcm failed");
    audio.handle = NULL;
//    if (NULL != midi.handle)
//        if (snd_seq_close(midi.handle) < 0)
//            cerr << "Error closing Alsa midi connection" << endl;
//    midi.handle = NULL;
}

void AlsaEngine::out(const Stereo<Sample> smps)
{
    pthread_mutex_lock(&outBuf_mutex);
    outBuf.push(smps);
    pthread_cond_signal(&outBuf_cv);
    pthread_mutex_unlock(&outBuf_mutex);
}


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


bool AlsaEngine::prepHwparams()
{
    unsigned int buffer_time = audio.period_time * 4;
    unsigned int desired_samplerate = audio.samplerate;
    snd_pcm_format_t format = SND_PCM_FORMAT_S16; // Alsa appends _LE/_BE? hmmm
    snd_pcm_access_t axs = SND_PCM_ACCESS_MMAP_INTERLEAVED;
    snd_pcm_hw_params_t  *hwparams = NULL;
    snd_pcm_hw_params_alloca(&hwparams);
    if (alsaBad(snd_pcm_hw_params_any(audio.handle, hwparams),
                "alsa audio no playback configurations available"))
        goto bail_out;
    if (alsaBad(snd_pcm_hw_params_set_periods_integer(audio.handle, hwparams),
                "alsa audio cannot restrict period size to integral value"))
        goto bail_out;
    if (!alsaBad(snd_pcm_hw_params_set_access(audio.handle, hwparams, axs),
                 "alsa audio mmap not possible"))
        pcmWrite = &snd_pcm_mmap_writei;
    else
    {
        axs = SND_PCM_ACCESS_RW_INTERLEAVED;
        if (alsaBad(snd_pcm_hw_params_set_access(audio.handle, hwparams, axs),
                     "alsa audio failed to set access, both mmap and rw failed"))
            goto bail_out;
        pcmWrite = &snd_pcm_writei;
    }
    if (alsaBad(snd_pcm_hw_params_set_format(audio.handle, hwparams, format),
                "alsa audio failed to set sample format"))
        goto bail_out;
    alsaBad(snd_pcm_hw_params_set_rate_resample(audio.handle, hwparams, 1),
            "alsa audio failed to set allow resample");
    if (alsaBad(snd_pcm_hw_params_set_rate_near(audio.handle, hwparams,
                                                &audio.samplerate, NULL),
                "alsa audio failed to set sample rate to "
                + stringFrom<int>(desired_samplerate)))
        goto bail_out;
    if (alsaBad(snd_pcm_hw_params_set_channels(audio.handle, hwparams, 2),
                "alsa audio failed to set channels to 2"))
        goto bail_out;
    if (!alsaBad(snd_pcm_hw_params_set_buffer_time_near(audio.handle, hwparams,
                 &buffer_time, NULL), "initial buffer time setting failed"))
    {
        if (alsaBad(snd_pcm_hw_params_get_buffer_size(hwparams, &audio.buffer_size),
                    "alsa audio failed to get buffer size"))
            goto bail_out;
        if (alsaBad(snd_pcm_hw_params_set_period_time_near(audio.handle, hwparams,
                    &audio.period_time, NULL), "failed to set period time"))
            goto bail_out;
        if (alsaBad(snd_pcm_hw_params_get_period_size(hwparams, &audio.period_size,
                    NULL), "alsa audio failed to get period size"))
            goto bail_out;
    }
    else
    {
        if (alsaBad(snd_pcm_hw_params_set_period_time_near(audio.handle, hwparams,
                    &audio.period_time, NULL), "failed to set period time"))
            goto bail_out;
        audio.buffer_size = audio.period_size * 4;
        if (alsaBad(snd_pcm_hw_params_set_buffer_size_near(audio.handle, hwparams,
                    &audio.buffer_size), "failed to set buffer size"))
            goto bail_out;
    }
    if (alsaBad(snd_pcm_hw_params(audio.handle, hwparams),
                "alsa audio failed to set hardware parameters"))
		goto bail_out;
    if (alsaBad(snd_pcm_hw_params_get_buffer_size(hwparams, &audio.buffer_size),
                "alsa audio failed to get buffer size"))
        goto bail_out;
    if (alsaBad(snd_pcm_hw_params_get_period_size(hwparams, &audio.period_size,
                NULL), "failed to get period size"))
        goto bail_out;
    return true;

bail_out:
    if (audio.handle != NULL)
        Close();
    return false;
}


bool AlsaEngine::prepSwparams()
{
    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
	snd_pcm_uframes_t boundary;
    if (alsaBad(snd_pcm_sw_params_current(audio.handle, swparams),
                 "alsa audio failed to get swparams"))
        goto bail_out;
    if (alsaBad(snd_pcm_sw_params_get_boundary(swparams, &boundary),
                "alsa audio failed to get boundary"))
        goto bail_out;
    if (alsaBad(snd_pcm_sw_params_set_start_threshold(audio.handle, swparams,
                                                      boundary + 1),
                "failed to set start threshold")) // explicit start, not auto start
        goto bail_out;
    if (alsaBad(snd_pcm_sw_params_set_stop_threshold(audio.handle, swparams,
                                                    boundary),
               "alsa audio failed to set stop threshold"))
        goto bail_out;
    if (alsaBad(snd_pcm_sw_params(audio.handle, swparams),
                "alsa audio failed to set software parameters"))
        goto bail_out;
    return true;

bail_out:
    return false;
}


bool AlsaEngine::alsaBad(int op_result, string err_msg)
{
    bool isbad = (op_result < 0); // (op_result < 0) -> is bad -> return true
    if (isbad)
        cerr << "Error, alsa audio: " << err_msg << ": "
             << string(snd_strerror(op_result)) << endl;
    return isbad;
}


void *AlsaEngine::_AudioThread(void *arg)
{
    return (static_cast<AlsaEngine*>(arg))->AudioThread();
}


void *AlsaEngine::AudioThread()
{  
    if (NULL == audio.handle)
    {
        cerr << "Error, null pcm handle into AlsaEngine::AudioThread" << endl;
        return NULL;
    }
    set_realtime();
    alsaBad(snd_pcm_start(audio.handle), "alsa audio pcm start failed");
    while (!threadStop)
    {
        cout << "AlsaEngine THREAD" << endl;
        const Stereo<Sample> smps = getNext();

        audio.pcm_state = snd_pcm_state(audio.handle);
        if (audio.pcm_state != SND_PCM_STATE_RUNNING)
        {
            switch (audio.pcm_state)
            {
                case SND_PCM_STATE_XRUN:
                case SND_PCM_STATE_SUSPENDED:
                    if (!xrunRecover())
                        break;
                    // else fall through to ...
                case SND_PCM_STATE_SETUP:
                    if (alsaBad(snd_pcm_prepare(audio.handle),
                                "alsa audio pcm prepare failed"))
                        break;
                case SND_PCM_STATE_PREPARED:
                    alsaBad(snd_pcm_start(audio.handle), "pcm start failed");
                    break;
                default:
                    cout << "AlsaEngine::AudioThread, weird SND_PCM_STATE: "
                         << audio.pcm_state << endl;
                    break;
            }
            audio.pcm_state = snd_pcm_state(audio.handle);
        }
        if (audio.pcm_state == SND_PCM_STATE_RUNNING)
        {
            const short *tmp = interleave(smps);
            Write(tmp);
            delete [] tmp;
        }
        else
            ;//config.cfg.verbose
             //   && cerr << "Error, audio pcm still not RUNNING" << endl;
             cerr << "Error, audio pcm still not running";
    }
    return NULL;
}


void AlsaEngine::Write(const short *InterleavedSmps)
{
    snd_pcm_uframes_t towrite = getBuffersize();
    snd_pcm_sframes_t wrote = 0;
    const short int *data = InterleavedSmps;
    while (towrite > 0)
    {
        //wrote = pcmWrite(audio.handle, &data, towrite);
        wrote = snd_pcm_writei(audio.handle, data, towrite);
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
                    alsaBad(-EBADFD, "alsa audio unfit for writing");
                    break;
                case -EPIPE:
                    xrunRecover();
                    break;
                case -ESTRPIPE:
                    Recover(wrote);
                    break;
                default:
                    alsaBad(wrote, "alsa audio, snd_pcm_writei ==> weird state");
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
            if (!alsaBad(snd_pcm_prepare(audio.handle),
                         "Error, AlsaEngine failed to recover from suspend"))
                isgood = true;
            break;
        case -EPIPE:
            if (!alsaBad(snd_pcm_prepare(audio.handle),
                         "Error, AlsaEngine failed to recover from underrun"))
                isgood = true;
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
        if (!alsaBad(snd_pcm_drop(audio.handle), "pcm drop failed"))
            if (!alsaBad(snd_pcm_prepare(audio.handle), "pcm prepare failed"))
                isgood = true;
        ;//config.cfg.verbose
         //   && cout << "Info, xrun recovery " << ((isgood) ? "good" : "not good")
         //           << endl;
    }
    return isgood;
}


bool AlsaEngine::Start(void)
{
    int chk;
    pthread_attr_t attr;
    threadStop = false;
    if (NULL != audio.handle)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&audio.pThread, &attr, _AudioThread, this);
    }

//    if (NULL != midi.handle)
//    {
//        chk = pthread_attr_init(&attr);
//        chk = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//        chk = pthread_create(&midi.pThread, &attr, _MidiThread, this);
//    }
    return true;

bail_out:
    cerr << "Error - bail out of AlsaEngine::Start()" << endl;
    Close();
    threadStop = true;
    return false;
}


void AlsaEngine::Stop(void)
{
    threadStop = true;
    
    if (NULL != audio.handle && audio.pThread)
        if (pthread_cancel(audio.pThread))
            cerr << "Error, failed to cancel Alsa audio thread" << endl;
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

const Stereo<Sample> AlsaEngine::getNext()
{
    Stereo<Sample> ans;
    if(outBuf.empty())//fetch samples if possible
    {
        if(true)//FIXME care about locking state later on
            //mgr.requestSamples()!=-1)//samples are being prepared
        {
            manager->requestSamples();
            pthread_mutex_lock(&outBuf_mutex);
            pthread_cond_wait(&outBuf_cv, &outBuf_mutex);
            ans = outBuf.front();
            outBuf.pop();
            pthread_mutex_unlock(&outBuf_mutex);
        }
    }
    else
    {
        pthread_mutex_lock(&outBuf_mutex);
        ans = outBuf.front();
        outBuf.pop();
        pthread_mutex_unlock(&outBuf_mutex);
    }
    return ans;
}


const short *AlsaEngine::interleave(const Stereo<Sample> smps)const
{
    //hm, this seems less than optimum
    //TODO remove this excessive allocation/deallocation once things are
    //integrated
    short *shortInterleaved = new short[smps.l().size()*2+1002];//over allocation
    /**\todo TODO fix overallocation*/
    int idx = 0;//possible off by one error here
    double scaled;
    for (int frame = 0; frame < smps.l().size(); ++frame)
    {   // with a nod to libsamplerate ...
        scaled = smps.l()[frame] * (8.0 * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
        scaled = smps.r()[frame] * (8.0 * 0x10000000);
        shortInterleaved[idx++] = (short int)(lrint(scaled) >> 16);
    }
}
