/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             NoiseGateTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Noise gate audio plugin.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        NoiseGate

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class NoiseGate  : public juce::AudioProcessor
{
public:
    //==============================================================================
    NoiseGate()
        :AudioProcessor(BusesProperties()//3つのステレオ入出力端子を定義 Input x2 Output x1
                        .withInput("input", juce::AudioChannelSet::stereo())
                        .withOutput("Output", juce::AudioChannelSet::stereo())
                        .withInput("Sidechain", juce::AudioChannelSet::stereo())
                    )
    {
        //スレッショルドを定義 0~1で
        addParameter(threshold = new juce::AudioParameterFloat("threshold","Threshold",0.0f,1.0f,0.5f));
        //アルファ
        addParameter(alpha = new juce::AudioParameterFloat("alpha","Alpha",0.0f,1.0f,0.8f));
    }

    //==============================================================================
    bool isBusesLayoutSupported(const BusesLayout& layouts)const override
    {
        //the sidechain can take any layout, the main bus needs to be the same on the input and output
        //メインの入力と出力が同じチャンネルセット　かつ　メインの入力が無効ではない ->true
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        &&! layouts.getMainInputChannelSet().isDisabled();
    }

    //==============================================================================
    void prepareToPlay(double,int) override
    {
        //ローパスのフィルター係数。SCシグナルとアルファによって決まり、ゲートの挙動を決定する
        lowPassCoeff = 0.0f;
        //ゲートになる前のサンプル数(??)
        sampleCountDown = 0;
    }

    void releaseResources() override          {}
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        //メインバッファをSCバッファから切り離して後の処理をできるようにする
        auto mainInputOutput = getBusBuffer(buffer,true,0);
        auto sideChainInput = getBusBuffer(buffer,true,1);
        
        //パラメータのコピーを取得
        auto alphaCopy = alpha->get();
        auto thresholdCopy = threshold->get();
        
        //外側のfor loopでサンプルごとに見ていき、内側のfor loopでチャンネルごとで見ていく
        for(auto j=0; j<buffer.getNumSamples();++j)
        {
            auto mixedSamples = 0.0f;
            
            //SCの全てのチャンネルの信号を足し合わせて、チャンネル数で割ることで平均化し、モノラル音源にする
            for (auto i=0; i<sideChainInput.getNumChannels(); ++i)
                mixedSamples += sideChainInput.getReadPointer(i)[j];
            
            mixedSamples /= static_cast<float> (sideChainInput.getNumChannels());
            //平滑化（ローパスをかけてる）
            lowPassCoeff = (alphaCopy*lowPassCoeff)+((1.0f-alphaCopy)*mixedSamples);
            
            //係数がthresholdより大きかったらカウントダウンをサンプルレートに戻す
            if(lowPassCoeff>=thresholdCopy)
            {
                sampleCountDown = (int) getSampleRate();
            }
            
            //LRごとに、もしsampleCountDownが０より大きかったら、インプットをそのままアウトプットに流す
            for(auto i=0; i<mainInputOutput.getNumChannels();++i)
                *mainInputOutput.getWritePointer(i,j) = sampleCountDown>0 ? *mainInputOutput.getReadPointer(i,j)
            
            if(sampleCountDown>0)
                --sampleCountDown;
        }
        
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override            { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                                { return true; }
    const juce::String getName() const override                    { return "Noise Gate"; }
    bool acceptsMidi() const override                              { return false; }
    bool producesMidi() const override                             { return false; }
    double getTailLengthSeconds() const override                   { return 0.0; }
    int getNumPrograms() override                                  { return 1; }
    int getCurrentProgram() override                               { return 0; }
    void setCurrentProgram (int) override                          {}
    const juce::String getProgramName (int) override               { return {}; }
    void changeProgramName (int, const juce::String&) override     {}

    bool isVST2() const noexcept                                   { return (wrapperType == wrapperType_VST); }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream stream (destData, true);

        stream.writeFloat (*threshold);
        stream.writeFloat (*alpha);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

        threshold->setValueNotifyingHost (stream.readFloat());
        alpha->setValueNotifyingHost (stream.readFloat());
    }

private:
    //==============================================================================
    juce::AudioParameterFloat* threshold;
    juce::AudioParameterFloat* alpha;
    int sampleCountDown;
    
    float lowPassCoeff;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGate)
};
