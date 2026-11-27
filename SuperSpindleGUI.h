#pragma once

#include "IPlug_include_in_plug_hdr.h"

#include "SuperSpindleDSP.h"

class SuperSpindleGUI
{
public:
  // Pédale à l'échelle 1:1 depuis (x, y)
  // slot est l'indice du début des knobs. +15 pour les LEDs
  static void Build(iplug::igraphics::IGraphics* pGraphics, float x, float y, int slot, bool displayLed);	
};