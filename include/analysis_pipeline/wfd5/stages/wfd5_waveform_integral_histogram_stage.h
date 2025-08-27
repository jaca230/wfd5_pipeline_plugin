#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include <string>
#include <map>

struct ChannelHistInfo {
    int bins = 100;
    double xMin = 0.0;
    double xMax = 10000.0;
};

class WFD5WaveformIntegralHistogramStage : public BaseStage {
public:
    WFD5WaveformIntegralHistogramStage() = default;
    ~WFD5WaveformIntegralHistogramStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5WaveformIntegralHistogramStage"; }

private:
    std::string inputLabel_;
    std::string outputLabel_;
    std::string presampleLabel_;
    std::string titlePrefix_;

    // channel map key: "detector_subdet_crate_amc_ch"
    std::map<std::string, ChannelHistInfo> channelMap_;

    void FillHistograms(TList* histList, const TList* inputList);

    ClassDefOverride(WFD5WaveformIntegralHistogramStage, 3);
};

#endif
