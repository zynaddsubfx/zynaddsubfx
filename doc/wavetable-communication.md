# Wavetable communication

This explains how RT and non-RT threads communicate when new waves for the
wavetable need to be generated.

## Wavetable layout

A Tensor3 (3D-Tensor) has the following contents:

```
  ooooooo 1D-Tensor (frequeny values in Hz)      --
                                                  |
  ooooooo 1D-Tensor (semantic values, e.g.        | "wavetable scales"
                     random seeds or              |
                     basewave function params)   --

  ooooooo 1D-Tensor (float buffer = 1 wave)      --
  ^                                               |
  |                                               | "wavetable data"
  o...ooo 2D Tensor (Tensor1s for each freq)      | or just
  ^       with ringbuffer semantics (see below)   | "waves"
  |       (MW produces, AD note consumes)         |
  |                                               |
  o...ooo 3D Tensor (Tensor2s for each semantic) --

```

Ringbuffer layout:

```
  no required space holder (r=w is unambiguous internally)             wrap
         |                                                               |
         v               |--reserved for write--|                        v
  ...----|--can be read--|                      |--can be reserved next...
  ...oooooooo............oooo...................oooo......................
         |               |                      |
         r               w_delayed              w              <- pointers
```

Wavetable layout:

```
 o scales for ringbuffer (Tensor1, semantics + freqs)
 o ringbuffer (Tensor3, wave data)

 o timestamp of params for current ringbuffer
 o timestamp of params for next ("to be filled") ringbuffer

 o wavetable mode (e.g. random seeds, basefunc parameter, ...)
```

ADnote contains two wavetables per OscilGen:

- current wavetable (in used, can be refilled with new waves)
- next wavetable (under construction after parameter change)

## Asynchonicity principles

MiddleWare and ADnoteParams are run by 2 different threads at the same
time. This can cause issues, like outdated requests. To avoid this, the
following principles have been set up:

* wavetable requests induced by parameter changes always carry a timestamp
* any side should not rely on the other side handling the asynchonicity
  "well", i.e. it can not be relied on the other side detectig messages
  with outdated timestamps

## Sequence diagram

The following (not necessarily UML standard) diagram explains
how and when new wavetables are being generated. There are two
entry points, marked with an "X".

```
    MW (non-RT)                                                 ADnote (RT)
    |                                                           |
    |   wavetable-relevant params changed                       |
    |<---X                                                      |
    |                                                           |
    |  If "wavetable-params-changed" is not suppressed:         |
    |--/path/to/advoice/wavetable-params-changed:Ti:Fi--------->|
    |      Inform ADnote that data relevant for wavetable       |
    |      creation has changed                                 |
    |      - path: osc or mod-osc? (T/F)                        |
    |      - unique timestamp of parameter change (i)           |
    |                                                           |
    - suppress further "wavetable-params-changed"               |
    |                                                           |
    |                          - mark current ringbuffer        |
    |                            "outdated until                |
    |                            waves for timestamp arrive"    |
    |                                                           |
    |                            Master periodically checks if  |
    |                            ADnote still has enough        |
    |                            wavetables (ringbuffer read    |
    |                            space) or if waves require a   |
    |                            complete update (due to paste  |
    |                            or loading)               X--->|
    |                                                           |
    |                          - increase ringbuffer's reserved |
    |                            write space (by increading `w`)|
    |                                                           |
    | If the wavetable request comes from a parameter change,   |
    | or the current wavetable is not outdated                  |
    |<---request-wavetable                                      |
    |    :sTFiii:iiiTFiii:sFTiii:sFFiii:iiiFTiii:iiiFFiii-------|
    |      Inform MW that new waves can be generated            | 
    |      - path of OscilGen (s or iii is voice path, T/F is   |
    |        OscilGen path)                                     |
    |      - Wavetable modulation boolean (T/F)                 |
    |      - In case of parameter change:                       |
    |          parameter change timestamp                       |
    |        else                                               |
    |          0 (parameter change timestamp is implicitly the  |
    |             one of the latest parameter change which      |
    |             ADnote observed)                              |
    |      - external modulator index (or -1 if none)           |
    |      - Presonance boolean (i)                             |
    |      - if triggered by waves consumed:                    |
    |        quadrupel (semantic + freq indices,                |
    |        semantic + freq values, iiif or iiff each)         |
    |        for the waves that need to be regenerated          |
    |                                                           |
    - stop suppressing further "wavetable-params-changed"       |
    - store wavetable request in queue                          |
    - handle wavetable requests in next cycle                   |
      to generate waves (and scales, if parameters changed)     |
    |                                                           |
    |      If MW has not observed any parameter change after    |
    |      the parameter change time of this request,           |
    |      and this request comes from parameter change         |
    |------/path/to/ad/voice/set-wavetable:Tb:Fb--------------->|
    |      pass new WaveTable struct to ADnoteParams            |
    |      - path: osc or mod-osc? (T/F)                        |
    |      - WaveTable for given semantic (b)                   |
    |                                                           |
    |                          - swap passed with next          |
    |                            WaveTable                      |
    |                                                           |
    |<-----free:sb----------------------------------------------|
    |      recycle the WaveTable                                |
    |                                                           |
    - free the WaveTable (includes freeing all                  |
      contained sub-Tensors)                                    |
    |                                                           |
    |      If MW has not observed any parameter change after    |
    |      the parameter change time of this request:           |
    |------/path/to/ad/voice/set-waves:Tiib:Fiib:Tiiib:Fiiib--->|
    |      pass new wave(s) to ADnote                           |
    |      - path: osc or mod-osc? (T/F)                        |
    |      - timestamp of inducing parameter change (0 if none) |
    |      - frequency index (i)                                |
    |      - optional: semantic index (= write position) (i)    |
    |      - 2D/1D Tensor for given freq/semantic+freq (b)      |
    |                                                           |
    |                If the wave does not come from an outdated |
    |                parameter change (OK if from up-to-date    |
    |                parameter change or not from parameter     |
    |                change at all):                            |
    |                          - if Tensor2s are passed:        |
    |                            swap all Tensor1s from the     |
    |                            passed Tensor2 with the        |
    |                            Tensor1s from the "next"       |
    |                            ringbuffer                     |
    |                          - if Tensor1s are passed:        |
    |                            swap the Tensor1 with the      |
    |                            Tensor1 at given frequency     |
    |                            from the "current" ringbuffer  |
    |                          - increase respective ringbuffer |
    |                            write pointer `w_delayed` by   |
    |                            number of semantics inserted   |
    |                          - if Tensor2s were passed,       |
    |                            if the last freq has           |
    |                            arrived: swap next and current |
    |                            WaveTable                      |
    |                                                           |
    |<-----free:sb----------------------------------------------|
    |      recycle the Tensor1 which is now unused, OR          |
    |      recycle the Tensor2 which includes the now           |
    |      unused (because of swap) Tensor1s                    |
    |                                                           |
    - free the Tensor1/2 (includes freeing any                  |
      contained sub-Tensors)                                    |
```
