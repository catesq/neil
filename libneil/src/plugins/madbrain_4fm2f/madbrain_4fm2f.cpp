// MadBrain's 4fm2f
// written by Hubert Lamontagne (aka MadBrain)
// ported to zzub by jmmcd <jamesmichaelmcdermott@gmail.com>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <fcntl.h>
#include <zzub/signature.h>
#include <zzub/plugin.h>
#include "waves2.h"

#pragma optimize ("a", on)

#define MAX_CHANNELS 32
#define ENV_TURBO_ATTACK -1
#define ENV_ATTACK 0
#define ENV_DECAY 1
#define ENV_SUSTAIN 2
#define ENV_RELEASE 3
#define ENV_END 4

class C4fm2f;
float downscale = 1.0f/32768.0f;
C4fm2f *pz;

int speedtable[] =
{
	        0, //0
	    0x600, //1
	    0x800, //2
	    0xc00, //3
	   0x1000, //4
	   0x1800, //5
	   0x2000, //6
	   0x3000, //7
	   0x4000, //8
	   0x6000, //9
	   0x8000, //10
	   0xc000, //11
	  0x10000, //12
	  0x18000, //13
	  0x20000, //14
	  0x30000, //15
	  0x40000, //16
	  0x60000, //17
	  0x80000, //18
	  0xc0000, //19
	 0x100000, //20
	 0x180000, //21
	 0x200000, //22
	 0x300000, //23
	 0x400000, //24
	 0x600000, //25
	 0x800000, //26
	 0xc00000, //27
	0x1000000, //28
	0x1800000, //29
	0x2000000, //30
	0x3000000, //31
	0x4000000, //32
	0x4000000, //32(turbo)
	0x4000000, //32(turbo)
	0x4000000, //32(turbo)
	0x4000000  //32(turbo)
};

const char *speednames[] =
{
	"infinite",//0
	 "16.0 s", //1
	 "12.0 s", //2
	 "8.0 s", //3
	 "6.0 s", //4
	 "4.0 s", //5
	 "3.0 s", //6
	 "2.0 s", //7
	 "1.5 s", //8
	 "990 ms", //9
	 "740 ms", //10
	 "500 ms", //11
	 "370 ms", //12
	 "250 ms", //13
	 "185 ms", //14
	 "125 ms", //15
	"93 ms", //16
	"62 ms", //17
	"46 ms", //18
	"31 ms", //19
	"23 ms", //20
	"16 ms", //21
	"12 ms", //22
	"7.7 ms", //23
	"5.8 ms", //24
	"4.0 ms", //25
	"2.9 ms", //26
	"1.9 ms", //27
	"1.4 ms", //28
	"0.97 ms", //29
	"0.72 ms", //30
	"0.48 ms", //31
	"0.36 ms"  //32
};

int routingcarriers[]= // which operators are carriers
{
	0, // algo 0 doesn't exist
	1, //  1: 4->3->2->1      
	1, //  2: 3+4 -> 2 -> 1   
	1, //  3: 2+(4->3) -> 1   
	1, //  4: 2+3+4 -> 1      
	1, //  5: 4 -> 2+3 -> 1
	3, //  6: 3+4 -> 1+2
	3, //  7: 4 -> 3 -> 1+2
	3, //  8: 4 -> 1+(3->2)
	7, //  9: 4 -> 1+2+3
	5, // 10: 2->1 + 4->3
	3, // 11: 1 + 4->3->2
	3, // 12: 1 + (3+4)->2
	7, // 13: 1 + 4->(2+3)
	7, // 14: 1 + 2 + 4->3
	15,// 15: 1+2+3+4
};

int algovol[]= // volume attenuation for carriers, per algorithm
{
	0x9000000, // algo 0 doesn't exist
	0x9000000, //  1: 4->3->2->1      
	0x9000000, //  2: 3+4 -> 2 -> 1   
	0x9000000, //  3: 2+(4->3) -> 1   
	0x9000000, //  4: 2+3+4 -> 1      
	0x9000000, //  5: 4 -> 2+3 -> 1
	0xa000000, //  6: 3+4 -> 1+2
	0xa000000, //  7: 4 -> 3 -> 1+2
	0xa000000, //  8: 4 -> 1+(3->2)
	0xab00000, //  9: 4 -> 1+2+3
	0xa000000, // 10: 2->1 + 4->3
	0xa000000, // 11: 1 + 4->3->2
	0xa000000, // 12: 1 + (3+4)->2
	0xab00000, // 13: 1 + 4->(2+3)
	0xab00000, // 14: 1 + 2 + 4->3
	0xb000000, // 15: 1+2+3+4
};

/************************************************
*           Routing: Oscillators                *
*  1: 4->3->2->1      
*  2: 3+4 -> 2 -> 1   
*  3: 2+(4->3) -> 1   
*  4: 2+3+4 -> 1      

*  5: 4 -> 2+3 -> 1
*  6: 3+4 -> 1+2

*  7: 4 -> 3 -> 1+2
*  8: 4 -> 1+(3->2)
*  9: 4 -> 1+2+3

* 10: 2->1 + 4->3

* 11: 1 + 4->3->2
* 12: 1 + (3+4)->2
* 13: 1 + 4->(2+3)

* 14: 1 + 2 + 4->3
* 15: 1+2+3+4

*************************************************/

const zzub::parameter *paraRouting = 0;

