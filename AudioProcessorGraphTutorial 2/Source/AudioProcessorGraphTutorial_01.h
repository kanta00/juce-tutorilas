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

 name:             AudioProcessorGraphTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Explores the audio processor graph.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_plugin_client, juce_audio_processors,
                   juce_audio_utils, juce_core, juce_data_structures, juce_dsp,
                   juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             AudioProcessor
 mainClass:        TutorialProcessor

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class ProcessorBase  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ProcessorBase()  {}
 
    //==============================================================================
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioSampleBuffer&, juce::MidiBuffer&) override {}
 
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override          { return nullptr; }
    bool hasEditor() const override                              { return false; }
 
    //==============================================================================
    const juce::String getName() const override                  { return {}; }
    bool acceptsMidi() const override                            { return false; }
    bool producesMidi() const override                           { return false; }
    double getTailLengthSeconds() const override                 { return 0; }
 
    //==============================================================================
    int getNumPrograms() override                                { return 0; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}
 
    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override       {}
    void setStateInformation (const void*, int) override         {}
 
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};

//==============================================================================
//440Hzのサイン波を生成する
class OscillatorProcessor : public ProcessorBase
{
public:
    //コンストラクタでオシレータのDSPの設定を記述
    OscillatorProcessor()
    {
        oscillator.setFrequency(440.0f);
        oscillator.initialise([](float x){return std::sin(x);});
    }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        //サンプルレートとブロックごとのサンプル数を格納
        juce::dsp::ProcessSpec spec {sampleRate, static_cast<juce::uint32> (samplesPerBlock)};
        //dsp::Oscillatorオブジェクトに入れる
        oscillator.prepare(spec);
    }
    //バッファを受け取り、dspで音声を処理
    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer&) override
    {
        //オーディオブロックを用意して
        juce::dsp::AudioBlock<float> block (buffer);
        //それをcontextに変換する
        juce::dsp::ProcessContextReplacing<float> context(block);
        //contextをdsp::Oscillatorに入れる
        oscillator.process(context);
    }
    void reset() override
    {
        oscillator.reset();
    }
    const juce::String getName() const override {return "Oscillator";}
private:
    juce::dsp::Oscillator<float> oscillator;
};


//==============================================================================
//ゲインを-6.0f下げる
class GainProcessor : public ProcessorBase
{
public:
    //dspの設定
    GainProcessor()
    {
        gain.setGainDecibels(-6.0f);
    }
    //dspにdawの設定を伝える
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec {sampleRate, static_cast<juce::uint32>(samplesPerBlock),2};
        gain.prepare(spec);
    }
    //dspで音声を処理
    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer&) override
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        gain.process(context);
    }
    //リセット
    void reset() override
    {
        gain.reset();
    }
    //名前のゲッター
    const juce::String getName() const override{return "Gain";}
private:
    //dspでgainを指定
    juce::dsp::Gain<float> gain;
};

//==============================================================================
//1kHz以下を削るシンプルなハイパスフィルター
class FilterProcessor : public ProcessorBase
{
public:
    FilterProcessor(){}
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        //ハイパスフィルターを設定
        *filter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate,1000.0f);
        
        juce::dsp::ProcessSpec spec {sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2};
        filter.prepare(spec);
    }
    //取得したオーディオサンプルバッファからdsp::ProcessContextReplacingオブジェクトを生成し、フィルター処理を行う
    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer&) override
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }
    void reset() override
    {
        filter.reset();
    }
    
    const juce::String getName() const override {return "Filter";}
private:
    //dsp::IIR::Filterはモノラルしか対応しないが、
    //ProcessorDuplicatorオブジェクトを生成することで、その内部に変数を共有するLRで２つのフィルターのセットを生成できる
    //（変数はdsp::IIR::Coefficientsに格納）
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,juce::dsp::IIR::Coefficients<float>> filter;
};

