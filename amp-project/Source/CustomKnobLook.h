#pragma once
#include <JuceHeader.h>

class CustomKnobLook : public juce::LookAndFeel_V4
{
public:
    CustomKnobLook()
    {
        knobImage = juce::ImageCache::getFromMemory(
            BinaryData::KnobTexture_png,
            BinaryData::KnobTexture_pngSize);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        if (knobImage.isValid())
        {
            float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            float centerX = x + width * 0.5f;
            float centerY = y + height * 0.5f;

            // Scale image to match slider size
            float imageW = knobImage.getWidth();
            float imageH = knobImage.getHeight();
            float scale = juce::jmin(width / imageW, height / imageH);

            juce::AffineTransform t = juce::AffineTransform::rotation(angle, imageW * 0.5f, imageH * 0.5f)
                .scaled(scale)
                .translated(centerX - imageW * 0.5f * scale, centerY - imageH * 0.5f * scale);

            g.drawImageTransformed(knobImage, t);
        }
    }

private:
    juce::Image knobImage;
};
