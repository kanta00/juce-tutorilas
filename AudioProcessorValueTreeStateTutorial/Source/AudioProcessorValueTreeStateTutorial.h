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

 name:             AudioProcessorValueTreeStateTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores the audio processor value tree state.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        TutorialProcessor

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class GenericEditor : public juce::AudioProcessorEditor
{
public:
    enum
    {
        paramControlHeight = 40,
        paramLabelWidth    = 80,
        paramSliderWidth   = 300
    };
    
    //型が長すぎるので短い名前をつける
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

    //コンストラクタ
    GenericEditor (juce::AudioProcessor& parent, juce::AudioProcessorValueTreeState& vts)
    :AudioProcessorEditor(parent),
    valueTreeState(vts)
    {
        //gainのラベルをセットして表示
        gainLabel.setText("Gain",juce::dontSendNotification);
        addAndMakeVisible(gainLabel);
        
        //gainスライダー
        addAndMakeVisible(gainSlider);
        //gainのパラメータとスライダを結びつける
        //Attatchmentのコンストラクタにバリューツリーステイト、パラメータID、GUIオブジェクトを入れる
        gainAttachment.reset(new SliderAttachment(valueTreeState,"gain",gainSlider));
        
        //位相反転ボタン
        invertButton.setButtonText("Invert Phase");
        addAndMakeVisible(invertButton);
        invertAttachment.reset(new ButtonAttachment(valueTreeState, "invertPhase", invertButton));
        
        //ウィンドウサイズ
        setSize(paramSliderWidth+paramLabelWidth, juce::jmax(100, paramControlHeight*2));
        
        
        
    }
    void resized() override
    {
        auto r = getLocalBounds();

        auto gainRect = r.removeFromTop (paramControlHeight);
        gainLabel.setBounds (gainRect.removeFromLeft (paramLabelWidth));
        gainSlider.setBounds (gainRect);

        invertButton.setBounds (r.removeFromTop (paramControlHeight));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

private:
    //valueTreeStateのポインタ
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    //gain
    //ラベル
    juce::Label gainLabel;
    //スライダ
    juce::Slider gainSlider;
    //gainのパラメータとスライダを結びつける　上でtypedefした
    std::unique_ptr<SliderAttachment> gainAttachment;
    
    //ボタン
    //ボタン
    juce::ToggleButton invertButton;
    //アタッチメント　上でtypedefした
    std::unique_ptr<ButtonAttachment> invertAttachment;
};

//==============================================================================
class TutorialProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    TutorialProcessor()
    //ValueTreeにくっつけるパラメータを入れる　privateでparametersをjuce::AudioProcessorValueTreeStateでいれた
    //AudioProcessorValueTreeStateでパラメータを作ると自動的にAudioProcessorにも反映される
    //第一引数　Processorのポインタ、第二引数　UndoManager*、第三引数 識別するやつjuce::Identifier、第四引数 ParameterLayout
    :parameters(*this, nullptr, juce::Identifier("APVTSTutorial"),
                {
        //gain
        std::make_unique<juce::AudioParameterFloat> ("gain",//ID(should be unique)
                                                     "Gain",//Name
                                                     0.0f,//min
                                                     1.0f,//max
                                                     0.5f),//default
        //invertPhase
        std::make_unique<juce::AudioParameterBool> ("invertPhase",//ID
                                                    "Invert Phase",//Name
                                                    false)//default
    })
    {
        //ValueTreeの変数のポインタを取得
        phaseParameter = parameters.getRawParameterValue("invertPhase");
        gainParameter = parameters.getRawParameterValue("gain");
    }

    //==============================================================================
    void prepareToPlay (double, int) override
    {
        //boolean→位相の数値　スターつけると実体が得られる
        auto phase = *phaseParameter < 0.5f? 1.0 : -1.0f;
        //gain
        previousGain = *gainParameter * phase;
    }

    void releaseResources() override {}

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer&) override
    {
        auto phase = *phaseParameter < 0.5f ? 1.0f: -1.0f;
        auto currentGain = *gainParameter * phase;
        
        if(currentGain == previousGain)
        {
            buffer.applyGain(currentGain);
        }
        else
        {
            buffer.applyGainRamp(0, buffer.getNumSamples(),previousGain,currentGain);
            previousGain = currentGain;
        }
    }

    //==============================================================================
    //GUIを呼び出すところ　上でGenericEditorを定義してるのでそこに*thisとparameters(AudioProcessorValueTreeState)を入れる
    juce::AudioProcessorEditor* createEditor() override          { return new GenericEditor (*this, parameters); }
    bool hasEditor() const override                              { return true; }

    //==============================================================================
    const juce::String getName() const override                  { return "APVTS Tutorial"; }
    bool acceptsMidi() const override                            { return false; }
    bool producesMidi() const override                           { return false; }
    double getTailLengthSeconds() const override                 { return 0; }

    //==============================================================================
    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}

    //==============================================================================
    //DAWにParamをXMLで保存
    void getStateInformation (juce::MemoryBlock& destData) override
    {
        //ValueTreeStateの情報をコピー
        auto state = parameters.copyState();
        //xmlを作成し、state.createXml()を入れる
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        //バイナリに書き込む
        copyXmlToBinary(*xml, destData);
    }
    
    //DAWからXMLを取り出してParamを読み込む
    void setStateInformation (const void* data, int sizeInBytes) override
    {
        //xmlStateという名前で取得
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
        
        //xmlがあれば
        if(xmlState.get()!= nullptr)
            //APVTSTutorial？というIdentifierがあるか
            if(xmlState->hasTagName(parameters.state.getType()))
                //xmlを読み込んで書き換え
                parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    float previousGain;

    //atomic変数とは、不可分な読み出しと書き込み、及び読み書きを同時に与える変数
    //コンストラクタの最後で設定
    std::atomic<float>* phaseParameter = nullptr;
    std::atomic<float>* gainParameter = nullptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TutorialProcessor)
};