const zzub::parameter *paraOsc4Waveform = 0;
const zzub::parameter *paraOsc4Frequency = 0;
const zzub::parameter *paraOsc4Finetune = 0;
const zzub::parameter *paraOsc4Volume = 0;
const zzub::parameter *paraOsc4Attack = 0;
const zzub::parameter *paraOsc4Decay = 0;
const zzub::parameter *paraOsc4Sustain = 0;
const zzub::parameter *paraOsc4Release = 0;

const zzub::parameter *paraOsc3Waveform = 0;
const zzub::parameter *paraOsc3Frequency = 0;
const zzub::parameter *paraOsc3Finetune = 0;
const zzub::parameter *paraOsc3Volume = 0;
const zzub::parameter *paraOsc3Attack = 0;
const zzub::parameter *paraOsc3Decay = 0;
const zzub::parameter *paraOsc3Sustain = 0;
const zzub::parameter *paraOsc3Release = 0;

const zzub::parameter *paraOsc2Waveform = 0;
const zzub::parameter *paraOsc2Frequency = 0;
const zzub::parameter *paraOsc2Finetune = 0;
const zzub::parameter *paraOsc2Volume = 0;
const zzub::parameter *paraOsc2Attack = 0;
const zzub::parameter *paraOsc2Decay = 0;
const zzub::parameter *paraOsc2Sustain = 0;
const zzub::parameter *paraOsc2Release = 0;

const zzub::parameter *paraOsc1Waveform = 0;
const zzub::parameter *paraOsc1Frequency = 0;
const zzub::parameter *paraOsc1Finetune = 0;
const zzub::parameter *paraOsc1Volume = 0;
const zzub::parameter *paraOsc1Attack = 0;
const zzub::parameter *paraOsc1Decay = 0;
const zzub::parameter *paraOsc1Sustain = 0;
const zzub::parameter *paraOsc1Release = 0;

const zzub::parameter *paraLpfCutoff = 0;
const zzub::parameter *paraLpfResonance = 0;
const zzub::parameter *paraLpfKeyFollow = 0;
const zzub::parameter *paraLpfEnvelope = 0;
const zzub::parameter *paraLpfAttack = 0;
const zzub::parameter *paraLpfDecay = 0;
const zzub::parameter *paraLpfSustain = 0;
const zzub::parameter *paraLpfRelease = 0;

const zzub::parameter *paraNote = 0;
const zzub::parameter *paraVolume = 0;

// Parameter structures
#pragma pack(1)

class gvals // variables coming from the sliders
{
public:
	unsigned char routing;
	unsigned char osc4_wave;
	unsigned char osc4_freq;
	unsigned char osc4_fine;
	unsigned char osc4_vol;
	unsigned char osc4_a;
	unsigned char osc4_d;
	unsigned char osc4_s;
	unsigned char osc4_r;
	unsigned char osc3_wave;
	unsigned char osc3_freq;
	unsigned char osc3_fine;
	unsigned char osc3_vol;
	unsigned char osc3_a;
	unsigned char osc3_d;
	unsigned char osc3_s;
	unsigned char osc3_r;
	unsigned char osc2_wave;
	unsigned char osc2_freq;
	unsigned char osc2_fine;
	unsigned char osc2_vol;
	unsigned char osc2_a;
	unsigned char osc2_d;
	unsigned char osc2_s;
	unsigned char osc2_r;
	unsigned char osc1_wave;
	unsigned char osc1_freq;
	unsigned char osc1_fine;
	unsigned char osc1_vol;
	unsigned char osc1_a;
	unsigned char osc1_d;
	unsigned char osc1_s;
	unsigned char osc1_r;
	unsigned char lpf_cutoff;
	unsigned char lpf_reso;
	unsigned char lpf_kf;
	unsigned char lpf_env;
	unsigned char lpf_a;
	unsigned char lpf_d;
	unsigned char lpf_s;
	unsigned char lpf_r;
};

class tvals
{
public:
	unsigned char note;
	unsigned char volume;
};

class ovals
{
public:
	unsigned char wave;
	unsigned char freq;
	unsigned char fine;
	unsigned char vol;
	unsigned char a;
	unsigned char d;
	unsigned char s;
	unsigned char r;
};

class fvals
{
public:
	unsigned char cutoff;
	unsigned char reso;
	unsigned char kf;
	unsigned char env;
	unsigned char a;
	unsigned char d;
	unsigned char s;
	unsigned char r;
};

#pragma pack()

class eg
{

public:
	inline void work();
	void on(int fact);
	void off();
	void init();
	void stop();
	int calc_level(unsigned char n);

	// State
public:

	struct
	{
		int a;
		int d;
		int s;
		int r;
		int volume;
	}p;
	struct
	{
		int mode;
		int val;
		int speed;
		int limit;
	}s;
	int out;
	int old_out;
	int ramp_out;
	int ramp;
	int factor;
};

class filter
{
public:
	void tick(int sr);
	inline void minitick();
	inline float generate(float input);
	void init();
	void stop();
public:
	fvals fv;
	tvals tv;
	eg env;

	int cutoff_bias;
	int cutoff,kf;

	float note_adj;
	float cutoff_cache, resonance;
	float o1,o2;
};

class oscillator
{
public:
	void tick(int algo, int oscnum, int sr);
	void init();
	void stop();

	// State
public:
	ovals ov;
	tvals tv;
	eg env;

	unsigned int pos;
	unsigned int freq;
    int wave;
	
	struct
	{
		int freq;
		int fine;
		float note;
	}fc;

