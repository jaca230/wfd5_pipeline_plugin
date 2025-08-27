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
                    std::string key = item.value("detectorSystem", "") + "_"
                                    + item.value("subdetector", "") + "_"
                                    + std::to_string(int(item.value("crateNum", 0))) + "_"
                                    + std::to_string(int(item.value("amcSlotNum", 0))) + "_"
                                    + std::to_string(int(item.value("channelNum", 0)));

                    ChannelHistInfo info;
                    info.bins = item.value("bins", 100);
                    info.xMin = item.value("xMin", 0.0);
                    info.xMax = item.value("xMax", 10000.0);

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

        std::string key = wi->detectorSystem + "_"
                        + wi->subdetector + "_"
                        + std::to_string(wi->crateNum) + "_"
                        + std::to_string(wi->amcNum) + "_"
                        + std::to_string(wi->channelTag);

        auto it = channelMap_.find(key);
        if (it == channelMap_.end()) {
            spdlog::debug("[{}] No histogram info for waveform key '{}'; skipping", Name(), key);
            continue;
        }

        const auto& info = it->second;

        TH1D* hist = dynamic_cast<TH1D*>(histList->FindObject(key.c_str()));
        if (!hist) {
            std::string histTitle = titlePrefix_ + " - " + key;
            hist = new TH1D(key.c_str(), histTitle.c_str(), info.bins, info.xMin, info.xMax);
            hist->SetDirectory(nullptr);
            histList->Add(hist);
        }

        hist->Fill(wi->integral);
    }
}
