/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator{
    
  //produces the FFT data from an audio buffer
    
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity){
        const auto fftSize = getFFTSize();
        
        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex +  fftSize, fftData.begin());
        
        //first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);
        
        //then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());
        
        int numBins = (int)fftSize / 2;
        
        //normalize the fft values
        for(int i = 0; i < numBins; ++i){
            fftData[i] /= (float) numBins;
        }
        
        //convert them to decibels
        for(int i = 0; i < numBins; ++i){
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }
        
        fftDataFifo.push(fftData);
    }
    
    void changeOrder(FFTOrder newOrder){
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>
        
        order = newOrder;
        auto fftSize = getFFTSize();
        
        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
        
        fftData.clear();
        fftData.resize(fftSize * 2, 0);
        
        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const {
        return 1 << order;
    }
    int getNumAvailableFFTDataBlocks() const {
        return fftDataFifo.getNumAvailableForReading();
    }
    //==============================================================================
    bool getFFTData(BlockType& fftData){
        return fftDataFifo.pull(fftData);
    }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    Fifo<BlockType> fftDataFifo;
};

//this class below takes in fft data and produces a path
template<typename PathType>
struct AnalyzerPathGenerator{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity){
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();
        
        int numBins = (int)fftSize / 2;
        
        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());
        
        auto map = [bottom, top, negativeInfinity](float v){
            return juce::jmap(v, negativeInfinity, 0.f, float(bottom), top);
        };
        
        auto y = map(renderData[0]);
        
        jassert(!std::isnan(y) && !std::isinf(y));
        
        p.startNewSubPath(0, y);
        
        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels
        
        for(int binNum = 1; binNum < numBins; binNum += pathResolution){
            y = map(renderData[binNum]);
            
            jassert(!std::isnan(y) && !std::isinf(y));
            
            if(!std::isnan(y) && !std::isinf(y)){
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }
        
        pathFifo.push(p);
    }
    
    int getNumPathsAvailable() const {
        return pathFifo.getNumAvailableForReading();
    }
    
    bool getPath(PathType& path){
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix): juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox), param(&rap), suffix(unitSuffix){
        
        setLookAndFeel(&lnf);
    }
    
    ~RotarySliderWithLabels(){
        setLookAndFeel(nullptr);
    }
    
    struct LabelPos{
        float pos;
        juce::String label;
    };
    
    juce::Array<LabelPos> labels;
    
    void paint(juce::Graphics &g) override;
    
    juce::Rectangle<int> getSliderBounds() const;
    
    int getTextHeight() const{
        return 14;
    }
    
    juce::String getDisplayString() const;
    
private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
    juce::Array<juce::String> parameterLabels{
      "LowCut Freq", "LowCut Slope", "Peak Freq", "Peak Gain", "Peak Quality", "HighCut Freq", "HighCut Slope"
    };
};

struct PathProducer{
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf): leftChannelFifo(&scsf){
        //choosing the order specifies the range of the bins for displaying the spectrum
        /*
         48000 / 2048 = 23hz -> size of a bin
         will give us a lot of resolution at the upper end, but not a lot at the lower end
         */
        //higher order needs more CPU, better resolution at lower end as well
        
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath;}
private:
    //not simple, building a system
    //audio buffers from host -> Single Channel Sample Fifo (SCSF) (spits out fixed sized blocks) -> FFT data generator (spits out FFT data blocks) -> Path producer (spits out juce::Path instances) -> GUI which consumes the data and renders the path
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChannelFifo;
    
    //this block is what we send our left and right audio through
    //the new blocks are appended to the end, and then shifted over to the left and  processed
    juce::AudioBuffer<float> monoBuffer;
    
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    
    AnalyzerPathGenerator<juce::Path> pathProducer;
    
    juce::Path leftChannelFFTPath;
};

struct ResponseCurveComponent: juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override{}
    
    void timerCallback() override;//used to be virtual but removed it to implement
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void toggleAnalysisEnablement(bool enabled){
        shouldShowFFTAnalysis = enabled;
    }

private:
    SimpleEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanges{false};
    
    MonoChain monoChain;
    
    void updateChain();
    
    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    
    juce::Rectangle<int> getAnalysisArea(); //labels for the response curve grid
    
    PathProducer leftPathProducer, rightPathProducer;
    
    bool shouldShowFFTAnalysis = true;
};

//==============================================================================
struct PowerButton : juce::ToggleButton{
    
};
struct AnalyzerButton : juce::ToggleButton{
    void resized() override{
        //so everytime we open and close the gui, we get a new path
        auto bounds = getLocalBounds();
        auto insetRect = bounds.reduced(4);
        
        randomPath.clear();
        
        juce::Random r;
        
        randomPath.startNewSubPath(insetRect.getX(), insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        
        for(auto x  = insetRect.getX() + 1; x < insetRect.getRight(); x += 2){
            randomPath.lineTo(x, insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        }
    }
    
    juce::Path randomPath;
};
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    float converter = 0.501961/128;
    
    SimpleEQAudioProcessor& audioProcessor;
    
    RotarySliderWithLabels peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider;
    
    RotarySliderWithLabels lowCutSlopeSlider, highCutSlopeSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;
    
    PowerButton lowCutBypassButton, peakBypassButton, highCutBypassButton;
    AnalyzerButton analyzerEnabledButton;
    
    using ButtonAttachment = APVTS::ButtonAttachment;
    ButtonAttachment lowCutBypassButtonAttachment, peakBypassButtonAttachment, highCutBypassButtonAttachment, analyzerEnabledButtonAttachment;
    
    std::vector<juce::Component*> getComps();
    
    LookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
