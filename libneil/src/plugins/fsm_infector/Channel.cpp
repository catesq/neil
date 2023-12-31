#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <zzub/plugin.h>
#include "DSPChips.h"

#include "infector.h"

namespace fsm {

CChannel::CChannel()
{
  Frequency=0.01f;
  FilterEnv.m_nState=4;
  pTrack=NULL;
  PhaseOSC1=0.0f;
  PhaseOSC2=0.0f;
  PhaseOSC3=0.0f;
  Phase1=0;
  Phase2=0;
}

void CChannel::init()
{
}

void CChannel::ClearFX()
{
}

void CChannel::Reset()
{
	AmpEnv.NoteOff();
	FilterEnv.NoteOff();
	AmpEnv.m_fSilence=1.0/128.0;
  Frequency=0.01f;
	pTrack=NULL;

}

void CChannel::NoteReset()
{
  PhaseOSC1=0.0f;
  PhaseOSC2=0.0f;
  Filter.ResetFilter();
  Phase1=0;
  Phase2=0;
}

}
