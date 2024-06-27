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
#include <vector>
#include <set>
#include <algorithm>
#include <stdlib.h>
#include "libs/audio/audio_service.h"
#include "libs/base/timer.h"
#include "libs/base/filesystem.h"
#include "libs/tpu/edgetpu_manager.h"
#include "libs/tpu/edgetpu_op.h"
#include "libs/tensorflow/audio_models.h"
#include "libs/tensorflow/utils.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_error_reporter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_interpreter.h"
#include "third_party/tflite-micro/tensorflow/lite/micro/micro_mutable_op_resolver.h"

#include "../../base/util/FsUtil.h"
#include "../../lib/arduprof/ArduProf.h"
#include "../thread/QueueMainM7.h"
#include "./AlarmInterpreter.h"
#include "./AudioBridge.h"

// using namespace coralmicro;

static std::set<int> _idAlarmList = {392};

static constexpr char labelPath[] = "/models/yamnet_class_map.csv";
static constexpr char lineTerminator[] = "\n";
static constexpr char fieldSeparator[] = ",";

static constexpr float kThreshold = 0.3;
static constexpr int kTopK = 5;

#ifdef YAMNET_CPU
static constexpr char kModelPath[] = "/models/yamnet_spectra_in.tflite";
// static constexpr char kModelPath[] = "/coralmicro/models/yamnet_spectra_in.tflite";
static constexpr bool kUseTpu = false;
#else
// static constexpr char kModelPath[] = "/models/yamnet_spectra_in_edgetpu.tflite";
static constexpr char kModelPath[] = "/coralmicro/models/yamnet_spectra_in_edgetpu.tflite";
static constexpr bool kUseTpu = true;
#endif

////////////////////////////////////////////////////////////////////////////////////////////
AlarmInterpreter *AlarmInterpreter::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
bool AlarmInterpreter::init(void)
{
    static std::vector<uint8_t> tfliteBinData;
    DBGLOG(Debug, "Loading:  %s ...", kModelPath);
    if (!coralmicro::LfsReadFile(kModelPath, &tfliteBinData))
    {
        DBGLOG(Error, "Failed to load %s", kModelPath);
        return false;
    }

    static const auto *model = tflite::GetModel(tfliteBinData.data());
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        DBGLOG(Error, "Model schema version is %lu, supported is %d", model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    DBGLOG(Debug, "Model schema version is %lu, supported is %d", model->version(), TFLITE_SCHEMA_VERSION);

#ifndef YAMNET_CPU
    static auto tpu_context = coralmicro::EdgeTpuManager::GetSingleton()->OpenDevice();
    if (!tpu_context)
    {
        DBGLOG(Error, "Failed to get EdgeTpu context");
        return false;
    }
    DBGLOG(Debug, "EdgeTpuManager::GetSingleton()->OpenDevice() success");
#endif

    DBGLOG(Debug, "Initializing tflite::MicroMutableOpResolver ...");
    static tflite::MicroErrorReporter error_reporter;
    static auto resolver = coralmicro::tensorflow::SetupYamNetResolver</*tForTpu=*/kUseTpu>();

    // An area of memory to use for input, output, and intermediate arrays.
    static constexpr int kTensorArenaSize = 8 * 1024 * 1024;
    STATIC_TENSOR_ARENA_IN_SDRAM(tensor_arena, kTensorArenaSize);

    DBGLOG(Debug, "Initializing tflite::MicroInterpreter ...");
    static tflite::MicroInterpreter interpreter{model, resolver, tensor_arena,
                                                kTensorArenaSize, &error_reporter};
    if (interpreter.AllocateTensors() != kTfLiteOk)
    {
        DBGLOG(Error, "AllocateTensors() failed");
        return false;
    }

    auto instance = getInstance();

    if (!coralmicro::tensorflow::PrepareAudioFrontEnd(
            &instance->frontendState, coralmicro::tensorflow::AudioModel::kYAMNet))
    {
        DBGLOG(Error, "coralmicro::tensorflow::PrepareAudioFrontEnd() failed.");
        return false;
    }

    instance->_interpreter = &interpreter;
    DBGLOG(Debug, "_interpreter=%p", getInstance()->_interpreter);

    return true;
}

bool AlarmInterpreter::inference(void)
{
    auto interpreter = _interpreter;
    if (!interpreter)
    {
        return false;
    }

    auto input_tensor = interpreter->input_tensor(0);
    auto preprocess_start = coralmicro::TimerMillis();
    coralmicro::tensorflow::YamNetPreprocessInput(AudioBridge::getAudioInput()->data(), input_tensor, &frontendState);

    // Reset frontend state.
    FrontendReset(&frontendState);
    auto preprocess_end = coralmicro::TimerMillis();
    if (interpreter->Invoke() != kTfLiteOk)
    {
        DBGLOG(Debug, "interpreter->Invoke() failed");
        return false;
    }
    auto current_time = coralmicro::TimerMillis();
    // DBGLOG(Debug,
    //        "Yamnet preprocess time: %lums, invoke time: %lums, total: %lums",
    //        static_cast<uint32_t>(preprocess_end - preprocess_start),
    //        static_cast<uint32_t>(current_time - preprocess_end),
    //        static_cast<uint32_t>(current_time - preprocess_start));
    auto results =
        coralmicro::tensorflow::GetClassificationResults(interpreter, kThreshold, kTopK);
    std::sort(results.begin(), results.end(), [](const coralmicro::tensorflow::Class &a, const coralmicro::tensorflow::Class &b)
              { return a.score > b.score; });
    uint8_t npuResult = ResultAlarmOff;
    for (const auto &result : results)
    {
        if (_idAlarmList.count(static_cast<int>(result.id)))
        {
            npuResult = ResultAlarmOn;
            DBGLOG(Debug, "result: id=%d, score=%f", result.id, result.score);
        }
    }
    if (_npuResult != npuResult)
    {
        // DBGLOG(Debug, "postEvent(EventIPC, EventNpuResult, %d)", npuResult);
        _npuResult = npuResult;
        QueueMainM7::getInstance()->postEvent(EventIPC, EventNpuResult, npuResult);
    }

    return true;
}

tflite::MicroInterpreter *AlarmInterpreter::getInterpreter(void)
{
    return _instance ? _instance->_interpreter : nullptr;
}

AlarmInterpreter *AlarmInterpreter::getInstance(void)
{
    if (!_instance)
    {
        static AlarmInterpreter instance;
        _instance = &instance;
    }
    return _instance;
}
