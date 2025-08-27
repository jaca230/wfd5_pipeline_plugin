#include "analysis_pipeline/wfd5/stages/wfd5_waveform_integral_histogram_stage.h"

#include <TList.h>
#include <TH1D.h>
#include <TObject.h>
#include <spdlog/spdlog.h>
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

#include "data_products/wfd5/WaveformIntegral.hh"

using namespace dataProducts;
using json = nlohmann::json;

ClassImp(WFD5WaveformIntegralHistogramStage)

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void WFD5WaveformIntegralHistogramStage::OnInit() {
    inputLabel_ = parameters_.value("input_product", "WaveformIntegralCollection");
    outputLabel_ = parameters_.value("product_name", "WaveformIntegralHistogramCollection");
    titlePrefix_ = parameters_.value("title_prefix", "Integral");

    // load channel map from external JSON file
    if (parameters_.contains("channel_map_file")) {
        std::string filename = parameters_.value("channel_map_file", "");
        if (!filename.empty()) {
            std::ifstream file(filename);
            if (!file) {
                spdlog::error("[{}] Failed to open channel map file '{}'", Name(), filename);
            } else {
                json mapJson;
                file >> mapJson;

                for (const auto& item : mapJson) {
                    ChannelHistInfo info;
                    info.detectorSystem = item.value("detectorSystem", "");
                    info.subdetector = item.value("subdetector", "");
                    info.bins = item.value("bins", 100);
                    info.xMin = item.value("xMin", 0.0);
                    info.xMax = item.value("xMax", 10000.0);

                    // store in map by detector_subdetector as key
                    std::string key = info.detectorSystem + "_" + info.subdetector;
                    channelMap_[key] = info;
                }
                spdlog::debug("[{}] Loaded {} entries from '{}'", Name(), channelMap_.size(), filename);
            }
        }
    }

    spdlog::debug("[{}] Initialized with input '{}', output '{}', channelMap size={}",
                  Name(), inputLabel_, outputLabel_, channelMap_.size());
}


void WFD5WaveformIntegralHistogramStage::Process() {
    if (!getDataProductManager()->hasProduct(inputLabel_)) {
        spdlog::warn("[{}] Input '{}' not found", Name(), inputLabel_);
        return;
    }

    auto inputHandle = getDataProductManager()->checkoutRead(inputLabel_);
    const auto* inputList = dynamic_cast<const TList*>(inputHandle->getObject());
    if (!inputList) {
        spdlog::error("[{}] Input '{}' is not a TList", Name(), inputLabel_);
        return;
    }

    // get/create histogram list
    TList* histList = nullptr;
    if (getDataProductManager()->hasProduct(outputLabel_)) {
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        histList = dynamic_cast<TList*>(outHandle->getObject());
    } else {
        auto newList = std::make_unique<TList>();
        newList->SetOwner(kTRUE);
        auto pdp = std::make_unique<PipelineDataProduct>();
        pdp->setName(outputLabel_);
        pdp->setObject(std::move(newList));
        pdp->addTag("WFD5");
        pdp->addTag("histogram_list");
        getDataProductManager()->addOrUpdate(outputLabel_, std::move(pdp));
        auto outHandle = getDataProductManager()->checkoutWrite(outputLabel_);
        histList = dynamic_cast<TList*>(outHandle->getObject());
    }
    if (!histList) {
        spdlog::error("[{}] Failed to get histogram list '{}'", Name(), outputLabel_);
        return;
    }

    FillHistograms(histList, inputList);
}

void WFD5WaveformIntegralHistogramStage::FillHistograms(TList* histList, const TList* inputList) {
    for (const TObject* obj : *inputList) {
        auto* wi = dynamic_cast<const WaveformIntegral*>(obj);
        if (!wi) continue;

        // Construct histogram name (unique per crate/amc/channel)
        std::string histName = wi->detectorSystem + "_" + wi->subdetector + "_"
                             + std::to_string(wi->crateNum) + "_"
                             + std::to_string(wi->amcNum) + "_"
                             + std::to_string(wi->channelTag);

        // Find existing histogram
        TH1D* hist = dynamic_cast<TH1D*>(histList->FindObject(histName.c_str()));
        if (!hist) {
            // Look for channel map info by detector/subdetector
            ChannelHistInfo info;
            auto it = std::find_if(channelMap_.begin(), channelMap_.end(),
                [&](const auto& kv) {
                    const auto& cinfo = kv.second;
                    return cinfo.detectorSystem == wi->detectorSystem &&
                           cinfo.subdetector == wi->subdetector;
                });
            if (it != channelMap_.end()) {
                info = it->second;
            } else {
                spdlog::debug("[{}] No channel map info for {}_{}; using default histogram params",
                              Name(), wi->detectorSystem, wi->subdetector);
            }

            std::string histTitle = titlePrefix_ + " - " + histName;
            hist = new TH1D(histName.c_str(), histTitle.c_str(), info.bins, info.xMin, info.xMax);
            hist->SetDirectory(nullptr);
            histList->Add(hist);
        }

        hist->Fill(wi->integral);
    }
}

