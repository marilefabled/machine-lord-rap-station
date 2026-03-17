#pragma once
#include <JuceHeader.h>

class FactoryLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FactoryLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange.withAlpha(0.6f));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey.withAlpha(0.2f));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222222));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
        setColour(juce::Label::textColourId, juce::Colours::orange.withAlpha(0.8f));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override {
        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 4.0f;
        auto arcRadius = radius - lineW * 0.5f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(outline);
        g.setOpacity(0.3f);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0) {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(fill);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        auto thumbWidth = lineW * 2.0f;
        juce::Path thumb;
        thumb.addRectangle(-thumbWidth * 0.5f, -radius, thumbWidth, radius * 0.4f);
        g.setColour(juce::Colours::white);
        g.fillPath(thumb, juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto color = backgroundColour;
        if (shouldDrawButtonAsHighlighted) color = color.brighter(0.2f);
        if (shouldDrawButtonAsDown) color = color.darker(0.2f);

        g.setColour(color);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(button.findColour(juce::ComboBox::outlineColourId).withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
};
