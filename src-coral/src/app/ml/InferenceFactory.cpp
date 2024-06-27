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
#include "libs/base/timer.h"

#include "./InferenceFactory.h"
#include "../rpc/StringUtil.h"
#include "../../lib/arduprof/ArduProf.h"

using namespace coralmicro;

///////////////////////////////////////////////////////////////////////
static constexpr float kThreshold = 0.5;
static constexpr int kTopK = 5;

static std::array<int16_t, tensorflow::kYamnetAudioSize> audio_input;

///////////////////////////////////////////////////////////////////////
InferenceFactory *InferenceFactory::_instance = nullptr;

///////////////////////////////////////////////////////////////////////
bool InferenceFactory::inference(tflite::MicroInterpreter *interpreter, FrontendState *frontend_state)
{
    configASSERT(interpreter);
    if (!interpreter)
    {
        return false;
    }

    auto input_tensor = interpreter->input_tensor(0);
    auto preprocess_start = TimerMillis();
    tensorflow::YamNetPreprocessInput(audio_input.data(), input_tensor,
                                      frontend_state);
    // Reset frontend state.
    FrontendReset(frontend_state);
    auto preprocess_end = TimerMillis();
    if (interpreter->Invoke() != kTfLiteOk)
    {
        printf("Failed to invoke on test input\r\n");
        vTaskSuspend(nullptr);
    }
    auto current_time = TimerMillis();
    printf(
        "Yamnet preprocess time: %lums, invoke time: %lums, total: "
        "%lums\r\n",
        static_cast<uint32_t>(preprocess_end - preprocess_start),
        static_cast<uint32_t>(current_time - preprocess_end),
        static_cast<uint32_t>(current_time - preprocess_start));
    auto results =
        tensorflow::GetClassificationResults(interpreter, kThreshold, kTopK);
    printf("%s\r\n", tensorflow::FormatClassificationOutput(results).c_str());

    return true;
}
