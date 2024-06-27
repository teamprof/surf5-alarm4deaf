/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "libs/audio/audio_service.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "libs/tensorflow/audio_models.h"
#include "./AudioBridge.h"

static constexpr int kNumDmaBuffers = 2;
static constexpr int kDmaBufferSizeMs = 50;
static constexpr int kDmaBufferSize =
    kNumDmaBuffers * coralmicro::tensorflow::kYamnetSampleRateMs * kDmaBufferSizeMs;
static constexpr int kAudioServicePriority = 4;
static constexpr int kDropFirstSamplesMs = 150;

static coralmicro::AudioDriverBuffers<kNumDmaBuffers, kDmaBufferSize> audio_buffers;
static coralmicro::AudioDriver audio_driver(audio_buffers);

static std::array<int16_t, coralmicro::tensorflow::kYamnetAudioSize> audio_input;
static coralmicro::LatestSamples audio_latest(
    MsToSamples(coralmicro::AudioSampleRate::k16000_Hz, coralmicro::tensorflow::kYamnetDurationMs));

std::array<int16_t, coralmicro::tensorflow::kYamnetAudioSize> *AudioBridge::getAudioInput(void)
{
    return &audio_input;
}

void AudioBridge::init(void)
{
    static coralmicro::AudioDriverConfig audio_config{coralmicro::AudioSampleRate::k16000_Hz, kNumDmaBuffers, kDmaBufferSizeMs};
    static coralmicro::AudioService audio_service(&audio_driver, audio_config, kAudioServicePriority, kDropFirstSamplesMs);
    audio_service.AddCallback(
        &audio_latest,
        +[](void *ctx, const int32_t *samples, size_t num_samples)
        {
            static_cast<coralmicro::LatestSamples *>(ctx)->Append(samples, num_samples);
            return true;
        });
}

void AudioBridge::accessLatestSamples(void)
{
    audio_latest.AccessLatestSamples(
        [](const std::vector<int32_t> &samples, size_t start_index)
        {
            size_t i, j = 0;
            // Starting with start_index, grab until the end of the buffer.
            for (i = 0; i < samples.size() - start_index; ++i)
            {
                audio_input[i] = samples[i + start_index] >> 16;
            }
            // Now fill the rest of the data with the beginning of the
            // buffer.
            for (j = 0; j < samples.size() - i; ++j)
            {
                audio_input[i + j] = samples[j] >> 16;
            }
        });
}