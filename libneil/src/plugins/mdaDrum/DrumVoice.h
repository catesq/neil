#pragma once

#include "DrumDefines.h"
#include "math.h"

/*
 * The oringal mda drum generated samples from a drum preset. Each note/variation of the drum was sampled separately.
 *
 * This version pre generates some data then generates the sound when played
 *
 * The drumvoice is the pre generated data and can be reused to play any note of the drum.
 *
 * It has a messy internal state and I have not yet tried to enable realtime parameter editing.
 */


class DrumVoice {
public:
    DrumVoice(uint16_t drum_id, float* drum_params, uint16_t max_block_size);

    DrumVoice ();
    ~DrumVoice();

    void init(int samplesPerBlock, int sampleRate);
    void init_env(int env_id, float *src);
    bool is_playing();

    bool matches(uint32_t check) {
        return drum_id == check; 
    }

    bool operator==(const DrumVoice& cmp_voice) {
        return drum_id == cmp_voice.drum_id;
    }

    uint16_t id() { return drum_id; }

    void startNote (const int midiNoteNumber, const float velocity, int sampleRate, float adj_timestretch = 1.0f);
    void stopNote (const bool allowTailOff);
    void renderNextBlock (float*, int numSamples);
    void clearCurrentNote();
private:
    
    float pluginParams[COUNT_PRESET_PARAMS] = {
        0.0, 100.0, 0.0, 0.0, 0.0, 0.0,
       0.0, 100.0, 1.0,100.0,   1.1, 0.0,    1.1,0.0,     1.1,0.0,
     1.0, 128.0, 200.0, 50.0, 50.0, 0.0,
       0.0,100.0,  0.119,30.0,  0.238,8.0,   0.456,0.0,   0.5,0.0,
     0.0, 128.0, 0.0, 0.0,
       0.0,100.0,  0.0202,45.0, 0.0625,18.0, 0.1199,8.0,  0.456,0.0,
     0.0, 128.0, 100.0, 0.0, 0.0, 60.0, 1.0, 0.0, 0.0, 0.0, 0.0,
       0.0,100.0,  0.304,37.0,  0.118,10.0,  0.471,0.0,   0.0,0.0,
       0.0,100.0,  0.304,37.0,  0.118,10.0,  0.471,0.0,   0.0,0.0,
     0.0, 128.0, 100.0, 0.0,
       0.0,100.0,  0.0372,38.0, 0.1047,20.0, 0.2449,10.0, 0.478,0.0,
     0.0, 128.0, 100.0, 0.0,
       0.0,100.0,  0.0372,38.0, 0.1047,20.0, 0.2449,10.0, 0.478,0.0,
     0.0, 128.0, 0.0, 0.0
};

    uint16_t drum_id;
    bool makeSound = false;
    int cx = 0;
    bool show_data = false;
    uint16_t max_block_size;

    
    float timestretch{};         //overall time scaling
    int   sampleRate;
//    int currentlyPlayingNote = -1, currentlyPlayingSound = 0;

//    int numVoice;


//    double currentAngle, angleDelta,
    float mem_t, mem_o, mem_n, mem_b, mem_tune, mem_time;
    double level, tailOff;


    float* DF = nullptr;
    float* phi = nullptr;
    float envpts[8][3][32];    //envelope/time-level/point
    float envData[8][6];       //envelope running status
//    int   chkOn[8], sliLev[8]; //section on/off and level
    bool  chkOn[8];
    float sliLev[8]; //section on/off and level

    short clippoint{};


    long  Length, tpos, tplus;
    float x[3];
    float MasterTune, randmax, randmax2;
    int   MainFilter, HighPass;

//    long  NON, NT, TON, DiON, TDroop, DStep;
    long NT, TDroop, DStep;
    bool TON,NON,DiON;
    float a, b, c, d, g, TT, TTT, TL, NL, F1, F2, Fsync;
    float TphiStart, Tphi, TDroopRate, ddF, DAtten, DGain;

//    long  BON, BON2, BFStep, BFStep2, botmp;
    long  BFStep, BFStep2, botmp;
    bool BON, BON2;
    float BdF, BdF2, BPhi, BPhi2, BF, BF2, BQ, BQ2, BL, BL2;

//    long  OON, OF1Sync, OF2Sync, OMode, OW1, OW2;
    long  OF1Sync, OF2Sync, OMode, OW1, OW2;
    bool OON;
    float Ophi1, Ophi2, OF1, OF2, OL, Ot, OBal1, OBal2, ODrive;
    float Ocf1, Ocf2, OcF, OcQ, OcA, Oc[6][2];  //overtone cymbal mode
    float Oc0, Oc1, Oc2;

    float MFfb, MFtmp, MFres, MFin, MFout;
    float DownAve;
    long  DownStart, DownEnd;

    //==============================================================================
    inline int longestEnv () {
        long eon;
        float l = 0.f;

        for (int e = 1; e < 7; e++)  { // 3
            eon = e - 1;

            if (eon > 2) {
                eon = eon - 1;
            }

            if (chkOn[eon] == 1) {
                if (envData[e][MAX] > l) {
                    l = envData[e][MAX];
                }
            }
        }

        return 256 + (int) l;
    }

    //==============================================================================
    inline float loudestEnv () {
        float loudest = 0.f;
        int i = 0;

        do {
            if (chkOn[i]) {
                if (sliLev[i] > loudest) {
                    loudest = (float) sliLev[i];
                }
            }
        } while(++i < 5);

        return (loudest * loudest);
    }

    //==============================================================================
    inline void updateEnv (int e, long t) {
        float endEnv, dT;

        envData[e][NEXTT] = envpts[e][0][(long)(envData[e][PNT] + 1.f)]; // * timestretch; //get next point

        if(envData[e][NEXTT] < 0) {
            envData[e][NEXTT] = (sampleRate*10+100) * timestretch; //if end point, hold
        }

        envData[e][ENV] = envpts[e][1][(long)(envData[e][PNT] + 0.f)] * 0.01f; //this level
        endEnv = envpts[e][1][(long)(envData[e][PNT] + 1.f)] * 0.01f;          //next level

        dT = envData[e][NEXTT] - (float)t;
        if(dT < 1.0) {
            dT = 1.0;
        }

        envData[e][dENV] = (endEnv - envData[e][ENV]) / dT;
        envData[e][PNT] = envData[e][PNT] + 1.0f;
    }

    //==============================================================================
    inline float getWaveform (float ph, int form) {
        float w;

        switch (form) {
        case 0:  //sine
            w = (float) sin (fmod (ph, PI2));
            break;
        case 1:  //sine^2
            w = (float) fabs (2.0f * (float) sin (fmod (0.5f * ph, PI2))) - 1.f;
            break;
        case 2:  //tri
            while (ph < PI2) ph += PI2;
            w = 0.6366197f * (float) fmod (ph, PI2) - 1.f;
            if (w > 1.f) w = 2.f - w;
            break;
        case 3:  //saw
            w = ph - PI2 * (float)(int)(ph / PI2);
            w = (0.3183098f * w) - 1.f;
            break;
        default: //square
            w = (sin (fmod (ph, PI2)) > 0.0) ? 1.f: -1.f;
            break;
        }

        return w;
    }
};

