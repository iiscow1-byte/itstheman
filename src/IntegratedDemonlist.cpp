#include "IntegratedDemonlist.hpp"
#include <jasmine/web.hpp>
#include <sstream>

using namespace geode::prelude;

std::vector<IDListDemon> IntegratedDemonlist::aredl;
std::vector<IDDemonPack> IntegratedDemonlist::aredlPacks;
std::vector<IDListDemon> IntegratedDemonlist::aredlOfficial;
std::vector<IDListDemon> IntegratedDemonlist::challengeList;
std::vector<IDListDemon> IntegratedDemonlist::allList;
bool IntegratedDemonlist::aredlLoaded = false;
bool IntegratedDemonlist::aredlOfficialLoaded = false;
bool IntegratedDemonlist::challengeListLoaded = false;
bool IntegratedDemonlist::allListLoaded = false;

static constexpr const char* DEMONLIST_BASE = "https://list-production-2b7d.up.railway.app";

static void loadDemonlistPage(
    TaskHolder<web::WebResponse>* listener,
    int after,
    Function<void()> success,
    CopyableFunction<void(int)> failure
) {
    std::string url = fmt::format("{}/api/v2/demons/listed/?limit=100", DEMONLIST_BASE);
    if (after > 0) url += fmt::format("&after={}", after);

    listener->spawn(
        web::WebRequest().get(url),
        [listener, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            int lastPosition = 0;
            int count = 0;

            for (auto& level : jasmine::web::getArray(res)) {
                count++;

                auto level_id = level.get<int>("level_id");
                if (!level_id.isOk()) continue;

                auto position = level.get<int>("position");
                if (!position.isOk()) continue;

                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                lastPosition = position.unwrap();
                IDListDemon demon(level_id.unwrap(), position.unwrap(), std::move(name).unwrap());
                auto tierResult = level.get<int>("tier");
                demon.tier = tierResult.isOk() ? tierResult.unwrap() : -1;

                IntegratedDemonlist::aredl.insert(
                    std::ranges::upper_bound(IntegratedDemonlist::aredl, demon, [](const IDListDemon& a, const IDListDemon& b) {
                        return a.position < b.position;
                    }),
                    std::move(demon)
                );
            }

            if (count >= 100 && lastPosition > 0) {
                loadDemonlistPage(listener, lastPosition, std::move(success), std::move(failure));
            } else {
                IntegratedDemonlist::aredlLoaded = true;
                success();
            }
        }
    );
}

void IntegratedDemonlist::loadAREDL(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    aredlLoaded = false;
    aredl.clear();
    loadDemonlistPage(&listener, 0, std::move(success), std::move(failure));
}

void IntegratedDemonlist::loadAREDLPacks(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    aredlPacks.clear();
    success();
}

void IntegratedDemonlist::loadAREDLOfficial(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    aredlOfficialLoaded = false;
    aredlOfficial.clear();

    listener.spawn(
        web::WebRequest().get("https://api.aredl.net/v2/api/aredl/levels"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            for (auto& level : jasmine::web::getArray(res)) {
                auto legacy = level.get<bool>("legacy");
                if (legacy.isOk() && legacy.unwrap()) continue;

                auto id = level.get<int>("level_id");
                if (!id.isOk()) continue;
                auto position = level.get<int>("position");
                if (!position.isOk()) continue;
                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                IDListDemon demon(id.unwrap(), position.unwrap(), std::move(name).unwrap());
                IntegratedDemonlist::aredlOfficial.insert(
                    std::ranges::upper_bound(IntegratedDemonlist::aredlOfficial, demon, [](const IDListDemon& a, const IDListDemon& b) {
                        return a.position < b.position;
                    }),
                    std::move(demon)
                );
            }

            IntegratedDemonlist::aredlOfficialLoaded = true;
            success();
        }
    );
}

static std::vector<std::string> parseCSVRow(const std::string& line) {
    std::vector<std::string> fields;
    bool inQuotes = false;
    std::string field;
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else if (c != '\r') {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

static int parseTier(const std::string& tierStr) {
    std::string t = tierStr;
    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
    while (!t.empty() && (t.back() == ' ' || t.back() == '\t' || t.back() == '\r')) t.pop_back();

    std::string lower = t;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });

    if (lower.find("sub") != std::string::npos) return 0;

    static const std::string prefix = "tier ";
    if (lower.size() > prefix.size() && lower.substr(0, prefix.size()) == prefix) {
        auto numStr = t.substr(prefix.size());
        try {
            size_t pos = 0;
            int val = std::stoi(numStr, &pos);
            if (pos == numStr.size()) return val;
        } catch (...) {}
    }
    return 0;
}

