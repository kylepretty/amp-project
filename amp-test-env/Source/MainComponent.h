#pragma once

#include <JuceHeader.h>

class MainComponent : public juce::Component,
    public juce::Slider::Listener
{
public:
    MainComponent()
    {
        // Initialize gain sliders
        for (auto* slider : { &lowSlider, &midSlider, &highSlider })
        {
            addAndMakeVisible(slider);
            slider->setRange(-24.0, 24.0, 0.1);
            slider->setValue(0.0);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
            slider->addListener(this);
        }

        // Initialize Q sliders
        for (auto* slider : { &lowQSlider, &midQSlider, &highQSlider })
        {
            addAndMakeVisible(slider);
            slider->setRange(0.1, 10.0, 0.01);
            slider->setValue(0.707);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
            slider->addListener(this);
        }

        lowSlider.setName("Low Gain");
        midSlider.setName("Mid Gain");
        highSlider.setName("High Gain");

        lowQSlider.setName("Low Q");
        midQSlider.setName("Mid Q");
        highQSlider.setName("High Q");

        sampleRate = 44100.0;
        updateFilters();
        setSize(600, 500);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
        g.setColour(juce::Colours::white);

        auto bounds = getLocalBounds().reduced(20);
        auto graphArea = bounds.removeFromBottom(300);

        // Draw frequency response
        g.drawRect(graphArea);

        juce::Path responseCurve;
        const double maxFreq = 20000.0;
        const double minFreq = 20.0;
        const int numPoints = 200;

        for (int i = 0; i < numPoints; ++i)
        {
            double freq = minFreq * std::pow(maxFreq / minFreq, static_cast<double>(i) / (numPoints - 1));
            double magnitude = getMagnitudeForFrequency(freq);

            double x = graphArea.getX() + i * graphArea.getWidth() / static_cast<double>(numPoints);
            double y = graphArea.getCentreY() - (magnitude * graphArea.getHeight() / 48.0);

            if (i == 0)
                responseCurve.startNewSubPath(static_cast<float>(x), static_cast<float>(y));
            else
                responseCurve.lineTo(static_cast<float>(x), static_cast<float>(y));
        }

        g.setColour(juce::Colours::lime);
        g.strokePath(responseCurve, juce::PathStrokeType(2.0f));

        // Draw frequency labels
        g.setColour(juce::Colours::white);
        auto drawLabel = [&](double freq)
            {
                double proportion = std::log(freq / minFreq) / std::log(maxFreq / minFreq);
                float x = static_cast<float>(graphArea.getX() + proportion * graphArea.getWidth());
                g.drawText(juce::String(freq) + "Hz",
                    static_cast<int>(x - 20),
                    graphArea.getBottom() + 5,
                    40,
                    20,
                    juce::Justification::centred);
            };

        drawLabel(20);
        drawLabel(100);
        drawLabel(1000);
        drawLabel(10000);
        drawLabel(20000);

        // Draw labels for control sections
        g.setFont(16.0f);
        auto controlsArea = bounds.removeFromTop(120);
        g.drawText("Gain Controls", controlsArea.removeFromTop(20), juce::Justification::centredLeft);
        g.drawText("Q Controls", controlsArea.removeFromTop(60), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);

        // Gain controls
        auto gainControls = bounds.removeFromTop(60);
        gainControls.removeFromTop(20);
        lowSlider.setBounds(gainControls.removeFromLeft(180));
        midSlider.setBounds(gainControls.removeFromLeft(180));
        highSlider.setBounds(gainControls.removeFromLeft(180));

        // Q controls
        auto qControls = bounds.removeFromTop(60);
        qControls.removeFromTop(20);
        lowQSlider.setBounds(qControls.removeFromLeft(180));
        midQSlider.setBounds(qControls.removeFromLeft(180));
        highQSlider.setBounds(qControls.removeFromLeft(180));
    }

    void sliderValueChanged(juce::Slider*) override
    {
        updateFilters();
        repaint();
    }

private:
    void updateFilters()
    {
        double lowFreq = 100.0;
        double midFreq = 1000.0;
        double highFreq = 5000.0;

        // Create filter coefficients
        *lowFilter.coefficients = *juce::dsp::IIR::Coefficients<double>::makeLowPass(
            sampleRate, lowFreq, lowQSlider.getValue());

        *midFilter.coefficients = *juce::dsp::IIR::Coefficients<double>::makeBandPass(
            sampleRate, midFreq, midQSlider.getValue());

        *highFilter.coefficients = *juce::dsp::IIR::Coefficients<double>::makeHighPass(
            sampleRate, highFreq, highQSlider.getValue());

        lowGain = std::pow(10.0, lowSlider.getValue() / 20.0);
        midGain = std::pow(10.0, midSlider.getValue() / 20.0);
        highGain = std::pow(10.0, highSlider.getValue() / 20.0);
    }

    double getMagnitudeForFrequency(double frequency)
    {
        const int bufferSize = 512;
        juce::AudioBuffer<double> buffer(1, bufferSize);
        double phase = 0.0;
        double phaseInc = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;

        for (int i = 0; i < bufferSize; ++i)
        {
            buffer.setSample(0, i, std::sin(phase));
            phase += phaseInc;
        }

        juce::AudioBuffer<double> lowOutput = buffer;
        juce::AudioBuffer<double> midOutput = buffer;
        juce::AudioBuffer<double> highOutput = buffer;

        // Process through each filter
        auto processFilter = [](juce::dsp::IIR::Filter<double>& filter,
            juce::AudioBuffer<double>& buffer,
            double gain)
            {
                auto* channelData = buffer.getWritePointer(0);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    channelData[i] = filter.processSample(channelData[i]) * gain;
                }
            };

        processFilter(lowFilter, lowOutput, lowGain);
        processFilter(midFilter, midOutput, midGain);
        processFilter(highFilter, highOutput, highGain);

        double summedMagnitude = 0.0;
        for (int i = 0; i < bufferSize; ++i)
        {
            double sum = lowOutput.getSample(0, i) +
                midOutput.getSample(0, i) +
                highOutput.getSample(0, i);
            summedMagnitude += sum * sum;
        }

        summedMagnitude = std::sqrt(summedMagnitude / bufferSize);
        return 20.0 * std::log10(summedMagnitude + 1e-6);
    }

    // Gain sliders
    juce::Slider lowSlider;
    juce::Slider midSlider;
    juce::Slider highSlider;

    // Q sliders
    juce::Slider lowQSlider;
    juce::Slider midQSlider;
    juce::Slider highQSlider;

    // IIR Filters
    juce::dsp::IIR::Filter<double> lowFilter;
    juce::dsp::IIR::Filter<double> midFilter;
    juce::dsp::IIR::Filter<double> highFilter;

    double sampleRate;
    double lowGain = 1.0;
    double midGain = 1.0;
    double highGain = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};