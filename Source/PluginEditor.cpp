/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider){
    
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    auto enabled = slider.isEnabled();
    
    g.setColour(enabled ? Colour(232u, 194u, 159u) : Colours::darkgrey);
    g.fillEllipse(bounds);
    
    g.setColour(enabled ? Colour(255u, 155u, 64u) : Colours::grey);
    g.drawEllipse(bounds, 3.f);
    
    
    //FIXXXX ??? ??!!! Add under each circle the name of the property that it is changing!!!
    
    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider)){
        auto center = bounds.getCentre();
        Path p;
        
        g.setColour(enabled ? Colour(255u, 155u, 64u) : Colours::grey);
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        
        p.addRoundedRectangle(r, 2.f);
        
        //rotation stuff
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngleRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngleRad, center.getX(), center.getY()));
        
        g.fillPath(p);
        
        //text in center of slider
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto stringWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(stringWidth + 4, rswl->getTextHeight() + 2);//just so its a little bigger than the bounded box for our text
        r.setCentre(center);
        
        g.setColour(Colour()); //makes the rectangle background transparent
        g.fillRect(r);
        
        g.setColour(enabled ? Colour(232u, 92u, 26u) : Colours::grey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

void LookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){
    using namespace juce;
    
    if(auto*  pb = dynamic_cast<PowerButton*>(&toggleButton)){
        Path powerButton;
        
        auto bounds = toggleButton.getLocalBounds();
        //    g.setColour(Colours::red);
        //    g.drawRect(bounds);
        
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 7;//JUCE_LIVE_CONSTANT(6);//can use juce live constant to tweak it
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
        
        float ang = 40.f;//JUCE_LIVE_CONSTANT(30);//determines how big top gap is in the arc
        
        size -= 8;//JUCE_LIVE_CONSTANT(6);//changes inner arc
        
        powerButton.addCentredArc(r.getCentreX(), r.getCentreY(), size * 0.5, size * 0.5, 0.f, degreesToRadians(ang), degreesToRadians(360.f - ang), true);//using ang to adjust how visuals look
        
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        
        //can use strokePath, but want to use custom stuff so use path constructor
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        
        //now specify color
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(232u, 92u, 26u);
        //red-orange is active, gray is off
        
        g.setColour(color);
        g.strokePath(powerButton, pst);
        
        g.drawEllipse(r, 2.f);
    }
    else if(auto* ab = dynamic_cast<AnalyzerButton*>(&toggleButton)){
        auto color = ! toggleButton.getToggleState() ? Colours::dimgrey : Colour(232u, 194u, 159u);
        
        g.setColour(color);
        
        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        
        g.strokePath(ab->randomPath, PathStrokeType(1.f));
    }
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g){
    using namespace juce;
    
    //want starting at around 7 o'clock and ending at around 5 o'clock
    //12 o'clock is 0 deg and we're using radians
    
    auto startAngle = degreesToRadians(180.f + 45.f);
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
    
    auto space = getLocalBounds().removeFromBottom(12);
    auto labelBounds = space.removeFromTop(10);
    g.setColour(Colours::blue);
    g.drawRect(labelBounds);
    
    
    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAngle, endAngle, *this);
    //jmap maps from slider values to reg values
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour(Colour(255u, 155u, 64u));
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for(int i = 0; i < numChoices; ++i){
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAngle, endAngle);
        
        //center point
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
    //adding parameter labels for each slider
    g.setColour(Colour(232u, 194u, 159u));
    g.setFont(9);
    
    for(int i = 0; i < parameterLabels.size(); ++i){
        auto center = labelBounds.toFloat().getCentre();
        Rectangle<float> r;
        auto str = parameterLabels[i];
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
    
}

