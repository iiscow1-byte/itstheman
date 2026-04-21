#include "IntegratedDemonlist.hpp"
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::vector<IDListDemon> IntegratedDemonlist::aredl;
std::vector<IDDemonPack> IntegratedDemonlist::aredlPacks;
std::vector<IDListDemon> IntegratedDemonlist::pemonlist;
bool IntegratedDemonlist::aredlLoaded = false;
bool IntegratedDemonlist::pemonlistLoaded = false;

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

void IntegratedDemonlist::loadPemonlist(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://pemonlist.com/api/list?limit=150&version=2"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            pemonlistLoaded = true;
            pemonlist.clear();

            for (auto& level : jasmine::web::getArray(res, "data")) {
                auto id = level.get<int>("level_id");
                if (!id.isOk()) continue;

                auto position = level.get<int>("placement");
                if (!position.isOk()) continue;

                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                IDListDemon demon(id.unwrap(), position.unwrap(), std::move(name).unwrap());

                pemonlist.insert(std::ranges::upper_bound(pemonlist, demon, [](const IDListDemon& a, const IDListDemon& b) {
                    return a.position < b.position;
                }), std::move(demon));
            }

            success();
        }
    );
}