	struct
	{
		int osc;
		int chan;
		int algo;
	}vc;
};

class channel
{

public:
	void tick(int sr);
	inline void minitick();
	inline int generate1();
	inline int generate2();
	inline int generate3();
	inline int generate4();
	inline int generate5();
	inline int generate6();
	inline int generate7();
	inline int generate8();
	inline int generate9();
	inline int generate10();
	inline int generate11();
	inline int generate12();
	inline int generate13();
	inline int generate14();
	inline int generate15();
	void init();
	void stop();
	bool isactive();
	void Work(float *psamples, int numsamples);

	// State
public:

	gvals gv;
	tvals tv;

	oscillator osc[4];
	filter lpf;
	int algo;
	int clock,clock_rate;

};

class C4fm2f: public zzub::plugin
{
public:
  C4fm2f();
  virtual ~C4fm2f();
  virtual void init(zzub::archive *); // OU *pi ou *arc);
  virtual void process_controller_events() {}
  virtual void process_events();
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual const char * describe_value(int param, int value);
  virtual void command(int i) {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual void OutputModeChanged(bool stereo) { }
  
  // ::zzub::plugin methods
  virtual void destroy();
  virtual void stop();
  virtual void attributes_changed() {}
  virtual void set_track_count(int);
  virtual void mute_track(int) {}
  virtual bool is_track_muted(int) const { return false; }
  virtual void midi_note(int, int, int) {}
  virtual void event(unsigned int) {}
  virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
  virtual void stop_wave() {}
  virtual int get_wave_envelope_play_position(int) { return -1; }
  virtual const char* describe_param(int) { return 0; }
  virtual bool set_instrument(const char*) { return false; }
  virtual void get_sub_menu(int, zzub::outstream*) {}
  virtual void add_input(const char*) {}
  virtual void delete_input(const char*) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }

  virtual void Tick();
  virtual bool Work(float *psamples, int numsamples, int const mode);
  
  public:
	gvals gval; // Store your global parameters here
	tvals tval[MAX_CHANNELS];
	channel channels[MAX_CHANNELS];
	int active_channels;

};

/**********************************************************
*           C4fm2f:C4fm2f constructor                     *
**********************************************************/

C4fm2f::C4fm2f()
{
	int i;
	global_values = &gval;
	track_values = tval;
	attributes   = NULL;
	for (i=0;i<MAX_CHANNELS;i++)
	{
		channels[i].init();
	}
	active_channels = 1;
}

C4fm2f::~C4fm2f() {}

void C4fm2f::destroy() { 
  delete this; 
}

void eg::init()
{
	s.mode=ENV_END;
	s.val=0x10000000;
	s.speed=0;
	s.limit=0x10000001;
	p.a=16;
	p.d=16;
	p.s=16;
	p.r=16;
	p.volume=0x20000000;
	out=0;
	old_out=0;
	ramp=0;
}

void eg::stop()
{
	s.mode=ENV_END;
	s.val=0x10000000;
	s.speed=0;
	s.limit=0x10000001;
	out=p.volume+s.val;
	out=(0x2000000-(out&0xffffff))>>(out>>24);
}

void eg::on(int clock_rate)
{
	if(p.a == 31)
	{
		s.mode = ENV_DECAY;
		s.val = 0;
		s.speed = speedtable[p.d];
		s.limit=(p.s&31)*0x400000;

	}
	else
	{
		s.mode=ENV_TURBO_ATTACK;
		s.val=0x7ffffff;
		s.speed=speedtable[p.a + 4];
	}
	factor = 32768/clock_rate;
	old_out = 0;
}


void eg::off()
{
	if(s.mode==ENV_END)
		return;
	if(s.mode==ENV_RELEASE)
		return;

	if(s.mode==ENV_ATTACK || s.mode==ENV_TURBO_ATTACK)
	{
		s.val=(s.val>>12);
		s.val*=s.val;
		s.val<<=1;
	}

	s.speed=speedtable[p.r];
	s.mode=ENV_RELEASE;
	s.limit=0x10000000;

}

inline void eg::work()
{
	if(s.mode==ENV_TURBO_ATTACK)
	{
		s.val-=s.speed;
		if(s.val<0x4800000)
		{
			s.val-=0x4800000;
			s.val>>=2;
			s.val+=0x4800000;
			if(s.val<0)
				s.val = 0;
			s.mode=ENV_ATTACK;
			s.speed=speedtable[p.a];
		}
		out=(s.val>>12);
		out*=out;
		out>>=3;
	}
	else if(s.mode==ENV_ATTACK)
	{
		s.val-=s.speed;
		if(s.val<0)
		{
			s.val=0;
			s.mode=ENV_DECAY;
			s.speed=speedtable[p.d];
			s.limit=(p.s&31)*0x400000;
		}
		out=(s.val>>12);
		out*=out;
		out>>=3;
	}
	else
	{
		s.val+=s.speed;
		if(s.val>s.limit)
		{
			s.val=s.limit;
			if(s.mode==ENV_DECAY) 
			{
				if(p.s>=32)
				{
					s.speed=0;
					s.mode=ENV_SUSTAIN;
					s.limit=s.val+1;
				}
				else
				{
					s.speed=speedtable[p.r];
					s.mode=ENV_RELEASE;
					s.limit=0x10000000;
				}
			}
			else
			{
				s.speed=0;
				s.mode=ENV_END;
				s.limit=s.val+1;
			}
		}
		out=s.val;

	}

	out+=p.volume;
	if(out>=0x20000000)
		out=0;
	else
		out=(0x2000000-(out&0xffffff))>>(out>>24);

	ramp = ((out - old_out)*factor)>>15;
	ramp_out = old_out;
	old_out = out;
}