juce::String RotarySliderWithLabels::getDisplayString() const{
    
    //this helps show the dB/octave rather than the index values for the slopes
    if(auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    
    //this will truncate the Hz to kHz if the value is over 1000 Hz
    juce::String str;
    bool addK = false;
    
    if(auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)){
        float val = getValue();
        if(val >= 1000.f){
            val /= 1000.f; //1001 / 1000 = 1.001, but we just want two decimal places
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
        jassertfalse;//this shouldn't happen!
    
    if(suffix.isNotEmpty()){
        str << " ";
        if(addK)
            str << "k";
        
        str << suffix;
    }
    
    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p), leftPathProducer(audioProcessor.leftChannelFifo), rightPathProducer(audioProcessor.rightChannelFifo){ //leftChannelFifo(&audioProcessor.leftChannelFifo){
    const auto& params = audioProcessor.getParameters();
    for(auto param : params){
        param->addListener(this);
    }
    
    
    
    //this will update the gui whenever we close and reopen it
    updateChain();
    
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent(){
    const auto& params = audioProcessor.getParameters();
    for(auto param : params){
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue){
    parametersChanges.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate){
    //for spectrum analyzer, here we are coordinating the SCSF, FFT Data Generator, Path Producer, and GUI together
    //while there are buffers to pull we're gonna send to FFT Data Gen
    
    juce::AudioBuffer<float> tempIncomingBuffer;
    
    while(leftChannelFifo->getNumCompleteBuffersAvailable() > 0){
        if(leftChannelFifo->getAudioBuffer(tempIncomingBuffer)){
            //important to maintain the order of the incoming audio thread
            //we also need to make sure the buffers sent to FFT are the right size
            auto size = tempIncomingBuffer.getNumSamples();
            
            //shifting over the data
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0), monoBuffer.getReadPointer(0, size), monoBuffer.getNumSamples() - size);//the channel is 0 because it is the left channel
            
            //copying from starting point to end
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            //monoBuffer never changes size
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48);//bottom of spectrum analyzer is -48 which would be -inf on the display
        }
    }
    
    //now need to turn blocks into path
    //if we can pull a buffer, generate a path
    /*
     if there are FFT data buffers to pull
        if we can pull a buffer
            generate a path
     */
    //const auto fftBounds = getAnalysisArea().toFloat();
    const auto fftSize =  leftChannelFFTDataGenerator.getFFTSize();
    /*
     48000 / 2048 = 23hz <- this is the bin width
     */
    const auto binWidth = sampleRate / (double)fftSize;
    
    while(leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0){
        std::vector<float> fftData;
        if(leftChannelFFTDataGenerator.getFFTData(fftData)){
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);//last num is neg inf and this is just the smallest value of the display
        }
    }
    
    /*
     while there are paths that can be pulled
        pull as many as we can
            display the most recent path
     */
    while(pathProducer.getNumPathsAvailable()){
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback(){
    
    if(shouldShowFFTAnalysis){
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }
    
    //dont want to always be doing this, only want when we update the curve
    if(parametersChanges.compareAndSetBool(false, true)){
        DBG("params changed");
        //update the monoChain
        updateChain();
        //signal a repaint -> new response curve
        //repaint();
        //we're changing paths all the time, so we need to repaint all the time
    }
     
    repaint();
}

void ResponseCurveComponent::updateChain(){
    //this will update the response curve to show changed parameters when we save and exit the plugin
    
    //update the monochain from apvts
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    g.drawImage(background, getLocalBounds().toFloat());
    
    auto responseArea = getAnalysisArea();
    
    auto w = responseArea.getWidth();
    
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(w);
    
    for(int i = 0; i < w; ++i){
        double mag = 1.f; //starting gain of 1
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        if(!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        //if chain itself isn't bypassed then we can check if each individual chain is bypassed
        if(!monoChain.isBypassed<ChainPositions::LowCut>()){
            if(!lowcut.isBypassed<0>())
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!lowcut.isBypassed<1>())
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!lowcut.isBypassed<2>())
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!lowcut.isBypassed<3>())
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if(!monoChain.isBypassed<ChainPositions::HighCut>()){
            if(!highcut.isBypassed<0>())
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!highcut.isBypassed<1>())
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!highcut.isBypassed<2>())
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if(!highcut.isBypassed<3>())
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    //convert magnitudes to path
    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input){
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for(size_t i = 1; i < mags.size(); ++i){
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    if(shouldShowFFTAnalysis){
        auto leftChannelFFTPath = leftPathProducer.getPath();
        
        //translating the leftChannel path to follow the responseArea
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(Colours::lightcoral);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));//???figure this out and why it isn't following the curve, but right is
        
        auto rightChannelFFTPath = rightPathProducer.getPath();
        //translating the rightChannel path to follow the responseArea
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(Colours::lightyellow);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }
    
    g.setColour(Colour(255u, 155u, 64u));
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.1, 1.f);//thickness of 1 pixel wide
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));//thickness w/ 2 pixels wide
    
}

