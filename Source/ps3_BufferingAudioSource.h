/*

Copyright (C) 2017 Xenakios

This program is free software; you can redistribute it and/or modify
it under the terms of version 3 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 3) for more details.

www.gnu.org/licenses

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class JUCE_API  MyBufferingAudioSource  : public PositionableAudioSource,
                                        private TimeSliceClient
{
public:
    //==============================================================================
    /** Creates a BufferingAudioSource.

        @param source                       the input source to read from
        @param backgroundThread             a background thread that will be used for the
                                            background read-ahead. This object must not be deleted
                                            until after any BufferingAudioSources that are using it
                                            have been deleted!
        @param deleteSourceWhenDeleted      if true, then the input source object will
                                            be deleted when this object is deleted
        @param numberOfSamplesToBuffer      the size of buffer to use for reading ahead
        @param numberOfChannels             the number of channels that will be played
        @param prefillBufferOnPrepareToPlay if true, then calling prepareToPlay on this object will
                                            block until the buffer has been filled
    */
	MyBufferingAudioSource(PositionableAudioSource* source,
                          TimeSliceThread& backgroundThread,
                          bool deleteSourceWhenDeleted,
                          int numberOfSamplesToBuffer,
                          int numberOfChannels = 2,
                          bool prefillBufferOnPrepareToPlay = true);

    ~MyBufferingAudioSource();

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;

    void releaseResources() override;

    void getNextAudioBlock (const AudioSourceChannelInfo&) override;

    void setNextReadPosition (int64 newPosition) override;

    int64 getNextReadPosition() const override;

    int64 getTotalLength() const override       { return source->getTotalLength(); }

    bool isLooping() const override             { return source->isLooping(); }

    bool waitForNextAudioBlockReady (const AudioSourceChannelInfo& info, const uint32 timeout);
	[[nodiscard]] double getPercentReady() const;
	int getNumberOfChannels() { return numberOfChannels; }
private:
    //==============================================================================
    OptionalScopedPointer<PositionableAudioSource> source;
    TimeSliceThread& backgroundThread;
    int numberOfSamplesToBuffer, numberOfChannels;
    AudioBuffer<float> buffer;
    CriticalSection bufferStartPosLock;
    WaitableEvent bufferReadyEvent;
    std::atomic<int64> bufferValidStart { 0 }, bufferValidEnd { 0 }, nextPlayPos { 0 };
    double sampleRate = 0;
    bool wasSourceLooping = false, isPrepared = false, prefillBuffer;

    bool readNextBufferChunk();
    void readBufferSection (int64 start, int length, int bufferOffset);
    int useTimeSlice() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyBufferingAudioSource)
};