int eg::calc_level(unsigned char n)
{
	int lev = 0;
	if (n==0)
		return 0x10000000;
	if (n<=8)
	{
		n <<=4;
		lev += 0x4000000;
	}
	if (n<=32)
	{
		n <<=2;
		lev += 0x2000000;
	}
	if (n<=64)
	{
		n <<=1;
		lev += 0x1000000;
	}
	// n = 65...128
	// n-64 = 1...64
	// 128-n = 63...0
	n = 128-n;
	// n = 0xFC0000...0
	return (lev + ((int)n<<18));
}

void oscillator::init()
{
	env.init();
	pos=0;
	freq=0;
    wave=0;
	
	fc.freq=0;
	fc.fine=0;
	fc.note=0;

	vc.osc=0x10000000;
	vc.chan=0x10000000;
	vc.algo=0x11000000;

}

void filter::init()
{
	env.init();
	cutoff_cache = 0.5;
	cutoff_bias = 0;
	cutoff = 120;
	kf = 0;
	note_adj = 0;
	resonance = 1;
	o1=o2=0;	
}

void filter::stop()
{
	env.stop();
}

void filter::tick(int sr)
{
    /**/    /**/    /**/    /**/    /**/    /**/    
/**/    /**/    /**/    /**/    /**/    /**/    /**/ 
    /**/    /**/    /**/    /**/    /**/    /**/    
	
	float cutoff_target;

	if (fv.cutoff != 0xff)
		cutoff = fv.cutoff;
	if (fv.reso != 0xff)
		resonance = pow(0.5,(fv.reso+10)/14.0);

	if (fv.kf != 0xff)
		kf = fv.kf;

	if (fv.env != 0xff)
		env.p.volume = env.calc_level(fv.env);
	
	if (fv.a != 0xff)
		env.p.a = fv.a;
	if (fv.d != 0xff)
		env.p.d = fv.d;
	if (fv.s != 0xff)
		env.p.s = fv.s;
	if (fv.r != 0xff)
		env.p.r = fv.r;

	if (tv.note != zzub::note_value_none)
	if (tv.note != zzub::note_value_off)
	{
		env.on(sr/2750);
		note_adj = ((tv.note>>4)-5)*16.0 + (tv.note%16)/12.0*16.0;
	}

	cutoff_target = pow(0.5,(118-(cutoff + note_adj*kf/128.0) )/16.0);
	cutoff_bias = *((int *)(&cutoff_target)) - 0x3f800000;
	cutoff_bias >>= 1;
	
	if (tv.note == zzub::note_value_off)
		env.off();	

}

inline void filter::minitick()
{
	int accum=0;

	accum -= env.out;

	accum -= cutoff_bias;

	if(accum>=0x8000000)
		cutoff_cache=0;
	if(accum<0)
		cutoff_cache=1;
	else
		cutoff_cache=((0x800000-(accum&0x3fffff))>>(accum>>22))*0.06125*0.06125 *0.06125*0.06125 *0.06125 *0.125;

}

inline float filter::generate(float input)
{
	float t;
	
	t = o1 * (1-cutoff_cache*resonance);
	if(t>32768)
		t=32768;
	if(t<-32768)
		t=-32768;
	o1 = t + (input - o2)*cutoff_cache;
	o2 = o2 * (1-cutoff_cache*resonance) + o1*cutoff_cache;

	return(o2);
}

void oscillator::stop()
{
	env.stop();
}


void oscillator::tick(int algo, int oscnum, int sr)
{


	if (ov.wave != 0xff)
		wave = ov.wave-1;

	if (ov.freq != 0xff)
		fc.freq = ov.freq;
	if (ov.fine != 0xff)
		fc.fine = ov.fine;

	if (ov.vol != 0xff)
		vc.osc = ((64-ov.vol)<<21);
	
	if (ov.a != 0xff)
		env.p.a = ov.a;
	if (ov.d != 0xff)
		env.p.d = ov.d;
	if (ov.s != 0xff)
		env.p.s = ov.s;
	if (ov.r != 0xff)
		env.p.r = ov.r;
	
	if (tv.note != zzub::note_value_none)
	if (tv.note != zzub::note_value_off)
	{
		env.on(sr/2750);
		fc.note = pow(2,(tv.note>>4)-5+((tv.note%16)-10.0)/12.0 )*440.0/sr;
		pos=0;
		vc.chan=0;
	}
	
	if (tv.note == zzub::note_value_off)
		env.off();	

	freq = (((float)fc.freq + (float)(fc.fine)/250.0) * fc.note) * 0x10000000fLL;

	if (tv.volume != 0xff)
		vc.chan = ((64-tv.volume)<<21);
	if(!(routingcarriers[algo]&(1<<oscnum)))
		vc.chan = 0;
	vc.algo = routingcarriers[algo]&(1<<oscnum)  ?  algovol[algo]  :  0x6000000 ;

	env.p.volume = vc.osc + vc.chan + vc.algo; 
	// egvol += 1000000h: amp /= 2

	// egvol =        0h: amp = 2000000h: fm = 512 (max internal vol)
	// egvol =  6000000h: amp =   80000h: fm =  8  (max user vol, modulator)
	// egvol =  9000000h: amp =   10000h: fm =  1  (max user vol carrier algo 1:  -32768..32768)
	// egvol =  B000000h: amp =    4000h: fm = 1/4 (max user vol carrier algo 15: -8192..8191)
	// egvol =  C000000h: amp =    2000h: fm = 1/8 (~vol=1 modulator)
	// egvol =  F000000h: amp =     400h: fm = 1/64(~vol=1 carrier algo 1)
	// egvol = 11000000h: amp =     100h: fm =1/256(~vol=1 carrier algo 15)

}

