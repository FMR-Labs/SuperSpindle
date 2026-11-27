#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "SuperSpindleDSP.h"

using namespace iplug;
using namespace igraphics;

const int kNumPresets = 1;

class SuperSpindle final : public Plugin
{
public:
  SuperSpindle(const InstanceInfo& info);
  ~SuperSpindle() override = default;

  void OnIdle() override;

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;

private:
  SuperSpindleDSP mDSP;
#endif
};
