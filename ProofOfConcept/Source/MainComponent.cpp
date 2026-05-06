#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(800, 600);

    if (!connect(9000)) {
        juce::Logger::outputDebugString("BLAD: Nie udalo sie otworzyc portu 9000!");
    }
    else {
        juce::Logger::outputDebugString("SUKCES: Port 9000 otwarty poprawnie!");
    }

    addListener(this, "/fallout/player/rotation");

    // sender.connect("127.0.0.1", 9000); 
    // startTimer(500);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = float(sampleRate);
    updatePhaseDelta();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    // --- ZABEZPIECZENIE PRZED "ZAWIESZONĄ NUTĄ" ---
    // Zliczamy próbki. Jeśli gra nic nie wyśle przez ponad 100ms, wymuszamy ciszę.
    samplesWithoutMessage += bufferToFill.numSamples;
    if (samplesWithoutMessage > currentSampleRate * 0.1f) {
        targetVolume = 0.0f;
    }

    for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        // --- PROSTY SYSTEM ADSR (Attack / Decay) ---
        // Płynnie zbliżamy currentVolume do targetVolume, żeby nie było trzasków
        if (currentVolume < targetVolume) {
            currentVolume += 0.001f; // Szybki atak
        }
        else if (currentVolume > targetVolume) {
            currentVolume -= 0.00005f; // Powolne wybrzmienie ("ogon")
            if (currentVolume < 0.0f) currentVolume = 0.0f; // Zabezpieczenie przed ujemną głośnością
        }
        // -------------------------------------------

        // Mnożymy sinusoidę przez naszą dynamiczną głośność
        float sampleValue = std::sin(currentPhase) * currentVolume;

        leftBuffer[sample] = sampleValue;
        rightBuffer[sample] = sampleValue;
        currentPhase += phaseDelta;

        if (currentPhase >= 2.0f * juce::MathConstants<float>::pi)
            currentPhase -= 2.0f * juce::MathConstants<float>::pi;
    }
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
}

//==============================================================================
void MainComponent::oscMessageReceived(const juce::OSCMessage& message)
{
    if (message.size() == 1 && message[0].isFloat32())
    {
        float incomingData = message[0].getFloat32();

        juce::Logger::outputDebugString("ROTACJA Z GRY: " + juce::String(incomingData));

        // Porównujemy starą klatkę z nową. Ruch myszką powoduje zmianę rotacji.
        if (std::abs(incomingData - lastRotation) > 0.001f)
        {
            targetVolume = 0.5f; // Włączamy głośność na 50%

            // Prawdziwa rotacja to zazwyczaj przedział 0 - 6.28 (radiany). 
            // Mapujemy to na częstotliwość od 200 Hz do ok. 1140 Hz.
            frequency = 200.0f + (incomingData * 150.0f);

            updatePhaseDelta();
        }
        else
        {
            targetVolume = 0.0f; // Postać stoi w miejscu = cisza
        }

        lastRotation = incomingData;
        samplesWithoutMessage = 0; // RESETUJEMY BEZPIECZNIK, bo dotarł nowy pakiet!
    }
}

//==============================================================================
void MainComponent::timerCallback()
{
    // Testowy timer (wyłączony w konstruktorze)
    sender.send("/fallout/player/rotation", testHealth);

    testHealth -= 10.0f;
    if (testHealth < 0.0f)
        testHealth = 100.0f;
}