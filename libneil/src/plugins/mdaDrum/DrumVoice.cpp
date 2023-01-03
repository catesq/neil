#include "DrumVoice.h"
#include <stdlib.h>
#include <string.h>

static unsigned long memsize = 0;

DrumVoice::DrumVoice (uint16_t drum_id, float *params, uint16_t max_block_size) : 
                drum_id(drum_id),
                max_block_size(max_block_size) {

    memcpy(pluginParams, params, COUNT_PRESET_PARAMS * sizeof(float));

    DF = new float [max_block_size];
    phi = new float [max_block_size];
    memsize = (char*)((&DownEnd)+1) - (char*)(envpts);
}

DrumVoice::~DrumVoice() { }

void DrumVoice::init_env(int env_id, float *src) {
    float *dest_time = &envpts[env_id][0][0];
    float *dest_gain = &envpts[env_id][1][0];

    for(int pt = 0; pt < MAX_ENV_POINTS-1; pt++) {
        if(*src < 0) {
            break;
        }

        *dest_time++ = *src++ * this->sampleRate * timestretch;
        *dest_gain++ = *src++;
    }

    *dest_time-- = -1;
    *dest_gain = 0;
    envData[env_id][MAX] = *dest_time;
}

bool DrumVoice::is_playing() {
    return !makeSound || tpos >= Length;
}