//==============================================================================
class TutorialProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    //インクルード　名前を省略
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;
    //==============================================================================
    TutorialProcessor()
        //
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          //インスタンス化　名前(new クラス)
          mainProcessor    (new juce::AudioProcessorGraph()),
          muteInput        (new juce::AudioParameterBool ("mute", "Mute Input", true)),
          processorSlot1   (new juce::AudioParameterChoice("slot1", "Slot 1", processorChoices,0)),
          processorSlot2   (new juce::AudioParameterChoice("slot2", "Slot 2", processorChoices,0)),
          processorSlot3   (new juce::AudioParameterChoice("slot3", "Slot 3", processorChoices,0)),
          bypassSlot1      (new juce::AudioParameterBool("bypass1", "Bypass 1", false)),
          bypassSlot2      (new juce::AudioParameterBool("bypass2", "Bypass 2", false)),
          bypassSlot3      (new juce::AudioParameterBool("bypass3", "Bypass 3", false))
    {
        addParameter (muteInput);

        addParameter (processorSlot1);
        addParameter (processorSlot2);
        addParameter (processorSlot3);

        addParameter (bypassSlot1);
        addParameter (bypassSlot2);
        addParameter (bypassSlot3);
    }

    //==============================================================================
    //DAWにどのInOutの組み合わせが使えるかを伝える　今回は mono-to-mono、stereo-to-stereのみ許可
    //DAWのlayoutsを引数
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        //InかOutが有効でなかったら拒否
        if(layouts.getMainInputChannelSet()==juce::AudioChannelSet::disabled()
           || layouts.getMainOutputChannelSet()==juce::AudioChannelSet::disabled())
            return false;
        
        //Outputがmonoかstereoじゃなかったら拒否
        if(layouts.getMainOutputChannelSet()!=juce::AudioChannelSet::mono()
           && layouts.getMainOutputChannelSet()!=juce::AudioChannelSet::stereo())
            return false;
        
        //InputとOutputのチャネル数が一致してなかったら拒否
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        //setなんとか　パラメータをセットするセッター
        mainProcessor->setPlayConfigDetails(getMainBusNumOutputChannels(),
                                            getMainBusNumOutputChannels(),
                                            sampleRate,
                                            samplesPerBlock);
        
        //グラフでもprepareToPlayする
        mainProcessor->prepareToPlay(sampleRate, samplesPerBlock);

        initialiseGraph();
    }

    void releaseResources() override
    {
        mainProcessor->releaseResources();
    }

    void processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midiMessages) override
    {
        //余分なアウトプットチャネルがあれば、そのバッファをクリア
        for(int i=getTotalNumInputChannels(); i<getTotalNumOutputChannels();++i)
            buffer.clear(i,0,buffer.getNumSamples());
        
        //グラフの形状が変わったらアップデートする
        updateGraph();
        
        //オーディオグラフで音をバッファに書き込む
        mainProcessor->processBlock(buffer,midiMessages);
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override          { return new juce::GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                              { return true; }

    //==============================================================================
    const juce::String getName() const override                  { return "Graph Tutorial"; }
    bool acceptsMidi() const override                            { return true; }
    bool producesMidi() const override                           { return true; }
    double getTailLengthSeconds() const override                 { return 0; }

    //==============================================================================
    int getNumPrograms() override                                { return 1; }
    int getCurrentProgram() override                             { return 0; }
    void setCurrentProgram (int) override                        {}
    const juce::String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const juce::String&) override   {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock&) override       {}
    void setStateInformation (const void*, int) override         {}

private:
    //==============================================================================
    void initialiseGraph()
    {
        mainProcessor->clear();
        
        //addNodeの返り値はノードのポインタ
        audioInputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
        audioOutputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
        midiInputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::midiInputNode));
        midiOutputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::midiOutputNode));
        
        connectAudioNodes();
        connectMidiNodes();
    }

    
    //グラフに変更があった場合、まずmainProcessorからそのノードを取り除く
    //そして、slotsのi番目に変更のあったエフェクトをセットして、
    //if(hasChanged)以下で、mainProcessorの接続を解きほぐし、結びつける
    void updateGraph()
    {
        bool hasChanged = false;
        
        //for ループを回すために３つのノードの設定を配列にまとめる
        //３つある選択ボックスをまとめたArray choices
        juce::Array<juce::AudioParameterChoice*> choices {
            processorSlot1,
            processorSlot2,
            processorSlot3
        };
        
        //3つあるバイパスボタンをまとめたArray bypasses
        juce::Array<juce::AudioParameterBool*> bypasses {
            bypassSlot1,
            bypassSlot2,
            bypassSlot3
        };
        
        //NodeのArrayを定義し、直列に追加
        juce::ReferenceCountedArray<Node> slots;
        slots.add(slot1Node);
        slots.add(slot2Node);
        slots.add(slot3Node);
        
        //３つの選択でループ
        for (int i=0; i<3; ++i)
        {
            //選択
            auto& choice = choices.getReference(i);
            //ノードのスロット
            auto slot = slots.getUnchecked(i);
            
            //選択しているのがEmpty Stateだった場合
            if (choice->getIndex()==0)
            {
                //何か入っていた場合
                if(slot!=nullptr)
                {
                    //mainProcessorからノードを取り除く
                    mainProcessor->removeNode(slot.get());
                    //nullでノードをセット
                    slots.set(i,nullptr);
                    hasChanged = true;
                }
            }
            //選択しているのがOscillatorだった場合
            else if (choice->getIndex()==1)
            {
                //何か入っていた場合
                if (slot!= nullptr)
                {
                    //オシレータならそのまま
                    if(slot->getProcessor()->getName()=="Oscillator")
                        continue;
                    //そうじゃなかったらmainProcessorから除く
                    mainProcessor->removeNode(slot.get());
                }
                //オシレータをmainProcessorにセットし、slotsのi番目にポインタを渡す
                slots.set(i, mainProcessor->addNode(std::make_unique<OscillatorProcessor>()));
                hasChanged=true;
            }
            //選択しているのがGainだった場合
            else if (choice->getIndex()==2)
            {
                if(slot!= nullptr)
                {
                    if(slot->getProcessor()->getName()=="Gain")
                        continue;
                    //そうじゃなかったら除く
                    mainProcessor->removeNode(slot.get());
                }
                //gainをセット
                slots.set(i, mainProcessor->addNode(std::make_unique<GainProcessor>()));
                hasChanged=true;
            }
            //選択しているのがFilterだった場合
            else if (choice->getIndex()==3)
            {
                if(slot!= nullptr)
                {
                    if(slot->getProcessor()->getName()=="Filter")
                        continue;
                    mainProcessor->removeNode(slot.get());
                }
                //filterをセット
                slots.set(i, mainProcessor->addNode(std::make_unique<FilterProcessor>()));
                hasChanged=true;
            }
        }
        
        //変更があった場合にのみ
        //mainProcessorを解きほぐし、結合
        //1:ノードの結合を解きほぐす
        //2:slotsからnullptrを除いたものを配列activeSlotsに格納
        if(hasChanged)
        {
            //1:全ての結合を解きほぐす
            for(auto connection : mainProcessor->getConnections())
                mainProcessor->removeConnection(connection);
            
            juce::ReferenceCountedArray<Node> activeSlots;
            
            //2:slotsからnullptrを除いたものを配列activeSlotsに格納
            for (auto slot :slots)
            {
                //slotがemptyじゃなかったら
                if(slot!= nullptr)
                {
                    //activeSlotsにslotを追加（配列に参照を渡す）
                    activeSlots.add(slot);
                    
                    //slotのNodeに全体の設定の情報を与える
                    slot->getProcessor()->setPlayConfigDetails(getMainBusNumOutputChannels(),
                                                                getMainBusNumOutputChannels(),
                                                                getSampleRate(),getBlockSize());
                }
            }
            //activeSlotsが空だったらinとoutをそのまま繋げる
            if(activeSlots.isEmpty())
            {
                connectAudioNodes();
            }
            //activeSlotsが一つ以上あった場合
            else
            {
                //activeSlots同士をmainProcessor内部で繋げる
                //内部のコネクションの数はノードの数-1
                for(int i=0; i<activeSlots.size()-1 ; ++i)
                {
                    for(int channel=0; channel<2; ++channel)
                    {
                        //エフェクトのノードを繋げる
                        mainProcessor->addConnection({
                            {activeSlots.getUnchecked(i)->nodeID, channel},
                            {activeSlots.getUnchecked(i+1)->nodeID, channel}});
                    }
                }
                //IOとエフェクトを繋げる
                for(int channel = 0; channel<2; ++channel)
                {
                    //Audio Inputと最初のエフェクトを繋げる
                    mainProcessor->addConnection({
                        {audioInputNode->nodeID, channel},
                        {activeSlots.getFirst()->nodeID, channel}
                    });
                    //最後のエフェクトとAudioOutputを繋げる
                    mainProcessor->addConnection({
                        {activeSlots.getLast()->nodeID, channel},
                        {audioOutputNode->nodeID, channel}
                    });
                }
            }
            
            //midiはそのまま繋げる（エフェクトが何もないから）
            connectMidiNodes();
            
            //全てのバスが有効であるか確認する
            for (auto node : mainProcessor->getNodes())
                node->getProcessor()->enableAllBuses();
        }
        
        //bypassの設定
        for (int i=0; i<3;++i)
        {
            //slotとbypassを取り出して、slot（Node型）にbypassを適用
            auto slot = slots.getUnchecked(i);
            auto& bypass = bypasses.getReference(i);
            if (slot!= nullptr)
                //bypass->get()はbool
                slot->setBypassed(bypass->get());
        }
        
        audioInputNode->setBypassed(muteInput->get());
        
        slot1Node = slots.getUnchecked(0);
        slot2Node = slots.getUnchecked(1);
        slot3Node = slots.getUnchecked(2);
    }
    
    void connectAudioNodes()
    {
        //オーディオプロセッサグラフのaudioの信号をLRで繋げる
        for (int channel = 0; channel<2;++channel)
            mainProcessor->addConnection({{audioInputNode->nodeID, channel},{audioOutputNode->nodeID, channel}});
    }
    
    void connectMidiNodes()
    {
        //オーディオプロセッサグラフのmidiの信号を繋げる
        mainProcessor->addConnection({ {midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
            {midiOutputNode->nodeID,juce::AudioProcessorGraph::midiChannelIndex}
        });
    }

    //==============================================================================
    juce::StringArray processorChoices{"Empty","Oscillator","Gain","Filter"};
    
    //オーディオプロセッサグラフ
    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;
    
    //ユーザーの選択で変わる部分
    juce::AudioParameterBool* muteInput;
    
    juce::AudioParameterChoice* processorSlot1;
    juce::AudioParameterChoice* processorSlot2;
    juce::AudioParameterChoice* processorSlot3;
    
    juce::AudioParameterBool* bypassSlot1;
    juce::AudioParameterBool* bypassSlot2;
    juce::AudioParameterBool* bypassSlot3;
    
    //直列に繋がれるノード1,2,3のポインタ
    Node::Ptr slot1Node;
    Node::Ptr slot2Node;
    Node::Ptr slot3Node;
    
    //ノード
    //Node = juce::AudioProcessorGraph::Node;
    //オーディオのInOut
    Node::Ptr audioInputNode;
    Node::Ptr audioOutputNode;
    //MidiのInOut
    Node::Ptr midiInputNode;
    Node::Ptr midiOutputNode;
    

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TutorialProcessor)
};
