
#include "SuperSpindleDSP.h"

#include "../FMR_Library/FMR_Lib/FMR_Neural.h"
#include "../FMR_Library/FMR_NeuralDrive/FMR_SuperSpindle.h"
#include "../FMR_Library/FMR_Lib/FMR_Resampler.h"
#include "../FMR_Library/FMR_Lib/FMR_ConvPot.h"

#include <algorithm>
#include <cmath>

using iplug::sample;

namespace
{
  // Ordre = EParams du VST3 autonome (hors footswitch). Noms stables : ne jamais les renommer.
  constexpr const char* kParamNames[] = { "bass", "middle", "treble", "gain", "type", "volume", "footsw"};

  // Valeurs initiales de SuperSpindle.cpp, normalisées 0..1.
  // valeurs redondantes avec Parameters dans SuperSpindleDPS.h ! Est-ce que l'on peut se passer de l'un ou de l'autres ?
  constexpr float kParamDefaults[] = { 0.5f, 0.5f, 0.5f, 0.3f, 0, 0.5f, 1 };

  inline float Clamp01(float val) { return std::clamp(val, 0.0f, 1.0f); }
  inline int Steps(float val, int n) { return static_cast<int>(std::lround(Clamp01(val) * n)); }
}

SuperSpindleDSP::SuperSpindleDSP()
{
  mDrive = std::make_unique<ND_SuperSpindle>();
  mRate = std::make_unique<FMR_RateAdapter<sample>>();
}

SuperSpindleDSP::~SuperSpindleDSP() = default;

void SuperSpindleDSP::Reset(double sampleRate, int blockSize)
{
  // Le moteur tourne à 48 kHz ; le resampler adapte la fréquence du host.
  mRate->Init(sampleRate, std::max(blockSize, 64));
  ResetEngine();
}

void SuperSpindleDSP::ResetEngine()
{
  InitFilter(&filterBass, 10.0f, LP_1st_order, 1.0f);
  InitFilter(&filterMiddle, 10.0f, LP_1st_order, 1.0f);
  InitFilter(&filterTreble, 10.0f, LP_1st_order, 1.0f);

  InitFilter(&filterGain, 10.0f, LP_1st_order, 1.0f);
  InitFilter(&filterVolume, 10.0f, LP_1st_order, 1.0f);

  currentBass = 0.0f, currentMiddle = 0.0f, currentTreble = 0.0f;
  currentGain = 0.0f, currentVolume = 0.0f;
}

int SuperSpindleDSP::GetLatency() const
{
  return mRate->Latency();
}

void SuperSpindleDSP::SetLed(int led, float color, float value, int decay)
{
    pedalLed[led].color = color;
    pedalLed[led].value = value;
    pedalLed[led].decay = decay;
}

float SuperSpindleDSP::GetLedColor(int led)
{
    return pedalLed[led].color;
}

float SuperSpindleDSP::GetLedValue(int led)
{
    return pedalLed[led].value;
}

int SuperSpindleDSP::GetLedDecay(int led)
{
    return pedalLed[led].decay;
}

const char* SuperSpindleDSP::ParamName(int k) const
{
  return (k >= 0 && k < SrSp_kNumParams) ? kParamNames[k] : "";
}

float SuperSpindleDSP::ParamDefault(int k) const
{
  return (k >= 0 && k < SrSp_kNumParams) ? kParamDefaults[k] : 0.5f;
}

void SuperSpindleDSP::SetParam(int k, float val)
{
  switch (k)
  {
    case SrSp_kBass:	mParams.bass   = 0.001f + Clamp01(val) * 0.998f; break;
    case SrSp_kMiddle:	mParams.middle = 0.001f + Clamp01(val) * 0.998f; break;
    case SrSp_kTreble:	mParams.treble = 0.001f + Clamp01(val) * 0.998f; break;

    case SrSp_kGain:	mParams.gain = 0.1f + Clamp01(val) * (0.999f - 0.1f); break;
    case SrSp_kType:	mParams.type = Steps(val, 2); break;
    case SrSp_kVolume:	mParams.volume = PotCurve_0_23[static_cast<unsigned short int>(Steps(val, 4095))]; break;

    case SrSp_kFootSw:	mParams.footSw = Steps(val, 1); break;
    default: break;
  }
}