static void loadChallengeListPage(
    TaskHolder<web::WebResponse>* listener,
    int afterId,
    Function<void()> success,
    CopyableFunction<void(int)> failure
) {
    std::string url = "https://challengelist.gd/api/v2/demons/?limit=100";
    if (afterId > 0) url += fmt::format("&after={}", afterId);

    listener->spawn(
        web::WebRequest().get(url),
        [listener, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            int lastId = 0;
            int count = 0;

            for (auto& level : jasmine::web::getArray(res)) {
                count++;

                auto hidden = level.get<bool>("hidden");
                if (hidden.isOk() && hidden.unwrap()) continue;

                auto internalId = level.get<int>("id");
                if (internalId.isOk()) lastId = internalId.unwrap();

                auto level_id = level.get<int>("level_id");
                if (!level_id.isOk()) continue;

                auto position = level.get<int>("position");
                if (!position.isOk()) continue;

                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                IDListDemon demon(level_id.unwrap(), position.unwrap(), std::move(name).unwrap());
                IntegratedDemonlist::challengeList.insert(
                    std::ranges::upper_bound(IntegratedDemonlist::challengeList, demon, [](const IDListDemon& a, const IDListDemon& b) {
                        return a.position < b.position;
                    }),
                    std::move(demon)
                );
            }

            if (count >= 100 && lastId > 0) {
                loadChallengeListPage(listener, lastId, std::move(success), std::move(failure));
            } else {
                IntegratedDemonlist::challengeListLoaded = true;
                success();
            }
        }
    );
}

void IntegratedDemonlist::loadChallengeList(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    challengeListLoaded = false;
    challengeList.clear();
    loadChallengeListPage(&listener, 0, std::move(success), std::move(failure));
}

static const std::array<const char*, 6> ALL_LIST_URLS = {
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=0&single=true&output=csv",
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=1036115495&single=true&output=csv",
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=1989779679&single=true&output=csv",
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=516171001&single=true&output=csv",
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=1985672631&single=true&output=csv",
    "https://docs.google.com/spreadsheets/d/e/2PACX-1vQBDKoXQm4Zxzt_Fuv7WhdVhcTB80lsDCXqSYPMm62Ee2DJDHzhQScoxbp7Jj33OFgkPV9WEJlNyhML/pub?gid=1875166663&single=true&output=csv",
};

void IntegratedDemonlist::loadAllList(
    std::array<TaskHolder<web::WebResponse>, 6>& listeners,
    Function<void()> success,
    CopyableFunction<void(int)> failure
) {
    allListLoaded = false;
    allList.clear();
    allList.reserve(70000);

    auto buffers = std::make_shared<std::array<std::string, 6>>();
    auto remaining = std::make_shared<int>(6);
    auto anyFailed = std::make_shared<bool>(false);
    auto onDone = std::make_shared<Function<void()>>(std::move(success));

    for (int i = 0; i < 6; ++i) {
        listeners[i].spawn(
            web::WebRequest().userAgent("Mozilla/5.0").get(ALL_LIST_URLS[i]),
            [i, buffers, remaining, anyFailed, onDone, failure](web::WebResponse res) mutable {
                if (*anyFailed) return;
                if (!res.ok()) {
                    *anyFailed = true;
                    return failure(res.code());
                }
                auto textResult = res.string();
                if (!textResult.isOk()) {
                    *anyFailed = true;
                    return failure(0);
                }
                (*buffers)[i] = textResult.unwrap();

                if (--(*remaining) == 0) {
                    int pos = 1;
                    for (int j = 0; j < 6; ++j) {
                        std::istringstream stream((*buffers)[j]);
                        std::string line;
                        int row = 0;
                        while (std::getline(stream, line)) {
                            ++row;
                            if (row <= 3) continue;
                            auto fields = parseCSVRow(line);
                            if (fields.size() < 6) continue;
                            auto& name = fields[1];
                            auto& tierStr = fields[3];
                            auto& idStr = fields[5];
                            if (name.empty() || idStr.empty()) continue;
                            int levelId = 0;
                            try { levelId = std::stoi(idStr); } catch (...) { continue; }
                            if (levelId == 0) continue;
                            allList.emplace_back(levelId, pos++, name, parseTier(tierStr));
                        }
                    }
                    allListLoaded = true;
                    (*onDone)();
                }
            }
        );
    }
}
