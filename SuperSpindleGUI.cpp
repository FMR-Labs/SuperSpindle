
#include "SuperSpindleGUI.h"
#include "IControls.h"
#include "../FMR_Library/FMR_Pedal/PedalFactory.h"

void SuperSpindleGUI::Build(iplug::igraphics::IGraphics* pGraphics, float x, float y, int slot, bool displayLed)
{
	using namespace iplug::igraphics;

	const IBitmap knobBitmap = pGraphics->LoadBitmap(KNOB_FN, 101);
	const IBitmap switchBitmap = pGraphics->LoadBitmap(SWITCH_FN, 3);
	const IBitmap footswBitmap = pGraphics->LoadBitmap(FOOTSW_FN, 2);
	const IBitmap boitierBitmap = pGraphics->LoadBitmap(BOITIER_FN, 1);

	const IBitmap fondBitmap = pGraphics->LoadBitmap(SUPERSPINDLE_FN, 1);

	auto at = [&](const IRECT& r) { return r.GetTranslated(x, y); };

	//  x /  y     
	// 42 / 81		126 / 81	210 / 81
	// 42 / 200		126 / 200	210 / 200
	//				126 / 337
	//				126 / 415

	// 49 / 129		133 / 129	217 / 129
	// 49 / 248		133 / 248	217 / 248
	//				133 / 385
	//				133 / 463

	const IRECT boitier = at(IRECT(0, 0, 266, 535));
	const IRECT ledCtrl = at(IRECT(84, 337, 182, 435));
	const IRECT faceAvant = at(IRECT(7, 48, 259, 528));

	const IRECT bassKnob = at(IRECT(0, 80, 98, 178));
	const IRECT middleKnob = at(IRECT(84, 80, 182, 178));
	const IRECT trebleKnob = at(IRECT(165, 80, 263, 178));

	const IRECT gainKnob = at(IRECT(0, 199, 98, 297));
	const IRECT typeSwitch = at(IRECT(84, 199, 182, 297));
	const IRECT volumeKnob = at(IRECT(165, 199, 263, 297));
	const IRECT volumeLed = at(IRECT(145, 179, 283, 317));

	const IRECT footSwitch = at(IRECT(84, 414, 182, 512));
	const IRECT inputLed = at(IRECT(64, 394, 202, 532));
	
	pGraphics->AttachControl(new IBitmapControl(boitier, boitierBitmap));
	pGraphics->AttachControl(new iplug::igraphics::ILEDControl(ledCtrl, LedRouge), slot + SrSp_kCtrlLED + kNumStartLED);
	pGraphics->AttachControl(new IBitmapControl(faceAvant, fondBitmap));

	pGraphics->AttachControl(new IBKnobControl(bassKnob, knobBitmap, slot + SrSp_kBass));
	pGraphics->AttachControl(new IBKnobControl(middleKnob, knobBitmap, slot + SrSp_kMiddle));
	pGraphics->AttachControl(new IBKnobControl(trebleKnob, knobBitmap, slot + SrSp_kTreble));

	pGraphics->AttachControl(new IBKnobControl(gainKnob, knobBitmap, slot + SrSp_kGain));
	pGraphics->AttachControl(new IBKnobControl(typeSwitch, switchBitmap, slot + SrSp_kType));
//	if( displayLed )	pGraphics->AttachControl(new iplug::igraphics::ILEDControl(volumeLed, LedVerte), slot + SrSp_kCtrlOutputLED + kNumStartLED);
	pGraphics->AttachControl(new IBKnobControl(volumeKnob, knobBitmap, slot + SrSp_kVolume));

	if( displayLed )	pGraphics->AttachControl(new iplug::igraphics::ILEDControl(inputLed, LedVerte), slot + SrSp_kCtrlInputLED + kNumStartLED);
	pGraphics->AttachControl(new IBSwitchControl(footSwitch, footswBitmap, slot + SrSp_kFootSw));
}

PEDAL_FACTORY(SuperSpindle, SuperSpindleDSP, SuperSpindleGUI)