void SuperSpindleDSP::ProcessBlock( sample** inputs, sample** outputs, int nFrames, int nChans )
{
  if (mRate->Active())
  {
    // Host -> 48 kHz -> moteur -> host, par tranches <= MaxBlock().
    for (int done = 0; done < nFrames; )
    {
      const int n = std::min(nFrames - done, mRate->MaxBlock());

      sample* in[ND_SUPER_SPINDLE_NB_CHANNELS];
      sample* out[ND_SUPER_SPINDLE_NB_CHANNELS];

      for (int c = 0; c < nChans; c++)
      {
        in[c] = inputs[c] + done;
        out[c] = outputs[c] + done;
      }

      const int n48 = mRate->ToEngine(in, n, nChans);
      ProcessEngine(mRate->Planar(), mRate->Planar(), n48, nChans, mParams);
      mRate->FromEngine(n48, out, n, nChans);

      done += n;
    }
  }
  else
  {
    ProcessEngine(inputs, outputs, nFrames, nChans, mParams);
  }
}

void SuperSpindleDSP::levelColorLed(int led, float level, float fullScale, float greenAt, float orangeAt)
{
  
  const float normalized = std::clamp(level / fullScale, 0.0f, 1.0f);
  
  if (normalized < 0.005f)	pedalLed[led].value = 0.0f;	// éteinte
  else					    pedalLed[led].value = 1.0f;	// allumée

  if (normalized < greenAt)			pedalLed[led].color = LedVerte;
  else if (normalized < orangeAt)	pedalLed[led].color = LedOrange;
  else								pedalLed[led].color = LedRouge;

  if( displayedLed )	pedalLed[led].decay = LedDecay;
  else  				pedalLed[led].decay = 0;
}

void SuperSpindleDSP::ProcessEngine(sample** inputs, sample** outputs, int nFrames, int nChans, const Parameters& params)
{
  float inputPeak = 0.0f;
  float outputPeak = 0.0f;

  mDrive->Request(params.type);

  for (int s = 0; s < nFrames; s++)
  {
	currentBass = Filter1st(&filterBass, params.bass);
	currentMiddle = Filter1st(&filterMiddle, params.middle);
	currentTreble = Filter1st(&filterTreble, params.treble);

	currentGain = Filter1st(&filterGain, params.gain);
	currentVolume = Filter1st(&filterVolume, params.volume);

	mDrive->Tick();
	mDrive->SetEQ(currentBass, currentMiddle, currentTreble);
	  
    for (int c = 0; c < nChans; c++)
    {
      if( params.footSw == 1 )
	  {
        SetLed(SrSp_kCtrlLED, LedRouge, 1.0f, 0);
	    const float input = inputs[c][s];
        inputPeak = std::max(inputPeak, std::fabs(input));
	    levelColorLed(SrSp_kCtrlInputLED, inputPeak, 4.5f, moins9dB, moins3dB);

		const float output = currentVolume * mDrive->Process(c, (float)std::clamp(input, -4.5f, 4.5f), currentGain);
	    
        outputPeak = std::max(outputPeak, std::fabs(output));
	    levelColorLed(SrSp_kCtrlOutputLED, outputPeak, 2.0f, moins9dB, moins3dB);

	  	outputs[c][s] = output;
	  }
      else
      {
        outputs[c][s] = inputs[c][s];
        SetLed(SrSp_kCtrlLED, LedRouge, 0.0f, 0);
        SetLed(SrSp_kCtrlInputLED, LedVerte, 0.0f, 0);
        SetLed(SrSp_kCtrlOutputLED, LedVerte, 0.0f, 0);
      }
    }
  }
}