void DrumVoice::startNote (const int midiNoteNumber, const float velocity, int sampleRate, float adj_timestretch) {
//    currentAngle = 0.0;
    makeSound = true;
    level = velocity * 0.15;
    tailOff = 0.0;
    this->sampleRate = sampleRate;
    
    mem_t = 1.0f;
    mem_o = 1.0f;
    mem_n = 1.0f;
    mem_b = 1.0f;
    mem_tune = 0.0f;
    mem_time = 1.0f;

    memset(DF, 0, sizeof(float) * max_block_size);
    memset(phi, 0, sizeof(float) * max_block_size);

    memset(envpts, 0, memsize);

    timestretch = .01f * mem_time * pluginParams [PP_MAIN_STRETCH] * adj_timestretch;
    timestretch = std::min (16.f, std::max (1.f/16.f, timestretch));

    DGain = (float) pow (10.0, 0.05 * pluginParams [PP_MAIN_GAIN]);

    // 446 is c (in octave 4) in hz. this drum machine appears to use c-4 as base frequency (implied by the 1.059461 ^ MasterTune... 1.059 * 440 = 466)
    float noteRatio = NEIL_NOTE_IN_HERTZ(midiNoteNumber) / 466.163762;

    MasterTune = pluginParams [PP_MAIN_TUNING];
    MasterTune = (float) pow(1.0594631f, MasterTune + mem_tune) * std::min(std::max(0.25f, noteRatio), 4.0f);

    MainFilter = (int) pluginParams [PP_MAIN_FILTER];

    MFres = 0.0101f * pluginParams [PP_MAIN_RESONANCE];
    MFres = (float)pow(MFres, 0.5f);

    HighPass = (int) pluginParams [PP_MAIN_HIGHPASS];

    init_env(7, &pluginParams[PP_MAIN_ENV_T1TIME]);

    // TONE --------------
    TON = chkOn[0] = pluginParams [PP_TONE_ON] > 0.f;
    sliLev[0] = pluginParams [PP_TONE_LEVEL];
    TL = (float)(sliLev[0] * sliLev[0]) * mem_t;

    init_env(1, &pluginParams[PP_TONE_ENV_T1TIME]);

    F1 = MasterTune * PI2 * pluginParams[PP_TONE_F1] / sampleRate;

    if (fabs (F1) < 0.001f) {
        F1 = 0.001f; // to prevent overtone ratio div0
    }

    F2 = MasterTune * PI2 * pluginParams[PP_TONE_F2] / sampleRate;
    Fsync = F2;
    TDroopRate = pluginParams[PP_TONE_DROOP];

    if (TDroopRate > 0.f) {
        TDroopRate = (float) pow (10.0f, (TDroopRate - 20.0f) / 30.0f);
        TDroopRate = TDroopRate * -4.f / envData[1][MAX];
        TDroop = 1;
        F2 = F1 + ((F2 - F1) / (1.f - (float) exp (TDroopRate * envData[1][MAX])));
        ddF = F1 - F2;
    } else {
        TDroop = 0;
        ddF = F2 - F1;
    }

    Tphi = pluginParams[PP_TONE_PHASE] / 57.29578f; // degrees > radians

    // NOISE --------------
    NON = chkOn[1] = pluginParams [PP_NOIZ_ON] > 0.f;
    sliLev[1] = pluginParams[PP_NOIZ_LEVEL];
    NT = (int) pluginParams [PP_NOIZ_SLOPE];

    init_env(2, &pluginParams[PP_NOIZ_ENV_T1TIME]);

    NL = (float)(sliLev[1] * sliLev[1]) * mem_n;
    if (NT < 0) {
        a = 1.f + (NT / 105.f);
        b = 0.f;
        c = 0.f;
        d = -NT / 105.f;
        g = (1.f + 0.0005f * NT * NT) * NL;
    } else {
        a = 1.f;
        b = -NT / 50.f;
        c = (float)fabs((float)NT) / 100.f;
        d = 0.f;
        g = NL;
    }
    x[0] = 0.f, x[1] = 0.f, x[2] = 0.f;
    if (pluginParams [PP_NOIZ_FIXEDSEQ] > 0.f)
        srand(1); // fixed random sequence

    // OVERTONES --------------
    OON = chkOn[2] = pluginParams[PP_OTON_ON] > 0.f;
    sliLev[2] = pluginParams[PP_OTON_LEVEL];
    OL = (float)(sliLev[2] * sliLev[2]) * mem_o;

    init_env(3, &pluginParams[PP_OTON1_ENV_T1TIME]);

    init_env(4, &pluginParams[PP_OTON2_ENV_T1TIME]);

            /* Overtones Method 0..2 */
    OMode = (int) pluginParams [PP_OTON_METHOD];
    OF1 = MasterTune * PI2 * pluginParams [PP_OTON_F1] / sampleRate;
    OF2 = MasterTune * PI2 * pluginParams [PP_OTON_F2] / sampleRate;
    OW1 = (int) pluginParams [PP_OTON_WAVE1];
    OW2 = (int) pluginParams [PP_OTON_WAVE2];
    OBal2 = pluginParams [PP_OTON_PARAM];
    ODrive = (float) pow (OBal2, 3.0f) / (float) pow (50.0f, 3.0f);
    OBal2 *= 0.01f;
    OBal1 = 1.f - OBal2;
    Ophi1 = Tphi;
    Ophi2 = Tphi;

    if (MainFilter == 0) {
        MainFilter = (int) pluginParams [PP_OTON_FILTER];
    }

    if((pluginParams [PP_OTON_TRACK1] > 0.f) && TON) {
        OF1Sync = 1;
        OF1 = OF1 / F1;
    }

    if((pluginParams [PP_OTON_TRACK2] > 0.f) && TON) {
        OF2Sync = 1;
        OF2 = OF2 / F1;
    }

    OcA = 0.28f + OBal1 * OBal1;  //overtone cymbal mode
    OcQ = OcA * OcA;
    OcF = (1.8f - 0.7f * OcQ) * 0.92f; //multiply by env 2
    OcA *= 1.0f + 4.0f * OBal1;  //level is a compromise!
    Ocf1 = PI2 / OF1;
    Ocf2 = PI2 / OF2;

    for (int i = 0; i < 6; i++) {
        Oc[i][0] = Oc[i][1] = Ocf1 + (Ocf2 - Ocf1) * 0.2f * (float)i;
    }

    // NOISE BAND 1 --------------
    BON = chkOn[3] = pluginParams[PP_NBA1_ON] > 0.f;
    sliLev[3] = (int) pluginParams[PP_NBA1_LEVEL];
    BL = (float) (sliLev[3] * sliLev[3]) * mem_b;
    BF = MasterTune * PI2 * pluginParams[PP_NBA1_F] / sampleRate;
    BPhi = PI2 / 8.f;
    //        getEnv (5, "0,100 2250,30 4500,0"); /* Noiseband1 Envelope */

    // INIT_ENV_PTS(envpts[5], PP_NBA1)
    init_env(5, &pluginParams[PP_NBA1_ENV_T1TIME]);

    BFStep = (int) pluginParams [PP_NBA1_DF];
    BQ = (float) BFStep;
    BQ = BQ * BQ / (10000.f - 6600.f * ((float) sqrt(BF) - 0.19f));
    BFStep = 1 + (int)((40.f - (BFStep / 2.5f)) / (BQ + 1.f + (1.f * BF)));

    // NOISE BAND 2 --------------
    chkOn[4] = pluginParams[PP_NBA2_ON] > 0.f;
    BON2 = chkOn[4];
    sliLev[4] = pluginParams [PP_NBA2_LEVEL];
    BL2 = (float)(sliLev[4] * sliLev[4]) * mem_b;
    BF2 = MasterTune * PI2 * pluginParams [PP_NBA2_F] / sampleRate;
    BPhi2 = PI2 / 8.f;
    
    init_env(6, &pluginParams[PP_NBA2_ENV_T1TIME]);

    BFStep2 = (int) pluginParams[PP_NBA2_DF];
    BQ2 = (float) BFStep2;
    BQ2 = BQ2 * BQ2 / (10000.f - 6600.f * ((float) sqrt(BF2) - 0.19f));
    BFStep2 = 1 + (int)((40 - (BFStep2 / 2.5)) / (BQ2 + 1 + (1 * BF2)));

    // DISTORTION --------------
    DiON  = chkOn[5] = pluginParams [PP_DIST_ON] > 0.f;
    DStep = 1 + (int) pluginParams [PP_DIST_RATE];
    if (DStep == 7) DStep = 20;
    if (DStep == 6) DStep = 10;
    if (DStep == 5) DStep = 8;
    clippoint = 32700;
    DAtten = 1.0f;
    if (DiON) {
        DAtten = DGain * loudestEnv();
        if (DAtten > 32700)
            clippoint = 32700;
        else
            clippoint = DAtten;

        DAtten = (float) pow (2.0, 2.0 * (int) pluginParams[PP_DIST_BITS]);
        DGain *= DAtten * (float) pow (10.0, 0.05 * (int) pluginParams[PP_DIST_CLIPPING]);
    }

    // prepare envelopes
    randmax = 1.f / static_cast<float>(RAND_MAX);
    randmax2 = 2.f * randmax;

    for (int i = 1; i < 8; i++) {
        envData[i][NEXTT] = 0;
        envData[i][PNT] = 0;
    }

    Length = longestEnv();
    tpos = 0;
}

