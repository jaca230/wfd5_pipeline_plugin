#ifndef WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_DYNAMIC_H
#define WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_DYNAMIC_H

#include "analysis_pipeline/core/stages/base_stage.h"
#include <string>

/**
 * @class WFD5WaveformIntegralHistogramStageDynamic
 * @brief Pipeline stage reading a TList of WaveformIntegral objects
 *        and accumulating one TH1D histogram per {crate, amc, channel} triple.
 *
 * Two modes:
 *  - Fixed/relative range (legacy).
 *  - Dynamic range: buffer N presamples, then set range = mean + offset ± sigma*multiplier.
 */
class WFD5WaveformIntegralHistogramStageDynamic : public BaseStage {
public:
    WFD5WaveformIntegralHistogramStageDynamic() = default;
    ~WFD5WaveformIntegralHistogramStageDynamic() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "WFD5WaveformIntegralHistogramStageDynamic"; }

    struct IntegralCut {
        std::string detectorSystem;
        std::string subdetector;
        double minCut = -1e9;
        double maxCut = 1e9;
    };

private:
    std::string inputLabel_;
    std::string outputLabel_;
    std::string presampleLabel_;
    std::string titlePrefix_;
    int bins_ = 100;

    bool useRelativeRange_ = false;
    double relativeMin_ = 0.0;
    double relativeMax_ = 0.0;
    double min_ = 0.0;
    double max_ = 10000.0;

    bool useDynamic_ = false;
    int dynamicSampleSize_ = 100;
    double dynamicMeanOffset_ = 0.0;
    double dynamicSigmaMultiplier_ = 3.0;

    std::vector<IntegralCut> integralCuts_;

    void FillHistograms(TList* histList, TList* presampleList, const TList* inputList);

    ClassDefOverride(WFD5WaveformIntegralHistogramStageDynamic, 2);
};

#endif // WFD5_PIPELINE_PLUGIN_STAGES_WFD5_WAVEFORM_INTEGRAL_HISTOGRAM_STAGE_DYNAMIC_H
