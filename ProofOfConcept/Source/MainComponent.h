#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
	float currentPhase = 0.0f; // Current point in the wave
	float phaseDelta = 0.0f; // How much to advance the phase for each sample
	float frequency = 440.0f; // Frequency of the sine wave
	float currentSampleRate = 44100.0f; // Sample rate of the audio device

    // How many cycles of the wave we go through per sample
    void updatePhaseDelta()
    {
		auto cyclesPerSample = frequency / currentSampleRate;
		phaseDelta = cyclesPerSample * 2.0f * juce::MathConstants<float>::pi;
	}


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
