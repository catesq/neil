#ifndef AFX_CHANNEL_H__2EE63E18_CAF9_44DA_9C74_8891690EEB93__INCLUDED_
#define AFX_CHANNEL_H__2EE63E18_CAF9_44DA_9C74_8891690EEB93__INCLUDED_

#include "ISample.h"
#include "IInstrument.h"
#include "Svf.hpp"
#include "zzub/zzub.h"

class CTrack;
class CMatildeTrackerMachine;

class CChannel {
private:
  static const int NFILTERS = 3;
  int sample_rate = zzub_default_rate;
public:
  CChannel();
  virtual ~CChannel();
  void Free();
  void Reset();
  void SetRampTime(int iRamp);
  void SetVolume(float fVol) { 
    m_fVolume = fVol; 
    UpdateAmp(); 
  }
  void SetPan(float fPan) { 
    m_fPan = fPan; 
    UpdateAmp(); 
  }
  void SetVolumeAndPan(float fVolume, float fPan) { 
    m_fVolume = fVolume; 
    m_fPan = fPan; 
    UpdateAmp(); 
  }
  bool Generate_Move(float **psamples, int numsamples);
  bool Generate_Add(float **psamples, int numsamples);
  int GetWaveEnvPlayPos(const int env);
  bool Release();

  void set_filter_bypass(bool on) {
    for (int i = 0; i < NFILTERS; i++) {
      this->filters[i].set_bypass(on);
    }
  }

  void set_filter_mode(Svf::FilterMode mode) {
    for (int i = 0; i < NFILTERS; i++) {
      this->filters[i].set_mode(mode);
    }
  }

  void set_filter_sampling_rate(int sampling_rate) {
    sample_rate = sampling_rate;
    for (int i = 0; i < NFILTERS; i++) {
      this->filters[i].set_sampling_rate(sampling_rate);
    }
  }

  void set_filter_cutoff(float cutoff) {
    for (int i = 0; i < NFILTERS; i++) {
      this->filters[i].set_cutoff(cutoff);
    }
  }

  void set_filter_resonance(float resonance) {
    for (int i = 0; i < NFILTERS; i++) {
      this->filters[i].set_resonance(resonance);
    }
  }

  SurfDSPLib::CResampler m_Resampler;
  SurfDSPLib::CAmp m_Amp;

  Svf filters[NFILTERS];

  CEnvelope m_VolumeEnvelope;
  CEnvelope m_PanningEnvelope;
  CEnvelope m_PitchEnvelope;

  CTrack *m_pOwner;
  CMatildeTrackerMachine *m_pMachine;
  
  ISample *m_pSample;
  IInstrument *m_pInstrument;
  /*
    const CWaveLevel *m_pWaveLevel;
    const CWaveInfo *m_pWaveInfo;
  */

  bool m_oFree;
  float	m_fPitchEnvFreq;
protected:
  float	m_fVolume;
  float	m_fPan;

  void UpdateAmp() {
#ifdef MONO
    if (m_pSample)
      m_Amp.SetVolume(m_fVolume * m_pSample->GetVolume(), 0);
    else
      m_Amp.SetVolume(m_fVolume, 0);
#else
    if(m_pSample)
      m_Amp.SetVolume(m_fVolume * m_pSample->GetVolume() * 
		      (1.0f - m_fPan), m_fVolume * m_pSample->GetVolume() * 
		      (1.0f + m_fPan) );
    else
      m_Amp.SetVolume(m_fVolume * (1.0f - m_fPan), 
		      m_fVolume * (1.0f + m_fPan));
#endif
  }
};

#endif // AFX_CHANNEL_H__2EE63E18_CAF9_44DA_9C74_8891690EEB93__INCLUDED_