void DrumVoice::clearCurrentNote() {
    makeSound = false;
}

void DrumVoice::stopNote (const bool allowTailOff) {
    if (allowTailOff) {
        // start a tail-off by setting this flag. The render callback will pick up on
        // this and do a fade out, calling clearCurrentNote() when it's finished.
        if (makeSound && tailOff <= 0.001) // we only need to begin a tail-off if it's not already doing so - the stopNote method could be called more than once.
            tailOff = 1.0;
    } else {
        // we're being told to stop playing immediately, so reset everything..
        clearCurrentNote();
    }
}

inline uint32_t bitcat(bool* arr, uint8_t size) {
    uint32_t bits = 0;
    for(auto i=0; i<size; i++)
        bits |= (*arr++) ? 1 << i : 0;
    return bits;
}

//==============================================================================
void DrumVoice::renderNextBlock (float *outbuf, int numSamples) {
//    if (angleDelta != 0.0 && tpos < Length) {
    if (!makeSound || tpos >= Length) {
        return;
    }
    
    int t;
    tplus = tpos + (numSamples - 1);

    if(NON) { //noise
        for(t=tpos; t<=tplus; t++) {
            if(t < envData[2][NEXTT]) {
                envData[2][ENV] = envData[2][ENV] + envData[2][dENV];
            } else {
                updateEnv(2, t);
            }

            x[2] = x[1];
            x[1] = x[0];
            x[0] = (randmax2 * (float)rand()) - 1.f;
            TT = a * x[0] + b * x[1] + c * x[2] + d * TT;
            DF[t - tpos] = TT * g * envData[2][ENV];
        }

        if(t>=envData[2][MAX]) {
            NON=false;
        }
    } else {
        for(int j=0; j<numSamples; j++) {
            DF[j]=0.f;
        }
    }

    if(TON) { //tone {
        TphiStart = Tphi;
        if(TDroop==1) {
            for(t=tpos; t<=tplus; t++)
                phi[t - tpos] = F2 + (ddF * (float)exp(t * TDroopRate));
        } else {
            for(t=tpos; t<=tplus; t++) {
                phi[t - tpos] = F1 + (t / envData[1][MAX]) * ddF;
            }
        }

        for(t=tpos; t<=tplus; t++) {
            int totmp = t - tpos;
            if(t < envData[1][NEXTT]) {
                envData[1][ENV] = envData[1][ENV] + envData[1][dENV];
            } else {
                updateEnv(1, t);
            }

            Tphi = Tphi + phi[totmp];
            DF[totmp] += TL * envData[1][ENV] * (float)sin(fmod(Tphi,PI2));//overflow?
        } if(t>=envData[1][MAX]) {
            TON=false;
        }
    } else  {
        for(int j=0; j<numSamples; j++) {
            phi[j]=F2; //for overtone sync
        }
    }

    if(BON) { //noise band 1
        for(t=tpos; t<=tplus; t++) {
            if(t < envData[5][NEXTT]) {
                envData[5][ENV] = envData[5][ENV] + envData[5][dENV];
            } else {
                updateEnv(5, t);
            }

            if((t % BFStep) == 0) {
                BdF = randmax * (float)rand() - 0.5f;
            }
            BPhi = BPhi + BF + BQ * BdF;
            botmp = t - tpos;
            DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi,PI2)) * envData[5][ENV] * BL;
        }

        if(t>=envData[5][MAX]) {
            BON=false;
        }
    }

    if(BON2) { //noise band 2
        for(t=tpos; t<=tplus; t++) {

            if(t < envData[6][NEXTT]) {
                envData[6][ENV] = envData[6][ENV] + envData[6][dENV];
            }  else {
                updateEnv(6, t);
            }

            if((t % BFStep2) == 0) {
                BdF2 = randmax * (float)rand() - 0.5f;
            }

            BPhi2 = BPhi2 + BF2 + BQ2 * BdF2;
            botmp = t - tpos;
            DF[botmp] = DF[botmp] + (float)cos(fmod(BPhi2,PI2)) * envData[6][ENV] * BL2;
        }

        if(t>=envData[6][MAX]) {
            BON2=false;
        }
    }

    for (t=tpos; t<=tplus; t++) {
        if(OON)  { //overtones
            if(t<envData[3][NEXTT]) {
                envData[3][ENV] = envData[3][ENV] + envData[3][dENV];
            } else {
                if(t>=envData[3][MAX])  { //wait for OT2
                    envData[3][ENV] = 0;
                    envData[3][dENV] = 0;
                    envData[3][NEXTT] = 999999;
                } else {
                    updateEnv(3, t);
                }
            }

            if(t<envData[4][NEXTT]) {
                envData[4][ENV] = envData[4][ENV] + envData[4][dENV];
            } else {
                if(t>=envData[4][MAX]) { //wait for OT1
                    envData[4][ENV] = 0;
                    envData[4][dENV] = 0;
                    envData[4][NEXTT] = 999999;
                } else {
                    updateEnv(4, t);
                }
            }

            //
            TphiStart = TphiStart + phi[t - tpos];
            if(OF1Sync==1) {
                Ophi1 = TphiStart * OF1;
            } else {
                Ophi1 = Ophi1 + OF1;
            }

            if(OF2Sync==1) {
                Ophi2 = TphiStart * OF2;
            } else {
                Ophi2 = Ophi2 + OF2;
            }

            Ot=0.0f;
            switch (OMode) {
            case 0: //add
                Ot = OBal1 * envData[3][ENV] * getWaveform (Ophi1, OW1);
                Ot = OL * (Ot + OBal2 * envData[4][ENV] * getWaveform (Ophi2, OW2));
                break;

            case 1: //FM
                Ot = ODrive * envData[4][ENV] * getWaveform (Ophi2, OW2);
                Ot = OL * envData[3][ENV] * getWaveform (Ophi1 + Ot, OW1);
                break;

            case 2: //RM
                Ot = (1 - ODrive / 8) + (((ODrive / 8) * envData[4][ENV]) * getWaveform (Ophi2, OW2));
                Ot = OL * envData[3][ENV] * getWaveform (Ophi1, OW1) * Ot;
                break;

            case 3: //808 Cymbal
                for(int j=0; j<6; j++) {
                    Oc[j][0] += 1.0f;

                    if(Oc[j][0]>Oc[j][1]) {
                        Oc[j][0] -= Oc[j][1];
                        Ot = OL * envData[3][ENV];
                    }
                }

                Ocf1 = envData[4][ENV] * OcF;  //filter freq
                Oc0 += Ocf1 * Oc1;
                Oc1 += Ocf1 * (Ot + Oc2 - OcQ * Oc1 - Oc0);  //bpf
                Oc2 = Ot;
                Ot = Oc1;
                break;
            }
        }

        if(MainFilter==1)  {
            //filter overtones
            if(t<envData[7][NEXTT]) {
                envData[7][ENV] = envData[7][ENV] + envData[7][dENV];
            } else {
                updateEnv(7, t);
            }


            MFtmp = envData[7][ENV];
            if(MFtmp >0.2f) {
                MFfb = 1.001f - (float)pow(10.0f, MFtmp - 1);
            } else {
                MFfb = 0.999f - 0.7824f * MFtmp;
            }

            MFtmp = Ot + MFres * (1.f + (1.f/MFfb)) * (MFin - MFout);
            MFin = MFfb * (MFin - MFtmp) + MFtmp;
            MFout = MFfb * (MFout - MFin) + MFin;

            DF[t - tpos] = DF[t - tpos] + (MFout - (HighPass * Ot));
        } else if(MainFilter==2)  { //filter all
            if(t<envData[7][NEXTT]) {
                envData[7][ENV] = envData[7][ENV] + envData[7][dENV];
            } else {
                updateEnv(7, t);
            }

            MFtmp = envData[7][ENV];
            if(MFtmp >0.2f) {
                MFfb = 1.001f - (float)pow(10.0f, MFtmp - 1);
            } else {
                MFfb = 0.999f - 0.7824f * MFtmp;
            }

            MFtmp = DF[t - tpos] + Ot + MFres * (1.f + (1.f/MFfb)) * (MFin - MFout);
            MFin = MFfb * (MFin - MFtmp) + MFtmp;
            MFout = MFfb * (MFout - MFin) + MFin;

            DF[t - tpos] = MFout - (HighPass * (DF[t - tpos] + Ot));
        } else {  //no filter
            DF[t - tpos] = DF[t - tpos] + Ot;
        }
    }

    // bit resolution
    if (DiON) {
        for (int j = 0; j < numSamples; j++) {
            DF[j] = DGain * (int)(DF[j] / DAtten);
        }

        // downsampling
        for (int j = 0; j < numSamples; j += DStep) {
            DownAve = 0;
            DownStart = j;
            DownEnd = j + DStep - 1;
            for(int jj = DownStart; jj <= DownEnd; jj++) {
                DownAve = DownAve + DF[jj];
            }

            DownAve = DownAve / DStep;
            for(int jj = DownStart; jj <= DownEnd; jj++) {
                DF[jj] = DownAve;
            }
        }
    } else {
        for (int j = 0; j < numSamples; j++) {
            DF[j] *= DGain;
        }
    }

    // render output
    const float scale = 1.0f / 0xffff;


    if (tailOff > 0) {
        for (int j = 0; j < numSamples; j++)  {
            //clipping + output
            float thisGain = level * tailOff;

            if (DF[j] > clippoint) {
                outbuf[j] += (float)(scale * clippoint) * thisGain;
            } else {
                if(DF[j] < -clippoint) {
                    outbuf[j] += (float)(scale * -clippoint) * thisGain;
                } else {
                    outbuf[j] += (float)(scale * (short) DF[j]) * thisGain;
                }
            }

            tailOff *= 0.999;

            if (tailOff <= 0.001) {
                clearCurrentNote();
                tailOff = 0.0;
                break;
            }
        }
    } else {
        for (int j = 0; j < numSamples; j++)  {
            //clipping + output
            float thisGain = level;

            if (DF[j] > clippoint) {
                outbuf[j] += (float)(scale * clippoint) * thisGain;
            } else if(DF[j] < -clippoint) {
                outbuf[j] += (float)(scale * -clippoint) * thisGain;
            } else {
                outbuf[j] += (float)(scale * (short) DF[j]) * thisGain;
            }
        }
    }

    tpos += numSamples;
    if (tpos >= Length) {
        clearCurrentNote();
        tpos = 0;
    }
}