void channel::init()
{
	osc[0].init();
	osc[1].init();
	osc[2].init();
	osc[3].init();
	lpf.init();
	algo=1;
	clock=1;
	clock_rate=16;
}

void channel::stop()
{
	osc[0].stop();
	osc[1].stop();
	osc[2].stop();
	osc[3].stop();
	lpf.stop();
}

inline void channel::minitick()
{
	if(!--clock)
	{
		clock=clock_rate;
		osc[3].env.work();
		osc[2].env.work();
		osc[1].env.work();
		osc[0].env.work();
		lpf.env.work();
		lpf.minitick();
	}
	osc[3].env.ramp_out += osc[3].env.ramp;
	osc[2].env.ramp_out += osc[2].env.ramp;
	osc[1].env.ramp_out += osc[1].env.ramp;
	osc[0].env.ramp_out += osc[0].env.ramp;
}

inline int channel::generate1()
{
	    unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        t =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        t =(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate2()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t+=(waveforms[osc[2].wave][   osc[2].pos >>20]*osc[2].env.ramp_out);
        t =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        t =(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate3()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        t+=(waveforms[osc[1].wave][   osc[1].pos >>20]*osc[1].env.ramp_out);
        t =(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate4()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t+=(waveforms[osc[2].wave][   osc[2].pos >>20]*osc[2].env.ramp_out);
        t+=(waveforms[osc[1].wave][   osc[1].pos >>20]*osc[1].env.ramp_out);
        t =(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate5()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        r =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r+=(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r =(waveforms[osc[0].wave][(r+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate6()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t+=(waveforms[osc[2].wave][   osc[2].pos >>20]*osc[2].env.ramp_out);
        r =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r+=(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate7()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r+=(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate8()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        r =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r =(waveforms[osc[1].wave][(r+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r+=(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate9()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        r =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r+=(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r+=(waveforms[osc[0].wave][(t+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate10()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r =(waveforms[osc[1].wave][   osc[1].pos >>20]*osc[1].env.ramp_out);
        r =(waveforms[osc[0].wave][(r+osc[0].pos)>>20]*osc[0].env.ramp_out);
	return(t+r);
}

inline int channel::generate11()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        t =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        t+=(waveforms[osc[0].wave][   osc[0].pos >>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate12()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t+=(waveforms[osc[2].wave][   osc[2].pos >>20]*osc[2].env.ramp_out);
        t =(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        t+=(waveforms[osc[0].wave][   osc[0].pos >>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate13()
{
        unsigned int t,r;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        r =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        r+=(waveforms[osc[1].wave][(t+osc[1].pos)>>20]*osc[1].env.ramp_out);
        r+=(waveforms[osc[0].wave][   osc[0].pos >>20]*osc[0].env.ramp_out);
	return(r);
}

inline int channel::generate14()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t =(waveforms[osc[2].wave][(t+osc[2].pos)>>20]*osc[2].env.ramp_out);
        t+=(waveforms[osc[1].wave][   osc[1].pos >>20]*osc[1].env.ramp_out);
        t+=(waveforms[osc[0].wave][   osc[0].pos >>20]*osc[0].env.ramp_out);
	return(t);
}

inline int channel::generate15()
{
        unsigned int t;
	minitick();
	osc[3].pos+=osc[3].freq;
	osc[2].pos+=osc[2].freq;
	osc[1].pos+=osc[1].freq;
	osc[0].pos+=osc[0].freq;
        t =(waveforms[osc[3].wave][   osc[3].pos >>20]*osc[3].env.ramp_out);
        t+=(waveforms[osc[2].wave][   osc[2].pos >>20]*osc[2].env.ramp_out);
        t+=(waveforms[osc[1].wave][   osc[1].pos >>20]*osc[1].env.ramp_out);
        t+=(waveforms[osc[0].wave][   osc[0].pos >>20]*osc[0].env.ramp_out);
	return(t);
}


void channel::Work(float *psamples, int numsamples)
{
	int j;

	switch(algo)
	{
	case 1:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate1()*0.0000152587890625f);
		break;
	case 2:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate2()*0.0000152587890625f);
		break;
	case 3:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate3()*0.0000152587890625f);
		break;
	case 4:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate4()*0.0000152587890625f);
		break;
	case 5:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate5()*0.0000152587890625f);
		break;
	case 6:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate6()*0.0000152587890625f);
		break;
	case 7:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate7()*0.0000152587890625f);
		break;
	case 8:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate8()*0.0000152587890625f);
		break;
	case 9:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate9()*0.0000152587890625f);
		break;
	case 10:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate10()*0.0000152587890625f);
		break;
	case 11:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate11()*0.0000152587890625f);
		break;
	case 12:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate12()*0.0000152587890625f);
		break;
	case 13:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate13()*0.0000152587890625f);
		break;
	case 14:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate14()*0.0000152587890625f);
		break;
	case 15:
		for(j=0;j<numsamples;j++)
			psamples[j] += lpf.generate((float)generate15()*0.0000152587890625f);
		break;
	case 16:
		break;
	}

}

bool channel::isactive()
{
	if((routingcarriers[algo]&1) && (osc[0].env.s.mode!=ENV_END)) return true;
	if((routingcarriers[algo]&2) && (osc[1].env.s.mode!=ENV_END)) return true;
	if((routingcarriers[algo]&4) && (osc[2].env.s.mode!=ENV_END)) return true;
	if((routingcarriers[algo]&8) && (osc[3].env.s.mode!=ENV_END)) return true;
	return false;
}

void channel::tick(int sr)
{

	clock_rate=sr/2750;

	if (gv.routing != 0xff)
		algo = gv.routing;
	for (int i=0;i<4;i++)
	{
		osc[i].ov    = *(ovals *)((char *)(&gv)+1+(3-i)*8);
		osc[i].tv    = tv;
		osc[i].tick(algo,i,sr);
	}
	lpf.fv = *(fvals *)((char *)(&gv)+1+(4)*8);
	lpf.tv = tv;
	lpf.tick(sr);

	if (tv.note != zzub::note_value_none)
	if (tv.note != zzub::note_value_off)
	{
		clock = 1;
	}
}

void C4fm2f::Tick()
{
	int i;
	for (i=0;i<active_channels;i++)
	{
		channels[i].gv                    = gval;
		channels[i].tv                    = tval[i];
		channels[i].tick(pz->_master_info->samples_per_second);
	}
}

bool C4fm2f::Work(float *psamples, int numsamples, int const)
{
	int i,j;
	int flag=0;


	for (i=0;i<active_channels;i++)
		if(channels[i].isactive()) flag=1;

	if(!flag)	return false;


	for(j=0;j<numsamples;j++)
		psamples[j] = 0;



	for (i=0;i<active_channels;i++)
	{
		if(channels[i].isactive())
			channels[i].Work(psamples,numsamples);
	}


	

	return true;
}

char const * C4fm2f::describe_value(int const param, int const value)
{
	static char txt[50];
	switch(param){
	case 0: // Algo
		switch(value)
		{
		case 0:
			sprintf(txt,"Bug!"); 
			break;
		case 1:
			sprintf(txt,"4->3->2->1"); 
			break;
		case 2:
			sprintf(txt,"3+4 -> 2 -> 1"); 
			break;
		case 3:
			sprintf(txt,"2+(4->3) -> 1"); 
			break;
		case 4:
			sprintf(txt,"2+3+4 -> 1"); 
			break;
		case 5:
			sprintf(txt,"4 -> 2+3 -> 1"); 
			break;
		case 6:
			sprintf(txt,"3+4 -> 1+2"); 
			break;
		case 7:
			sprintf(txt,"4 -> 3 -> 1+2"); 
			break;
		case 8:
			sprintf(txt,"4 -> 1+(3->2)"); 
			break;
		case 9:
			sprintf(txt,"4 -> 1+2+3"); 
			break;
		case 10:
			sprintf(txt,"2->1 + 4->3"); 
			break;
		case 11:
			sprintf(txt,"1 + 4->3->2"); 
			break;
		case 12:
			sprintf(txt,"1 + (3+4)->2"); 
			break;
		case 13:
			sprintf(txt,"1 + 4->(2+3)"); 
			break;
		case 14:
			sprintf(txt,"1 + 2 + 4->3"); 
			break;
		case 15:
			sprintf(txt,"1 + 2 + 3 + 4"); 
			break;
		case 16:
			sprintf(txt,"%x %x",channels[0].osc[0].env.p.volume,channels[0].osc[0].env.p.volume); 
			//pCB->MessageBox(txt);
			break;
		default:
			sprintf(txt,"Bug!"); 
			break;
		}
		return txt;
	case 1: // Waveform
	case 9:
	case 17:
	case 25:

		switch(value)
		{
		case 0:
			sprintf(txt,"Bug!"); 
			break;
		case 1:
			sprintf(txt,"Sine"); 
			break;
		case 2:
			sprintf(txt,"Half-sine"); 
			break;
		case 3:
			sprintf(txt,"Abs-sine"); 
			break;
		case 4:
			sprintf(txt,"Alt-sine"); 
			break;
		case 5:
			sprintf(txt,"Camel-sine"); 
			break;
		case 6:
			sprintf(txt,"Sawed-sine"); 
			break;
		case 7:
			sprintf(txt,"Squared-sine"); 
			break;
		case 8:
			sprintf(txt,"Duty-sine"); 
			break;
		case 9:
			sprintf(txt,"Fb-even"); 
			break;
		case 10:
			sprintf(txt,"Fb-odd"); 
			break;
		case 11:
			sprintf(txt,"Fb-half"); 
			break;
		case 12:
			sprintf(txt,"Square"); 
			break;
		case 13:
			sprintf(txt,"Square 2"); 
			break;
		case 14:
			sprintf(txt,"Saw"); 
			break;
		case 15:
			sprintf(txt,"Saw 2"); 
			break;
		case 16:
			sprintf(txt,"Triangle"); 
			break;
//		case 17:
//			sprintf(txt,"%x %x",channels[0].osc[0].env.p.volume,channels[0].osc[0].env.p.volume); 
//			pCB->MessageBox(txt);
//			break;
		default:
			sprintf(txt,"Bug!"); 
			break;
		}
		return txt;

	case 2: // Freq
	case 10:
	case 18:
	case 26:
		sprintf(txt,"%d",value); 
		return txt;
	case 3: // Fine
	case 11:
	case 19:
	case 27:
		sprintf(txt,"+%.3f",(double)(value)/250.0); 
		return txt;
	case 4: // Volume
	case 12:
	case 20:
	case 28:
		sprintf(txt,"%d",value);
		return txt;
	case 5: // A
	case 13:
	case 21:
	case 29:
	case 37:
		return speednames[value];
	case 6: // D
	case 14:
	case 22:
	case 30:
	case 38:
		return speednames[value];
	case 7: // S
	case 15:
	case 23:
	case 31:
	case 39:
		if(value>=32)
			sprintf(txt,"%d,sus",value-32); 
		else
			sprintf(txt,"%d,no sus",value); 
		return txt;
	case 8: // R
	case 16:
	case 24:
	case 32:
	case 40:
		return speednames[value];
	case 33: // Cutoff
	case 34: // Resonance
	case 35: // KF
	case 36: // Envelope
		sprintf(txt,"%d",value);
		return txt;


	case 42: // Volume
		sprintf(txt,"%d",value); 
		return txt;
	case 41: // Note
		sprintf(txt,"Note"); 
		return txt;
	default:
		sprintf(txt,"Bug!"); 
		return txt;
	}
}

void C4fm2f::stop()
{
	int i;
	for (i=0;i<MAX_CHANNELS;i++)
		channels[i].stop();
}

void C4fm2f::set_track_count(int const n)
{
	int i;

	if(n<active_channels)
		for(i=n;i<active_channels;i++)
			channels[i].stop();

	else
		for(i=active_channels;i<n;i++)
		{
			channels[i].init();
			channels[i]=channels[0];
			channels[i].stop();
		}
	active_channels = n;
}

void C4fm2f::process_events()
{
	int i;
	for (i=0;i<active_channels;i++)
	{
		channels[i].gv = gval;
		channels[i].tv = tval[i];
		channels[i].tick(_master_info->samples_per_second);
	}
}

void C4fm2f::init(zzub::archive *)
{
	int i;
	for (i=0;i<active_channels;i++)
		channels[i].init();
}


bool C4fm2f::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
  if (mode != zzub::process_mode_write)
    return false;

  bool retval = Work(pout[0], numsamples, mode);
  for (int i = 0; i < numsamples; i++) {
    pout[0][i] *= downscale;
    pout[1][i] = pout[0][i];
  }
  return retval;
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

struct C4fm2f_plugin_info : zzub::info {
  C4fm2f_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_output | zzub::plugin_flag_is_instrument;
    this->min_tracks = 1;
    this->max_tracks = MAX_CHANNELS;
    this->name = "Madbrain's 4fm2f";
    this->short_name = "4fm2f";
    this->author = "Madbrain";
    this->uri = "jamesmichaelmcdermott@gmail.com/generator/4fm2f;1";

    paraRouting = &add_global_parameter()
      .set_byte()
      .set_name("Routing--")
      .set_description("Routing/Algorithm")
      .set_value_min(1)
      .set_value_max(15)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);

    paraOsc4Waveform = &add_global_parameter()
      .set_byte()
      .set_name("Osc4---Wave")
      .set_description("Osc4: Waveform")
      .set_value_min(1)
      .set_value_max(16)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc4Frequency = &add_global_parameter()
      .set_byte()
      .set_name("        |--Freq")
      .set_description("Osc4: Frequency")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc4Finetune = &add_global_parameter()
      .set_byte()
      .set_name("        |--Fine")
      .set_description("Osc4: Finetune")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraOsc4Volume = &add_global_parameter()
      .set_byte()
      .set_name("        |--Volume")
      .set_description("Osc4: Volume")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc4Attack = &add_global_parameter()
      .set_byte()
      .set_name("        |--A")
      .set_description("Osc4: Attack")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc4Decay = &add_global_parameter()
      .set_byte()
      .set_name("        |--D")
      .set_description("Osc4: Decay")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    paraOsc4Sustain = &add_global_parameter()
      .set_byte()
      .set_name("        |--S")
      .set_description("Osc4: Sustain")
      .set_value_min(0)
      .set_value_max(63)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(31);
    paraOsc4Release = &add_global_parameter()
      .set_byte()
      .set_name("        |--R")
      .set_description("Osc4: Release")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);

    paraOsc3Waveform = &add_global_parameter()
      .set_byte()
      .set_name("Osc3---Wave")
      .set_description("Osc3: Waveform")
      .set_value_min(1)
      .set_value_max(16)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc3Frequency = &add_global_parameter()
      .set_byte()
      .set_name("        |--Freq")
      .set_description("Osc3: Frequency")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc3Finetune = &add_global_parameter()
      .set_byte()
      .set_name("        |--Fine")
      .set_description("Osc3: Finetune")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraOsc3Volume = &add_global_parameter()
      .set_byte()
      .set_name("        |--Volume")
      .set_description("Osc3: Volume")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc3Attack = &add_global_parameter()
      .set_byte()
      .set_name("        |--A")
      .set_description("Osc3: Attack")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc3Decay = &add_global_parameter()
      .set_byte()
      .set_name("        |--D")
      .set_description("Osc3: Decay")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    paraOsc3Sustain = &add_global_parameter()
      .set_byte()
      .set_name("        |--S")
      .set_description("Osc3: Sustain")
      .set_value_min(0)
      .set_value_max(63)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(31);
    paraOsc3Release = &add_global_parameter()
      .set_byte()
      .set_name("        |--R")
      .set_description("Osc3: Release")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);

    paraOsc2Waveform = &add_global_parameter()
      .set_byte()
      .set_name("Osc2---Wave")
      .set_description("Osc2: Waveform")
      .set_value_min(1)
      .set_value_max(16)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc2Frequency = &add_global_parameter()
      .set_byte()
      .set_name("        |--Freq")
      .set_description("Osc2: Frequency")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc2Finetune = &add_global_parameter()
      .set_byte()
      .set_name("        |--Fine")
      .set_description("Osc2: Finetune")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraOsc2Volume = &add_global_parameter()
      .set_byte()
      .set_name("        |--Volume")
      .set_description("Osc2: Volume")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc2Attack = &add_global_parameter()
      .set_byte()
      .set_name("        |--A")
      .set_description("Osc2: Attack")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc2Decay = &add_global_parameter()
      .set_byte()
      .set_name("        |--D")
      .set_description("Osc2: Decay")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    paraOsc2Sustain = &add_global_parameter()
      .set_byte()
      .set_name("        |--S")
      .set_description("Osc2: Sustain")
      .set_value_min(0)
      .set_value_max(63)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(31);
    paraOsc2Release = &add_global_parameter()
      .set_byte()
      .set_name("        |--R")
      .set_description("Osc2: Release")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);

    paraOsc1Waveform = &add_global_parameter()
      .set_byte()
      .set_name("Osc1---Wave")
      .set_description("Osc1: Waveform")
      .set_value_min(1)
      .set_value_max(16)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc1Frequency = &add_global_parameter()
      .set_byte()
      .set_name("        |--Freq")
      .set_description("Osc1: Frequency")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(1);
    paraOsc1Finetune = &add_global_parameter()
      .set_byte()
      .set_name("        |--Fine")
      .set_description("Osc1: Finetune")
      .set_value_min(0)
      .set_value_max(0xfe)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraOsc1Volume = &add_global_parameter()
      .set_byte()
      .set_name("        |--Volume")
      .set_description("Osc1: Volume")
      .set_value_min(0)
      .set_value_max(64)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc1Attack = &add_global_parameter()
      .set_byte()
      .set_name("        |--A")
      .set_description("Osc1: Attack")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraOsc1Decay = &add_global_parameter()
      .set_byte()
      .set_name("        |--D")
      .set_description("Osc1: Decay")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    paraOsc1Sustain = &add_global_parameter()
      .set_byte()
      .set_name("        |--S")
      .set_description("Osc1: Sustain")
      .set_value_min(0)
      .set_value_max(63)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(31);
    paraOsc1Release = &add_global_parameter()
      .set_byte()
      .set_name("        |--R")
      .set_description("Osc1: Release")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);

    paraLpfCutoff = &add_global_parameter()
      .set_byte()
      .set_name("Lpf---Cutoff")
      .set_description("Lpf: Cutoff")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x6c);
    paraLpfResonance = &add_global_parameter()
      .set_byte()
      .set_name("Lpf---Reso")
      .set_description("Lpf: Resonance")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x0);
    paraLpfKeyFollow = &add_global_parameter()
      .set_byte()
      .set_name("Lpf---KF")
      .set_description("Lpf: Key Follow")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLpfEnvelope = &add_global_parameter()
      .set_byte()
      .set_name("Lpf---Env")
      .set_description("Lpf: Envelope")
      .set_value_min(0)
      .set_value_max(0x80)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraLpfAttack = &add_global_parameter()
      .set_byte()
      .set_name("        |--A")
      .set_description("Lpf: Attack")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(32);
    paraLpfDecay = &add_global_parameter()
      .set_byte()
      .set_name("        |--D")
      .set_description("Lpf: Decay")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(8);
    paraLpfSustain = &add_global_parameter()
      .set_byte()
      .set_name("        |--S")
      .set_description("Lpf: Sustain")
      .set_value_min(0)
      .set_value_max(63)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(31);
    paraLpfRelease = &add_global_parameter()
      .set_byte()
      .set_name("        |--R")
      .set_description("Lpf: Release")
      .set_value_min(0)
      .set_value_max(32)
      .set_value_none(0xff)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(16);


    paraNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note")
      .set_value_min(zzub::note_value_min)
      .set_value_max(zzub::note_value_max)
      .set_value_none(zzub::note_value_none)
      .set_flags(zzub::parameter_flag_event_on_edit) // not in original -- jmmcd
      .set_value_default(0x80);
    paraVolume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("00-40:Volume")
      .set_value_min(0)
      .set_value_max(0x40)
      .set_value_none(0xff)
      .set_flags(0)
      .set_value_default(0x40);

  } 
  virtual zzub::plugin* create_plugin() const { return new C4fm2f(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} madbrain_4fm2f_info;

struct C4fm2fplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&madbrain_4fm2f_info);
  }
  
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
  virtual void destroy() { delete this; }
  // Returns the uri of the collection to be identified,
  // return zero for no uri. Collections without uri can not be 
  // configured.
  virtual const char *get_uri() { return 0; }
  
  // Called by the host to set specific configuration options,
  // usually related to paths.
  virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new C4fm2fplugincollection();
}