void ResponseCurveComponent::resized(){
    //our response curve image will go here because resized is called before paint
    
    //drawing the grid in the background of the response curve
    //create a new background image
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    //create a graphics context that will draw into the image
    Graphics g(background);
    
    //vertical lines
    Array<float> freqs{
        //start of human hearing range and increasing by factors of 10, standard freqs
        20, /*30, 40, */50, 100,
        200, /*300, 400, */500, 1000,
        2000, /*3000, 4000, */5000, 10000,
        20000//top of human hearing range
    };
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    juce::Array<float> xs;
    for(auto f : freqs){
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    
    g.setColour(Colours::dimgrey);//setting lines to dimgrey
    
    for(auto x : xs){
        g.drawVerticalLine(x, top, bottom);
    }
    
    //horizontal lines
    juce::Array<float> gain{
        -24, -12, 0, 12, 24
    };
    
    for(auto gDB : gain){
        auto y = jmap(gDB, -24.f, 24.f, float(bottom), float(top));
        //g.drawHorizontalLine(y, 0.f, getWidth());
        //drawing 0 dB line as a diff color to make it stand out
        g.setColour(gDB == 0.f ? Colour(232u, 194u, 159u): Colours::darkgrey);
        g.drawHorizontalLine(y, float(left), float(right));
    }
    
    //drawing freq labels
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    for(int i = 0; i < freqs.size(); ++i){
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        if(f >= 1000.f){
            f /= 1000.f; //1001 / 1000 = 1.001, but we just want two decimal places
            addK = true;
        }
        
        str << f;
        if(addK)
            str << "k";
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
         
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    //drawing gain labels
    for(auto gDB : gain){
        auto y = jmap(gDB, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if(gDB > 0)
            str << "+";
        //don't need to add negative because already stated in gain array
        str << gDB;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        if(gDB == 0.f)
            r.setX(getWidth() - 2  * textWidth);
        else
            r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y); //we don't know the y, but we know x and know we want the center of the rect to be lined up with the y value for the corresponding gDB
        g.setColour(gDB == 0.f ? Colour(232u, 194u, 159u): Colours::lightgrey);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
        //now we are drawing the labels for the spectrum analyzer
        str.clear();
        str << (gDB - 24.f);
        
        if(gDB == 24.f)
            r.setX(0.5 * textWidth);
        else
            r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea(){
    auto bounds = getLocalBounds();
    
//    bounds.reduce(10, //JUCE_LIVE_CONSTANT(5),
//                  8 //JUCE_LIVE_CONSTANT(5));//make sure to put each juce constant on its own line
//                  );
    
    //want instead to reduce area on tops and sides for labels of freq and gain
    bounds.removeFromTop(12);
    bounds.removeFromBottom(5);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea(){
    auto bounds = getRenderArea();
    
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    
    return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
        peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),  
        peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
        peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
        lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
        highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
        lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Octave"),
        highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Octave"),
        responseCurveComponent(audioProcessor),
        peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
        peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
        peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider), 
        lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
        highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
        lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
        highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
        
        lowCutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
        peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
        highCutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton),
        analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20kHz"});
    
    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});
    
    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10.0"});
    
    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20kHz"});
    
    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20kHz"});
    
    lowCutSlopeSlider.labels.add({0.f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});
    
    highCutSlopeSlider.labels.add({0.f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"});
    
    for(auto* comp: getComps()){
        addAndMakeVisible(comp);
    }
    
    peakBypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);
    
    auto safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
    peakBypassButton.onClick = [safePtr](){
        if(auto* comp = safePtr.getComponent()){
            auto bypassed = comp->peakBypassButton.getToggleState();
            
            //if bypassed, sliders should not be enabled
            comp->peakFreqSlider.setEnabled(!bypassed);
            comp->peakGainSlider.setEnabled(!bypassed);
            comp->peakQualitySlider.setEnabled(!bypassed);
        }
    };
    
    lowCutBypassButton.onClick = [safePtr](){
        if(auto* comp = safePtr.getComponent()){
            auto bypassed = comp->lowCutBypassButton.getToggleState();
            
            //if bypassed, sliders should not be enabled
            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };
    
    highCutBypassButton.onClick = [safePtr](){
        if(auto* comp = safePtr.getComponent()){
            auto bypassed = comp->highCutBypassButton.getToggleState();
            
            //if bypassed, sliders should not be enabled
            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };
    
    analyzerEnabledButton.onClick = [safePtr]{
        if(auto* comp = safePtr.getComponent()){
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };
    
    setSize (600, 500);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    //since we registered as a listener we need to deregister as a listener (constructor then destructor)
    peakBypassButton.setLookAndFeel(nullptr);
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    //JUCE_LIVE_CONSTANT(value) is very helpful
    auto bounds = getLocalBounds();
    
    auto analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(100);
    analyzerEnabledArea.setX(5);
    analyzerEnabledArea.removeFromTop(2);
    
    analyzerEnabledButton.setBounds(analyzerEnabledArea);
    bounds.removeFromTop(5);
    
     
    float heightRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f;//allows us to adjust so we can see how tall our rectangle for the response curve should be
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * heightRatio);
    
    responseCurveComponent.setBounds(responseArea);
    
    bounds.removeFromTop(5); //can also be done in line 86 where you add 5 pixels to the getY or getX to adjust position
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakBypassButton.setBounds(bounds.removeFromTop(25));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    
    //peakFreqSlider.setColour(peakFreqSlider.getComponentID(), juce::Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 1.0f)); figure out!!!!
    //Colour::fromFloatRGBA(converter*94, converter*144, converter*236, 1.0f)());
    //customize plugin once done!!??!!!
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps(){
    return{
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        
        &lowCutBypassButton,
        &highCutBypassButton,
        &peakBypassButton,
        &analyzerEnabledButton
    };
}
